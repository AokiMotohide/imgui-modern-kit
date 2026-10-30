#include <imkit/show/surface.h>
#include "internal.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

namespace imkit::show {
using namespace impl;

namespace {
float Fin(float v, float fallback) { return std::isfinite(v) ? v : fallback; }
float Snap1(float v, float step) { return step > 0 ? std::round(v / step) * step : v; }

constexpr const char* PalettePayload = "IMKIT_SURFACE_KIND";

struct Px {
    ImVec2 min, max;
    ImVec2 size() const { return {max.x - min.x, max.y - min.y}; }
};

const SurfaceItem* FindItem(std::span<const SurfaceItem> items, StableId id) {
    for (const SurfaceItem& it : items)
        if (it.id == id && id != 0)
            return &it;
    return nullptr;
}

ImGuiMouseCursor HandleCursor(int handle) {
    switch (handle) {
    case 0: case 4: return ImGuiMouseCursor_ResizeNWSE;
    case 2: case 6: return ImGuiMouseCursor_ResizeNESW;
    case 1: case 5: return ImGuiMouseCursor_ResizeNS;
    default: return ImGuiMouseCursor_ResizeEW;
    }
}

// Draws the real control of an item. Records what the operator did in `request`.
void DrawItem(const SurfaceItem& it, const Px& box, const ShowColors& c, const ComponentOptions& options, SurfaceRequest& request,
              bool editing) {
    const float u = Unit();
    const ImVec2 size = box.size();
    const float round = ControlRound(options);
    auto* draw = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(box.min);
    ImGui::PushID(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(it.id)));
    if (editing)
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (it.disabled && !editing)
        ImGui::BeginDisabled();
    const char* label = Safe(it.label);
    auto operated = [&] { request.operated = it.id; };
    switch (it.kind) {
    case SurfaceKind::Button: {
        ConsoleButtonOptions bo;
        bo.size = size;
        bo.mode = it.mode;
        bo.tone = it.color;
        bo.solid = it.solid;
        bo.labelScale = it.labelScale;
        bo.hint = Safe(it.text);
        bo.binding = it.binding;
        const ConsoleEvent e = ConsoleButton(label[0] ? label : "Key", it.on, bo, options);
        if (e.pressed || e.released || e.changed) {
            operated();
            request.pressed |= e.pressed;
            request.released |= e.released;
            request.toggled |= e.changed;
        }
        break;
    }
    case SurfaceKind::Fader: {
        if (!it.value)
            break;
        FaderOptions fo;
        fo.vertical = size.y >= size.x;
        fo.size = size;
        fo.minimum = it.minimum;
        fo.maximum = it.maximum;
        fo.defaultValue = it.defaultValue;
        fo.binding = it.binding;
        const ValueEdit e = Fader(label, it.value, fo, options);
        if (e) {
            operated();
            request.value = e;
        }
        break;
    }
    case SurfaceKind::Knob: {
        if (!it.value)
            break;
        KnobOptions ko;
        ko.diameter = std::max(u, std::min(size.x, size.y - u * 1.7f));
        ko.minimum = it.minimum;
        ko.maximum = it.maximum;
        ko.defaultValue = it.defaultValue;
        ko.binding = it.binding;
        ImGui::SetCursorScreenPos({box.min.x + std::max(0.f, size.x - ko.diameter) * .5f, box.min.y});
        const ValueEdit e = Knob(label, it.value, ko, options);
        if (e) {
            operated();
            request.value = e;
        }
        break;
    }
    case SurfaceKind::XYPad: {
        if (!it.value || !it.value2)
            break;
        XYPadOptions xo;
        xo.size = size;
        xo.minimumX = xo.minimumY = it.minimum;
        xo.maximumX = xo.maximumY = it.maximum;
        xo.defaultX = xo.defaultY = it.defaultValue;
        const ValueEdit e = XYPad(label, it.value, it.value2, xo, options);
        if (e) {
            operated();
            request.value = e;
        }
        break;
    }
    case SurfaceKind::Meter: {
        if (!it.value)
            break;
        MeterOptions mo;
        mo.size = size;
        mo.vertical = size.y >= size.x;
        if (it.minimum < it.maximum && !(it.minimum == 0.f && it.maximum == 1.f)) {
            mo.minimumDb = it.minimum;
            mo.maximumDb = it.maximum;
        }
        MeterBar(label[0] ? label : "Meter", std::span<const float>(it.value, 1), std::span<const float>(it.value, 1), mo, options);
        break;
    }
    case SurfaceKind::Display: {
        ReadoutItem(size);
        draw->AddRectFilled(box.min, box.max, U32(Recessed(c, .45f)), round);
        draw->AddRect(box.min, box.max, U32(c.border, detail::FrameLineAlpha(.7f)), round, 0, detail::FrameLineWidth(1.f));
        draw->PushClipRect(box.min, box.max, true);
        if (label[0])
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .8f, {box.min.x + u * .4f, box.min.y + u * .25f}, U32(c.muted), label);
        const float fs = std::clamp(size.y * .46f, ImGui::GetFontSize(), ImGui::GetFontSize() * 3.f);
        const char* text = Safe(it.text);
        const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0.f, text);
        draw->AddText(ImGui::GetFont(), fs, {box.min.x + (size.x - ts.x) * .5f, box.min.y + size.y * .5f - ts.y * .5f + (label[0] ? u * .15f : 0.f)},
                      U32(it.color.w > 0 ? it.color : c.text), text);
        draw->PopClipRect();
        AnnotateText(options, label[0] ? label : "Display", text, accessibility::SemanticRole::Status);
        break;
    }
    case SurfaceKind::Image: {
        ImageButtonView v = it.image;
        if (!v.label || !v.label[0])
            v.label = label;
        v.on = it.on ? *it.on : v.on;
        if (it.color.w > 0 && v.accent.w <= 0)
            v.accent = it.color;
        ImageButtonOptions io;
        io.size = size;
        io.mode = it.mode;
        io.editable = !editing;
        io.binding = it.binding;
        const ImageButtonRequest e = ImageButton("image", v, io, options);
        if (e.pressed || e.released || e.changed || e.chooseImage || e.clearImage || e.adjustImage || e.payload) {
            operated();
            request.image = e;
            request.pressed |= e.pressed;
            request.released |= e.released;
            request.toggled |= e.changed;
            if (e.changed && it.on)
                *it.on = !*it.on;
        }
        break;
    }
    case SurfaceKind::Panel: {
        draw->AddRectFilled(box.min, box.max, U32(Mix(c.surface, c.text, .03f)), round);
        draw->AddRect(box.min, box.max, U32(c.border, detail::FrameLineAlpha(.7f)), round, 0, detail::FrameLineWidth(1.f));
        if (label[0]) {
            draw->PushClipRect(box.min, box.max, true);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .82f, {box.min.x + u * .45f, box.min.y + u * .3f}, U32(c.muted), label);
            draw->AddLine({box.min.x + u * .4f, box.min.y + u * 1.1f}, {box.max.x - u * .4f, box.min.y + u * 1.1f}, U32(c.border, .6f), 1.f);
            draw->PopClipRect();
        }
        break;
    }
    case SurfaceKind::Custom: {
        if (!it.draw)
            break;
        ImGui::PushClipRect(box.min, box.max, true);
        it.draw(it, size, it.user);
        ImGui::PopClipRect();
        break;
    }
    }
    if (it.disabled && !editing)
        ImGui::EndDisabled();
    if (editing)
        ImGui::PopItemFlag();
    ImGui::PopID();
}
} // namespace

// ---- Selection -------------------------------------------------------------------------------------------------
bool SurfaceIsSelected(const SurfaceState& state, StableId id) {
    const int n = std::min<int>(state.selectedCount, static_cast<int>(state.selected.size()));
    for (int i = 0; i < n; ++i)
        if (state.selected[static_cast<std::size_t>(i)] == id)
            return true;
    return false;
}

bool SurfaceSelect(SurfaceState& state, StableId id, bool additive) {
    if (id == 0)
        return false;
    if (!additive) {
        state.selected[0] = id;
        state.selectedCount = 1;
        return true;
    }
    const int n = std::min<int>(state.selectedCount, static_cast<int>(state.selected.size()));
    for (int i = 0; i < n; ++i)
        if (state.selected[static_cast<std::size_t>(i)] == id) {
            for (int k = i; k + 1 < n; ++k)
                state.selected[static_cast<std::size_t>(k)] = state.selected[static_cast<std::size_t>(k + 1)];
            state.selectedCount = n - 1;
            return true;
        }
    if (n >= static_cast<int>(state.selected.size()))
        return false;
    state.selected[static_cast<std::size_t>(n)] = id;
    state.selectedCount = n + 1;
    return true;
}

void SurfaceClearSelection(SurfaceState& state) { state.selectedCount = 0; }

// ---- Geometry --------------------------------------------------------------------------------------------------
SurfaceRect SnapRect(SurfaceRect r, float step, int columns, int rows, float minSize) {
    const float cols = static_cast<float>(std::max(1, columns)), rws = static_cast<float>(std::max(1, rows));
    const float minW = std::min(std::max(0.f, minSize), cols), minH = std::min(std::max(0.f, minSize), rws);
    r.w = std::clamp(Snap1(Fin(r.w, minW), step), minW, cols);
    r.h = std::clamp(Snap1(Fin(r.h, minH), step), minH, rws);
    r.x = std::clamp(Snap1(Fin(r.x, 0.f), step), 0.f, cols - r.w);
    r.y = std::clamp(Snap1(Fin(r.y, 0.f), step), 0.f, rws - r.h);
    return r;
}

SurfaceRect ResizeRect(SurfaceRect start, SurfaceHandle handle, ImVec2 delta, float step, float minSize, int columns, int rows) {
    const float cols = static_cast<float>(std::max(1, columns)), rws = static_cast<float>(std::max(1, rows));
    const float minW = std::min(std::max(0.f, minSize), cols), minH = std::min(std::max(0.f, minSize), rws);
    const float dx = Fin(delta.x, 0.f), dy = Fin(delta.y, 0.f);
    float left = Fin(start.x, 0.f), top = Fin(start.y, 0.f);
    float right = left + Fin(start.w, minW), bottom = top + Fin(start.h, minH);
    const bool l = handle == SurfaceHandle::TopLeft || handle == SurfaceHandle::BottomLeft || handle == SurfaceHandle::Left;
    const bool r = handle == SurfaceHandle::TopRight || handle == SurfaceHandle::BottomRight || handle == SurfaceHandle::Right;
    const bool t = handle == SurfaceHandle::TopLeft || handle == SurfaceHandle::TopRight || handle == SurfaceHandle::Top;
    const bool b = handle == SurfaceHandle::BottomLeft || handle == SurfaceHandle::BottomRight || handle == SurfaceHandle::Bottom;
    if (l)
        left = std::clamp(Snap1(left + dx, step), 0.f, right - minW);
    if (r)
        right = std::clamp(Snap1(right + dx, step), left + minW, cols);
    if (t)
        top = std::clamp(Snap1(top + dy, step), 0.f, bottom - minH);
    if (b)
        bottom = std::clamp(Snap1(bottom + dy, step), top + minH, rws);
    return {left, top, right - left, bottom - top};
}

int HitHandle(ImVec2 min, ImVec2 max, ImVec2 point, float handleSize) {
    const float cx = (min.x + max.x) * .5f, cy = (min.y + max.y) * .5f;
    const ImVec2 spots[8] = {{min.x, min.y}, {cx, min.y}, {max.x, min.y}, {max.x, cy}, {max.x, max.y}, {cx, max.y}, {min.x, max.y}, {min.x, cy}};
    const float half = std::max(1.f, handleSize) * .5f;
    int best = -1;
    float bestDistance = FLT_MAX;
    for (int i = 0; i < 8; ++i) {
        const float dx = std::abs(point.x - spots[i].x), dy = std::abs(point.y - spots[i].y);
        if (dx <= half && dy <= half && dx + dy < bestDistance) {
            best = i;
            bestDistance = dx + dy;
        }
    }
    return best;
}

SurfaceGuides AlignGuides(SurfaceRect moving, std::span<const SurfaceRect> others, float threshold) {
    SurfaceGuides g;
    const float mx[3] = {moving.x, moving.x + moving.w * .5f, moving.x + moving.w};
    const float my[3] = {moving.y, moving.y + moving.h * .5f, moving.y + moving.h};
    float bestX = threshold + 1.f, bestY = threshold + 1.f;
    for (const SurfaceRect& o : others) {
        const float ox[3] = {o.x, o.x + o.w * .5f, o.x + o.w};
        const float oy[3] = {o.y, o.y + o.h * .5f, o.y + o.h};
        for (int i = 0; i < 3; ++i)
            for (int k = 0; k < 3; ++k) {
                const float ddx = ox[k] - mx[i], ddy = oy[k] - my[i];
                if (std::abs(ddx) <= threshold && std::abs(ddx) < bestX) {
                    bestX = std::abs(ddx);
                    g.hasX = true;
                    g.dx = ddx;
                    g.x = ox[k];
                }
                if (std::abs(ddy) <= threshold && std::abs(ddy) < bestY) {
                    bestY = std::abs(ddy);
                    g.hasY = true;
                    g.dy = ddy;
                    g.y = oy[k];
                }
            }
    }
    return g;
}

const char* SurfaceKindName(SurfaceKind kind) {
    switch (kind) {
    case SurfaceKind::Button: return "Button";
    case SurfaceKind::Fader: return "Fader";
    case SurfaceKind::Knob: return "Knob";
    case SurfaceKind::XYPad: return "XY pad";
    case SurfaceKind::Meter: return "Meter";
    case SurfaceKind::Display: return "Display";
    case SurfaceKind::Image: return "Image";
    case SurfaceKind::Panel: return "Panel";
    default: return "Custom";
    }
}

// ---- Palette ---------------------------------------------------------------------------------------------------
void SurfacePalette(const char* id, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const SurfaceKind kinds[] = {SurfaceKind::Button, SurfaceKind::Fader, SurfaceKind::Knob, SurfaceKind::XYPad,
                                 SurfaceKind::Meter,  SurfaceKind::Display, SurfaceKind::Image, SurfaceKind::Panel};
    ImGui::PushID(id);
    const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    for (std::size_t i = 0; i < std::size(kinds); ++i) {
        const SurfaceKind kind = kinds[i];
        const char* name = Str(options, FrameText("show.surface.%d", static_cast<int>(kind)), SurfaceKindName(kind));
        const ImVec2 ts = ImGui::CalcTextSize(name);
        const ImVec2 size{ts.x + u * 1.4f, u * 1.3f};
        if (i > 0) {
            if (FitsOnLine(right, size.x, ImGui::GetStyle().ItemSpacing.x))
                ImGui::SameLine();
        }
        ImGui::PushID(static_cast<int>(kind));
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
        ImGui::InvisibleButton("##kind", size);
        const bool hovered = ImGui::IsItemHovered();
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            const int value = static_cast<int>(kind);
            ImGui::SetDragDropPayload(PalettePayload, &value, sizeof(value));
            ImGui::TextUnformatted(name);
            ImGui::EndDragDropSource();
        }
        draw->AddRectFilled(a, b, U32(Mix(c.input, c.text, hovered ? .09f : .03f)), size.y * .4f);
        draw->AddRect(a, b, U32(c.border, detail::FrameLineAlpha(.8f)), size.y * .4f, 0, detail::FrameLineWidth(1.f));
        draw->AddText({a.x + u * .7f, a.y + (size.y - ts.y) * .5f}, U32(c.text), name);
        SemanticItem(options, name, accessibility::SemanticRole::Button, accessibility::SemanticAction::None);
        ImGui::PopID();
    }
    ImGui::PopID();
}

// ---- Surface ---------------------------------------------------------------------------------------------------
SurfaceRequest ControlSurface(const char* id, std::span<const SurfaceItem> items, SurfaceState& state, SurfaceOptions o, ComponentOptions options) {
    SurfaceRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const int cols = std::max(1, o.columns), rows = std::max(1, o.rows);
    state.selectedCount = std::clamp(state.selectedCount, 0, static_cast<int>(state.selected.size()));
    const float step = o.snap ? std::max(0.f, o.snapStep) : 0.f;

    // Viewport and the pixel size of one grid cell.
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 view = o.size;
    float cell;
    if (o.fit) {
        if (view.x <= 0)
            view.x = std::max(u * 4.f, avail.x);
        if (view.y <= 0)
            view.y = view.x * static_cast<float>(rows) / static_cast<float>(cols);
        cell = std::max(4.f, std::min(view.x / static_cast<float>(cols), view.y / static_cast<float>(rows)));
    } else {
        cell = o.cell > 0 ? o.cell : u * 1.4f;
    }
    const ImVec2 extent{cell * static_cast<float>(cols), cell * static_cast<float>(rows)};

    ImGui::PushID(id);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 areaMax{origin.x + extent.x, origin.y + extent.y};
    const float round = ControlRound(options);
    const bool editing = state.editing;
    auto box = [&](const SurfaceRect& r) {
        return Px{{origin.x + r.x * cell, origin.y + r.y * cell}, {origin.x + (r.x + r.w) * cell, origin.y + (r.y + r.h) * cell}};
    };
    auto cellsOf = [&](ImVec2 p) { return ImVec2{(p.x - origin.x) / cell, (p.y - origin.y) / cell}; };

    draw->AddRectFilled(origin, areaMax, U32(Recessed(c, .3f)), round);
    draw->AddRect(origin, areaMax, U32(c.border, detail::FrameLineAlpha(.8f)), round, 0, detail::FrameLineWidth(1.f));
    if (editing && o.showGrid)
        for (int gx = 1; gx < cols; ++gx)
            for (int gy = 1; gy < rows; ++gy)
                draw->AddCircleFilled({origin.x + static_cast<float>(gx) * cell, origin.y + static_cast<float>(gy) * cell}, 1.2f, U32(c.muted, .4f), 6);
    draw->PushClipRect(origin, areaMax, true);

    StableId hoveredId = 0;
    if (editing) {
        // Resize handles of a single unlocked selection take the pointer before the item overlays do.
        const SurfaceItem* single = state.selectedCount == 1 ? FindItem(items, state.selected[0]) : nullptr;
        if (single && !single->locked && !single->hidden) {
            const Px p = box(single->rect);
            const float hs = std::max(12.f, u * .55f);
            const ImVec2 cx{(p.min.x + p.max.x) * .5f, (p.min.y + p.max.y) * .5f};
            const ImVec2 spots[8] = {p.min, {cx.x, p.min.y}, {p.max.x, p.min.y}, {p.max.x, cx.y}, p.max, {cx.x, p.max.y}, {p.min.x, p.max.y}, {p.min.x, cx.y}};
            for (int h = 0; h < 8; ++h) {
                ImGui::SetCursorScreenPos({spots[h].x - hs * .5f, spots[h].y - hs * .5f});
                ImGui::PushID(h + 900000);
                ImGui::InvisibleButton("##handle", {hs, hs});
                if (ImGui::IsItemHovered())
                    ImGui::SetMouseCursor(HandleCursor(h));
                if (ImGui::IsItemActivated()) {
                    state.gesture = 2;
                    state.handle = h;
                    state.dragging = false;
                    state.startMouse = io.MousePos;
                    state.startRects[0] = single->rect;
                }
                ImGui::PopID();
            }
        }
        // Item overlays, topmost first so that the visually front item gets the pointer.
        for (std::size_t n = items.size(); n-- > 0;) {
            const SurfaceItem& it = items[n];
            if (it.id == 0 || it.hidden)
                continue;
            const Px p = box(it.rect);
            ImGui::PushID(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(it.id)));
            ImGui::SetCursorScreenPos(p.min);
            ImGui::InvisibleButton("##edit", p.size());
            if (ImGui::IsItemHovered()) {
                hoveredId = it.id;
                if (state.gesture == 0 && !it.locked)
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
            if (ImGui::IsItemActivated() && state.gesture == 0) {
                state.pressed = it.id;
                const bool additive = io.KeyShift || io.KeyCtrl;
                if (additive) {
                    SurfaceSelect(state, it.id, true);
                    request.selectionChanged = true;
                } else if (!SurfaceIsSelected(state, it.id)) {
                    SurfaceSelect(state, it.id, false);
                    request.selectionChanged = true;
                }
                state.gesture = 1;
                state.dragging = false;
                state.startMouse = io.MousePos;
                for (int k = 0; k < state.selectedCount; ++k) {
                    const SurfaceItem* s = FindItem(items, state.selected[static_cast<std::size_t>(k)]);
                    state.startRects[static_cast<std::size_t>(k)] = s ? s->rect : SurfaceRect{};
                }
            }
            const bool selected = SurfaceIsSelected(state, it.id);
            SemanticItem(options, FrameText("%s, %s", Safe(it.label)[0] ? it.label : SurfaceKindName(it.kind), SurfaceKindName(it.kind)),
                         accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, selected);
            ImGui::PopID();
        }
    }

    // The controls themselves, bottom to top.
    for (const SurfaceItem& it : items) {
        if (it.id == 0 || it.hidden)
            continue;
        DrawItem(it, box(it.rect), c, options, request, editing);
    }

    if (editing) {
        // Empty surface: marquee selection and palette drops.
        ImGui::SetCursorScreenPos(origin);
        ImGui::InvisibleButton("##surface", extent);
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(PalettePayload)) {
                if (p->DataSize == sizeof(int)) {
                    const ImVec2 at = cellsOf(io.MousePos);
                    request.add = true;
                    request.addKind = static_cast<SurfaceKind>(*static_cast<const int*>(p->Data));
                    request.addAt = {std::clamp(Snap1(at.x, step), 0.f, static_cast<float>(cols)), std::clamp(Snap1(at.y, step), 0.f, static_cast<float>(rows))};
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::IsItemActivated() && state.gesture == 0) {
            state.gesture = 3;
            state.dragging = false;
            state.startMouse = io.MousePos;
            if (!(io.KeyShift || io.KeyCtrl) && state.selectedCount > 0) {
                SurfaceClearSelection(state);
                request.selectionChanged = true;
            }
        }
    }

    // Gestures: move, resize and marquee.
    SurfaceGuides guides;
    ImVec2 marqueeMin{}, marqueeMax{};
    bool marquee = false;
    if (editing && state.gesture != 0) {
        const bool down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const ImVec2 d{io.MousePos.x - state.startMouse.x, io.MousePos.y - state.startMouse.y};
        if (!state.dragging && down && d.x * d.x + d.y * d.y >= 9.f)
            state.dragging = true;
        if (state.gesture == 3) {
            marquee = state.dragging && down;
            marqueeMin = {std::min(state.startMouse.x, io.MousePos.x), std::min(state.startMouse.y, io.MousePos.y)};
            marqueeMax = {std::max(state.startMouse.x, io.MousePos.x), std::max(state.startMouse.y, io.MousePos.y)};
            if (!down) {
                if (state.dragging) {
                    if (!(io.KeyShift || io.KeyCtrl))
                        SurfaceClearSelection(state);
                    for (const SurfaceItem& it : items) {
                        if (it.id == 0 || it.hidden)
                            continue;
                        const Px p = box(it.rect);
                        if (p.max.x >= marqueeMin.x && p.min.x <= marqueeMax.x && p.max.y >= marqueeMin.y && p.min.y <= marqueeMax.y &&
                            !SurfaceIsSelected(state, it.id))
                            SurfaceSelect(state, it.id, true);
                    }
                    request.selectionChanged = true;
                }
                state.gesture = 0;
                state.dragging = false;
            }
        } else {
            const bool resize = state.gesture == 2;
            const int count = resize ? 1 : state.selectedCount;
            SurfaceLayoutEdit& edit = request.layout;
            auto fill = [&](bool useStart, ImVec2 deltaCells) {
                edit.resize = resize;
                edit.count = count;
                if (resize) {
                    const SurfaceRect r = useStart ? state.startRects[0]
                                                   : ResizeRect(state.startRects[0], static_cast<SurfaceHandle>(state.handle), deltaCells, step, o.minItemSize, cols, rows);
                    edit.items[0] = {state.selected[0], r};
                    return;
                }
                float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
                for (int k = 0; k < count; ++k) {
                    const SurfaceRect& r = state.startRects[static_cast<std::size_t>(k)];
                    minX = std::min(minX, r.x);
                    minY = std::min(minY, r.y);
                    maxX = std::max(maxX, r.x + r.w);
                    maxY = std::max(maxY, r.y + r.h);
                }
                float dx = useStart ? 0.f : Snap1(deltaCells.x, step), dy = useStart ? 0.f : Snap1(deltaCells.y, step);
                if (!useStart && step > 0) {
                    static thread_local std::vector<SurfaceRect> others;
                    others.clear();
                    for (const SurfaceItem& it : items)
                        if (it.id != 0 && !it.hidden && !SurfaceIsSelected(state, it.id))
                            others.push_back(it.rect);
                    guides = AlignGuides({minX + dx, minY + dy, maxX - minX, maxY - minY}, others, 6.f / cell);
                    if (guides.hasX)
                        dx += guides.dx;
                    if (guides.hasY)
                        dy += guides.dy;
                }
                dx = std::clamp(dx, -minX, static_cast<float>(cols) - maxX);
                dy = std::clamp(dy, -minY, static_cast<float>(rows) - maxY);
                for (int k = 0; k < count; ++k) {
                    SurfaceRect r = state.startRects[static_cast<std::size_t>(k)];
                    const SurfaceItem* it = FindItem(items, state.selected[static_cast<std::size_t>(k)]);
                    if (!it || !it->locked) {
                        r.x += dx;
                        r.y += dy;
                    }
                    edit.items[static_cast<std::size_t>(k)] = {state.selected[static_cast<std::size_t>(k)], r};
                }
            };
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && state.dragging) {
                fill(true, {});
                edit.cancelled = true;
                state.gesture = 0;
                state.dragging = false;
                state.announced = false;
            } else if (down) {
                if (state.dragging) {
                    fill(false, {d.x / cell, d.y / cell});
                    edit.started = !state.announced;
                    edit.changed = true;
                    state.announced = true;
                }
            } else {
                if (state.dragging) {
                    fill(false, {d.x / cell, d.y / cell});
                    edit.started = !state.announced;
                    edit.finished = true;
                    edit.changed = true;
                } else if (state.gesture == 1 && !(io.KeyShift || io.KeyCtrl) && state.selectedCount > 1) {
                    SurfaceSelect(state, state.pressed, false);
                    request.selectionChanged = true;
                }
                state.gesture = 0;
                state.dragging = false;
                state.announced = false;
                state.handle = -1;
            }
        }
    }

    // Keyboard: nudge, delete, duplicate, select all, deselect.
    if (editing && state.gesture == 0 && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !io.WantTextInput) {
        const float nudge = io.KeyShift ? 1.f : (step > 0 ? step : .5f);
        float dx = 0, dy = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
            dx = -nudge;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
            dx = nudge;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            dy = -nudge;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            dy = nudge;
        if ((dx != 0 || dy != 0) && state.selectedCount > 0) {
            SurfaceLayoutEdit& edit = request.layout;
            edit.started = edit.changed = edit.finished = true;
            edit.count = state.selectedCount;
            for (int k = 0; k < state.selectedCount; ++k) {
                const SurfaceItem* it = FindItem(items, state.selected[static_cast<std::size_t>(k)]);
                SurfaceRect r = it ? it->rect : SurfaceRect{};
                if (it && !it->locked) {
                    r.x = std::clamp(r.x + dx, 0.f, static_cast<float>(cols) - r.w);
                    r.y = std::clamp(r.y + dy, 0.f, static_cast<float>(rows) - r.h);
                }
                edit.items[static_cast<std::size_t>(k)] = {state.selected[static_cast<std::size_t>(k)], r};
            }
        }
        if (state.selectedCount > 0 && (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false)))
            request.removeSelected = true;
        if (state.selectedCount > 0 && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
            request.duplicateSelected = true;
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false)) {
            SurfaceClearSelection(state);
            for (const SurfaceItem& it : items)
                if (it.id != 0 && !it.hidden && !SurfaceSelect(state, it.id, true))
                    break;
            request.selectionChanged = true;
        }
        if (state.selectedCount > 0 && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            SurfaceClearSelection(state);
            request.selectionChanged = true;
        }
    }

    // Edit decorations: selection, hover, handles, guides and the marquee.
    if (editing) {
        if (hoveredId != 0 && !SurfaceIsSelected(state, hoveredId))
            if (const SurfaceItem* it = FindItem(items, hoveredId)) {
                const Px p = box(it->rect);
                draw->AddRect(p.min, p.max, U32(c.accent, .45f), round, 0, 1.5f);
            }
        for (int k = 0; k < state.selectedCount; ++k) {
            const SurfaceItem* it = FindItem(items, state.selected[static_cast<std::size_t>(k)]);
            if (!it || it->hidden)
                continue;
            const Px p = box(it->rect);
            draw->AddRect(p.min, p.max, U32(c.accent), round, 0, 2.f);
            if (it->locked)
                GlyphLock(draw, {p.max.x - u * .5f, p.min.y + u * .5f}, u * .6f, U32(c.accent), true);
            if (state.selectedCount == 1 && !it->locked) {
                const ImVec2 cx{(p.min.x + p.max.x) * .5f, (p.min.y + p.max.y) * .5f};
                const ImVec2 spots[8] = {p.min, {cx.x, p.min.y}, {p.max.x, p.min.y}, {p.max.x, cx.y}, p.max, {cx.x, p.max.y}, {p.min.x, p.max.y}, {p.min.x, cx.y}};
                const float hs = std::max(4.f, u * .17f);
                for (const ImVec2& s : spots) {
                    draw->AddRectFilled({s.x - hs, s.y - hs}, {s.x + hs, s.y + hs}, U32(c.surface), 2.f);
                    draw->AddRect({s.x - hs, s.y - hs}, {s.x + hs, s.y + hs}, U32(c.accent), 2.f, 0, 1.5f);
                }
            }
        }
        if (state.dragging && guides.hasX)
            draw->AddLine({origin.x + guides.x * cell, origin.y}, {origin.x + guides.x * cell, areaMax.y}, U32(c.warning, .8f), 1.f);
        if (state.dragging && guides.hasY)
            draw->AddLine({origin.x, origin.y + guides.y * cell}, {areaMax.x, origin.y + guides.y * cell}, U32(c.warning, .8f), 1.f);
        if (marquee) {
            draw->AddRectFilled(marqueeMin, marqueeMax, U32(c.accent, .12f));
            draw->AddRect(marqueeMin, marqueeMax, U32(c.accent, .8f), 0.f, 0, 1.f);
        }
        if (state.dragging && state.gesture != 3 && request.layout.count > 0) {
            const SurfaceRect& r = request.layout.items[0].rect;
            ImGui::SetTooltip("%.1f, %.1f   %.1f x %.1f", static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.w), static_cast<double>(r.h));
        }
    }
    draw->PopClipRect();
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(extent);
    ImGui::PopID();
    return request;
}
} // namespace imkit::show
