#include "widgets_internal.h"
#include "draw_internal.h"
#include "imgui_internal_compat.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

// Wave 2 (part 3 & 4) widgets: combos, selectables, headers, trees, tabs, progress bars and colour edits.
namespace imkit {
namespace {
constexpr float kPiHalf = 1.57079632679f;

ImU32 ColorU32(const ResolvedPartStyle &r, PartState state, PartColor color, float alphaMul = 1.f) {
    ImVec4 c = r.Color(state, color);
    c.w *= alphaMul;
    return ImGui::GetColorU32(c);
}

void DrawBoxBorder(ImDrawList *draw, ImVec2 min, ImVec2 max, float radius, ImU32 color, float thickness, bool disabled) {
    if (disabled) {
        DrawDashedRect(draw, min, max, radius, color, std::max(1.f, thickness), 4.f);
    } else if (thickness > 0.f) {
        DrawSmoothRectBorder(draw, min, max, color, radius, thickness);
    }
}

const char *FitPreview(const char *text, float width, char (&buffer)[256]) {
    if (!text || ImGui::CalcTextSize(text).x <= width)
        return text;
    const char *ellipsis = "...";
    const float room = std::max(0.f, width - ImGui::CalcTextSize(ellipsis).x);
    std::size_t length = std::min<std::size_t>(std::strlen(text), sizeof(buffer) - 4);
    while (length > 0 && ImGui::CalcTextSize(text, text + length).x > room)
        --length;
    while (length > 0 && (static_cast<unsigned char>(text[length]) & 0xC0) == 0x80)
        --length; // Never cut inside a UTF-8 sequence.
    std::memcpy(buffer, text, length);
    std::strcpy(buffer + length, ellipsis);
    return buffer;
}

const char *ZeroSeparatedItem(void *items, int index) {
    const char *item = static_cast<const char *>(items);
    for (; index > 0 && *item; --index)
        item += std::strlen(item) + 1;
    return *item ? item : nullptr;
}

std::string FormatV(const char *format, va_list args) {
    va_list copy;
    va_copy(copy, args);
    char stackBuf[512];
    const int needed = std::vsnprintf(stackBuf, sizeof(stackBuf), format ? format : "", copy);
    va_end(copy);
    if (needed < 0)
        return {};
    if (static_cast<std::size_t>(needed) < sizeof(stackBuf))
        return std::string(stackBuf, static_cast<std::size_t>(needed));
    std::string out(static_cast<std::size_t>(needed) + 1, '\0');
    std::vsnprintf(out.data(), out.size(), format, args);
    out.resize(static_cast<std::size_t>(needed));
    return out;
}

void DrawTreeIndentGuides(ImDrawList *draw, ImVec2 itemMin, ImVec2 itemMax, ImU32 guideColor) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    const float indentStep = ImGui::GetStyle().IndentSpacing;
    if (indentStep <= 0.f)
        return;
    const float baseX = window->Pos.x + window->WindowPadding.x - window->Scroll.x;
    const float limitX = itemMin.x - 2.f;
    for (float gx = baseX + indentStep * .5f; gx < limitX; gx += indentStep)
        draw->AddLine({gx, itemMin.y}, {gx, itemMax.y}, guideColor, 1.f);
}

bool TreeNodeModern(const void *ptrId, const char *strId, ImGuiTreeNodeFlags flags, const char *text,
                    const TreeStyle &style) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Tree, style);
        const bool open = ptrId ? ImGui::TreeNodeEx(ptrId, flags, "%s", text) : ImGui::TreeNodeEx(strId, flags, "%s", text);
        detail::DrawSelectionMark((flags & ImGuiTreeNodeFlags_Selected) != 0);
        return open;
    }

    const ResolvedPartStyle r = ResolvePartStyle(Part::Tree, style);
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImDrawList *draw = window->DrawList;
    auto &splitter = detail::SharedSplitter();
    splitter.Split(draw, 2);
    splitter.SetCurrentChannel(draw, 1);

    const ImVec4 clear{0, 0, 0, 0};
    ImGui::PushStyleColor(ImGuiCol_Text, clear);
    ImGui::PushStyleColor(ImGuiCol_Header, clear);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, clear);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, clear);
    const float padX = r.Var(PartVar::PaddingX);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {padX, ImGui::GetStyle().FramePadding.y});

    const bool open = ptrId ? ImGui::TreeNodeEx(ptrId, flags, "%s", text) : ImGui::TreeNodeEx(strId, flags, "%s", text);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    splitter.SetCurrentChannel(draw, 0);

    if (ImGui::IsItemVisible()) {
        const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        const ImGuiID id = ImGui::GetItemID();
        const bool disabled = detail::IsCurrentItemDisabled();
        const bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
        const bool hovered = ImGui::IsItemHovered() && !disabled;
        const bool active = ImGui::IsItemActive() && !disabled;
        const float radius = r.Var(PartVar::Radius);
        const float hover01 = AnimatePart(ImHashData(&id, sizeof id, 0x7468u), hovered ? 1.f : 0.f);

        if (selected) {
            DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Selected, PartColor::Fill), radius);
        } else if (active) {
            DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Pressed, PartColor::Fill), radius);
        } else if (hover01 > 0.f) {
            DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Hover, PartColor::Fill, hover01), radius);
        }

        const ImU32 guideCol = ColorU32(r, PartState::Rest, PartColor::Border, .32f);
        DrawTreeIndentGuides(draw, a, b, guideCol);

        const float fontSize = ImGui::GetFontSize();
        const float centerY = (a.y + b.y) * .5f;
        const bool isLeaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
        const bool isBullet = (flags & ImGuiTreeNodeFlags_Bullet) != 0;
        const ImU32 markCol = ColorU32(r, disabled ? PartState::Disabled : PartState::Rest, PartColor::Mark);
        if (!isLeaf && !isBullet) {
            const float angle = AnimatePart(ImHashData(&id, sizeof id, 0x746fu), open ? kPiHalf : 0.f);
            const float chevSize = fontSize * .44f;
            const float markW = std::max(1.f, r.Var(PartVar::MarkWidth));
            DrawChevronRotated(draw, {a.x + fontSize * .5f + padX * .25f, centerY}, chevSize, markCol, angle, markW);
        } else if (isBullet) {
            draw->AddCircleFilled({a.x + fontSize * .5f + padX * .25f, centerY}, fontSize * .14f, markCol, 12);
        }

        const char *textEnd = ImGui::FindRenderedTextEnd(text);
        if (textEnd > text) {
            const float textX = a.x + ImGui::GetTreeNodeToLabelSpacing();
            const float textY = centerY - fontSize * .5f;
            const ImU32 textCol = ColorU32(r, disabled ? PartState::Disabled : PartState::Rest, PartColor::Text);
            draw->AddText(ImGui::GetFont(), fontSize, {textX, textY}, textCol, text, textEnd);
        }

        if (ImGui::IsItemFocused() && GImGui->NavCursorVisible && !disabled)
            DrawFocusRing(draw, a, b, radius, 1.f, r.Var(PartVar::FocusRing));
    }

    splitter.Merge(draw);
    return open;
}

bool SelectableImpl(const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size,
                    const SelectableStyle &style) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Selectable, style);
        const bool pressed = ImGui::Selectable(label, selected, flags, size);
        detail::DrawSelectionMark(selected);
        return pressed;
    }

    const ResolvedPartStyle r = ResolvePartStyle(Part::Selectable, style);
    ImDrawList *draw = ImGui::GetWindowDrawList();
    auto &splitter = detail::SharedSplitter();
    splitter.Split(draw, 2);
    splitter.SetCurrentChannel(draw, 1);

    const ImVec4 clear{0, 0, 0, 0};
    ImGui::PushStyleColor(ImGuiCol_Header, clear);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, clear);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, clear);

    ImVec2 actualSize = size;
    if (actualSize.y <= 0.f && r.varLayers[static_cast<std::size_t>(PartVar::Height)] != PartLayer::Default)
        actualSize.y = r.Var(PartVar::Height);

    const bool pressed = ImGui::Selectable(label, selected, flags, actualSize);

    ImGui::PopStyleColor(3);
    splitter.SetCurrentChannel(draw, 0);

    if (ImGui::IsItemVisible()) {
        const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        const ImGuiID id = ImGui::GetItemID();
        const bool disabled = detail::IsCurrentItemDisabled() || (flags & ImGuiSelectableFlags_Disabled) != 0;
        const bool hovered = ImGui::IsItemHovered() && !disabled;
        const bool active = ImGui::IsItemActive() && !disabled;
        const float radius = r.Var(PartVar::Radius);
        const float hover01 = AnimatePart(ImHashData(&id, sizeof id, 0x7368u), hovered ? 1.f : 0.f);

        if (selected) {
            DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Selected, PartColor::Fill), radius);
            if (hover01 > 0.f)
                DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Hover, PartColor::Fill, hover01 * .35f), radius);
        } else if (active) {
            DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Pressed, PartColor::Fill), radius);
        } else if (hover01 > 0.f) {
            DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Hover, PartColor::Fill, hover01), radius);
        }

        if (selected) {
            const float markW = std::max(1.f, r.Var(PartVar::MarkWidth));
            const float h = b.y - a.y;
            const float insetY = std::max(2.f, h * .2f);
            const ImVec2 barMin{a.x + 2.f, a.y + insetY};
            const ImVec2 barMax{a.x + 2.f + markW, b.y - insetY};
            if (barMax.y > barMin.y)
                DrawSmoothRect(draw, barMin, barMax, ColorU32(r, PartState::Selected, PartColor::Mark), markW * .5f);
        }

        if (ImGui::IsItemFocused() && GImGui->NavCursorVisible && !disabled)
            DrawFocusRing(draw, a, b, radius, 1.f, r.Var(PartVar::FocusRing));
    }

    splitter.Merge(draw);
    return pressed;
}

bool CollapsingHeaderImpl(const char *label, bool *visible, ImGuiTreeNodeFlags flags, const HeaderStyle &style) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Header, style);
        return visible ? ImGui::CollapsingHeader(label, visible, flags) : ImGui::CollapsingHeader(label, flags);
    }

    if (visible && !*visible)
        return false;

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ResolvedPartStyle r = ResolvePartStyle(Part::Header, style);
    const ImGuiID id = window->GetID(label);
    ImGuiStorage *storage = window->DC.StateStorage;
    bool open = storage->GetInt(id, (flags & ImGuiTreeNodeFlags_DefaultOpen) ? 1 : 0) != 0;
    if (GImGui->NextItemData.HasFlags & ImGuiNextItemDataFlags_HasOpen) {
        open = GImGui->NextItemData.OpenVal;
        storage->SetInt(id, open ? 1 : 0);
        GImGui->NextItemData.ClearFlags();
    }

    const float padX = r.Var(PartVar::PaddingX);
    const float fontSize = ImGui::GetFontSize();
    const float height = std::max(r.Var(PartVar::Height), fontSize + r.Var(PartVar::PaddingY) * 2.f);
    const float width = std::max(r.Var(PartVar::MinWidth), ImGui::GetContentRegionAvail().x);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max{min.x + width, min.y + height};

    const InteractFlags interactFlags = InteractFlags_Layout | (visible ? InteractFlags_AllowOverlap : 0);
    const InteractResult res = Interact(id, min, max, interactFlags);

    InteractResult closeRes;
    const float closeSide = std::max(12.f, fontSize * .9f);
    if (visible) {
        ImGui::PushID(label);
        const ImGuiID closeId = window->GetID("##close");
        ImGui::PopID();
        const ImVec2 cMin{max.x - padX * .5f - closeSide, min.y + (height - closeSide) * .5f};
        const ImVec2 cMax{cMin.x + closeSide, cMin.y + closeSide};
        closeRes = Interact(closeId, cMin, cMax, InteractFlags_NoNavFocus);
        if (closeRes.pressed)
            *visible = false;
    }

    if (res.pressed && !closeRes.hovered && !(flags & ImGuiTreeNodeFlags_Leaf)) {
        open = !open;
        storage->SetInt(id, open ? 1 : 0);
    }

    if (res.visible) {
        ImDrawList *draw = window->DrawList;
        const float radius = r.Var(PartVar::Radius);
        const float hoverAlpha = res.held ? .9f : res.hover01;
        if (res.held) {
            DrawSmoothRect(draw, min, max, ColorU32(r, PartState::Pressed, PartColor::Fill, .45f), radius);
        } else if (hoverAlpha > 0.f) {
            DrawSmoothRect(draw, min, max, ColorU32(r, PartState::Hover, PartColor::Fill, hoverAlpha * .45f), radius);
        }

        const float lineW = std::max(1.f, r.Var(PartVar::BorderWidth));
        const ImU32 lineCol = ColorU32(r, res.disabled ? PartState::Disabled : PartState::Rest, PartColor::Border, .38f);
        draw->AddLine({min.x, max.y - lineW * .5f}, {max.x, max.y - lineW * .5f}, lineCol, lineW);

        const float angle = AnimatePart(ImHashData(&id, sizeof id, 0x6865u), open ? kPiHalf : 0.f);
        const float chevSize = fontSize * .46f;
        const float markW = std::max(1.f, r.Var(PartVar::MarkWidth));
        const float centerY = (min.y + max.y) * .5f;
        const ImU32 markCol = ColorU32(r, res.disabled ? PartState::Disabled : PartState::Rest, PartColor::Mark);
        DrawChevronRotated(draw, {min.x + padX * .5f + chevSize * .5f, centerY}, chevSize, markCol, angle, markW);

        const char *textEnd = ImGui::FindRenderedTextEnd(label);
        if (textEnd > label) {
            const float textX = min.x + padX * .5f + chevSize + r.Var(PartVar::Gap) + 4.f;
            const float textY = centerY - fontSize * .5f;
            const ImU32 textCol = ColorU32(r, res.disabled ? PartState::Disabled : PartState::Rest, PartColor::Text);
            draw->AddText(ImGui::GetFont(), fontSize, {textX, textY}, textCol, label, textEnd);
        }

        if (visible && closeRes.visible) {
            if (closeRes.hover01 > 0.f) {
                const ImVec2 cCenter{(closeRes.min.x + closeRes.max.x) * .5f, (closeRes.min.y + closeRes.max.y) * .5f};
                draw->AddCircleFilled(cCenter, closeSide * .5f,
                                      ColorU32(r, PartState::Hover, PartColor::Fill, closeRes.hover01), 16);
            }
            const ImVec2 cCenter{(closeRes.min.x + closeRes.max.x) * .5f, (closeRes.min.y + closeRes.max.y) * .5f};
            detail::DrawCross(draw, cCenter, closeSide * .42f,
                              ColorU32(r, closeRes.disabled ? PartState::Disabled : PartState::Rest, PartColor::Text),
                              1.4f);
        }

        if (res.focusVisible)
            DrawFocusRing(draw, min, max, radius, res.focus01, r.Var(PartVar::FocusRing));
    }

    return open;
}

int ParseHexByte(const char *p) {
    auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    };
    const int hi = hexVal(p[0]), lo = hexVal(p[1]);
    return (hi >= 0 && lo >= 0) ? (hi << 4) | lo : -1;
}

bool ColorEditImpl(const char *label, float *col, bool hasAlpha, ImGuiColorEditFlags flags,
                   const ColorEditStyle &style) {
    constexpr ImGuiColorEditFlags stockOnly =
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_PickerHueBar |
        ImGuiColorEditFlags_PickerHueWheel;
    if (detail::ClassicLook() || (flags & (stockOnly & ~ImGuiColorEditFlags_DisplayHex))) {
        detail::WidgetStyleScope scope(Part::ColorEdit, style);
        return hasAlpha ? ImGui::ColorEdit4(label, col, flags) : ImGui::ColorEdit3(label, col, flags);
    }

    const ResolvedPartStyle r = ResolvePartStyle(Part::ColorEdit, style);
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    bool changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(label);

    const float boxSize = std::max(14.f, r.Var(PartVar::BoxSize));
    const float height = std::max(r.Var(PartVar::Height), boxSize);
    const float radius = r.Var(PartVar::Radius);
    const float borderW = r.Var(PartVar::BorderWidth);
    const float gap = r.Var(PartVar::Gap);
    const bool noPicker = (flags & ImGuiColorEditFlags_NoPicker) != 0;

    bool openPicker = false;
    if (!(flags & ImGuiColorEditFlags_NoSmallPreview)) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float yOff = (height - boxSize) * .5f;
        const ImVec2 swMin{pos.x, pos.y + yOff};
        const ImVec2 swMax{pos.x + boxSize, swMin.y + boxSize};
        const ImGuiID swId = window->GetID("##swatch");
        ImGui::ItemSize({boxSize, height});
        const InteractResult sw = Interact(swId, swMin, swMax, InteractFlags_None);
        if (sw.visible) {
            ImDrawList *draw = window->DrawList;
            const float swRadius = std::min(radius, boxSize * .35f);
            const ImVec4 preview{col[0], col[1], col[2], hasAlpha ? col[3] : 1.f};
            if (hasAlpha && col[3] < 1.f) {
                DrawSmoothRect(draw, swMin, swMax, ImGui::GetColorU32(ImVec4(.35f, .35f, .35f, 1.f)), swRadius);
            }
            DrawSmoothRect(draw, swMin, swMax, ImGui::GetColorU32(preview), swRadius);
            DrawBoxBorder(draw, swMin, swMax, swRadius,
                          ColorU32(r, sw.disabled ? PartState::Disabled : (sw.hovered ? PartState::Hover : PartState::Rest),
                                   PartColor::Border),
                          std::max(1.f, borderW), sw.disabled);
            if (sw.focusVisible)
                DrawFocusRing(draw, swMin, swMax, swRadius, sw.focus01, r.Var(PartVar::FocusRing));
        }
        if (sw.pressed && !noPicker)
            openPicker = true;
    }

    if (!(flags & ImGuiColorEditFlags_NoInputs)) {
        if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
            ImGui::SameLine(0.f, gap);

        const int r8 = std::clamp(static_cast<int>(std::round(col[0] * 255.f)), 0, 255);
        const int g8 = std::clamp(static_cast<int>(std::round(col[1] * 255.f)), 0, 255);
        const int b8 = std::clamp(static_cast<int>(std::round(col[2] * 255.f)), 0, 255);
        const int a8 = hasAlpha ? std::clamp(static_cast<int>(std::round(col[3] * 255.f)), 0, 255) : 255;

        char hexBuf[16];
        if (hasAlpha)
            std::snprintf(hexBuf, sizeof(hexBuf), "#%02X%02X%02X%02X", r8, g8, b8, a8);
        else
            std::snprintf(hexBuf, sizeof(hexBuf), "#%02X%02X%02X", r8, g8, b8);

        const float padX = r.Var(PartVar::PaddingX);
        const float minW = std::max(r.Var(PartVar::MinWidth),
                                    ImGui::CalcTextSize(hasAlpha ? "#FFFFFFFF" : "#FFFFFF").x + padX * 2.f);
        const ImVec2 fMin = ImGui::GetCursorScreenPos();
        const ImVec2 fMax{fMin.x + minW, fMin.y + height};

        ImDrawList *draw = window->DrawList;
        auto &splitter = detail::SharedSplitter();
        splitter.Split(draw, 2);
        splitter.SetCurrentChannel(draw, 1);

        const ImVec4 clear{0, 0, 0, 0};
        ImGui::PushStyleColor(ImGuiCol_FrameBg, clear);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, clear);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, clear);
        ImGui::PushStyleColor(ImGuiCol_Border, clear);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            {padX, std::max(0.f, (height - ImGui::GetFontSize()) * .5f)});
        ImGui::SetNextItemWidth(minW);
        if (ImGui::InputText("##hex", hexBuf, sizeof(hexBuf),
                             ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue)) {
            const char *digits = hexBuf[0] == '#' ? hexBuf + 1 : hexBuf;
            const std::size_t len = std::strlen(digits);
            if (len == 6 || (hasAlpha && len == 8)) {
                const int pr = ParseHexByte(digits), pg = ParseHexByte(digits + 2), pb = ParseHexByte(digits + 4);
                const int pa = len == 8 ? ParseHexByte(digits + 6) : 255;
                if (pr >= 0 && pg >= 0 && pb >= 0 && pa >= 0) {
                    col[0] = static_cast<float>(pr) / 255.f;
                    col[1] = static_cast<float>(pg) / 255.f;
                    col[2] = static_cast<float>(pb) / 255.f;
                    if (hasAlpha)
                        col[3] = static_cast<float>(pa) / 255.f;
                    changed = true;
                }
            }
        }
        const bool hexDisabled = detail::IsCurrentItemDisabled();
        const bool hexHovered = ImGui::IsItemHovered() && !hexDisabled;
        const bool hexActive = ImGui::IsItemActive() && !hexDisabled;
        const bool hexFocused = ImGui::IsItemFocused() && GImGui->NavCursorVisible && !hexDisabled;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        splitter.SetCurrentChannel(draw, 0);
        const PartState st = hexDisabled ? PartState::Disabled
                           : hexActive   ? PartState::Focused
                           : hexHovered  ? PartState::Hover
                                         : PartState::Rest;
        DrawSmoothRect(draw, fMin, fMax, ColorU32(r, st, PartColor::Fill), radius);
        DrawBoxBorder(draw, fMin, fMax, radius, ColorU32(r, st, PartColor::Border), borderW, hexDisabled);
        if (hexFocused)
            DrawFocusRing(draw, fMin, fMax, radius, 1.f, r.Var(PartVar::FocusRing));
        splitter.Merge(draw);
    }

    if (openPicker)
        ImGui::OpenPopup("##picker");

    if (ImGui::BeginPopup("##picker")) {
        const ImGuiColorEditFlags pickerFlags = (flags & ~ImGuiColorEditFlags_NoLabel) | ImGuiColorEditFlags_NoSidePreview;
        changed |= hasAlpha ? ImGui::ColorPicker4("##p", col, pickerFlags) : ImGui::ColorPicker3("##p", col, pickerFlags);
        ImGui::EndPopup();
    }

    if (!(flags & ImGuiColorEditFlags_NoLabel)) {
        const char *textEnd = ImGui::FindRenderedTextEnd(label);
        if (textEnd > label) {
            ImGui::SameLine(0.f, gap);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label, textEnd);
        }
    }

    ImGui::PopID();
    ImGui::EndGroup();
    return changed;
}
} // namespace

bool BeginCombo(const char *label, const char *preview, ImGuiComboFlags flags) {
    return imkit::BeginCombo(label, preview, flags, ComboStyle{});
}
bool BeginCombo(const char *label, const char *preview, ImGuiComboFlags flags, const ComboStyle &style) {
    constexpr ImGuiComboFlags native =
        ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview | ImGuiComboFlags_WidthFitPreview;
    if (detail::ClassicLook() || (flags & native)) {
        detail::WidgetStyleScope scope(Part::Combo, style);
        return ImGui::BeginCombo(label, preview, flags);
    }

    const ResolvedPartStyle r = ResolvePartStyle(Part::Combo, style);
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImDrawList *draw = window->DrawList;
    const ImGuiID comboId = window->GetID(label);

    const float padX = r.Var(PartVar::PaddingX);
    const float height = std::max(r.Var(PartVar::Height), ImGui::GetFontSize());
    const float calcW = ImGui::CalcItemWidth();
    const float width = std::max(calcW, r.Var(PartVar::MinWidth));
    if (width > calcW)
        ImGui::SetNextItemWidth(width);

    const ImVec2 frameMin = ImGui::GetCursorScreenPos();
    const ImVec2 frameMax{frameMin.x + width, frameMin.y + height};

    char buffer[256];
    const char *shown = FitPreview(preview, std::max(0.f, width - height - padX), buffer);

    auto &splitter = detail::SharedSplitter();
    splitter.Split(draw, 2);
    splitter.SetCurrentChannel(draw, 1);

    const ImVec4 clear{0, 0, 0, 0};
    ImGui::PushStyleColor(ImGuiCol_FrameBg, clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, clear);
    ImGui::PushStyleColor(ImGuiCol_Border, clear);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        {padX, std::max(0.f, (height - ImGui::GetFontSize()) * .5f)});

    const bool disabled = detail::IsCurrentItemDisabled();
    const bool open = ImGui::BeginCombo(label, shown, flags | ImGuiComboFlags_NoArrowButton);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    // Note: when open == true, CurrentWindow switched to the popup; draw into the original window's DrawList.
    splitter.SetCurrentChannel(draw, 0);
    const bool hovered = !disabled && (open ? (GImGui->HoveredId == comboId) : ImGui::IsItemHovered());
    const bool focused = !disabled && (GImGui->NavId == comboId) && GImGui->NavCursorVisible;
    const PartState st = disabled ? PartState::Disabled
                       : open     ? PartState::Pressed
                       : hovered  ? PartState::Hover
                                  : PartState::Rest;
    const float radius = r.Var(PartVar::Radius);
    const float borderW = r.Var(PartVar::BorderWidth);
    detail::DrawSurfaceRect(draw, frameMin, frameMax, r.Color(st, PartColor::Fill), radius,
                            detail::CurrentSurfaceStyle(), detail::CurrentIsDark(), false);
    DrawBoxBorder(draw, frameMin, frameMax, radius, ColorU32(r, st, PartColor::Border), borderW, disabled);
    if (focused)
        DrawFocusRing(draw, frameMin, frameMax, radius, 1.f, r.Var(PartVar::FocusRing));

    const float chevSize = ImGui::GetFontSize() * .48f;
    const float markW = std::max(1.f, r.Var(PartVar::MarkWidth));
    DrawChevron(draw, {frameMax.x - height * .5f - padX * .25f, (frameMin.y + frameMax.y) * .5f}, chevSize,
                ColorU32(r, disabled ? PartState::Disabled : PartState::Rest, PartColor::Mark),
                open ? ImGuiDir_Up : ImGuiDir_Down, markW);
    splitter.Merge(draw);

    return open;
}
void EndCombo() { ImGui::EndCombo(); }

bool Combo(const char *label, int *current, const char *(*getter)(void *, int), void *user, int count, int heightInItems) {
    return imkit::Combo(label, current, getter, user, count, heightInItems, ComboStyle{});
}
bool Combo(const char *label, int *current, const char *(*getter)(void *, int), void *user, int count, int heightInItems,
           const ComboStyle &style) {
    const auto &imguiStyle = ImGui::GetStyle();
    const char *preview = *current >= 0 && *current < count ? getter(user, *current) : nullptr;
    if (heightInItems != -1)
        ImGui::SetNextWindowSizeConstraints(
            {0, 0},
            {FLT_MAX, (ImGui::GetFontSize() + imguiStyle.ItemSpacing.y) * static_cast<float>(heightInItems) -
                          imguiStyle.ItemSpacing.y + imguiStyle.WindowPadding.y * 2});
    if (!imkit::BeginCombo(label, preview ? preview : "", 0, style))
        return false;
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        const bool selected = i == *current;
        const char *text = getter(user, i);
        if (imkit::Selectable(text ? text : "*Unknown item*", selected) && !selected) {
            *current = i;
            changed = true;
        }
        if (selected)
            ImGui::SetItemDefaultFocus();
        ImGui::PopID();
    }
    imkit::EndCombo();
    return changed;
}
bool Combo(const char *label, int *current, const char *const items[], int count, int heightInItems) {
    return imkit::Combo(label, current, items, count, heightInItems, ComboStyle{});
}
bool Combo(const char *label, int *current, const char *const items[], int count, int heightInItems,
           const ComboStyle &style) {
    return imkit::Combo(
        label, current, [](void *data, int index) { return static_cast<const char *const *>(data)[index]; },
        const_cast<void *>(static_cast<const void *>(items)), count, heightInItems, style);
}
bool Combo(const char *label, int *current, const char *zeroSeparated, int heightInItems) {
    return imkit::Combo(label, current, zeroSeparated, heightInItems, ComboStyle{});
}
bool Combo(const char *label, int *current, const char *zeroSeparated, int heightInItems, const ComboStyle &style) {
    int count = 0;
    for (const char *item = zeroSeparated; *item; item += std::strlen(item) + 1)
        ++count;
    return imkit::Combo(label, current, ZeroSeparatedItem, const_cast<char *>(zeroSeparated), count, heightInItems,
                        style);
}

bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
    return imkit::Selectable(label, selected, flags, size, SelectableStyle{});
}
bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size,
                const SelectableStyle &style) {
    return SelectableImpl(label, selected, flags, size, style);
}
bool Selectable(const char *label, bool *selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
    return imkit::Selectable(label, selected, flags, size, SelectableStyle{});
}
bool Selectable(const char *label, bool *selected, ImGuiSelectableFlags flags, const ImVec2 &size,
                const SelectableStyle &style) {
    const bool wasSelected = selected && *selected;
    const bool pressed = SelectableImpl(label, wasSelected, flags, size, style);
    if (pressed && selected && !(flags & ImGuiSelectableFlags_Disabled))
        *selected = !*selected;
    return pressed;
}

bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags) {
    return imkit::CollapsingHeader(label, flags, HeaderStyle{});
}
bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags, const HeaderStyle &style) {
    return CollapsingHeaderImpl(label, nullptr, flags, style);
}
bool CollapsingHeader(const char *label, bool *visible, ImGuiTreeNodeFlags flags) {
    return imkit::CollapsingHeader(label, visible, flags, HeaderStyle{});
}
bool CollapsingHeader(const char *label, bool *visible, ImGuiTreeNodeFlags flags, const HeaderStyle &style) {
    return CollapsingHeaderImpl(label, visible, flags, style);
}

bool ColorEdit3(const char *label, float col[3], ImGuiColorEditFlags flags) {
    return imkit::ColorEdit3(label, col, flags, ColorEditStyle{});
}
bool ColorEdit3(const char *label, float col[3], ImGuiColorEditFlags flags, const ColorEditStyle &style) {
    return ColorEditImpl(label, col, false, flags, style);
}
bool ColorEdit4(const char *label, float col[4], ImGuiColorEditFlags flags) {
    return imkit::ColorEdit4(label, col, flags, ColorEditStyle{});
}
bool ColorEdit4(const char *label, float col[4], ImGuiColorEditFlags flags, const ColorEditStyle &style) {
    return ColorEditImpl(label, col, true, flags, style);
}

void ProgressBar(float fraction, const ImVec2 &size, const char *overlay) {
    imkit::ProgressBar(fraction, size, overlay, ProgressStyle{});
}
void ProgressBar(float fraction, const ImVec2 &size, const char *overlay, const ProgressStyle &style) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Progress, style);
        ImGui::ProgressBar(fraction, size, overlay);
        return;
    }

    const ResolvedPartStyle r = ResolvePartStyle(Part::Progress, style);
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    const float trackH = std::max(2.f, r.Var(PartVar::TrackHeight));
    const float radius = r.Var(PartVar::Radius) > 0.f ? r.Var(PartVar::Radius) : trackH * .5f;
    const float gap = r.Var(PartVar::Gap);
    const float fontSize = ImGui::GetFontSize();

    const float clamped = std::isfinite(fraction) ? std::clamp(fraction, 0.f, 1.f) : 0.f;
    char pctBuf[16];
    std::snprintf(pctBuf, sizeof(pctBuf), "%3d%%", static_cast<int>(std::round(clamped * 100.f)));

    const float availW = ImGui::GetContentRegionAvail().x;
    float totalW = size.x < 0.f ? availW : (size.x == 0.f ? ImGui::CalcItemWidth() : size.x);
    totalW = std::max(totalW, r.Var(PartVar::MinWidth));
    const float pctW = ImGui::CalcTextSize("100%").x;
    const float trackW = std::max(24.f, totalW - pctW - gap);

    const bool hasOverlay = overlay && *overlay;
    const float rowH = std::max({trackH, fontSize, size.y});
    const float totalH = hasOverlay ? (rowH + gap * .5f + fontSize) : rowH;

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy({totalW, totalH});
    if (!ImGui::IsItemVisible())
        return;

    ImDrawList *draw = window->DrawList;
    const float trackY = pos.y + (rowH - trackH) * .5f;
    const ImVec2 trackMin{pos.x, trackY};
    const ImVec2 trackMax{pos.x + trackW, trackY + trackH};
    DrawSmoothRect(draw, trackMin, trackMax, ColorU32(r, PartState::Rest, PartColor::Fill), radius);
    if (r.Var(PartVar::BorderWidth) > 0.f)
        DrawSmoothRectBorder(draw, trackMin, trackMax, ColorU32(r, PartState::Rest, PartColor::Border), radius,
                             r.Var(PartVar::BorderWidth));

    if (clamped > 0.f) {
        const float fillW = std::min(trackW, std::max(trackH, trackW * clamped));
        DrawSmoothRect(draw, trackMin, {pos.x + fillW, trackMax.y}, ColorU32(r, PartState::Rest, PartColor::Mark),
                       radius);
    }

    const ImU32 textCol = ColorU32(r, PartState::Rest, PartColor::Text);
    draw->AddText(ImGui::GetFont(), fontSize, {pos.x + trackW + gap, pos.y + (rowH - fontSize) * .5f}, textCol, pctBuf);
    if (hasOverlay) {
        const ImU32 subCol = ColorU32(r, PartState::Rest, PartColor::Text, .72f);
        draw->AddText(ImGui::GetFont(), fontSize, {pos.x, pos.y + rowH + gap * .5f}, subCol, overlay);
    }
}

bool BeginTabItem(const char *label, bool *open, ImGuiTabItemFlags flags) {
    return imkit::BeginTabItem(label, open, flags, TabStyle{});
}
bool BeginTabItem(const char *label, bool *open, ImGuiTabItemFlags flags, const TabStyle &style) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Tab, style);
        return ImGui::BeginTabItem(label, open, flags);
    }

    const ResolvedPartStyle r = ResolvePartStyle(Part::Tab, style);
    const ImVec4 clear{0, 0, 0, 0};
    ImGui::PushStyleColor(ImGuiCol_Tab, clear);
    ImGui::PushStyleColor(ImGuiCol_TabHovered, clear);
    ImGui::PushStyleColor(ImGuiCol_TabSelected, clear);
    ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, clear);
    ImGui::PushStyleColor(ImGuiCol_TabDimmed, clear);
    ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected, clear);
    ImGui::PushStyleColor(ImGuiCol_TabDimmedSelectedOverline, clear);

    const float padX = r.Var(PartVar::PaddingX);
    const float padY = std::max(0.f, (r.Var(PartVar::Height) - ImGui::GetFontSize()) * .5f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {padX, padY});

    const bool active = ImGui::BeginTabItem(label, open, flags);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(7);

    if (ImGui::IsItemVisible()) {
        const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        ImDrawList *draw = ImGui::GetWindowDrawList();
        const bool hovered = ImGui::IsItemHovered();
        const float radius = r.Var(PartVar::Radius);
        const float borderW = r.Var(PartVar::BorderWidth);
        const bool customFill = r.colorLayers[static_cast<std::size_t>(active ? PartState::Selected : PartState::Rest)]
                                             [static_cast<std::size_t>(PartColor::Fill)] != PartLayer::Default;
        if (customFill) {
            const PartState st = active ? PartState::Selected : (hovered ? PartState::Hover : PartState::Rest);
            DrawSmoothRect(draw, a, b, ColorU32(r, st, PartColor::Fill), radius);
        } else if (hovered && !active) {
            DrawSmoothRect(draw, a, b, ColorU32(r, PartState::Hover, PartColor::Fill, .45f), radius);
        }
        if (borderW > 0.f && r.varLayers[static_cast<std::size_t>(PartVar::BorderWidth)] != PartLayer::Default) {
            DrawSmoothRectBorder(draw, a, b, ColorU32(r, active ? PartState::Selected : PartState::Rest, PartColor::Border),
                                 radius, borderW);
        }
        if (active) {
            const float indH = std::max(1.f, r.Var(PartVar::IndicatorHeight));
            const float inset = std::min(6.f, (b.x - a.x) * .15f);
            DrawSmoothRect(draw, {a.x + inset, b.y - indH}, {b.x - inset, b.y},
                           ColorU32(r, PartState::Selected, PartColor::Mark), indH * .5f);
        }
        if (ImGui::IsItemFocused() && GImGui->NavCursorVisible)
            DrawFocusRing(draw, a, b, radius, 1.f, r.Var(PartVar::FocusRing));
    }

    return active;
}

bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags) {
    return imkit::TreeNodeEx(label, flags, TreeStyle{});
}
bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags, const TreeStyle &style) {
    return TreeNodeModern(nullptr, label, flags, label ? label : "", style);
}
bool TreeNodeExV(const char *id, ImGuiTreeNodeFlags flags, const char *format, va_list args) {
    const std::string formatted = FormatV(format, args);
    return TreeNodeModern(nullptr, id, flags, formatted.c_str(), TreeStyle{});
}
bool TreeNodeExV(const void *id, ImGuiTreeNodeFlags flags, const char *format, va_list args) {
    const std::string formatted = FormatV(format, args);
    return TreeNodeModern(id, nullptr, flags, formatted.c_str(), TreeStyle{});
}
bool TreeNodeEx(const char *id, ImGuiTreeNodeFlags flags, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const bool open = imkit::TreeNodeExV(id, flags, format, args);
    va_end(args);
    return open;
}
bool TreeNodeEx(const void *id, ImGuiTreeNodeFlags flags, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const bool open = imkit::TreeNodeExV(id, flags, format, args);
    va_end(args);
    return open;
}
} // namespace imkit
