#include <imkit/show/beat.h>
#include "internal.h"
#include <imkit/range_slider.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

namespace imkit::show {
using namespace impl;

namespace {
constexpr float Pi = 3.14159265358979323846f;

// Small pill button; returns true on click.
bool Button(const char* id, const char* text, const ShowColors& c, const ComponentOptions& o, ImVec4 ink, ImVec4 tone = {}, ImVec2 size = {}) {
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    const ImVec2 ts = ImGui::CalcTextSize(text, nullptr, true);
    if (size.x <= 0)
        size.x = ts.x + u * 1.0f;
    if (size.y <= 0)
        size.y = u * 1.05f;
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ImGui::InvisibleButton("##btn", size);
    const bool hovered = ImGui::IsItemHovered(), held = ImGui::IsItemActive(), clicked = ImGui::IsItemClicked(), focused = ImGui::IsItemFocused();
    const ImVec4 fill = tone.w > 0 ? Mix(c.input, tone, held ? .45f : hovered ? .3f : .2f) : Mix(c.input, c.text, held ? .14f : hovered ? .09f : .03f);
    draw->AddRectFilled(a, b, U32(fill), size.y * .4f);
    draw->AddRect(a, b, U32(tone.w > 0 ? Mix(c.border, tone, .6f) : c.border, .8f), size.y * .4f, 0, 1.f);
    DrawTextCentered(draw, a, b, U32(ink), text);
    if (focused && ImGui::GetIO().NavVisible)
        draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, U32(c.focus), size.y * .4f + 2.f, 0, 2.f);
    SemanticItem(o, text, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
    ImGui::PopID();
    return clicked;
}
const char* Clock(double seconds) {
    const int t = static_cast<int>(std::max(0.0, seconds));
    return FrameText("%d:%02d", t / 60, t % 60);
}
} // namespace

// ---- Tempo -----------------------------------------------------------------------------------------------------
double TapTempo::Tap(double seconds) {
    if (!std::isfinite(seconds))
        return 0;
    if (count > 0 && seconds - taps[static_cast<std::size_t>(count - 1)] > 2.0)
        count = 0;
    if (count == static_cast<int>(taps.size())) {
        std::rotate(taps.begin(), taps.begin() + 1, taps.end());
        --count;
    }
    taps[static_cast<std::size_t>(count++)] = seconds;
    if (count < 2)
        return 0;
    std::array<double, 7> intervals{};
    const int n = count - 1;
    for (int i = 0; i < n; ++i)
        intervals[static_cast<std::size_t>(i)] = taps[static_cast<std::size_t>(i + 1)] - taps[static_cast<std::size_t>(i)];
    std::array<double, 7> sorted = intervals;
    std::sort(sorted.begin(), sorted.begin() + n);
    const double median = sorted[static_cast<std::size_t>(n / 2)];
    double sum = 0;
    int used = 0;
    for (int i = 0; i < n; ++i)
        if (std::abs(intervals[static_cast<std::size_t>(i)] - median) <= median * .3) {
            sum += intervals[static_cast<std::size_t>(i)];
            ++used;
        }
    return used > 0 && sum > 0 ? 60.0 * used / sum : 0.0;
}
BeatInfo BeatAt(double seconds, double bpm, double firstBeatSeconds, int beatsPerBar) {
    BeatInfo info;
    if (!(bpm > 0) || !std::isfinite(seconds) || !std::isfinite(bpm))
        return info;
    const int bar = std::max(1, beatsPerBar);
    info.beat = (seconds - firstBeatSeconds) * bpm / 60.0;
    const double whole = std::floor(info.beat);
    info.index = static_cast<int>(whole);
    info.phase = static_cast<float>(info.beat - whole);
    info.beatInBar = ((info.index % bar) + bar) % bar + 1;
    return info;
}
double PitchedBpm(double baseBpm, double percent) { return baseBpm * (1.0 + percent / 100.0); }

BeatClockRequest BeatClock(const char* id, const BeatClockView& view, ComponentOptions options) {
    BeatClockRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    ImGui::BeginGroup();
    // Phase ring with the beat number.
    const float d = u * 3.4f;
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::PushID("ring");
    ReadoutItem({d, d});
    const ImVec2 centre{a.x + d * .5f, a.y + d * .5f};
    const float r = d * .5f - 3.f;
    draw->AddCircle(centre, r, U32(c.border), 48, 4.f);
    const float phase = view.running ? Clamp01(view.beat.phase) : 0.f;
    detail::DrawRingArc(draw, centre, r, 4.f, -Pi * .5f, -Pi * .5f + 2.f * Pi * std::max(.02f, phase), U32(view.beat.beatInBar == 1 ? c.accent : c.text));
    const char* number = FrameText("%d", view.running ? view.beat.beatInBar : 0);
    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize() * 1.9f;
    const ImVec2 ns = font->CalcTextSizeA(fs, FLT_MAX, 0.f, number);
    draw->AddText(font, fs, {centre.x - ns.x * .5f, centre.y - ns.y * .5f}, U32(view.running ? c.text : c.muted), number);
    AnnotateText(options, "Beat", number, accessibility::SemanticRole::Status);
    ImGui::PopID();
    ImGui::SameLine(0, u * .8f);
    // BPM.
    ImGui::BeginGroup();
    {
        const float bs = ImGui::GetFontSize() * 2.6f;
        const char* text = FrameText("%.1f", view.bpm);
        const ImVec2 ts = font->CalcTextSizeA(bs, FLT_MAX, 0.f, text);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushID("bpm");
        ReadoutItem({ts.x + u * .2f, ts.y});
        draw->AddText(font, bs, p, U32(c.text), text);
        AnnotateText(options, "BPM", text, accessibility::SemanticRole::Status);
        ImGui::PopID();
        ImGui::TextColored(c.muted, "BPM");
        ImGui::SameLine();
        if (view.source >= 0 && view.source < static_cast<int>(view.sources.size()))
            ImGui::TextColored(view.synced ? c.muted : c.warning, "%s%s", view.sources[static_cast<std::size_t>(view.source)], view.synced ? "" : " (not locked)");
    }
    ImGui::EndGroup();
    ImGui::SameLine(0, u * .8f);
    // Controls.
    ImGui::BeginGroup();
    const bool shift = ImGui::GetIO().KeyShift;
    if (Button("tap", Str(options, "show.tap", "TAP"), c, options, c.text, c.accent, {u * 3.4f, u * 1.4f}))
        request.tap = true;
    ImGui::SameLine();
    if (Button("down", shift ? "-1" : "-0.1", c, options, c.text))
        request.bpmStep = shift ? -1.0 : -.1;
    ImGui::SameLine(0, 3.f);
    if (Button("up", shift ? "+1" : "+0.1", c, options, c.text))
        request.bpmStep = shift ? 1.0 : .1;
    if (Button("resync", Str(options, "show.resync", "Resync"), c, options, c.text))
        request.resync = true;
    ImGui::SameLine();
    if (Button("run", view.running ? Str(options, "show.stop", "Stop") : Str(options, "show.start", "Start"), c, options, c.text))
        request.toggleRunning = true;
    ImGui::SameLine();
    if (!view.sources.empty()) {
        const char* label = view.source >= 0 && view.source < static_cast<int>(view.sources.size()) ? view.sources[static_cast<std::size_t>(view.source)] : "Source";
        if (Button("source", label, c, options, view.synced ? c.text : c.warning, view.synced ? ImVec4{} : c.warning))
            ImGui::OpenPopup("##sources");
        if (ImGui::IsPopupOpen("##sources"))
            ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f});
        if (ImGui::BeginPopup("##sources")) {
            for (int i = 0; i < static_cast<int>(view.sources.size()); ++i)
                if (ImGui::Selectable(view.sources[static_cast<std::size_t>(i)], i == view.source))
                    request.sourceSelected = i;
            ImGui::EndPopup();
        }
    }
    ImGui::EndGroup();
    // Bar dots.
    ImGui::NewLine();
    for (int i = 1; i <= view.beatsPerBar; ++i) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushID(i + 100);
        ReadoutItem({u * .9f, u * .6f});
        ImGui::PopID();
        const bool on = view.running && view.beat.beatInBar == i;
        draw->AddCircleFilled({p.x + u * .3f, p.y + u * .3f}, u * (on ? .22f : .14f), U32(on ? (i == 1 ? c.accent : c.text) : c.border), 14);
        ImGui::SameLine(0, 0);
    }
    ImGui::NewLine();
    ImGui::EndGroup();
    ImGui::PopID();
    return request;
}

// ---- Waveforms -------------------------------------------------------------------------------------------------
namespace {
double Length(const WaveformSpan& t) {
    return t.length > 0 ? t.length : (t.bucketsPerSecond > 0 ? static_cast<double>(t.peaks.size()) / t.bucketsPerSecond : 0.0);
}
float PeakAt(const WaveformSpan& t, double seconds) {
    if (t.peaks.empty() || !(t.bucketsPerSecond > 0) || seconds < 0)
        return 0;
    const std::size_t i = static_cast<std::size_t>(seconds * t.bucketsPerSecond);
    return i < t.peaks.size() ? Clamp01(t.peaks[i]) : 0.f;
}
ImVec4 CueColor(const ShowColors& c, const CueMarker& m, std::size_t index) { return m.color.w > 0 ? m.color : c.layer(index); }
} // namespace

WaveformRequest ScrollingWaveform(const char* id, const WaveformSpan& track, ScrollingWaveformOptions o, ComponentOptions options) {
    WaveformRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = std::max(u * 10.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0)
        size.y = u * 4.f;
    const double window = std::max(1.0, o.windowSeconds);
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ImGui::InvisibleButton("##wave", size);
    const bool hovered = ImGui::IsItemHovered();
    float position = static_cast<float>(track.position);
    // Press on a cue flag selects the cue; anywhere else scrubs.
    const float flagH = u * .9f;
    int flagHit = -1;
    const auto secondsToX = [&](double s) { return a.x + size.x * .5f + static_cast<float>((s - track.position) / window) * size.x; };
    for (std::size_t i = 0; i < track.cues.size(); ++i) {
        const float x = secondsToX(track.cues[i].seconds);
        if (hovered && io.MousePos.y < a.y + flagH + 4.f && std::abs(io.MousePos.x - x) < u * .6f)
            flagHit = static_cast<int>(i);
    }
    const bool cueClick = ImGui::IsItemActivated() && flagHit >= 0;
    if (cueClick)
        request.cueClicked = flagHit;
    EditFrame frame(&position, cueClick);
    ImGuiStorage* st = frame.Storage();
    const ImGuiID anchorXKey = ImGui::GetID("##anchor.x"), anchorPosKey = ImGui::GetID("##anchor.pos");
    if (ImGui::IsItemActivated() && !cueClick) {
        st->SetFloat(anchorXKey, io.MousePos.x);
        st->SetFloat(anchorPosKey, position);
    } else if (frame.Active()) {
        const double length = Length(track);
        const double moved = (io.MousePos.x - st->GetFloat(anchorXKey)) / size.x * window;
        position = static_cast<float>(std::clamp<double>(st->GetFloat(anchorPosKey) - moved, 0.0, length > 0 ? length : 1e9));
    }
    frame.CancelOnEscape();
    request.scrub = frame.Done();

    draw->AddRectFilled(a, b, U32(c.input), u * .3f);
    draw->PushClipRect(a, b, true);
    const double half = window * .5;
    // Loop region.
    if (track.loopIn >= 0 && track.loopOut > track.loopIn)
        draw->AddRectFilled({secondsToX(track.loopIn), a.y}, {secondsToX(track.loopOut), b.y}, U32(c.warning, .16f));
    // Beat grid.
    if (track.bpm > 0) {
        const double beat = 60.0 / track.bpm;
        const long first = static_cast<long>(std::ceil((track.position - half - track.firstBeat) / beat));
        const long last = static_cast<long>(std::floor((track.position + half - track.firstBeat) / beat));
        for (long k = first; k <= last && last - first < 2000; ++k) {
            const float x = secondsToX(track.firstBeat + static_cast<double>(k) * beat);
            const bool bar = ((k % std::max(1, track.beatsPerBar)) + std::max(1, track.beatsPerBar)) % std::max(1, track.beatsPerBar) == 0;
            draw->AddLine({x, a.y + (bar ? flagH : flagH * 1.6f)}, {x, b.y}, U32(c.text, bar ? .35f : .14f), bar ? 1.5f : 1.f);
        }
    }
    // Waveform columns.
    const float mid = (a.y + b.y) * .5f, amp = (size.y - flagH) * .5f - 2.f;
    for (float x = a.x; x < b.x; x += 2.f) {
        const double t = track.position + (static_cast<double>(x - (a.x + size.x * .5f)) / size.x) * window;
        const float peak = PeakAt(track, t);
        if (peak <= 0)
            continue;
        const bool played = t < track.position;
        draw->AddLine({x, mid + flagH * .5f - peak * amp}, {x, mid + flagH * .5f + peak * amp}, U32(played ? Mix(c.accent, c.input, .45f) : c.accent), 1.6f);
    }
    // Cue flags.
    for (std::size_t i = 0; i < track.cues.size(); ++i) {
        const CueMarker& m = track.cues[i];
        const float x = secondsToX(m.seconds);
        if (x < a.x - 40 || x > b.x + 40)
            continue;
        const ImVec4 tone = CueColor(c, m, i);
        draw->AddLine({x, a.y}, {x, b.y}, U32(tone, .8f), 1.5f);
        const char* label = m.label && m.label[0] ? m.label : FrameText("%d", static_cast<int>(i) + 1);
        const ImVec2 ts = ImGui::CalcTextSize(label);
        draw->AddRectFilled({x, a.y}, {x + ts.x + u * .5f, a.y + flagH}, U32(tone), 3.f);
        draw->AddText({x + u * .25f, a.y + (flagH - ts.y) * .5f}, U32(InkFor(c, tone)), label);
    }
    draw->PopClipRect();
    // Playhead.
    draw->AddLine({a.x + size.x * .5f, a.y}, {a.x + size.x * .5f, b.y}, U32(c.text), 2.f);
    draw->AddRect(a, b, U32(frame.Active() ? c.accent : c.border), u * .3f, 0, frame.Active() ? 2.f : 1.f);
    detail::ValueNode node;
    node.name = id;
    node.value = request.scrub.value;
    node.minimum = 0;
    node.maximum = Length(track);
    node.smallChange = 1;
    node.largeChange = 10;
    node.text = FrameText("%s of %s", Clock(track.position), Clock(Length(track)));
    node.disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    if (const int dir = detail::AnnotateValue(options, node); dir != 0) {
        const double length = Length(track);
        position = static_cast<float>(std::clamp<double>(track.position + dir, 0.0, length > 0 ? length : 1e9));
        request.scrub = {true, true, true, false, track.position, position};
    }
    ImGui::PopID();
    return request;
}

WaveformRequest OverviewWaveform(const char* id, const WaveformSpan& track, OverviewWaveformOptions o, ComponentOptions options) {
    WaveformRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = std::max(u * 10.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0)
        size.y = u * 2.f;
    const double length = Length(track);
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ImGui::InvisibleButton("##overview", size);
    float position = static_cast<float>(track.position);
    EditFrame frame(&position);
    if ((ImGui::IsItemActivated() || frame.Active()) && length > 0)
        position = static_cast<float>(std::clamp<double>((io.MousePos.x - a.x) / size.x * length, 0.0, length));
    frame.CancelOnEscape();
    request.scrub = frame.Done();

    draw->AddRectFilled(a, b, U32(c.input), u * .25f);
    if (length > 0) {
        const float mid = (a.y + b.y) * .5f, amp = size.y * .5f - 2.f;
        const float playedX = a.x + static_cast<float>(track.position / length) * size.x;
        for (float x = a.x + 1.f; x < b.x - 1.f; x += 2.f) {
            const float peak = PeakAt(track, static_cast<double>(x - a.x) / size.x * length);
            if (peak > 0)
                draw->AddLine({x, mid - peak * amp}, {x, mid + peak * amp}, U32(x < playedX ? Mix(c.accent, c.input, .45f) : c.accent), 1.6f);
        }
        if (track.loopIn >= 0 && track.loopOut > track.loopIn)
            draw->AddRectFilled({a.x + static_cast<float>(track.loopIn / length) * size.x, a.y}, {a.x + static_cast<float>(track.loopOut / length) * size.x, b.y}, U32(c.warning, .2f));
        for (std::size_t i = 0; i < track.cues.size(); ++i) {
            const float x = a.x + static_cast<float>(track.cues[i].seconds / length) * size.x;
            draw->AddLine({x, a.y}, {x, b.y}, U32(CueColor(c, track.cues[i], i)), 2.f);
        }
        draw->AddLine({playedX, a.y}, {playedX, b.y}, U32(c.text), 2.f);
    }
    draw->AddRect(a, b, U32(c.border), u * .25f, 0, 1.f);
    detail::ValueNode node;
    node.name = id;
    node.value = track.position;
    node.minimum = 0;
    node.maximum = length;
    node.smallChange = 1;
    node.largeChange = 10;
    node.text = FrameText("%s of %s", Clock(track.position), Clock(length));
    if (const int dir = detail::AnnotateValue(options, node); dir != 0)
        request.scrub = {true, true, true, false, track.position, std::clamp<double>(track.position + dir, 0.0, length)};
    ImGui::PopID();
    return request;
}

// ---- Deck ------------------------------------------------------------------------------------------------------
DeckRequest Deck(const char* id, const DeckView& view, float diameter, ComponentOptions options) {
    DeckRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const float d = diameter > 0 ? diameter : u * 8.f;
    const ImVec4 tone = view.color.w > 0 ? view.color : (view.secondDeck ? c.busB : c.busA);
    ImGui::PushID(id);
    ImGui::BeginGroup();
    // Title row.
    ImGui::TextColored(c.text, "%s", view.title && view.title[0] ? view.title : (view.loaded ? "Untitled" : "No track loaded"));
    ImGui::TextColored(c.muted, "%s", Safe(view.artist));
    // Platter.
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 centre{a.x + d * .5f, a.y + d * .5f};
    const float r = d * .5f;
    ImGui::InvisibleButton("##platter", {d, d});
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID angleKey = ImGui::GetID("##angle"), insideKey = ImGui::GetID("##inside");
    const bool active = ImGui::IsItemActive();
    const auto angleOf = [&](ImVec2 m) { return std::atan2(m.y - centre.y, m.x - centre.x); };
    if (ImGui::IsItemActivated()) {
        st->SetBool(insideKey, std::hypot(io.MousePos.x - centre.x, io.MousePos.y - centre.y) <= r);
        st->SetFloat(angleKey, angleOf(io.MousePos));
    }
    if (active && st->GetBool(insideKey)) {
        const float now = angleOf(io.MousePos);
        float delta = now - st->GetFloat(angleKey);
        if (delta > Pi)
            delta -= 2.f * Pi;
        if (delta < -Pi)
            delta += 2.f * Pi;
        request.jogRadians = delta;
        request.scratching = true;
        st->SetFloat(angleKey, now);
    }
    draw->AddCircleFilled(centre, r, U32(c.raised), 72);
    draw->AddCircle(centre, r, U32(c.border), 72, 1.5f);
    // Progress ring.
    const float ringR = r - 5.f;
    draw->AddCircle(centre, ringR, U32(c.border, .6f), 72, 4.f);
    if (view.length > 0)
        detail::DrawRingArc(draw, centre, ringR, 4.f, -Pi * .5f, -Pi * .5f + 2.f * Pi * Clamp01(static_cast<float>(view.position / view.length)), U32(tone));
    // Rotating tick marks.
    const float inner = r * .66f;
    draw->AddCircleFilled(centre, inner, U32(Mix(c.input, c.text, active ? .08f : .03f)), 64);
    for (int i = 0; i < 24; ++i) {
        const float ang = view.platterAngle + static_cast<float>(i) * Pi / 12.f;
        const float len = i == 0 ? r * .2f : r * .08f;
        draw->AddLine({centre.x + std::cos(ang) * (inner - len), centre.y + std::sin(ang) * (inner - len)}, {centre.x + std::cos(ang) * (inner - 2.f), centre.y + std::sin(ang) * (inner - 2.f)},
                      U32(i == 0 ? tone : c.muted, i == 0 ? 1.f : .6f), i == 0 ? 3.f : 1.5f);
    }
    // Centre readout.
    {
        const char* elapsed = Clock(view.position);
        const char* remaining = FrameText("-%s", Clock(std::max(0.0, view.length - view.position)));
        ImFont* font = ImGui::GetFont();
        const float fs = ImGui::GetFontSize() * 1.5f;
        const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, elapsed);
        draw->AddText(font, fs, {centre.x - ts.x * .5f, centre.y - ts.y * .8f}, U32(c.text), elapsed);
        const ImVec2 rs = ImGui::CalcTextSize(remaining);
        draw->AddText({centre.x - rs.x * .5f, centre.y + u * .2f}, U32(c.muted), remaining);
        if (view.bpm > 0) {
            const char* bpm = FrameText("%.1f BPM", view.bpm);
            const ImVec2 bs = ImGui::CalcTextSize(bpm);
            draw->AddText({centre.x - bs.x * .5f, centre.y + u * 1.2f}, U32(tone), bpm);
        }
    }
    detail::ValueNode node;
    node.role = accessibility::SemanticRole::Group;
    node.name = id;
    node.value = view.position;
    node.maximum = view.length;
    node.text = FrameText("%s, %s", Clock(view.position), view.playing ? "playing" : "paused");
    node.readOnly = true;
    detail::AnnotateValue(options, node);

    // Buttons.
    const float bw = (d - u * .5f) / 3.f;
    if (Button("cue", Str(options, "show.cue", "CUE"), c, options, view.cueHeld ? c.onAccent : c.text, view.cueHeld ? tone : ImVec4{}, {bw, u * 1.5f})) {
        request.cuePressed = true;
    }
    request.cueReleased = ImGui::IsItemDeactivated();
    ImGui::SameLine(0, u * .25f);
    if (Button("play", view.playing ? Str(options, "show.pause", "PAUSE") : Str(options, "show.play", "PLAY"), c, options, view.playing ? c.onAccent : c.text, view.playing ? tone : ImVec4{}, {bw, u * 1.5f}))
        request.play = true;
    ImGui::SameLine(0, u * .25f);
    if (Button("sync", Str(options, "show.sync", "SYNC"), c, options, view.sync ? c.onAccent : c.text, view.sync ? c.success : ImVec4{}, {bw, u * 1.5f}))
        request.syncToggled = true;
    if (view.key && view.key[0])
        ImGui::TextColored(c.muted, "%s %s", Str(options, "show.key", "Key"), view.key);
    ImGui::EndGroup();
    ImGui::PopID();
    return request;
}

// ---- Hot cues --------------------------------------------------------------------------------------------------
HotCueEvent HotCuePads(const char* id, std::span<const HotCue> cues, ComponentOptions options) {
    HotCueEvent out;
    std::array<PadView, 8> pads{};
    const std::size_t n = std::min<std::size_t>(cues.size(), pads.size());
    for (std::size_t i = 0; i < n; ++i) {
        PadView& p = pads[i];
        const HotCue& cue = cues[i];
        p.state = cue.set ? PadState::Loaded : PadState::Empty;
        p.label = cue.label && cue.label[0] ? cue.label : (cue.set ? FrameText("Cue %d", static_cast<int>(i) + 1) : "");
        p.caption = cue.set ? Clock(cue.seconds) : "";
        p.color = cue.color;
    }
    PadGridOptions g;
    g.columns = 4;
    const PadEvent e = PadGrid(id, std::span<const PadView>(pads.data(), n), g, options);
    if (e.index >= 0 && static_cast<std::size_t>(e.index) < n) {
        out.index = e.index;
        const bool set = cues[static_cast<std::size_t>(e.index)].set;
        if (e.pressed)
            (set ? out.jump : out.store) = true;
        if (e.secondary && set)
            out.clear = true;
    }
    return out;
}

// ---- Channel strip and pitch -----------------------------------------------------------------------------------
namespace {
bool Kill(const char* id, bool* value, const ShowColors& c, const ComponentOptions& o, const char* name) {
    auto* draw = ImGui::GetWindowDrawList();
    const float s = Unit() * .8f;
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + s, a.y + s};
    ImGui::InvisibleButton("##kill", {s, s});
    bool changed = ImGui::IsItemClicked();
    if (SemanticItem(o, name, accessibility::SemanticRole::Toggle, accessibility::SemanticAction::Toggle, *value))
        changed = true;
    if (changed)
        *value = !*value;
    draw->AddRectFilled(a, b, U32(*value ? c.error : Mix(c.input, c.text, ImGui::IsItemHovered() ? .1f : .04f)), s * .28f);
    if (!*value)
        draw->AddRect(a, b, U32(c.border), s * .28f, 0, 1.f);
    DrawTextCentered(draw, a, b, U32(*value ? InkFor(c, c.error) : c.muted), "K");
    ImGui::PopID();
    return changed;
}
} // namespace

DjChannelRequest DjChannelStrip(const char* id, const DjChannel& ch, float faderHeight, ComponentOptions options) {
    DjChannelRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float h = faderHeight > 0 ? faderHeight : u * 7.f;
    const float width = u * 4.6f;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec4 tone = ch.color.w > 0 ? ch.color : c.layer(0);
    draw->AddRectFilled(a, {a.x + width, a.y + u}, U32(Mix(c.raised, tone, .5f)), 6.f, ImDrawFlags_RoundCornersTop);
    DrawTextFit(draw, {a.x + u * .4f, a.y + (u - ImGui::GetTextLineHeight()) * .5f}, width - u * .8f, U32(InkFor(c, Mix(c.raised, tone, .5f))), Safe(ch.name));
    ImGui::Dummy({width, u * 1.1f});
    const auto knob = [&](const char* name, float* v, float mn, float mx, float def, bool bipolar, const char* fmt, int control, bool* kill) {
        if (!v)
            return;
        KnobOptions ko;
        ko.minimum = mn;
        ko.maximum = mx;
        ko.defaultValue = def;
        ko.bipolar = bipolar;
        ko.format = fmt;
        ko.diameter = u * 1.6f;
        const ValueEdit e = Knob(name, v, ko, options);
        if (e) {
            request.edit = e;
            request.control = control;
            request.toggled = false;
        }
        if (kill) {
            ImGui::SameLine(0, 2.f);
            if (Kill(FrameText("%s kill", name), kill, c, options, FrameText("Kill %s", name))) {
                request.control = control;
                request.toggled = true;
            }
        }
    };
    knob(Str(options, "show.trim", "Trim"), ch.trimDb, -12.f, 12.f, 0.f, true, "%.0f", 0, nullptr);
    knob(Str(options, "show.high", "High"), ch.high, -24.f, 6.f, 0.f, false, "%.0f", 1, ch.killHigh);
    knob(Str(options, "show.mid", "Mid"), ch.mid, -24.f, 6.f, 0.f, false, "%.0f", 2, ch.killMid);
    knob(Str(options, "show.low", "Low"), ch.low, -24.f, 6.f, 0.f, false, "%.0f", 3, ch.killLow);
    knob(Str(options, "show.filter", "Filter"), ch.filter, -1.f, 1.f, 0.f, true, "%.2f", 4, nullptr);
    if (ch.gainDb) {
        const float ticks[] = {6, 0, -12, -48};
        FaderOptions fo;
        fo.scale = FaderScale::Decibel;
        fo.minimum = -60;
        fo.maximum = 6;
        fo.size = {u * 2.6f, h};
        fo.ticks = ticks;
        fo.tickFormat = "%.0f";
        fo.format = "%.1f";
        const ValueEdit e = Fader(FrameText("%s fader", Safe(ch.name)), ch.gainDb, fo, options);
        if (e) {
            request.edit = e;
            request.control = 5;
            request.toggled = false;
        }
        ImGui::SameLine(0, 4.f);
        MeterOptions mo;
        mo.size = {u * 1.1f, h - u * .9f};
        const float silent[1] = {-120.f};
        MeterBar(FrameText("%s meter", Safe(ch.name)), ch.levelsDb.empty() ? std::span<const float>(silent) : ch.levelsDb,
                 ch.peaksDb.empty() ? std::span<const float>(silent) : ch.peaksDb, mo, options);
    }
    if (ch.cue) {
        bool on = *ch.cue;
        if (Button("cue", Str(options, "show.pfl", "CUE"), c, options, on ? c.onAccent : c.text, on ? c.warning : ImVec4{}, {width, u * 1.1f})) {
            *ch.cue = !on;
            request.toggled = true;
            request.control = 6;
        }
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return request;
}

ValueEdit PitchFader(const char* id, float* percent, PitchFaderOptions o, ComponentOptions options, bool* keyLock) {
    ValueEdit edit;
    if (!percent)
        return edit;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    ImGui::PushID(id);
    ImGui::BeginGroup();
    if (o.baseBpm > 0) {
        const double pitched = PitchedBpm(o.baseBpm, *percent);
        const char* text = FrameText("%.1f", pitched);
        const ImVec2 ts = ImGui::CalcTextSize(text);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, (u * 2.4f - ts.x) * .5f));
        ImGui::TextColored(c.text, "%s", text);
    }
    const float ticks[] = {o.range, 0, -o.range};
    FaderOptions fo;
    fo.minimum = -o.range;
    fo.maximum = o.range;
    fo.defaultValue = 0;
    fo.size = {u * 2.4f, o.height > 0 ? o.height : u * 7.f};
    fo.ticks = ticks;
    fo.tickFormat = "%+.0f";
    fo.format = "%+.1f";
    fo.unit = "%";
    fo.step = .1f;
    fo.bipolar = true;
    edit = Fader(Str(options, "show.pitch", "Pitch"), percent, fo, options);
    if (keyLock) {
        bool on = *keyLock;
        if (Button("keylock", Str(options, "show.keyLock", "KEY"), c, options, on ? c.onAccent : c.text, on ? c.accent : ImVec4{}, {u * 2.4f, u * 1.05f}))
            *keyLock = !on;
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return edit;
}

// ---- Step sequencer --------------------------------------------------------------------------------------------
void StepSequencer(const char* id, const SequencerView& view, StepEditBuffer& edits, SequencerOptions o, ComponentOptions options) {
    if (view.lanes <= 0 || view.steps <= 0 || view.velocity.size() < static_cast<std::size_t>(view.lanes) * static_cast<std::size_t>(view.steps))
        return;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const float labelW = u * 5.f, rowH = u * 1.5f, headH = u * .9f;
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = std::max(u * 16.f, ImGui::GetContentRegionAvail().x);
    size.y = headH + rowH * static_cast<float>(view.lanes);
    const float cellW = (size.x - labelW) / static_cast<float>(view.steps);
    ImGui::PushID(id);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 g0{p0.x + labelW, p0.y + headH};
    ImGui::InvisibleButton("##grid", size);
    const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID modeKey = ImGui::GetID("##mode"), lastKey = ImGui::GetID("##last");
    const auto cellAt = [&](ImVec2 m, int& lane, int& step) {
        lane = static_cast<int>(std::floor((m.y - g0.y) / rowH));
        step = static_cast<int>(std::floor((m.x - g0.x) / cellW));
        return lane >= 0 && lane < view.lanes && step >= 0 && step < view.steps;
    };
    const auto velocityAt = [&](int lane, int step) { return view.velocity[static_cast<std::size_t>(lane) * static_cast<std::size_t>(view.steps) + static_cast<std::size_t>(step)]; };
    int lane = 0, step = 0;
    if (ImGui::IsItemActivated() && cellAt(io.MousePos, lane, step)) {
        st->SetBool(modeKey, velocityAt(lane, step) == 0); // true = paint on.
        st->SetInt(lastKey, -1);
    }
    if (active && cellAt(io.MousePos, lane, step)) {
        const int key = lane * 1000 + step;
        if (st->GetInt(lastKey, -1) != key) {
            st->SetInt(lastKey, key);
            const bool paintOn = st->GetBool(modeKey, true);
            const std::uint8_t target = paintOn ? o.defaultVelocity : 0;
            if (velocityAt(lane, step) != target)
                edits.Push({lane, step, target});
        }
    }
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && cellAt(io.MousePos, lane, step) && velocityAt(lane, step) != 0)
        edits.Push({lane, step, 0});
    if (hovered && io.KeyCtrl && io.MouseWheel != 0 && cellAt(io.MousePos, lane, step) && velocityAt(lane, step) != 0) {
        const int v = std::clamp(static_cast<int>(velocityAt(lane, step)) + (io.MouseWheel > 0 ? 8 : -8), 1, 127);
        edits.Push({lane, step, static_cast<std::uint8_t>(v)});
    }

    // Header: step numbers on beats.
    for (int s = 0; s < view.steps; s += std::max(1, view.stepsPerBeat))
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .8f, {g0.x + static_cast<float>(s) * cellW + 2.f, p0.y}, U32(c.muted), FrameText("%d", s / std::max(1, view.stepsPerBeat) + 1));
    std::size_t lit = 0;
    for (int l = 0; l < view.lanes; ++l) {
        const float y = g0.y + static_cast<float>(l) * rowH;
        const char* name = static_cast<std::size_t>(l) < view.laneNames.size() ? view.laneNames[static_cast<std::size_t>(l)] : "";
        draw->AddText({p0.x + u * .3f, y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), Safe(name));
        for (int s = 0; s < view.steps; ++s) {
            const std::uint8_t v = velocityAt(l, s);
            lit += v ? 1u : 0u;
            const bool beat = s % std::max(1, view.stepsPerBeat) == 0;
            const float gap = 2.f;
            const ImVec2 a{g0.x + static_cast<float>(s) * cellW + gap * .5f + (beat && s > 0 ? 2.f : 0.f), y + gap * .5f};
            const ImVec2 b{g0.x + static_cast<float>(s + 1) * cellW - gap * .5f, y + rowH - gap * .5f};
            draw->AddRectFilled(a, b, U32(beat ? Mix(c.input, c.text, .08f) : c.input), 4.f);
            if (v) {
                const float fill = .35f + .65f * static_cast<float>(v) / 127.f;
                draw->AddRectFilled({a.x, b.y - (b.y - a.y) * fill}, b, U32(c.accent), 4.f, fill >= .99f ? ImDrawFlags_None : ImDrawFlags_RoundCornersBottom);
                draw->AddRect(a, b, U32(c.accent), 4.f, 0, 1.f);
            }
            if (view.playhead == s)
                draw->AddRectFilled(a, b, U32(c.text, .2f), 4.f);
        }
    }
    if (hovered && cellAt(io.MousePos, lane, step)) {
        const ImVec2 a{g0.x + static_cast<float>(step) * cellW, g0.y + static_cast<float>(lane) * rowH};
        draw->AddRect(a, {a.x + cellW, a.y + rowH}, U32(c.text, .6f), 4.f, 0, 1.5f);
    }
    if (view.playhead >= 0 && view.playhead < view.steps) {
        const float x = g0.x + (static_cast<float>(view.playhead) + .5f) * cellW;
        draw->AddTriangleFilled({x - 4.f, p0.y + headH - 6.f}, {x + 4.f, p0.y + headH - 6.f}, {x, p0.y + headH - 1.f}, U32(c.accent));
    }
    AnnotateText(options, id, FrameText("%d of %d steps on", static_cast<int>(lit), view.lanes * view.steps), accessibility::SemanticRole::Group);
    ImGui::PopID();
}

// ---- Spectrum --------------------------------------------------------------------------------------------------
ValueEdit Spectrum(const char* id, std::span<const float> bandsDb, std::span<const float> peaksDb, float* bandLow, float* bandHigh, SpectrumOptions o,
                   ComponentOptions options) {
    ValueEdit edit;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = std::max(u * 10.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0)
        size.y = u * 5.f;
    const std::size_t n = std::min<std::size_t>(bandsDb.size(), 128);
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ReadoutItem(size);
    draw->AddRectFilled(a, b, U32(c.input), u * .3f);
    const float lo = o.minimumDb, hi = o.maximumDb > o.minimumDb ? o.maximumDb : o.minimumDb + 1;
    const float low = bandLow ? *bandLow : 0.f, high = bandHigh ? *bandHigh : 1.f;
    float maxDb = -200.f;
    if (n > 0) {
        const float barW = size.x / static_cast<float>(n);
        for (std::size_t i = 0; i < n; ++i) {
            const float db = std::isfinite(bandsDb[i]) ? bandsDb[i] : lo;
            maxDb = std::max(maxDb, db);
            const float t = Clamp01((db - lo) / (hi - lo));
            const float centre = (static_cast<float>(i) + .5f) / static_cast<float>(n);
            const bool inside = bandLow && centre >= low && centre <= high;
            const ImVec4 tone = inside ? c.accent : Mix(c.muted, c.input, .35f);
            const float x0 = a.x + static_cast<float>(i) * barW + 1.f, x1 = a.x + static_cast<float>(i + 1) * barW - 1.f;
            draw->AddRectFilled({x0, b.y - t * (size.y - 4.f)}, {x1, b.y - 2.f}, U32(tone), 2.f);
            if (i < peaksDb.size() && std::isfinite(peaksDb[i])) {
                const float pt = Clamp01((peaksDb[i] - lo) / (hi - lo));
                draw->AddLine({x0, b.y - pt * (size.y - 4.f)}, {x1, b.y - pt * (size.y - 4.f)}, U32(c.text, .8f), 1.5f);
            }
        }
    }
    draw->AddRect(a, b, U32(c.border), u * .3f, 0, 1.f);
    AnnotateText(options, id, FrameText("peak %.0f dB", maxDb), accessibility::SemanticRole::Meter);
    if (bandLow && bandHigh) {
        RangeSliderOptions rs;
        const char* bandLabel = FrameText("%s band", id);
        rs.width = std::max(u * 4.f, size.x - ImGui::CalcTextSize(bandLabel).x - ImGui::GetStyle().ItemInnerSpacing.x);
        rs.minGap = .02f;
        rs.valueLabels = false;
        if (RangeSlider(bandLabel, bandLow, bandHigh, 0.f, 1.f, "%.2f", rs, options)) {
            edit.changed = true;
            edit.value = *bandLow;
        }
    }
    ImGui::PopID();
    return edit;
}
} // namespace imkit::show
