#include <imkit/ptz.h>
#include "component_internal.h"
#include "draw_internal.h"
#include <imkit/widgets.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace imkit {
namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr float PadDeadZone = .30f; // Matches the Home well, so the joystick starts where the well ends.
constexpr float StoreHoldSeconds = .6f;
// Held-move codes kept in PtzState::held.
constexpr int HeldNone = 0, HeldPad = 1, HeldDirection = 2 /* 2..9 */, HeldZoomIn = 10, HeldZoomOut = 11;

struct Direction {
    const char* key;
    const char* fallback;
    ImVec2 unit; // Screen axes, y down, unit length.
};
// Clockwise from north.
const std::array<Direction, 8> Directions = {{
    {"ptz.up", "Up", {0, -1}},
    {"ptz.upRight", "Up right", {.70710678f, -.70710678f}},
    {"ptz.right", "Right", {1, 0}},
    {"ptz.downRight", "Down right", {.70710678f, .70710678f}},
    {"ptz.down", "Down", {0, 1}},
    {"ptz.downLeft", "Down left", {-.70710678f, .70710678f}},
    {"ptz.left", "Left", {-1, 0}},
    {"ptz.upLeft", "Up left", {-.70710678f, -.70710678f}},
}};

const char* Label(const ComponentOptions& o, const char* key, const char* fallback) {
    return o.locale ? o.locale->Text(key, fallback) : fallback;
}
const char* Safe(const char* s) { return s ? s : ""; }
// Stable storage for default preset names: accessibility nodes borrow the text until the frame is published.
const char* PresetName(int index) {
    static const auto names = [] {
        std::array<std::array<char, 12>, 32> table{};
        for (std::size_t i = 0; i < table.size(); ++i)
            std::snprintf(table[i].data(), table[i].size(), "Preset %zu", i + 1);
        return table;
    }();
    return index >= 0 && index < static_cast<int>(names.size()) ? names[static_cast<std::size_t>(index)].data() : "Preset";
}

using detail::Col;
using detail::Ease;
ImVec4 StyleColor(ImGuiCol c) { return ImGui::GetStyleColorVec4(c); }

// Chevron pointing along `unit` (screen axes), centred on p.
void DrawArrow(ImDrawList* draw, ImVec2 p, ImVec2 unit, float size, ImU32 color, float thickness) {
    const ImVec2 perp{-unit.y, unit.x};
    const float tip = size * .45f, back = size * .30f, side = size * .55f;
    draw->PathLineTo({p.x - unit.x * back + perp.x * side, p.y - unit.y * back + perp.y * side});
    draw->PathLineTo({p.x + unit.x * tip, p.y + unit.y * tip});
    draw->PathLineTo({p.x - unit.x * back - perp.x * side, p.y - unit.y * back - perp.y * side});
    draw->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
}
void DrawHome(ImDrawList* draw, ImVec2 c, float size, ImU32 color, float thickness) {
    const float h = size * .5f;
    draw->PathLineTo({c.x - h, c.y - h * .05f});
    draw->PathLineTo({c.x, c.y - h * .95f});
    draw->PathLineTo({c.x + h, c.y - h * .05f});
    draw->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
    draw->PathLineTo({c.x - h * .68f, c.y + h * .06f});
    draw->PathLineTo({c.x - h * .68f, c.y + h * .85f});
    draw->PathLineTo({c.x + h * .68f, c.y + h * .85f});
    draw->PathLineTo({c.x + h * .68f, c.y + h * .06f});
    draw->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
}
// Draws "label value" chips for the readout row; returns the width used.
float DrawChip(ImDrawList* draw, ImVec2 p, float height, const char* letter, const char* value, ImU32 fill, ImU32 letterColor,
               ImU32 valueColor) {
    const float pad = height * .35f, gap = height * .22f;
    const ImVec2 l = ImGui::CalcTextSize(letter), v = ImGui::CalcTextSize(value);
    const float width = pad + l.x + gap + v.x + pad;
    draw->AddRectFilled(p, {p.x + width, p.y + height}, fill, height * .5f);
    draw->AddText({p.x + pad, p.y + (height - l.y) * .5f}, letterColor, letter);
    draw->AddText({p.x + pad + l.x + gap, p.y + (height - v.y) * .5f}, valueColor, value);
    return width;
}
} // namespace

ImVec2 PtzPadVector(ImVec2 point, ImVec2 centre, float radius, float deadZone) {
    if (!(radius > 0) || !std::isfinite(point.x) || !std::isfinite(point.y))
        return {};
    const float dx = point.x - centre.x, dy = point.y - centre.y;
    const float length = std::hypot(dx, dy) / radius;
    deadZone = std::clamp(std::isfinite(deadZone) ? deadZone : 0.f, 0.f, .9f);
    if (length <= deadZone || length <= 0)
        return {};
    const float magnitude = std::min(1.f, (length - deadZone) / (1.f - deadZone));
    const float scale = magnitude / length;
    return {dx / radius * scale, dy / radius * scale};
}

PtzRequest PtzControl(const char* id, const PtzView& view, PtzState& state, PtzOptions layout, ComponentOptions options) {
    PtzRequest request;
    const auto& style = ImGui::GetStyle();
    auto* draw = ImGui::GetWindowDrawList();
    const float u = ImGui::GetFrameHeight();
    const float gap = std::max(4.f, style.ItemSpacing.x);
    const bool compact = layout.compact;
    const bool connected = view.connected;
    const float padSize = layout.diameter > 0 ? layout.diameter : u * (compact ? 4.2f : 6.5f);
    const float radius = padSize * .5f;
    const float totalWidth = std::max(padSize + u * 2, ImGui::GetContentRegionAvail().x);

    ImGui::PushID(id);
    ImGui::BeginGroup();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID heldKey = ImGui::GetID("##held"), padValidKey = ImGui::GetID("##padValid");
    if (!connected)
        ImGui::BeginDisabled();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec4 accent = detail::AccentColor(options);
    const ImU32 text = Col(StyleColor(ImGuiCol_Text)), dim = Col(StyleColor(ImGuiCol_TextDisabled)),
                hairline = Col(StyleColor(ImGuiCol_Border), .8f);
    ImVec4 success = options.theme ? options.theme->semantic.success : ImVec4(.30f, .78f, .50f, 1);
    float y = origin.y;

    // Heading row: camera name on the left, connection dot and status on the right.
    if (!compact && (*Safe(view.label) || *Safe(view.status))) {
        const float h = u * .95f;
        if (*Safe(view.label))
            draw->AddText({origin.x, y + (h - ImGui::GetFontSize()) * .5f}, text, view.label);
        const char* status = *Safe(view.status) ? view.status : Label(options, connected ? "ptz.connected" : "ptz.offline", connected ? "Connected" : "Offline");
        const float statusWidth = ImGui::CalcTextSize(status).x, dot = ImGui::GetFontSize() * .26f;
        const float right = origin.x + totalWidth;
        draw->AddText({right - statusWidth, y + (h - ImGui::GetFontSize()) * .5f}, dim, status);
        draw->AddCircleFilled({right - statusWidth - dot * 2.2f, y + h * .5f}, dot, Col(connected ? success : StyleColor(ImGuiCol_TextDisabled)), 16);
        y += h + style.ItemSpacing.y * .5f;
    }
    // Readout chips.
    if (!compact && layout.readout) {
        const float h = u * .8f;
        const ImU32 chip = Col(StyleColor(ImGuiCol_Button), .9f);
        char value[3][32];
        const PtzPosition& p = view.position;
        if (p.valid) {
            std::snprintf(value[0], sizeof(value[0]), "%+.1f\xC2\xB0", p.pan);
            std::snprintf(value[1], sizeof(value[1]), "%+.1f\xC2\xB0", p.tilt);
            std::snprintf(value[2], sizeof(value[2]), "%.1f\xC3\x97", p.zoom);
        } else
            for (auto& v : value)
                std::snprintf(v, sizeof(v), "\xE2\x80\x94");
        const char* letters[3] = {Label(options, "ptz.pan.short", "P"), Label(options, "ptz.tilt.short", "T"), Label(options, "ptz.zoom.short", "Z")};
        float x = origin.x;
        for (int i = 0; i < 3; ++i)
            x += DrawChip(draw, {x, y}, h, letters[i], value[i], chip, dim, text) + gap * .6f;
        y += h + style.ItemSpacing.y;
    }
    y += compact ? 0.f : style.ItemSpacing.y * .25f;

    // Geometry of the three columns.
    const float railW = u * 1.4f, railH = compact ? padSize : std::min(padSize, u * 3.8f), barW = std::max(4.f, ImGui::GetFontSize() * .3f);
    const bool bar = view.zoomFraction >= 0;
    const ImVec2 padMin{origin.x, y}, centre{padMin.x + radius, padMin.y + radius};
    const ImVec2 railMin{padMin.x + padSize + gap * 1.5f, padMin.y + (padSize - railH) * .5f};
    const float columnEnd = railMin.x + railW + (bar ? gap * .7f + barW : 0.f);
    const float rightX = columnEnd + gap * 1.8f, rightW = origin.x + totalWidth - rightX;
    const bool wantRight = !compact && (layout.speed || (layout.presets && !view.presets.empty()));
    const bool wide = wantRight && rightW >= u * 8.f;
    float bottom = padMin.y + padSize;

    // Pad face and Home well.
    const ImVec4 face = StyleColor(ImGuiCol_FrameBg);
    const float well = radius * PadDeadZone;
    draw->AddCircleFilled(centre, radius, Col(face), 72);
    draw->AddCircle(centre, radius, hairline, 72, 1.f);
    draw->AddCircle(centre, radius * .54f, Col(StyleColor(ImGuiCol_Border), .35f), 72, 1.f);

    // 1. Analog joystick over the whole square; the arrows and Home are submitted later and win overlaps.
    ImGui::SetCursorScreenPos(padMin);
    ImGui::SetNextItemAllowOverlap(); // The arrows and Home are submitted later and must be able to take the hover.
    ImGui::InvisibleButton("##pad", {padSize, padSize});
    const bool padActive = ImGui::IsItemActive();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    if (ImGui::IsItemActivated())
        storage->SetBool(padValidKey, std::hypot(mouse.x - centre.x, mouse.y - centre.y) <= radius);
    detail::SemanticItem(options, Label(options, "ptz.pad", "Pan and tilt pad"), accessibility::SemanticRole::Group,
                         accessibility::SemanticAction::None);

    ImVec2 velocity{};
    int held = HeldNone;
    if (padActive && storage->GetBool(padValidKey) && connected) {
        const ImVec2 v = PtzPadVector(mouse, centre, radius, PadDeadZone);
        velocity = {v.x, -v.y};
        held = HeldPad;
    }
    // 2. Eight arrow buttons at the rim ring; they also give keyboard and screen-reader access.
    const float ringR = radius * .74f, glyph = u * .52f, hit = std::max(u * 1.05f, radius * .34f);
    for (int i = 0; i < 8; ++i) {
        const Direction& d = Directions[static_cast<std::size_t>(i)];
        const ImVec2 p{centre.x + d.unit.x * ringR, centre.y + d.unit.y * ringR};
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos({p.x - hit * .5f, p.y - hit * .5f});
        ImGui::InvisibleButton("##dir", {hit, hit});
        const bool active = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
        const bool pressed = detail::SemanticItem(options, Label(options, d.key, d.fallback), accessibility::SemanticRole::Button,
                                                  accessibility::SemanticAction::Press);
        const float wash = Ease(options, ImGui::GetItemID(), active ? 1.f : hovered ? .5f : 0.f);
        if (wash > 0) {
            const float a = std::atan2(d.unit.y, d.unit.x);
            draw->PathArcTo(centre, radius * .70f, a - Pi / 8.f, a + Pi / 8.f, 10);
            draw->PathStroke(Col(accent, .30f * wash), 0, radius * .40f);
        }
        DrawArrow(draw, p, d.unit, glyph, active ? Col(accent) : Col(StyleColor(ImGuiCol_Text), hovered ? .95f : .62f), std::max(1.6f, u * .07f));
        if (focused)
            draw->AddCircle(p, hit * .52f, Col(accent), 24, 2.f);
        if ((active || pressed) && connected) {
            velocity = {d.unit.x, -d.unit.y};
            held = HeldDirection + i;
        }
        ImGui::PopID();
    }
    // 3. Home well.
    ImGui::SetCursorScreenPos({centre.x - well, centre.y - well});
    const bool home = ImGui::InvisibleButton("##home", {well * 2, well * 2}) && connected;
    {
        const bool active = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
        detail::SemanticItem(options, Label(options, "ptz.home", "Home"), accessibility::SemanticRole::Button,
                             accessibility::SemanticAction::Press);
        const float wash = Ease(options, ImGui::GetItemID(), active ? 1.f : hovered ? .55f : 0.f);
        draw->AddCircleFilled(centre, well, Col(StyleColor(ImGuiCol_WindowBg), .55f), 40);
        draw->AddCircleFilled(centre, well, Col(accent, .22f * wash), 40);
        draw->AddCircle(centre, well, hairline, 40, 1.f);
        DrawHome(draw, centre, well * 1.0f, wash > .6f ? Col(accent) : Col(StyleColor(ImGuiCol_Text), .75f), std::max(1.5f, u * .06f));
        if (focused)
            draw->AddCircle(centre, well + 2.f, Col(accent), 32, 2.f);
        request.home = home;
    }
    // Analog knob follows the pointer, clamped inside the rim.
    if (held == HeldPad) {
        const ImVec2 d{mouse.x - centre.x, mouse.y - centre.y};
        const float length = std::max(1e-3f, std::hypot(d.x, d.y)), reach = std::min(length, radius * .86f);
        const ImVec2 k{centre.x + d.x / length * reach, centre.y + d.y / length * reach};
        draw->AddLine(centre, k, Col(accent, .45f), 2.f);
        draw->AddCircleFilled(k, radius * .15f + 4.f, Col(accent, .25f), 28);
        draw->AddCircleFilled(k, radius * .15f, Col(accent), 28);
    }

    // Zoom rocker: top half zooms in, bottom half zooms out, both while held.
    const ImVec2 railMax{railMin.x + railW, railMin.y + railH};
    draw->AddRectFilled(railMin, railMax, Col(face), railW * .5f);
    draw->AddRect(railMin, railMax, hairline, railW * .5f, 0, 1.f);
    for (int half = 0; half < 2; ++half) {
        const bool zoomIn = half == 0;
        ImGui::PushID(zoomIn ? "zoom-in" : "zoom-out");
        const ImVec2 a{railMin.x, railMin.y + (zoomIn ? 0.f : railH * .5f)}, b{railMax.x, railMin.y + (zoomIn ? railH * .5f : railH)};
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##zoom", {railW, railH * .5f});
        const bool active = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
        const bool pressed = detail::SemanticItem(options, Label(options, zoomIn ? "ptz.zoomIn" : "ptz.zoomOut", zoomIn ? "Zoom in" : "Zoom out"),
                                                  accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
        const float wash = Ease(options, ImGui::GetItemID(), active ? 1.f : hovered ? .5f : 0.f);
        if (wash > 0) {
            draw->PushClipRect(a, b, true);
            draw->AddRectFilled(railMin, railMax, Col(accent, .28f * wash), railW * .5f);
            draw->PopClipRect();
        }
        const ImVec2 c{(a.x + b.x) * .5f, (a.y + b.y) * .5f};
        const float s = u * .24f, t = std::max(1.6f, u * .07f);
        const ImU32 glyphColor = active ? Col(accent) : Col(StyleColor(ImGuiCol_Text), hovered ? .95f : .65f);
        draw->AddLine({c.x - s, c.y}, {c.x + s, c.y}, glyphColor, t);
        if (zoomIn)
            draw->AddLine({c.x, c.y - s}, {c.x, c.y + s}, glyphColor, t);
        if (focused)
            draw->AddRect({a.x + 2, a.y + 2}, {b.x - 2, b.y - 2}, Col(accent), railW * .4f, 0, 2.f);
        if ((active || pressed) && connected) {
            request.zoom = zoomIn ? 1.f : -1.f;
            held = zoomIn ? HeldZoomIn : HeldZoomOut;
        }
        ImGui::PopID();
    }
    draw->AddLine({railMin.x + railW * .22f, railMin.y + railH * .5f}, {railMax.x - railW * .22f, railMin.y + railH * .5f}, hairline, 1.f);
    if (bar) {
        const float x0 = railMax.x + gap * .7f, f = std::clamp(view.zoomFraction, 0.f, 1.f);
        draw->AddRectFilled({x0, railMin.y}, {x0 + barW, railMax.y}, Col(detail::MixColor(face, StyleColor(ImGuiCol_Text), .16f)), barW * .5f);
        draw->AddRectFilled({x0, railMax.y - (railMax.y - railMin.y) * f}, {x0 + barW, railMax.y}, Col(accent, .85f), barW * .5f);
    }

    // Right column (or below): speed and presets.
    float cx = wide ? rightX : origin.x, cy = wide ? padMin.y : padMin.y + padSize + style.ItemSpacing.y * 1.6f;
    const float cw = wide ? rightW : totalWidth;
    if (!compact && layout.speed) {
        const char* speedLabel = Label(options, "ptz.speed", "Speed");
        draw->AddText({cx, cy + (u * .8f - ImGui::GetFontSize()) * .5f}, dim, speedLabel);
        int value = std::clamp(static_cast<int>(std::lround(state.speed * 100.f)), 1, 100);
        cy += u * .85f;
        ImGui::SetCursorScreenPos({cx, cy});
        ImGui::SetNextItemWidth(cw);
        if (imkit::SliderInt("##speed", &value, 1, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
            state.speed = static_cast<float>(value) / 100.f;
            request.speedChanged = true;
        }
        cy += u + style.ItemSpacing.y * 1.2f;
    }
    int recall = -1, store = -1;
    if (!compact && layout.presets && !view.presets.empty()) {
        const int columns = std::clamp(layout.presetColumns, 1, 8), count = static_cast<int>(view.presets.size());
        const char* header = Label(options, "ptz.presets", "Presets");
        draw->AddText({cx, cy + (u * .8f - ImGui::GetFontSize()) * .5f}, dim, header);
        cy += u * .85f;
        const float cell = std::max(u * 1.1f, (cw - gap * .6f * static_cast<float>(columns - 1)) / static_cast<float>(columns));
        const float cellH = std::max(u * 1.15f, cell * .62f);
        ImGuiStorage* storage = ImGui::GetStateStorage();
        for (int i = 0; i < count; ++i) {
            const PtzPreset& preset = view.presets[static_cast<std::size_t>(i)];
            const int col = i % columns, row = i / columns;
            const ImVec2 a{cx + static_cast<float>(col) * (cell + gap * .6f), cy + static_cast<float>(row) * (cellH + gap * .6f)};
            const ImVec2 b{a.x + cell, a.y + cellH};
            ImGui::PushID(i);
            ImGui::SetCursorScreenPos(a);
            const bool clicked = ImGui::InvisibleButton("##preset", {cell, cellH});
            const bool active = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
            const char* fallback = PresetName(i);
            const ImGuiID cellId = ImGui::GetItemID();
            const bool semantic = detail::SemanticItem(options, *Safe(preset.name) ? preset.name : fallback,
                                                       accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
            const bool shift = ImGui::GetIO().KeyShift;
            const float holdTime = active ? ImGui::GetIO().MouseDownDuration[0] : 0.f;
            if (active && holdTime >= StoreHoldSeconds && !storage->GetBool(cellId, false)) {
                storage->SetBool(cellId, true);
                store = i;
            }
            if (clicked || semantic) {
                if (!storage->GetBool(cellId, false)) {
                    if (shift || !preset.stored)
                        store = i;
                    else
                        recall = i;
                }
            }
            if (!active)
                storage->SetBool(cellId, false);
            const bool current = view.activePreset == i;
            const float wash = Ease(options, cellId, active ? 1.f : hovered ? .5f : 0.f);
            const float r = style.FrameRounding;
            if (preset.stored) {
                ImVec4 fill = StyleColor(ImGuiCol_Button);
                fill = detail::MixColor(fill, StyleColor(ImGuiCol_ButtonActive), wash * .8f);
                if (current)
                    fill = detail::MixColor(fill, accent, .22f);
                draw->AddRectFilled(a, b, Col(fill), r);
                draw->AddRect(a, b, current ? Col(accent) : hairline, r, 0, current ? 1.6f : 1.f);
            } else {
                draw->AddRectFilled(a, b, Col(accent, .10f * wash), r);
                detail::DrawDashedRect(draw, a, b, r, Col(StyleColor(ImGuiCol_Border), .9f), 1.f, std::max(2.5f, u * .1f));
            }
            if (active && holdTime > .12f && !storage->GetBool(cellId, false)) {
                const float f = std::min(1.f, holdTime / StoreHoldSeconds);
                draw->PushClipRect(a, {a.x + cell * f, b.y}, true);
                draw->AddRectFilled({a.x, b.y - 3.f}, {b.x, b.y}, Col(accent), 1.5f);
                draw->PopClipRect();
            }
            char number[12];
            std::snprintf(number, sizeof(number), "%d", i + 1);
            const ImVec2 size = ImGui::CalcTextSize(number);
            draw->AddText({(a.x + b.x - size.x) * .5f, (a.y + b.y - size.y) * .5f},
                          current ? Col(accent) : preset.stored ? text : dim, number);
            if (focused)
                draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, Col(accent), r + 2, 0, 2.f);
            if (connected && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
                const char* name = *Safe(preset.name) ? preset.name : fallback;
                ImGui::SetTooltip("%s\n%s", name,
                                  preset.stored ? Label(options, "ptz.presetHint", "Click to recall. Hold or Shift+click to save.")
                                                : Label(options, "ptz.presetEmptyHint", "Empty. Click to save the current position."));
            }
            ImGui::PopID();
        }
        const int rows = (count + columns - 1) / columns;
        cy += static_cast<float>(rows) * (cellH + gap * .6f);
    }
    bottom = std::max(bottom, cy);
    request.recallPreset = connected ? recall : -1;
    request.storePreset = connected ? store : -1;

    // Velocity, scaled by the speed setting; stop edge when a move ends or the camera disconnects.
    const float scale = std::clamp(state.speed, .05f, 1.f);
    request.velocity = {velocity.x * scale, velocity.y * scale};
    request.zoom *= scale;
    request.moving = request.velocity.x != 0 || request.velocity.y != 0 || request.zoom != 0;
    request.stop = storage->GetInt(heldKey) != HeldNone && held == HeldNone;
    storage->SetInt(heldKey, held);

    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy({totalWidth, bottom - origin.y});
    if (!connected)
        ImGui::EndDisabled();
    ImGui::EndGroup();
    ImGui::PopID();
    return request;
}
} // namespace imkit
