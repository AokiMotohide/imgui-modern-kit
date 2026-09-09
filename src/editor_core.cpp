#include <imkit/editor_core.h>
#include "transaction_support.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <cstring>

namespace imkit::editor {
namespace {
Tick Rounded(long double value) {
    if (value >= static_cast<long double>((std::numeric_limits<Tick>::max)()))
        return (std::numeric_limits<Tick>::max)();
    if (value <= static_cast<long double>((std::numeric_limits<Tick>::min)()))
        return (std::numeric_limits<Tick>::min)();
    return static_cast<Tick>(value);
}
bool DropRate(FrameRate r) {
    return r.denominator == 1001 && (r.numerator == 30000 || r.numerator == 60000);
}
ImVec2 Screen(Point p, const CanvasState &s, ImVec2 origin) {
    auto v = ToScreen(p, s, {origin.x, origin.y});
    return {static_cast<float>(v.x), static_cast<float>(v.y)};
}
void Diamond(ImDrawList *d, ImVec2 p, ImU32 color, float r = 5) {
    d->AddQuadFilled({p.x, p.y - r}, {p.x + r, p.y}, {p.x, p.y + r}, {p.x - r, p.y}, color);
}
bool Flag(PropertyFlags f, PropertyFlags bit) {
    return (static_cast<unsigned>(f) & static_cast<unsigned>(bit)) != 0;
}
void Action(EventBuffer &out, StableId id, std::uint64_t revision, EditKind kind, Value from = {},
            Value to = {}) {
    out.Push({id, revision, Phase::Commit, kind, from, to, CurrentModifiers()});
}
} // namespace
bool Valid(FrameRate r) {
    return r.numerator > 0 && r.denominator > 0;
}
Tick FrameToTick(std::int64_t f, FrameRate r) {
    return Valid(r) ? Rounded(std::round(static_cast<long double>(f) * TicksPerSecond * r.denominator /
                                         r.numerator))
                    : 0;
}
std::int64_t TickToFrame(Tick t, FrameRate r) {
    return Valid(r) ? Rounded(std::round(static_cast<long double>(t) * r.numerator /
                                         (static_cast<long double>(TicksPerSecond) * r.denominator)))
                    : 0;
}
double Seconds(Tick t) {
    return static_cast<double>(t) / TicksPerSecond;
}
Tick FromSeconds(double s) {
    return std::isfinite(s) ? Rounded(std::round(static_cast<long double>(s) * TicksPerSecond)) : 0;
}
bool FormatTimecode(Tick tick, FrameRate rate, bool drop, std::span<char> out) {
    if (!Valid(rate) || out.empty() || (drop && !DropRate(rate)))
        return false;
    auto frame = TickToFrame(tick, rate);
    const bool negative = frame < 0;
    // Tick range is narrower than frame range for supported timecode rates.
    if (frame == (std::numeric_limits<Tick>::min)())
        return false;
    if (negative)
        frame = -frame;
    const int fps = static_cast<int>(std::lround(static_cast<double>(rate.numerator) / rate.denominator));
    if (fps < 1 || fps > 120)
        return false;
    if (drop) {
        const int d = fps / 15, ten = fps * 600 - d * 9, minute = fps * 60 - d;
        const auto rest = frame % ten;
        frame += d * 9 * (frame / ten) + (rest >= d ? d * ((rest - d) / minute) : 0);
    }
    const auto hours = frame / (fps * 3600);
    int n = std::snprintf(out.data(), out.size(), "%s%02lld:%02lld:%02lld%c%02lld", negative ? "-" : "",
                          static_cast<long long>(hours), static_cast<long long>(frame / (fps * 60) % 60),
                          static_cast<long long>(frame / fps % 60), drop ? ';' : ':',
                          static_cast<long long>(frame % fps));
    return n >= 0 && static_cast<std::size_t>(n) < out.size();
}
bool ParseTimecode(std::string_view text, FrameRate r, bool drop, Tick &result) {
    if (!Valid(r) || (drop && !DropRate(r)))
        return false;
    bool negative = !text.empty() && text.front() == '-';
    if (negative)
        text.remove_prefix(1);
    if (text.size() != 11 || text[2] != ':' || text[5] != ':' || text[8] != (drop ? ';' : ':'))
        return false;
    int v[4]{};
    for (int i = 0; i < 4; ++i) {
        auto a = text[i * 3], b = text[i * 3 + 1];
        if (a < '0' || a > '9' || b < '0' || b > '9')
            return false;
        v[i] = (a - '0') * 10 + b - '0';
    }
    int fps = static_cast<int>(std::lround(static_cast<double>(r.numerator) / r.denominator));
    if (fps < 1 || fps > 120 || v[1] >= 60 || v[2] >= 60 || v[3] >= fps)
        return false;
    int d = drop ? fps / 15 : 0;
    if (drop && v[1] % 10 && v[2] == 0 && v[3] < d)
        return false;
    Tick minutes = v[0] * 60 + v[1];
    Tick f = ((minutes * 60) + v[2]) * fps + v[3] - d * (minutes - minutes / 10);
    result = FrameToTick(negative ? -f : f, r);
    return true;
}
Modifiers CurrentModifiers() {
    const auto &io = ImGui::GetIO();
    return {io.KeyShift, io.KeyCtrl, io.KeyAlt};
}
bool EventBuffer::Push(const Event &e) {
    if (count >= storage.size()) {
        overflow = true;
        return false;
    }
    storage[count++] = e;
    return true;
}
void EventBuffer::Clear() {
    count = 0;
    overflow = false;
}
std::span<const Event> EventBuffer::Events() const {
    return storage.first(count);
}
bool Transaction::Begin(StableId id, std::uint64_t rev, EditKind kind, Value value, Modifiers mod,
                        EventBuffer &out) {
    if (active)
        return false;
    Event event{id, rev, Phase::Begin, kind, value, value, mod};
    if (!out.Push(event))
        return false;
    draft = event;
    active = true;
    return true;
}
bool Transaction::Update(std::uint64_t rev, Value value, EventBuffer &out) {
    if (!active)
        return false;
    if (draft.phase == Phase::Commit || draft.phase == Phase::Cancel) {
        detail::ResumeTerminal(*this, rev, out);
        return false;
    }
    if (rev != draft.revision) {
        Cancel(out);
        return false;
    }
    Event event = draft;
    event.phase = Phase::Update;
    event.proposed = value;
    if (!out.Push(event))
        return false;
    draft = event;
    return true;
}
bool Transaction::Commit(std::uint64_t rev, EventBuffer &out) {
    if (!active)
        return false;
    if (rev != draft.revision) {
        Cancel(out);
        return false;
    }
    if (draft.phase == Phase::Cancel) {
        Cancel(out);
        return false;
    }
    draft.phase = Phase::Commit;
    if (!out.Push(draft))
        return false;
    active = false;
    return true;
}
void Transaction::Cancel(EventBuffer &out) {
    if (!active)
        return;
    draft.phase = Phase::Cancel;
    draft.proposed = draft.original;
    if (out.Push(draft))
        active = false;
}
bool Selection::Contains(StableId id) const {
    return std::find(storage.begin(), storage.begin() + count, id) != storage.begin() + count;
}
void Selection::Clear() {
    count = 0;
    active = 0;
}
bool Selection::Set(StableId id, bool additive, bool toggle) {
    if (!additive && !toggle)
        Clear();
    auto end = storage.begin() + count, it = std::find(storage.begin(), end, id);
    if (it != end) {
        if (toggle) {
            std::move(it + 1, end, it);
            --count;
        }
        active = count ? storage[count - 1] : 0;
        return true;
    }
    if (count == storage.size())
        return false;
    storage[count++] = id;
    active = id;
    return true;
}
SnapResult ResolveSnap(Tick t, std::span<const SnapCandidate> candidates, double scale, double threshold,
                       StableId exclude) {
    SnapResult result{t};
    result.distancePixels = threshold;
    if (!std::isfinite(scale) || scale <= 0 || threshold < 0)
        return result;
    for (const auto &c : candidates) {
        if (exclude && c.id == exclude)
            continue;
        double distance = std::abs(static_cast<double>(static_cast<long double>(c.tick) - t) * scale);
        if (distance > threshold)
            continue;
        if (!result.snapped || c.priority > result.candidate.priority ||
            (c.priority == result.candidate.priority && distance < result.distancePixels))
            result = {c.tick, true, c, distance};
    }
    return result;
}
Point ToScreen(Point v, const CanvasState &s, Point o) {
    return {o.x + (v.x - s.origin.x) * s.scale.x, o.y + (v.y - s.origin.y) * s.scale.y};
}
Point FromScreen(Point v, const CanvasState &s, Point o) {
    return {s.origin.x + (v.x - o.x) / s.scale.x, s.origin.y + (v.y - o.y) / s.scale.y};
}
Rect VisibleRange(const CanvasState &s, Point size) {
    return {s.origin, {s.origin.x + size.x / s.scale.x, s.origin.y + size.y / s.scale.y}};
}
void ZoomAt(CanvasState &s, Point anchor, Point factor) {
    Point at = FromScreen(anchor, s, {});
    s.scale = {std::clamp(s.scale.x * factor.x, 1e-9, 1e9), std::clamp(s.scale.y * factor.y, 1e-9, 1e9)};
    s.origin = {at.x - anchor.x / s.scale.x, at.y - anchor.y / s.scale.y};
}
void Fit(CanvasState &s, Rect r, Point size, double pad) {
    s.scale = {(std::max)(1., size.x - 2 * pad) / (std::max)(1e-9, r.max.x - r.min.x),
               (std::max)(1., size.y - 2 * pad) / (std::max)(1e-9, r.max.y - r.min.y)};
    s.origin = {r.min.x - pad / s.scale.x, r.min.y - pad / s.scale.y};
}
bool InPolygon(Point p, std::span<const Point> polygon) {
    if (polygon.size() < 3)
        return false;
    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        auto a = polygon[i], b = polygon[j];
        if (((a.y > p.y) != (b.y > p.y)) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
            inside = !inside;
    }
    return inside;
}
CanvasView BeginCanvas(const char *id, CanvasState &s, ImVec2 size, const Theme &t) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, t.colors.canvas);
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    CanvasView v;
    v.min = ImGui::GetCursorScreenPos();
    auto available = ImGui::GetContentRegionAvail();
    v.max = {v.min.x + available.x, v.min.y + available.y};
    v.hovered = ImGui::IsWindowHovered();
    if (v.hovered) {
        const auto &io = ImGui::GetIO();
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            s.origin.x -= io.MouseDelta.x / s.scale.x;
            s.origin.y -= io.MouseDelta.y / s.scale.y;
        }
        if (s.wheelZoom && io.MouseWheel && !ImGui::IsAnyItemActive()) {
            const double factor = std::pow(1.15, io.MouseWheel);
            ZoomAt(s, {io.MousePos.x - v.min.x, io.MousePos.y - v.min.y},
                   {factor, io.KeyShift || !s.wheelZoomY ? 1 : factor});
        }
        s.origin.x -= io.MouseWheelH * 32 / s.scale.x;
    }
    v.visible = VisibleRange(s, {available.x, available.y});
    ImGui::GetWindowDrawList()->PushClipRect(v.min, v.max, true);
    return v;
}
void EndCanvas() {
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}
void CanvasSelection(const CanvasView &v, CanvasState &s, const SelectionProvider &p, Selection &selection,
                     EventBuffer &out, const Theme &theme, bool lasso) {
    const auto &io = ImGui::GetIO();
    Point mouse = FromScreen({io.MousePos.x, io.MousePos.y}, s, {v.min.x, v.min.y});
    if (v.hovered && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseClicked(0)) {
        s.selecting = true;
        s.lasso = lasso;
        s.selectionStart = mouse;
        s.pathCount = 0;
        if (!s.selectionPath.empty())
            s.selectionPath[s.pathCount++] = mouse;
    }
    if (!s.selecting)
        return;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        s.selecting = false;
        return;
    }
    if (s.lasso && ImGui::IsMouseDown(0) && s.pathCount < s.selectionPath.size()) {
        const auto previous = s.selectionPath[s.pathCount - 1];
        if (std::hypot((mouse.x - previous.x) * s.scale.x, (mouse.y - previous.y) * s.scale.y) >= 3)
            s.selectionPath[s.pathCount++] = mouse;
    }
    auto *d = ImGui::GetWindowDrawList();
    auto color = ImGui::GetColorU32(theme.colors.focus);
    Rect bounds{{(std::min)(s.selectionStart.x, mouse.x), (std::min)(s.selectionStart.y, mouse.y)},
                {(std::max)(s.selectionStart.x, mouse.x), (std::max)(s.selectionStart.y, mouse.y)}};
    if (s.lasso) {
        for (std::size_t i = 0; i < s.pathCount; ++i) {
            auto a = s.selectionPath[i], b = s.selectionPath[(i + 1) % s.pathCount];
            d->AddLine(Screen(a, s, v.min), Screen(b, s, v.min), color);
            bounds.min.x = (std::min)(bounds.min.x, a.x);
            bounds.min.y = (std::min)(bounds.min.y, a.y);
            bounds.max.x = (std::max)(bounds.max.x, a.x);
            bounds.max.y = (std::max)(bounds.max.y, a.y);
        }
    } else
        d->AddRect(Screen(bounds.min, s, v.min), Screen(bounds.max, s, v.min), color);
    if (ImGui::IsMouseReleased(0)) {
        if (!io.KeyCtrl && !io.KeyShift)
            selection.Clear();
        auto points = p.query ? p.query(p.user, bounds) : std::span<const SelectablePoint>{};
        for (auto point : points)
            if (!point.locked &&
                (!s.lasso || InPolygon(point.position, s.selectionPath.first(s.pathCount)))) {
                if (!s.lasso && (point.position.x < bounds.min.x || point.position.x > bounds.max.x ||
                                 point.position.y < bounds.min.y || point.position.y > bounds.max.y))
                    continue;
                if (selection.Set(point.id, true))
                    Action(out, point.id, p.revision, s.lasso ? EditKind::LassoSelect : EditKind::BoxSelect);
            }
        s.selecting = false;
    }
}
void DrawGrid(const CanvasView &v, const CanvasState &s, Point spacing, const Theme &t) {
    auto *d = ImGui::GetWindowDrawList();
    auto color = ImGui::GetColorU32(t.colors.border);
    for (int axis = 0; axis < 2; ++axis) {
        double step = axis ? spacing.y : spacing.x, scale = axis ? s.scale.y : s.scale.x;
        if (step <= 0 || !std::isfinite(step))
            continue;
        while (step * scale < 20)
            step *= 10;
        double lo = axis ? v.visible.min.y : v.visible.min.x, hi = axis ? v.visible.max.y : v.visible.max.x;
        for (double p = std::ceil(lo / step) * step; p <= hi; p += step) {
            auto q = Screen(axis ? Point{v.visible.min.x, p} : Point{p, v.visible.min.y}, s, v.min);
            d->AddLine(q, axis ? ImVec2{v.max.x, q.y} : ImVec2{q.x, v.max.y}, color);
        }
    }
}
std::size_t MakeBindings(ShortcutPreset preset, std::span<Binding> dst) {
    const Binding defaults[] = {{Command::PlayPause, ImGuiKey_Space},
                                {Command::Pause, ImGuiKey_K},
                                {Command::PreviousFrame, ImGuiKey_LeftArrow},
                                {Command::NextFrame, ImGuiKey_RightArrow},
                                {Command::SetIn, ImGuiKey_I},
                                {Command::SetOut, ImGuiKey_O},
                                {Command::Loop, ImGuiMod_Ctrl | ImGuiKey_L},
                                {Command::Split, ImGuiMod_Ctrl | ImGuiKey_B},
                                {Command::Delete, ImGuiKey_Delete},
                                {Command::Duplicate, ImGuiMod_Ctrl | ImGuiKey_D},
                                {Command::SelectAll, ImGuiMod_Ctrl | ImGuiKey_A},
                                {Command::Fit, ImGuiKey_Home},
                                {Command::AddKey, ImGuiKey_I},
                                {Command::PreviousKey, ImGuiKey_UpArrow},
                                {Command::NextKey, ImGuiKey_DownArrow},
                                {Command::Undo, ImGuiMod_Ctrl | ImGuiKey_Z},
                                {Command::Redo, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z},
                                {Command::PlayReverse, ImGuiKey_J},
                                {Command::PlayForward, ImGuiKey_L}};
    auto n = (std::min)(dst.size(), std::size(defaults));
    std::copy_n(defaults, n, dst.begin());
    for (auto &b : dst.first(n)) {
        if (preset == ShortcutPreset::Premiere && b.command == Command::Split)
            b.chord = ImGuiMod_Ctrl | ImGuiKey_K;
        if (preset == ShortcutPreset::Blender && b.command == Command::Duplicate)
            b.chord = ImGuiMod_Shift | ImGuiKey_D;
    }
    return n;
}
bool CommandPressed(Command command, std::span<const Binding> bindings, bool focused) {
    if (!focused || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive())
        return false;
    for (auto b : bindings)
        if (b.command == command && ImGui::IsKeyChordPressed(b.chord))
            return true;
    return false;
}
double FollowPlayhead(double origin,double width,double head,AutoScroll mode) {
    if (!std::isfinite(origin) || !std::isfinite(width) || !std::isfinite(head) || width<=0 || mode==AutoScroll::Off)
        return origin;
    if (mode==AutoScroll::Page) {
        if (head<origin || head>=origin+width) return origin+std::floor((head-origin)/width)*width;
        return origin;
    }
    const double margin=width*.1;
    if (head>origin+width-margin) return head-width+margin;
    if (head<origin+margin) return head-margin;
    return origin;
}
void Transport(TimeState &s, std::span<const Binding> bindings) {
    Transport(s, bindings, nullptr);
}
void Transport(TimeState &s, std::span<const Binding> bindings, const IconAtlas *icons) {
    auto button = [&](const char *id, IconId icon, const char *label) {
        return icons ? IconButton(id, *icons, icon, label) : ImGui::Button(label);
    };
    bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (focused && !ImGui::GetIO().WantTextInput && !ImGui::IsAnyItemActive()) {
        if (CommandPressed(Command::PlayReverse, bindings, focused)) {
            s.playbackRate = s.playing && s.playbackRate < 0 ? s.playbackRate * 2 : -1;
            s.playing = true;
        }
        if (CommandPressed(Command::PlayForward, bindings, focused)) {
            s.playbackRate = s.playing && s.playbackRate > 0 ? s.playbackRate * 2 : 1;
            s.playing = true;
        }
    }
    if (button("play-pause", s.playing ? IconId::Pause : IconId::Play, s.playing ? "Pause" : "Play") || CommandPressed(Command::PlayPause, bindings, focused))
        s.playing = !s.playing;
    ImGui::SameLine();
    if (button("stop", IconId::Stop, "Stop") || CommandPressed(Command::Stop, bindings, focused)) {
        s.playing = false;
        s.playhead = s.inOut.first;
    }
    ImGui::SameLine();
    if (button("previous-frame", IconId::PreviousFrame, "Previous frame") || CommandPressed(Command::PreviousFrame, bindings, focused))
        s.playhead = FrameToTick(TickToFrame(s.playhead, s.rate) - 1, s.rate);
    ImGui::SameLine();
    if (button("next-frame", IconId::NextFrame, "Next frame") || CommandPressed(Command::NextFrame, bindings, focused))
        s.playhead = FrameToTick(TickToFrame(s.playhead, s.rate) + 1, s.rate);
    ImGui::SameLine();
    if (ImGui::Button("In") || CommandPressed(Command::SetIn, bindings, focused))
        s.inOut.first = (std::min)(s.playhead, s.inOut.last);
    ImGui::SameLine();
    if (ImGui::Button("Out") || CommandPressed(Command::SetOut, bindings, focused))
        s.inOut.last = (std::max)(s.playhead, s.inOut.first);
    ImGui::SameLine();
    if (CommandPressed(Command::Pause, bindings, focused)) s.playing = false;
    if (CommandPressed(Command::Loop, bindings, focused)) s.loop = !s.loop;
    ImGui::Checkbox("Loop", &s.loop);
    char text[32];
    FormatTimecode(s.playhead, s.rate, s.dropFrame, text);
    ImGui::SameLine();
    ImGui::TextUnformatted(text);
}
void TimeRuler(const char *id, TimeState &s, CanvasState &canvas, std::span<const Marker> markers,
               std::uint64_t rev, EventBuffer &out, const Theme &t, float height) {
    auto p = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    ImGui::InvisibleButton(id, {width, height});
    auto *d = ImGui::GetWindowDrawList();
    d->PushClipRect(p, {p.x + width, p.y + height}, true);
    if (!std::isfinite(canvas.scale.x) || canvas.scale.x<=0 || !std::isfinite(canvas.origin.x)) {
        d->PopClipRect();
        return;
    }
    double step = Valid(s.rate) ? static_cast<double>(s.rate.denominator)/s.rate.numerator : 1.;
    char tickLabel[32];
    FormatTimecode(-TicksPerSecond,s.rate,s.dropFrame,tickLabel);
    const float labelSpacing=ImGui::CalcTextSize(tickLabel).x+8;
    while (step * canvas.scale.x < labelSpacing) step *= 2;
    double minor=step;
    while (minor*.5*canvas.scale.x>=8 && minor*.5>=
           (Valid(s.rate)?static_cast<double>(s.rate.denominator)/s.rate.numerator:1.)) minor*=.5;
    const double right=canvas.origin.x+width/canvas.scale.x;
    // Integer indices avoid loss of loop progress at large absolute time positions.
    const double first=std::ceil(canvas.origin.x/minor);
    const int ticks=(std::min)(10000,static_cast<int>(std::ceil(width/(minor*canvas.scale.x)))+1);
    for (int i=0;i<ticks;++i) {
        double second=(first+i)*minor;
        if (second>=right) break;
        float x=p.x+static_cast<float>((second-canvas.origin.x)*canvas.scale.x);
        const bool major=std::abs(second/step-std::round(second/step))<1e-6;
        d->AddLine({x,p.y+height-(major?8:4)},{x,p.y+height},ImGui::GetColorU32(t.colors.muted));
        if (major) {
            char label[32];
            FormatTimecode(FromSeconds(second),s.rate,s.dropFrame,label);
            d->AddText({x+3,p.y},ImGui::GetColorU32(t.colors.muted),label);
        }
    }
    for (auto m : markers) {
        auto x = p.x + static_cast<float>((Seconds(m.tick) - canvas.origin.x) * canvas.scale.x);
        Diamond(d, {x, p.y + height - 5}, ImGui::GetColorU32(t.colors.warning));
    }
    float x = p.x + static_cast<float>((Seconds(s.playhead) - canvas.origin.x) * canvas.scale.x);
    d->AddLine({x, p.y}, {x, p.y + height}, ImGui::GetColorU32(t.colors.accent), 2);
    d->PopClipRect();
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        s.playhead = FromSeconds(canvas.origin.x + (ImGui::GetIO().MousePos.x - p.x) / canvas.scale.x);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        Action(out, 0, rev, EditKind::Marker, {}, Value{s.playhead});
}
Keyframe ResolveHandles(std::span<const Keyframe> keys, std::size_t index) {
    if (index >= keys.size())
        return {};
    Keyframe k = keys[index];
    if (k.handles == HandleMode::Free || k.handles == HandleMode::Aligned)
        return k;
    const auto &previous = index ? keys[index - 1] : k, &next = index + 1 < keys.size() ? keys[index + 1] : k;
    double leftTime = Seconds(k.tick - previous.tick), rightTime = Seconds(next.tick - k.tick);
    double leftSlope = leftTime > 0 ? (k.value - previous.value) / leftTime : 0,
           rightSlope = rightTime > 0 ? (next.value - k.value) / rightTime : 0;
    if (!index)
        leftSlope = rightSlope;
    if (index + 1 == keys.size())
        rightSlope = leftSlope;
    double slope = (leftSlope + rightSlope) * .5;
    if (k.handles == HandleMode::AutoClamped) {
        if (leftSlope * rightSlope <= 0)
            slope = 0;
        else
            slope = std::copysign(
                (std::min)(std::abs(slope), 3 * (std::min)(std::abs(leftSlope), std::abs(rightSlope))),
                slope);
    }
    k.left = {-leftTime / 3, -leftTime / 3 * (k.handles == HandleMode::Vector ? leftSlope : slope)};
    k.right = {rightTime / 3, rightTime / 3 * (k.handles == HandleMode::Vector ? rightSlope : slope)};
    return k;
}
Keyframe MoveHandle(const Keyframe &original, bool left, Point proposed) {
    Keyframe key = original;
    proposed.x = left ? (std::min)(0., proposed.x) : (std::max)(0., proposed.x);
    (left ? key.left : key.right) = proposed;
    if (key.handles == HandleMode::Aligned) {
        auto &opposite = left ? key.right : key.left;
        double length = std::hypot(opposite.x, opposite.y), newLength = std::hypot(proposed.x, proposed.y);
        if (newLength > 1e-12)
            opposite = {-proposed.x * length / newLength, -proposed.y * length / newLength};
    }
    if (key.handles == HandleMode::Auto || key.handles == HandleMode::AutoClamped ||
        key.handles == HandleMode::Vector)
        key.handles = HandleMode::Free;
    return key;
}
double Evaluate(std::span<const Keyframe> keys, Tick tick, Extrapolation extrapolation) {
    if (keys.empty())
        return 0;
    if (keys.size() == 1)
        return keys[0].value;
    auto first = keys.front().tick, last = keys.back().tick;
    if (extrapolation == Extrapolation::Repeat && last > first && (tick < first || tick > last)) {
        Tick length = last - first;
        tick = first + ((tick - first) % length + length) % length;
    }
    auto linear = [](const Keyframe &a, const Keyframe &b, Tick t) {
        return a.value +
               (b.value - a.value) * static_cast<double>(t - a.tick) / static_cast<double>(b.tick - a.tick);
    };
    if (tick <= first)
        return extrapolation == Extrapolation::Linear && keys[1].tick != first
                   ? linear(keys[0], keys[1], tick)
                   : keys.front().value;
    if (tick >= last)
        return extrapolation == Extrapolation::Linear && keys[keys.size() - 2].tick != last
                   ? linear(keys[keys.size() - 2], keys.back(), tick)
                   : keys.back().value;
    auto b = std::upper_bound(keys.begin(), keys.end(), tick,
                              [](Tick t, const Keyframe &k) { return t < k.tick; });
    auto a = b - 1;
    if (a->interpolation == Interpolation::Constant)
        return a->value;
    if (a->interpolation == Interpolation::Linear)
        return linear(*a, *b, tick);
    auto bez = [](double a, double b, double c, double d, double u) {
        double v = 1 - u;
        return v * v * v * a + 3 * v * v * u * b + 3 * v * u * u * c + u * u * u * d;
    };
    auto resolvedA = ResolveHandles(keys, static_cast<std::size_t>(a - keys.begin())),
         resolvedB = ResolveHandles(keys, static_cast<std::size_t>(b - keys.begin()));
    double length = Seconds(b->tick - a->tick), target = Seconds(tick - a->tick);
    double x1 = std::clamp(resolvedA.right.x, 0., length),
           x2 = std::clamp(length + resolvedB.left.x, x1, length), lo = 0, hi = 1;
    for (int i = 0; i < 40; ++i) {
        double u = (lo + hi) * .5;
        if (bez(0, x1, x2, length, u) < target)
            lo = u;
        else
            hi = u;
    }
    return bez(a->value, a->value + resolvedA.right.y, b->value + resolvedB.left.y, b->value, (lo + hi) * .5);
}
void CurveEditor(const char *id, const CurveProvider &provider, CurveState &s, Selection &selection,
                 EventBuffer &out, const Theme &t, ImVec2 size) {
    if (CommandPressed(Command::Fit,s.bindings,ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)))
        s.fitRequested=true;
    if (s.fitRequested && provider.bounds) {
        auto bounds=*provider.bounds;
        if (bounds.max.x==bounds.min.x) {bounds.min.x-=.5;bounds.max.x+=.5;}
        if (bounds.max.y==bounds.min.y) {bounds.min.y-=.5;bounds.max.y+=.5;}
        Fit(s.canvas,bounds,{size.x>0?size.x:ImGui::GetContentRegionAvail().x,
            size.y>0?size.y:ImGui::GetContentRegionAvail().y});
    }
    s.fitRequested=false;
    auto view = BeginCanvas(id, s.canvas, size, t);
    s.view = view;
    detail::ResumeTerminal(s.drag, provider.revision, out);
    DrawGrid(view, s.canvas, {1, 1}, t);
    if (s.drag.active && (provider.revision != s.drag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        s.drag.Cancel(out);
    auto keys = provider.query
                    ? provider.query(provider.user,
                                     {{FromSeconds(view.visible.min.x), FromSeconds(view.visible.max.x)},
                                      -view.visible.max.y,
                                      -view.visible.min.y})
                    : std::span<const Keyframe>{};
    StableId previewChannel=0;
    if (s.drag.active && s.drag.draft.phase!=Phase::Cancel && s.previewKeys.size()>=keys.size()) {
        std::copy(keys.begin(),keys.end(),s.previewKeys.begin());
        auto preview=s.previewKeys.first(keys.size());
        auto key=std::find_if(preview.begin(),preview.end(),[&](const auto &item){return item.id==s.drag.draft.target;});
        if (key!=preview.end()) {
            const auto mouse=ImGui::GetIO().MousePos;
            const double dx=(mouse.x-s.mouseStart.x)/s.canvas.scale.x,dy=-(mouse.y-s.mouseStart.y)/s.canvas.scale.y;
            previewChannel=key->channel;
            if (s.side) {
                auto first=std::find_if(preview.begin(),preview.end(),[&](const auto &item){return item.channel==key->channel;});
                auto last=std::find_if(first,preview.end(),[&](const auto &item){return item.channel!=key->channel;});
                *key=MoveHandle(ResolveHandles({first,last},static_cast<std::size_t>(key-first)),s.side<0,
                    {s.drag.draft.original.x+dx,s.drag.draft.original.y+dy});
            } else {
                key->tick=s.drag.draft.original.first+FromSeconds(dx);
                key->value=s.drag.draft.original.x+dy;
            }
            std::sort(preview.begin(),preview.end(),[](const auto &a,const auto &b){return a.channel!=b.channel?a.channel<b.channel:a.tick<b.tick;});
            keys=preview;
        }
    }
    auto *d = ImGui::GetWindowDrawList();
    StableId hit = 0;
    int side = 0;
    Value initial{};
    if (!s.activeChannel && !keys.empty()) s.activeChannel=keys.front().channel;
    std::size_t channelBegin = 0, channelEnd = 0;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (i == channelEnd) {
            channelBegin = i;
            channelEnd = i + 1;
            while (channelEnd < keys.size() && keys[channelEnd].channel == keys[i].channel)
                ++channelEnd;
        }
        const auto channelKeys = keys.subspan(channelBegin, channelEnd - channelBegin);
        const auto &k = keys[i];
        const bool ghost=s.ghostOtherChannels && s.activeChannel && k.channel!=s.activeChannel;
        auto curveColor=t.colors.accent;
        if (ghost) curveColor.w*=.4f;
        const auto resolved = ResolveHandles(channelKeys, i - channelBegin);
        Point v{Seconds(k.tick), -k.value};
        auto p = Screen(v, s.canvas, view.min);
        if (provider.sample && previewChannel!=k.channel && i==channelBegin) {
            const float width=view.max.x-view.min.x;
            const int segments=(std::max)(1,static_cast<int>(std::ceil(width/4)));
            ImVec2 previous{};
            for (int sample=0;sample<=segments;++sample) {
                const float x=view.min.x+width*sample/segments;
                const Tick tick=FromSeconds(s.canvas.origin.x+(x-view.min.x)/s.canvas.scale.x);
                const double value=provider.sample(provider.user,k.channel,tick,s.extrapolation);
                auto point=Screen({Seconds(tick),-value},s.canvas,view.min);
                if (sample && std::isfinite(point.y) && std::isfinite(previous.y) && (!ghost || sample%2))
                    d->AddLine(previous,point,ImGui::GetColorU32(curveColor),ghost?1.f:1.5f);
                previous=point;
            }
        }
        if ((!provider.sample || previewChannel==k.channel) && i && keys[i - 1].channel == k.channel) {
            auto &prev = keys[i - 1];
            ImVec2 old = Screen({Seconds(prev.tick), -prev.value}, s.canvas, view.min);
            for (int j = 1; j <= 32; ++j) {
                Tick tick = prev.tick + (k.tick - prev.tick) * j / 32;
                auto next =
                    Screen({Seconds(tick), -Evaluate(channelKeys, tick)}, s.canvas, view.min);
                if (!ghost || (j%2)) d->AddLine(old,next,ImGui::GetColorU32(curveColor),ghost?1.f:1.5f);
                old = next;
            }
        }
        auto mouse = ImGui::GetIO().MousePos;
        if (selection.Contains(k.id) && !ghost) {
            for (int h = -1; h <= 1; h += 2) {
                auto offset = h < 0 ? resolved.left : resolved.right;
                auto hp = Screen({v.x + offset.x, v.y - offset.y}, s.canvas, view.min);
                d->AddLine(p, hp, ImGui::GetColorU32(t.colors.muted));
                d->AddCircleFilled(hp, 4, ImGui::GetColorU32(t.colors.text));
                if (!k.locked && std::hypot(mouse.x - hp.x, mouse.y - hp.y) < 7) {
                    hit = k.id;
                    side = h;
                    initial = {k.tick, 0, h, 0, offset.x, offset.y};
                }
            }
        }
        if (ghost) d->AddQuad({p.x,p.y-5},{p.x+5,p.y},{p.x,p.y+5},{p.x-5,p.y},ImGui::GetColorU32(curveColor));
        else Diamond(d, p, ImGui::GetColorU32(selection.Contains(k.id) ? t.colors.warning : t.colors.text));
        if (!k.locked && std::hypot(mouse.x - p.x, mouse.y - p.y) < 7) {
            hit = k.id;
            side = 0;
            initial = {k.tick, 0, 0, 0, k.value};
        }
    }
    ImGui::PushID(id);
    if (view.hovered && ImGui::IsMouseReleased(1) && !s.drag.active) {
        s.contextKey=hit;
        ImGui::OpenPopup("key settings");
    }
    if (ImGui::BeginPopup("key settings")) {
        if (ImGui::MenuItem("Fit all channels",nullptr,false,provider.bounds.has_value())) s.fitRequested=true;
        ImGui::Checkbox("Ghost other channels",&s.ghostOtherChannels);
        if (provider.sample) {
            int mode=static_cast<int>(s.extrapolation);
            if (ImGui::Combo("Extrapolation",&mode,"Constant\0Linear\0Repeat\0"))
                s.extrapolation=static_cast<Extrapolation>(mode);
        }
        auto key=std::find_if(keys.begin(),keys.end(),[&](const auto &value){return value.id==s.contextKey;});
        if (key!=keys.end()) {
            ImGui::BeginDisabled(key->locked);
            if (ImGui::BeginMenu("Interpolation")) {
                const char *names[]={"Constant","Linear","Bezier"};
                for (int i=0;i<3;++i) if (ImGui::MenuItem(names[i],nullptr,static_cast<int>(key->interpolation)==i))
                    Action(out,key->id,provider.revision,EditKind::KeyInterpolation,
                        Value{0,0,0,0,static_cast<double>(key->interpolation)},Value{0,0,0,0,static_cast<double>(i)});
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Handle mode")) {
                const char *names[]={"Auto","Auto Clamped","Vector","Aligned","Free"};
                for (int i=0;i<5;++i) if (ImGui::MenuItem(names[i],nullptr,static_cast<int>(key->handles)==i))
                    Action(out,key->id,provider.revision,EditKind::KeyHandleMode,
                        Value{0,0,0,0,static_cast<double>(key->handles)},Value{0,0,0,0,static_cast<double>(i)});
                ImGui::EndMenu();
            }
            ImGui::EndDisabled();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    if (view.hovered && hit && ImGui::IsMouseClicked(0) && !s.drag.active) {
        auto active=std::find_if(keys.begin(),keys.end(),[&](const auto &key){return key.id==hit;});
        if (active!=keys.end()) s.activeChannel=active->channel;
        if (!selection.Contains(hit) || ImGui::GetIO().KeyCtrl)
            selection.Set(hit, ImGui::GetIO().KeyCtrl);
        s.side = side;
        s.mouseStart = {ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y};
        s.drag.Begin(hit, provider.revision, side ? EditKind::Handle : EditKind::Keyframe, initial,
                     CurrentModifiers(), out);
    }
    if (s.drag.active) {
        auto proposed = s.drag.draft.original;
        auto mouse = ImGui::GetIO().MousePos;
        double dx = (mouse.x - s.mouseStart.x) / s.canvas.scale.x,
               dy = -(mouse.y - s.mouseStart.y) / s.canvas.scale.y;
        if (s.side) {
            proposed.x += dx;
            proposed.y += dy;
        } else {
            proposed.first += FromSeconds(dx);
            proposed.x += dy;
        }
        if (ImGui::IsMouseDown(0) && !(proposed == s.drag.draft.proposed))
            s.drag.Update(provider.revision, proposed, out);
        if (ImGui::IsMouseReleased(0))
            s.drag.Commit(provider.revision, out);
    }
    EndCanvas();
}
void PropertyGrid(const char *id, const PropertyProvider &p, PropertyState &s, EventBuffer &out) {
    detail::ResumeTerminal(s.drag, p.revision, out);
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##search", "Search", s.search, sizeof(s.search));
    if (s.drag.active && (p.revision != s.drag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        s.drag.Cancel(out);
    if (ImGui::BeginTable("properties", 3,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGuiListClipper clipper;
        clipper.Begin(p.count, ImGui::GetFrameHeightWithSpacing());
        while (clipper.Step()) {
            auto rows = p.query ? p.query(p.user, clipper.DisplayStart,
                                          clipper.DisplayEnd - clipper.DisplayStart, s.search)
                                : std::span<const PropertyView>{};
            for (const auto &row : rows) {
                ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(row.id)));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                const bool locked=Flag(row.flags,PropertyFlags::Locked);
                ImGui::Text("%s%s%s%s%s", Flag(row.flags,PropertyFlags::Favorite)?"* ":"",
                    row.label,Flag(row.flags,PropertyFlags::Modified)?" (modified)":"",
                    Flag(row.flags,PropertyFlags::Override)?" (override)":"",locked?" [locked]":"");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", row.category);
                if (ImGui::BeginPopupContextItem("property state")) {
                    for (auto flag:{PropertyFlags::Favorite,PropertyFlags::Locked,PropertyFlags::Override}) {
                        const char *label=flag==PropertyFlags::Favorite?"Favorite":flag==PropertyFlags::Locked?"Locked":"Override";
                        bool enabled=Flag(row.flags,flag);
                        if (ImGui::MenuItem(label,nullptr,enabled))
                            Action(out,row.id,p.revision,EditKind::Toggle,
                                Value{0,0,0,0,static_cast<double>(flag),enabled?1.:0.},
                                Value{0,0,0,0,static_cast<double>(flag),enabled?0.:1.});
                    }
                    ImGui::EndPopup();
                }
                ImGui::TableNextColumn();
                ImGui::BeginDisabled(Flag(row.flags, PropertyFlags::Locked));
                double draft = s.drag.active && s.drag.draft.target == row.id ? s.draft : row.value;
                bool changed =
                    ImGui::DragScalar("##value", ImGuiDataType_Double, &draft, .01f, nullptr, nullptr,
                                      Flag(row.flags, PropertyFlags::Mixed) ? "Mixed" : "%.3f");
                if (ImGui::IsItemActivated()) {
                    s.draft = row.value;
                    s.drag.Begin(row.id, p.revision, EditKind::Property, Value{0, 0, 0, 0, row.value},
                                 CurrentModifiers(), out);
                }
                if (changed && s.drag.active) {
                    s.draft = draft;
                    s.drag.Update(p.revision, Value{0, 0, 0, 0, draft}, out);
                }
                if (ImGui::IsItemDeactivated() && s.drag.active && s.drag.draft.target == row.id)
                    s.drag.Commit(p.revision, out);
                if (ImGui::BeginPopupContextItem("actions")) {
                    if (ImGui::MenuItem("Reset"))
                        Action(out, row.id, p.revision, EditKind::Reset, Value{0, 0, 0, 0, row.value},
                               Value{0, 0, 0, 0, row.defaultValue});
                    if (ImGui::MenuItem("Favorite", nullptr, Flag(row.flags, PropertyFlags::Favorite)))
                        Action(out, row.id, p.revision, EditKind::Toggle, Value{0, 0, 0, 0, 8},
                               Value{0, 0, 0, 0, 8, Flag(row.flags, PropertyFlags::Favorite) ? 0. : 1.});
                    if (ImGui::MenuItem("Previous key"))
                        Action(out, row.id, p.revision, EditKind::PropertyKey, {},
                               Value{s.time,0,static_cast<Tick>(PropertyKeyAction::Previous),0,row.value});
                    if (ImGui::MenuItem("Next key"))
                        Action(out, row.id, p.revision, EditKind::PropertyKey, {},
                               Value{s.time,0,static_cast<Tick>(PropertyKeyAction::Next),0,row.value});
                    ImGui::EndPopup();
                }
                ImGui::TableNextColumn();
                if (s.icons ? IconButton("keyframe",*s.icons,IconId::Keyframe,
                        Flag(row.flags,PropertyFlags::Keyed)?"Remove keyframe":"Add keyframe",{ImGui::GetFontSize()})
                    : ImGui::SmallButton(Flag(row.flags, PropertyFlags::Keyed) ? "<>" : "+"))
                    Action(out, row.id, p.revision, EditKind::PropertyKey, {},
                        Value{s.time,0,static_cast<Tick>(Flag(row.flags,PropertyFlags::Keyed)?
                            PropertyKeyAction::Remove:PropertyKeyAction::Add),0,row.value});
                ImGui::EndDisabled();
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}
void AssetBrowser(const char *id, const AssetProvider &p, AssetState &s, Selection &selection,
                  EventBuffer &out, std::span<const char *const> path) {
    ImGui::PushID(id);
    for (std::size_t i=0;i<path.size();++i) {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::SmallButton(path[i]))
            Action(out,i<s.breadcrumbIds.size()?s.breadcrumbIds[i]:0,p.revision,EditKind::Navigate,{},
                   Value{static_cast<Tick>(i)});
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::TextUnformatted("/");
        ImGui::SameLine();
    }
    if (!path.empty())
        ImGui::NewLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##search", "Search", s.search, sizeof(s.search));
    ImGui::Checkbox("Grid", &s.grid);
    if (p.filteredCount) {
        ImGui::SameLine(); ImGui::SetNextItemWidth(100);
        ImGui::InputTextWithHint("##tag","Tag",s.tag,sizeof(s.tag));
        ImGui::SameLine(); ImGui::SetNextItemWidth(100);
        int status=s.status+1;
        if (ImGui::Combo("##status",&status,"All statuses\0Ready\0Loading\0Proxy\0Missing\0Error\0")) s.status=status-1;
    }
    const int count=p.filteredCount ? (std::max)(0,p.filteredCount(p.user,s.search)) : p.count;
    int columns = s.grid ? (std::max)(1, static_cast<int>(ImGui::GetContentRegionAvail().x / 120)) : 1;
    if (ImGui::BeginChild("items")) {
        ImGuiListClipper clipper;
        clipper.Begin((count + columns - 1) / columns, s.grid ? 144.f : ImGui::GetFrameHeightWithSpacing());
        while (clipper.Step()) {
            auto assets = p.query ? p.query(p.user, clipper.DisplayStart * columns,
                                            (clipper.DisplayEnd - clipper.DisplayStart) * columns, s.search)
                                  : std::span<const AssetView>{};
            int column = 0;
            for (const auto &a : assets) {
                if (column++ % columns)
                    ImGui::SameLine();
                ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(a.id)));
                const auto assetTop=ImGui::GetCursorScreenPos();
                ImGui::BeginGroup();
                if (s.grid && a.thumbnail.GetTexID())
                    ImGui::Image(a.thumbnail, {104, 70});
                else if (s.grid) {
                    auto pos = ImGui::GetCursorScreenPos();
                    ImGui::Dummy({104, 70});
                    ImGui::GetWindowDrawList()->AddRect(pos, {pos.x + 104, pos.y + 70},
                                                        ImGui::GetColorU32(ImGuiCol_Border));
                }
                if (s.renaming == a.id) {
                    ImGui::SetNextItemWidth(s.grid ? 104.f : -1.f);
                    if (ImGui::InputText("##rename", s.rename, sizeof(s.rename),
                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                        Event event{a.id, p.revision, Phase::Commit, EditKind::Rename};
                        std::snprintf(event.originalText.data(), event.originalText.size(), "%s", a.label);
                        std::snprintf(event.proposedText.data(), event.proposedText.size(), "%s", s.rename);
                        if (out.Push(event))
                            s.renaming = 0;
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                        s.renaming = 0;
                } else if (ImGui::Selectable(a.label, selection.Contains(a.id), 0, {s.grid ? 104.f : 0, 0})) {
                    selection.Set(a.id, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyCtrl);
                    Action(out, a.id, p.revision, EditKind::Select);
                }
                if (ImGui::BeginPopupContextItem("asset actions")) {
                    if (ImGui::MenuItem("Rename")) {
                        s.renaming = a.id;
                        std::snprintf(s.rename, sizeof(s.rename), "%s", a.label);
                    }
                    if (ImGui::MenuItem("Duplicate"))
                        Action(out, a.id, p.revision, EditKind::Duplicate);
                    if (ImGui::MenuItem("Remove"))
                        Action(out, a.id, p.revision, EditKind::Remove);
                    ImGui::EndPopup();
                }
                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("IMKIT_ASSET", &a.id, sizeof(a.id));
                    ImGui::TextUnformatted(a.label);
                    ImGui::EndDragDropSource();
                }
                if (a.tag && *a.tag) {
                    if (!s.grid) ImGui::SameLine();
                    ImGui::TextDisabled("%s",a.tag);
                }
                if (a.status != AssetStatus::Ready) {
                    if (!s.grid) ImGui::SameLine();
                    const char *labels[] = {"Ready", "Loading", "Proxy", "Missing", "Error"};
                    ImGui::TextDisabled("%s", labels[static_cast<int>(a.status)]);
                }
                if (s.grid) {
                    ImGui::SetCursorScreenPos({assetTop.x,assetTop.y+144-ImGui::GetStyle().ItemSpacing.y});
                    ImGui::Dummy({104,0});
                }
                ImGui::EndGroup();
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopID();
}
bool Splitter(const char *id, float &pane, float total, bool vertical, float minimum) {
    ImGui::InvisibleButton(id, vertical ? ImVec2{6, ImGui::GetContentRegionAvail().y}
                                        : ImVec2{ImGui::GetContentRegionAvail().x, 6});
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    if (!ImGui::IsItemActive())
        return false;
    pane = std::clamp(pane + (vertical ? ImGui::GetIO().MouseDelta.x : ImGui::GetIO().MouseDelta.y),
                      (std::min)(minimum, total * .5f), (std::max)(minimum, total - minimum));
    return true;
}
void StatusBar(std::string_view text, const Selection &selection) {
    ImGui::Separator();
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::SameLine();
    ImGui::Text("%zu selected", selection.count);
}
} // namespace imkit::editor
