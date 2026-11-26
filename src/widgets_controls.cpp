#include "widgets_internal.h"
#include "draw_internal.h"
#include "imgui_internal_compat.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string_view>

// Wave 2 (part 1 & 2) widgets: buttons, checkboxes, radios, sliders, drags and inputs.
namespace imkit {
namespace {
// Shrinks the frame height for check marks only, leaving inputs and buttons at the control height.
struct CompactFrame {
    CompactFrame() {
        const auto &style = ImGui::GetStyle();
        const float pad = std::max(1.f, ImGui::GetFontSize() * (detail::CurrentParts().checkHeightScale - 1.f) * .5f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {style.FramePadding.x, pad});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, std::min(style.FrameRounding, 6.f));
    }
    ~CompactFrame() { ImGui::PopStyleVar(2); }
    CompactFrame(const CompactFrame &) = delete;
    CompactFrame &operator=(const CompactFrame &) = delete;
};

struct FontSizeScope {
    bool pushed = false;
    explicit FontSizeScope(float size) {
        if (size > 0.f && std::fabs(size - ImGui::GetFontSize()) > .05f) {
            ImGui::PushFont(nullptr, size);
            pushed = true;
        }
    }
    ~FontSizeScope() {
        if (pushed)
            ImGui::PopFont();
    }
    FontSizeScope(const FontSizeScope &) = delete;
    FontSizeScope &operator=(const FontSizeScope &) = delete;
};

void DrawDashedCircle(ImDrawList *draw, ImVec2 center, float radius, ImU32 color, float thickness, float dash) {
    if (radius <= 0.f || (color & IM_COL32_A_MASK) == 0)
        return;
    const float circumference = 6.2831853f * radius;
    const float stepLen = std::max(2.f, dash * 1.8f);
    const int count = std::max(6, static_cast<int>(std::round(circumference / stepLen)));
    const float arcStep = 6.2831853f / static_cast<float>(count);
    const float dashArc = arcStep * .55f;
    for (int i = 0; i < count; ++i) {
        const float a0 = static_cast<float>(i) * arcStep;
        draw->PathArcTo(center, radius, a0, a0 + dashArc, 4);
        draw->PathStroke(color, ImDrawFlags_None, thickness);
    }
}

bool DrawModernButton(Part part, const char *label, const ImVec2 &size, const ButtonStyle &style, float hitPadY) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(part, style);
        return part == Part::SmallButton ? ImGui::SmallButton(label) : ImGui::Button(label, size);
    }
    const ResolvedPartStyle r = ResolvePartStyle(part, style);
    FontSizeScope fontScope(r.Var(PartVar::FontSize));

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const std::string_view visible = detail::VisibleLabel(label);
    const ImVec2 textSize =
        visible.empty() ? ImVec2(0, 0) : ImGui::CalcTextSize(visible.data(), visible.data() + visible.size(), false);
    const float padX = r.Var(PartVar::PaddingX);
    const float padY = r.Var(PartVar::PaddingY);
    const float minW = r.Var(PartVar::MinWidth);
    const float defaultH = std::max(r.Var(PartVar::Height), textSize.y + padY * 2.f);
    const float defaultW = std::max(minW, textSize.x + padX * 2.f);
    ImVec2 itemSize = ImGui::CalcItemSize(size, defaultW, defaultH);
    if (minW > 0.f)
        itemSize.x = std::max(itemSize.x, minW);

    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, {pos.x + itemSize.x, pos.y + itemSize.y});
    ImGui::ItemSize(bb, padY);

    const ImVec2 hitMin{bb.Min.x, bb.Min.y - hitPadY};
    const ImVec2 hitMax{bb.Max.x, bb.Max.y + hitPadY};
    const InteractResult res = Interact(id, hitMin, hitMax, InteractFlags_None);
    if (!res.visible)
        return res.pressed;

    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float radius = r.Var(PartVar::Radius);
    const float borderW = r.Var(PartVar::BorderWidth);
    const float pressDepth = res.disabled ? 0.f : r.Var(PartVar::PressDepth);
    const float shiftY = pressDepth * res.active01;

    ImVec4 fill = r.Color(res.disabled ? PartState::Disabled : PartState::Rest, PartColor::Fill);
    if (!res.disabled) {
        fill = detail::MixColor(fill, r.Color(PartState::Hover, PartColor::Fill), res.hover01);
        fill = detail::MixColor(fill, r.Color(PartState::Pressed, PartColor::Fill), res.active01);
    }

    if (pressDepth > 0.f && !res.disabled && fill.w > .05f) {
        const float shadowAlpha = .30f * fill.w * (1.f - res.active01);
        if (shadowAlpha > .01f) {
            DrawSmoothRect(draw, {bb.Min.x, bb.Min.y + pressDepth}, {bb.Max.x, bb.Max.y + pressDepth},
                           ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, shadowAlpha)), radius);
        }
    }

    const ImVec2 drawMin{bb.Min.x, bb.Min.y + shiftY};
    const ImVec2 drawMax{bb.Max.x, bb.Max.y + shiftY};
    detail::DrawSurfaceRect(draw, drawMin, drawMax, fill, radius, detail::CurrentSurfaceStyle(),
                            detail::CurrentIsDark(), !res.disabled && !res.held && pressDepth <= 0.f);

    if (res.disabled) {
        const float bw = std::max(1.f, borderW);
        DrawDashedRect(draw, drawMin, drawMax, radius,
                       ImGui::GetColorU32(r.Color(PartState::Disabled, PartColor::Border)), bw,
                       std::max(3.f, bw * 2.5f));
    } else if (borderW > 0.f) {
        ImVec4 border = r.Color(PartState::Rest, PartColor::Border);
        border = detail::MixColor(border, r.Color(PartState::Hover, PartColor::Border), res.hover01);
        border = detail::MixColor(border, r.Color(PartState::Pressed, PartColor::Border), res.active01);
        if (res.focused)
            border = detail::MixColor(border, r.Color(PartState::Focused, PartColor::Border), res.focus01);
        DrawSmoothRectBorder(draw, drawMin, drawMax, ImGui::GetColorU32(border), radius, borderW);
    }

    if (res.focus01 > 0.f)
        DrawFocusRing(draw, drawMin, drawMax, radius, res.focus01, r.Var(PartVar::FocusRing));

    if (!visible.empty()) {
        const float effectivePadX = std::min(padX, std::max(4.f, (itemSize.x - textSize.x) * .5f));
        const ImVec2 align = ImGui::GetStyle().ButtonTextAlign;
        const ImVec2 textPos{
            drawMin.x + effectivePadX +
                std::max(0.f, (drawMax.x - drawMin.x) - effectivePadX * 2.f - textSize.x) * align.x,
            drawMin.y + padY + std::max(0.f, (drawMax.y - drawMin.y) - padY * 2.f - textSize.y) * align.y};
        const ImVec4 clip{drawMin.x, drawMin.y, drawMax.x, drawMax.y};
        const ImVec4 textCol = r.Color(res.disabled ? PartState::Disabled : PartState::Rest, PartColor::Text);
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, ImGui::GetColorU32(textCol), visible.data(),
                      visible.data() + visible.size(), 0.f, &clip);
    }

    return res.pressed;
}

template <class T>
void FormatSliderValue(char *buf, std::size_t bufSize, const char *format, T value) {
    std::snprintf(buf, bufSize, format, value);
}

template <class T, class NativeCall>
bool DrawModernSlider(const char *label, T *value, T minimum, T maximum, const char *format, const char *defaultFormat,
                      ImGuiSliderFlags flags, const SliderStyle &style, bool vertical, ImVec2 vSize,
                      NativeCall nativeCall) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Slider, style);
        return nativeCall(label, value, minimum, maximum, format, flags);
    }
    const ResolvedPartStyle r = ResolvePartStyle(Part::Slider, style);
    FontSizeScope fontScope(r.Var(PartVar::FontSize));

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    if (ImGui::TempInputIsActive(id)) {
        detail::WidgetStyleScope scope(Part::Slider, style);
        return nativeCall(label, value, minimum, maximum, format, flags);
    }

    const auto &imguiStyle = ImGui::GetStyle();
    const float thumbSize = std::max(6.f, r.Var(PartVar::ThumbSize));
    const float targetHeight = std::max(r.Var(PartVar::Height), thumbSize);
    const float framePadY = std::max(0.f, (targetHeight - ImGui::GetFontSize()) * .5f);
    const bool pushPad = std::fabs(framePadY - imguiStyle.FramePadding.y) > .25f;
    if (pushPad)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {r.Var(PartVar::PaddingX), framePadY});

    const ImVec2 frameMin = ImGui::GetCursorScreenPos();
    const float frameW = vertical ? vSize.x : ImGui::CalcItemWidth();
    const float frameH = vertical ? vSize.y : ImGui::GetFrameHeight();
    const ImVec2 frameMax{frameMin.x + frameW, frameMin.y + frameH};

    ImDrawList *draw = ImGui::GetWindowDrawList();
    auto &splitter = detail::SharedSplitter();
    splitter.Split(draw, 2);
    splitter.SetCurrentChannel(draw, 1);

    const ImVec4 clear{0, 0, 0, 0};
    for (ImGuiCol col : {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_SliderGrab,
                         ImGuiCol_SliderGrabActive, ImGuiCol_Border, ImGuiCol_Text}) {
        ImGui::PushStyleColor(col, clear);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    const bool changed = nativeCall(label, value, minimum, maximum, format, flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(7);

    const bool disabled = detail::IsCurrentItemDisabled();
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    const bool focused = ImGui::IsItemFocused();
    const bool focusVisible = focused && GImGui->NavCursorVisible;

    const float hover01 = AnimatePart(ImHashData(&id, sizeof id, 0x68u), hovered ? 1.f : 0.f);
    const float active01 = AnimatePart(ImHashData(&id, sizeof id, 0x61u), active ? 1.f : 0.f);
    const float focus01 = AnimatePart(ImHashData(&id, sizeof id, 0x66u), focusVisible ? 1.f : 0.f);

    splitter.SetCurrentChannel(draw, 0);

    const char *fmt = (format && *format) ? format : defaultFormat;
    char valueBuf[64]{};
    FormatSliderValue(valueBuf, sizeof(valueBuf), fmt, *value);
    const ImVec2 valueSize = ImGui::CalcTextSize(valueBuf);
    const float gap = r.Var(PartVar::Gap);
    const float rightPad = imguiStyle.ItemInnerSpacing.x * .5f;
    const float valueBoxW =
        (!vertical && frameW > thumbSize * 2.5f + valueSize.x + gap + rightPad) ? (valueSize.x + gap + rightPad) : 0.f;

    const float trackH = std::max(2.f, std::min(r.Var(PartVar::TrackHeight), frameH));
    const float trackRadius = std::min(r.Var(PartVar::Radius), trackH * .5f);
    const float thumbR = thumbSize * .5f;

    const ImVec2 trackAreaMin = frameMin;
    const ImVec2 trackAreaMax{frameMax.x - valueBoxW, frameMax.y};

    ImVec4 trackFill = r.Color(disabled ? PartState::Disabled : PartState::Rest, PartColor::Fill);
    if (!disabled) {
        trackFill = detail::MixColor(trackFill, r.Color(PartState::Hover, PartColor::Fill), hover01);
        trackFill = detail::MixColor(trackFill, r.Color(PartState::Active, PartColor::Fill), active01);
    }
    ImVec4 markCol = r.Color(disabled ? PartState::Disabled : PartState::Rest, PartColor::Mark);
    if (!disabled) {
        markCol = detail::MixColor(markCol, r.Color(PartState::Hover, PartColor::Mark), hover01);
        markCol = detail::MixColor(markCol, r.Color(PartState::Active, PartColor::Mark), active01);
        if (!detail::CurrentIsDark()) {
            const ImVec4 textInk = r.Color(PartState::Rest, PartColor::Text);
            markCol = detail::MixToContrast(markCol, textInk, 0.f, 3.0f, {trackFill, ImVec4(1.f, 1.f, 1.f, 1.f)});
        }
    }

    const double span = static_cast<double>(maximum) - static_cast<double>(minimum);
    double raw = span != 0.0 ? (static_cast<double>(*value) - static_cast<double>(minimum)) / span : 0.0;
    if ((flags & ImGuiSliderFlags_Logarithmic) && span > 0.0 && *value > minimum && minimum > 0) {
        raw = std::log(static_cast<double>(*value) / static_cast<double>(minimum)) /
              std::log(static_cast<double>(maximum) / static_cast<double>(minimum));
    }
    const float ratio = std::isfinite(raw) ? static_cast<float>(std::clamp(raw, 0.0, 1.0)) : 0.f;

    if (vertical) {
        const float cx = (trackAreaMin.x + trackAreaMax.x) * .5f;
        const ImVec2 tMin{cx - trackH * .5f, trackAreaMin.y + thumbR};
        const ImVec2 tMax{cx + trackH * .5f, trackAreaMax.y - thumbR};
        DrawSmoothRect(draw, tMin, tMax, ImGui::GetColorU32(trackFill), trackRadius);
        const float thumbY = tMax.y - (tMax.y - tMin.y) * ratio;
        if (tMax.y - thumbY > .5f)
            DrawSmoothRect(draw, {tMin.x, thumbY}, tMax, ImGui::GetColorU32(markCol), trackRadius);
        if (disabled) {
            DrawDashedRect(draw, tMin, tMax, trackRadius,
                           ImGui::GetColorU32(r.Color(PartState::Disabled, PartColor::Border)), 1.f, 3.f);
        }
        const ImVec2 thumbCenter{cx, thumbY};
        if (!disabled && (active01 > .01f || hover01 > .01f)) {
            const float ringAlpha = std::max(active01 * .28f, hover01 * .16f);
            draw->AddCircleFilled(thumbCenter, thumbR + 4.f * std::max(active01, hover01 * .6f),
                                  ImGui::GetColorU32(detail::WithAlpha(markCol, ringAlpha)), 24);
        }
        const ImVec4 thumbFill = disabled ? markCol : ImVec4(1.f, 1.f, 1.f, 1.f);
        if (!disabled && detail::CurrentSurfaceStyle() == SurfaceStyle::Tactile)
            draw->AddCircleFilled({thumbCenter.x, thumbCenter.y + 1.f}, thumbR + .5f, IM_COL32(0, 0, 0, 64), 24);
        draw->AddCircleFilled(thumbCenter, thumbR, ImGui::GetColorU32(thumbFill), 24);
        draw->AddCircle(thumbCenter, thumbR - .5f, ImGui::GetColorU32(markCol), 24, 1.5f);
    } else {
        const float cy = (trackAreaMin.y + trackAreaMax.y) * .5f;
        const ImVec2 tMin{trackAreaMin.x + thumbR, cy - trackH * .5f};
        const ImVec2 tMax{trackAreaMax.x - thumbR, cy + trackH * .5f};
        DrawSmoothRect(draw, tMin, tMax, ImGui::GetColorU32(trackFill), trackRadius);
        const float thumbX = tMin.x + (tMax.x - tMin.x) * ratio;
        if (thumbX - tMin.x > .5f)
            DrawSmoothRect(draw, tMin, {thumbX, tMax.y}, ImGui::GetColorU32(markCol), trackRadius);
        if (disabled) {
            DrawDashedRect(draw, tMin, tMax, trackRadius,
                           ImGui::GetColorU32(r.Color(PartState::Disabled, PartColor::Border)), 1.f, 3.f);
        }
        const ImVec2 thumbCenter{thumbX, cy};
        if (!disabled && (active01 > .01f || hover01 > .01f)) {
            const float ringAlpha = std::max(active01 * .28f, hover01 * .16f);
            draw->AddCircleFilled(thumbCenter, thumbR + 4.f * std::max(active01, hover01 * .6f),
                                  ImGui::GetColorU32(detail::WithAlpha(markCol, ringAlpha)), 24);
        }
        const ImVec4 thumbFill = disabled ? markCol : ImVec4(1.f, 1.f, 1.f, 1.f);
        if (!disabled && detail::CurrentSurfaceStyle() == SurfaceStyle::Tactile)
            draw->AddCircleFilled({thumbCenter.x, thumbCenter.y + 1.f}, thumbR + .5f, IM_COL32(0, 0, 0, 64), 24);
        draw->AddCircleFilled(thumbCenter, thumbR, ImGui::GetColorU32(thumbFill), 24);
        draw->AddCircle(thumbCenter, thumbR - .5f, ImGui::GetColorU32(markCol), 24, 1.5f);
    }

    if (focus01 > 0.f)
        DrawFocusRing(draw, frameMin, frameMax, r.Var(PartVar::Radius), focus01, r.Var(PartVar::FocusRing));

    const ImVec4 textCol = r.Color(disabled ? PartState::Disabled : PartState::Rest, PartColor::Text);
    if (valueBoxW > 0.f) {
        const ImVec2 valPos{frameMax.x - rightPad - valueSize.x, frameMin.y + (frameH - valueSize.y) * .5f};
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), valPos, ImGui::GetColorU32(textCol), valueBuf);
    } else if (vertical) {
        const ImVec2 valPos{frameMin.x + (frameW - valueSize.x) * .5f, frameMin.y + 2.f};
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), valPos, ImGui::GetColorU32(textCol), valueBuf);
    }

    const std::string_view visible = detail::VisibleLabel(label);
    if (!visible.empty()) {
        const ImVec2 labelSize = ImGui::CalcTextSize(visible.data(), visible.data() + visible.size(), false);
        const ImVec2 labelPos{frameMax.x + imguiStyle.ItemInnerSpacing.x, frameMin.y + (frameH - labelSize.y) * .5f};
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), labelPos, ImGui::GetColorU32(textCol), visible.data(),
                      visible.data() + visible.size());
    }

    splitter.Merge(draw);
    if (pushPad)
        ImGui::PopStyleVar();
    return changed;
}

template <class NativeCall>
bool DrawModernDrag(const char *label, const DragStyle &style, NativeCall nativeCall) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Drag, style);
        return nativeCall();
    }
    const ResolvedPartStyle r = ResolvePartStyle(Part::Drag, style);
    FontSizeScope fontScope(r.Var(PartVar::FontSize));

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    if (ImGui::TempInputIsActive(id)) {
        detail::WidgetStyleScope scope(Part::Drag, style);
        return nativeCall();
    }

    const auto &imguiStyle = ImGui::GetStyle();
    const float framePadY = std::max(0.f, (r.Var(PartVar::Height) - ImGui::GetFontSize()) * .5f);
    const bool pushPad = std::fabs(framePadY - imguiStyle.FramePadding.y) > .25f ||
                         std::fabs(r.Var(PartVar::PaddingX) - imguiStyle.FramePadding.x) > .25f;
    if (pushPad)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {r.Var(PartVar::PaddingX), framePadY});

    const ImVec2 frameMin = ImGui::GetCursorScreenPos();
    const float frameW = ImGui::CalcItemWidth();
    const float frameH = ImGui::GetFrameHeight();
    const ImVec2 frameMax{frameMin.x + frameW, frameMin.y + frameH};

    ImDrawList *draw = ImGui::GetWindowDrawList();
    auto &splitter = detail::SharedSplitter();
    splitter.Split(draw, 2);
    splitter.SetCurrentChannel(draw, 1);

    const ImVec4 clear{0, 0, 0, 0};
    ImGui::PushStyleColor(ImGuiCol_FrameBg, clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, clear);
    ImGui::PushStyleColor(ImGuiCol_Border, clear);
    ImGui::PushStyleColor(ImGuiCol_Text, r.Color(detail::IsCurrentItemDisabled() ? PartState::Disabled : PartState::Rest,
                                                 PartColor::Text));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    const bool changed = nativeCall();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);

    const bool disabled = detail::IsCurrentItemDisabled();
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    const bool focused = ImGui::IsItemFocused();
    const bool focusVisible = focused && GImGui->NavCursorVisible;

    const float hover01 = AnimatePart(ImHashData(&id, sizeof id, 0x68u), hovered ? 1.f : 0.f);
    const float active01 = AnimatePart(ImHashData(&id, sizeof id, 0x61u), active ? 1.f : 0.f);
    const float focus01 = AnimatePart(ImHashData(&id, sizeof id, 0x66u), (focusVisible || active) ? 1.f : 0.f);

    splitter.SetCurrentChannel(draw, 0);

    const float radius = r.Var(PartVar::Radius);
    const float borderW = r.Var(PartVar::BorderWidth);
    ImVec4 fill = r.Color(disabled ? PartState::Disabled : PartState::Rest, PartColor::Fill);
    if (!disabled) {
        fill = detail::MixColor(fill, r.Color(PartState::Hover, PartColor::Fill), hover01);
        fill = detail::MixColor(fill, r.Color(PartState::Active, PartColor::Fill), active01);
    }
    DrawSmoothRect(draw, frameMin, frameMax, ImGui::GetColorU32(fill), radius);

    if (disabled) {
        const float bw = std::max(1.f, borderW);
        DrawDashedRect(draw, frameMin, frameMax, radius,
                       ImGui::GetColorU32(r.Color(PartState::Disabled, PartColor::Border)), bw,
                       std::max(3.f, bw * 2.5f));
    } else {
        ImVec4 border = r.Color(PartState::Rest, PartColor::Border);
        border = detail::MixColor(border, r.Color(PartState::Hover, PartColor::Border), hover01);
        border = detail::MixColor(border, r.Color(PartState::Focused, PartColor::Border), std::max(active01, focus01));
        const float bw = std::max(1.f, borderW);
        DrawSmoothRectBorder(draw, frameMin, frameMax, ImGui::GetColorU32(border), radius, bw);
    }

    if (frameW > 28.f) {
        const float padX = r.Var(PartVar::PaddingX);
        const float chevSize = std::clamp(ImGui::GetFontSize() * .34f, 4.f, 7.f);
        const float cy = (frameMin.y + frameMax.y) * .5f;
        const float cx = frameMin.x + padX + chevSize * .9f;
        const float alpha = disabled ? .25f : (active ? .85f : hovered ? .65f : .38f);
        const ImU32 scrubCol = ImGui::GetColorU32(
            detail::WithAlpha(r.Color(PartState::Rest, PartColor::Text), alpha));
        DrawChevron(draw, {cx - chevSize * .45f, cy}, chevSize, scrubCol, ImGuiDir_Left, 1.3f);
        DrawChevron(draw, {cx + chevSize * .45f, cy}, chevSize, scrubCol, ImGuiDir_Right, 1.3f);
    }

    if (focus01 > 0.f && !disabled)
        DrawFocusRing(draw, frameMin, frameMax, radius, focus01, r.Var(PartVar::FocusRing));

    splitter.Merge(draw);
    if (pushPad)
        ImGui::PopStyleVar();
    return changed;
}

template <class NativeCall>
bool DrawModernInput(const char *label, const InputStyle &style, bool hasStepButtons, NativeCall nativeCall) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Input, style);
        return nativeCall();
    }
    const ResolvedPartStyle r = ResolvePartStyle(Part::Input, style);
    FontSizeScope fontScope(r.Var(PartVar::FontSize));

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const auto &imguiStyle = ImGui::GetStyle();
    const float framePadY = std::max(0.f, (r.Var(PartVar::Height) - ImGui::GetFontSize()) * .5f);
    const bool pushPad = std::fabs(framePadY - imguiStyle.FramePadding.y) > .25f ||
                         std::fabs(r.Var(PartVar::PaddingX) - imguiStyle.FramePadding.x) > .25f;
    if (pushPad)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {r.Var(PartVar::PaddingX), framePadY});

    const ImVec2 frameMin = ImGui::GetCursorScreenPos();
    const float totalW = ImGui::CalcItemWidth();
    const float frameH = ImGui::GetFrameHeight();
    const float boxW = hasStepButtons ? std::max(1.f, totalW - (frameH + imguiStyle.ItemInnerSpacing.x) * 2.f) : totalW;
    const ImVec2 frameMax{frameMin.x + boxW, frameMin.y + frameH};

    ImDrawList *draw = ImGui::GetWindowDrawList();
    auto &splitter = detail::SharedSplitter();
    splitter.Split(draw, 2);
    splitter.SetCurrentChannel(draw, 1);

    const ImVec4 clear{0, 0, 0, 0};
    ImGui::PushStyleColor(ImGuiCol_FrameBg, clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, clear);
    ImGui::PushStyleColor(ImGuiCol_Border, clear);
    ImGui::PushStyleColor(ImGuiCol_Text, r.Color(detail::IsCurrentItemDisabled() ? PartState::Disabled : PartState::Rest,
                                                 PartColor::Text));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    const bool changed = nativeCall();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);

    const ImGuiID id = window->GetID(label);
    ImGuiID textId = id;
    if (hasStepButtons) {
        ImGui::PushID(label);
        textId = window->GetID("");
        ImGui::PopID();
    }
    const bool disabled = detail::IsCurrentItemDisabled();
    const bool active = hasStepButtons ? (GImGui->ActiveId == textId) : ImGui::IsItemActive();
    const bool hovered = hasStepButtons ? ImGui::IsMouseHoveringRect(frameMin, frameMax) : ImGui::IsItemHovered();
    const bool focused = (hasStepButtons ? (GImGui->NavId == textId) : ImGui::IsItemFocused()) || active;

    const float hover01 = AnimatePart(ImHashData(&id, sizeof id, 0x68u), hovered ? 1.f : 0.f);
    const float focus01 = AnimatePart(ImHashData(&id, sizeof id, 0x66u), focused ? 1.f : 0.f);

    splitter.SetCurrentChannel(draw, 0);

    const float radius = r.Var(PartVar::Radius);
    const float borderW = r.Var(PartVar::BorderWidth);
    ImVec4 fill = r.Color(disabled ? PartState::Disabled : PartState::Rest, PartColor::Fill);
    if (!disabled) {
        fill = detail::MixColor(fill, r.Color(PartState::Hover, PartColor::Fill), hover01);
        fill = detail::MixColor(fill, r.Color(PartState::Active, PartColor::Fill), focus01);
    }
    DrawSmoothRect(draw, frameMin, frameMax, ImGui::GetColorU32(fill), radius);

    if (disabled) {
        const float bw = std::max(1.f, borderW);
        DrawDashedRect(draw, frameMin, frameMax, radius,
                       ImGui::GetColorU32(r.Color(PartState::Disabled, PartColor::Border)), bw,
                       std::max(3.f, bw * 2.5f));
    } else {
        ImVec4 border = r.Color(PartState::Rest, PartColor::Border);
        border = detail::MixColor(border, r.Color(PartState::Hover, PartColor::Border), hover01);
        border = detail::MixColor(border, r.Color(PartState::Focused, PartColor::Border), focus01);
        const float bw = std::max(focused ? 1.5f : 1.f, borderW);
        DrawSmoothRectBorder(draw, frameMin, frameMax, ImGui::GetColorU32(border), radius, bw);
    }

    if (focus01 > 0.f && !disabled)
        DrawFocusRing(draw, frameMin, frameMax, radius, focus01, r.Var(PartVar::FocusRing));

    splitter.Merge(draw);
    if (pushPad)
        ImGui::PopStyleVar();
    return changed;
}

template <class T, class One>
bool SliderComponents(const char *label, T *values, int count, T minimum, T maximum, const char *format,
                      ImGuiSliderFlags flags, const SliderStyle &style, One one) {
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float width = (ImGui::CalcItemWidth() - spacing * static_cast<float>(count - 1)) / static_cast<float>(count);
    bool changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        if (i > 0)
            ImGui::SameLine(0, spacing);
        ImGui::SetNextItemWidth(width);
        changed |= one("", values + i, minimum, maximum, format, flags, style);
        ImGui::PopID();
    }
    const char *hidden = std::strstr(label, "##");
    if (hidden != label && *label) {
        ImGui::SameLine(0, spacing);
        ImGui::TextUnformatted(label, hidden);
    }
    ImGui::PopID();
    ImGui::EndGroup();
    return changed;
}
} // namespace

bool Button(const char *label, const ImVec2 &size) { return imkit::Button(label, size, ButtonStyle{}); }
bool Button(const char *label, const ImVec2 &size, const ButtonStyle &style) {
    return DrawModernButton(Part::Button, label, size, style, 0.f);
}
bool SmallButton(const char *label) { return imkit::SmallButton(label, ButtonStyle{}); }
bool SmallButton(const char *label, const ButtonStyle &style) {
    return DrawModernButton(Part::SmallButton, label, {0.f, 0.f}, style, 2.f);
}

bool Checkbox(const char *label, bool *value) { return imkit::Checkbox(label, value, CheckboxStyle{}); }
bool Checkbox(const char *label, bool *value, const CheckboxStyle &style) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Checkbox, style);
        CompactFrame compact;
        return ImGui::Checkbox(label, value);
    }
    const ResolvedPartStyle r = ResolvePartStyle(Part::Checkbox, style);
    FontSizeScope fontScope(r.Var(PartVar::FontSize));

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const std::string_view visible = detail::VisibleLabel(label);
    const ImVec2 textSize =
        visible.empty() ? ImVec2(0, 0) : ImGui::CalcTextSize(visible.data(), visible.data() + visible.size(), false);
    const float boxSize = std::max(8.f, r.Var(PartVar::BoxSize));
    const float gap = r.varLayers[static_cast<std::size_t>(PartVar::Gap)] != PartLayer::Default
                          ? r.Var(PartVar::Gap)
                          : (r.varLayers[static_cast<std::size_t>(PartVar::PaddingX)] != PartLayer::Default
                                 ? r.Var(PartVar::PaddingX)
                                 : r.Var(PartVar::Gap));
    const float height = std::max({r.Var(PartVar::Height), boxSize, textSize.y});
    const float totalW = boxSize + (visible.empty() ? 0.f : gap + textSize.x);

    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect totalBb(pos, {pos.x + totalW, pos.y + height});
    ImGui::ItemSize(totalBb, std::max(0.f, (height - textSize.y) * .5f));

    const InteractResult res = Interact(id, totalBb.Min, totalBb.Max, InteractFlags_None);
    if (res.pressed && value) {
        *value = !(*value);
        ImGui::MarkItemEdited(id);
    }

    const bool checked = value && *value;
    const float check01 = AnimatePart(ImHashData(&id, sizeof id, 0x63u), checked ? 1.f : 0.f);
    if (!res.visible)
        return res.pressed;

    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 boxMin{pos.x, pos.y + (height - boxSize) * .5f};
    const ImVec2 boxMax{boxMin.x + boxSize, boxMin.y + boxSize};
    const float radius = std::min(r.Var(PartVar::Radius), boxSize * .42f);
    const float borderW = r.Var(PartVar::BorderWidth);

    ImVec4 offFill = r.Color(res.disabled ? PartState::Disabled : PartState::Off, PartColor::Fill);
    if (!res.disabled)
        offFill = detail::MixColor(offFill, r.Color(PartState::Hover, PartColor::Fill), res.hover01);
    const ImVec4 onFill = r.Color(res.disabled ? PartState::Disabled : PartState::On, PartColor::Fill);
    const ImVec4 boxFill = detail::MixColor(offFill, onFill, check01);
    detail::DrawSurfaceRect(draw, boxMin, boxMax, boxFill, radius, detail::CurrentSurfaceStyle(),
                            detail::CurrentIsDark(), false);

    if (res.disabled) {
        const float bw = std::max(1.f, borderW);
        DrawDashedRect(draw, boxMin, boxMax, radius,
                       ImGui::GetColorU32(r.Color(PartState::Disabled, PartColor::Border)), bw,
                       std::max(3.f, bw * 2.5f));
    } else {
        const ImVec4 offBorder = r.Color(PartState::Off, PartColor::Border);
        const ImVec4 onBorder = r.Color(PartState::On, PartColor::Border);
        const ImVec4 borderCol = detail::MixColor(offBorder, onBorder, check01);
        DrawSmoothRectBorder(draw, boxMin, boxMax, ImGui::GetColorU32(borderCol), radius, std::max(1.f, borderW));
    }

    if (check01 > .01f) {
        const ImVec2 center{(boxMin.x + boxMax.x) * .5f, (boxMin.y + boxMax.y) * .5f};
        const ImVec4 markCol = r.Color(res.disabled ? PartState::Disabled : PartState::On, PartColor::Mark);
        DrawCheckMark(draw, center, boxSize * .62f, ImGui::GetColorU32(markCol),
                      std::max(1.2f, r.Var(PartVar::MarkWidth)), check01);
    }

    if (res.focus01 > 0.f)
        DrawFocusRing(draw, boxMin, boxMax, radius, res.focus01, r.Var(PartVar::FocusRing));

    if (!visible.empty()) {
        const ImVec4 textCol =
            r.Color(res.disabled ? PartState::Disabled : (checked ? PartState::On : PartState::Rest), PartColor::Text);
        const ImVec2 textPos{boxMax.x + gap, pos.y + (height - textSize.y) * .5f};
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, ImGui::GetColorU32(textCol), visible.data(),
                      visible.data() + visible.size());
    }

    return res.pressed;
}

bool RadioButton(const char *label, bool active) { return imkit::RadioButton(label, active, RadioStyle{}); }
bool RadioButton(const char *label, bool active, const RadioStyle &style) {
    if (detail::ClassicLook()) {
        detail::WidgetStyleScope scope(Part::Radio, style);
        CompactFrame compact;
        return ImGui::RadioButton(label, active);
    }
    const ResolvedPartStyle r = ResolvePartStyle(Part::Radio, style);
    FontSizeScope fontScope(r.Var(PartVar::FontSize));

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const std::string_view visible = detail::VisibleLabel(label);
    const ImVec2 textSize =
        visible.empty() ? ImVec2(0, 0) : ImGui::CalcTextSize(visible.data(), visible.data() + visible.size(), false);
    const float boxSize = std::max(8.f, r.Var(PartVar::BoxSize));
    const float gap = r.varLayers[static_cast<std::size_t>(PartVar::Gap)] != PartLayer::Default
                          ? r.Var(PartVar::Gap)
                          : (r.varLayers[static_cast<std::size_t>(PartVar::PaddingX)] != PartLayer::Default
                                 ? r.Var(PartVar::PaddingX)
                                 : r.Var(PartVar::Gap));
    const float height = std::max({r.Var(PartVar::Height), boxSize, textSize.y});
    const float totalW = boxSize + (visible.empty() ? 0.f : gap + textSize.x);

    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect totalBb(pos, {pos.x + totalW, pos.y + height});
    ImGui::ItemSize(totalBb, std::max(0.f, (height - textSize.y) * .5f));

    const InteractResult res = Interact(id, totalBb.Min, totalBb.Max, InteractFlags_None);
    const float dot01 = AnimatePart(ImHashData(&id, sizeof id, 0x72u), active ? 1.f : 0.f);
    if (!res.visible)
        return res.pressed;

    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float outerR = boxSize * .5f;
    const ImVec2 center{pos.x + outerR, pos.y + height * .5f};
    const ImVec2 boxMin{center.x - outerR, center.y - outerR};
    const ImVec2 boxMax{center.x + outerR, center.y + outerR};
    const float borderW = r.Var(PartVar::BorderWidth);

    ImVec4 fill = r.Color(res.disabled ? PartState::Disabled : PartState::Rest, PartColor::Fill);
    if (!res.disabled)
        fill = detail::MixColor(fill, r.Color(PartState::Hover, PartColor::Fill), res.hover01);
    draw->AddCircleFilled(center, outerR, ImGui::GetColorU32(fill), 24);

    if (res.disabled) {
        const float bw = std::max(1.f, borderW);
        DrawDashedCircle(draw, center, outerR - bw * .5f,
                         ImGui::GetColorU32(r.Color(PartState::Disabled, PartColor::Border)), bw,
                         std::max(3.f, bw * 2.5f));
    } else {
        const ImVec4 restBorder = r.Color(PartState::Rest, PartColor::Border);
        const ImVec4 onBorder = r.Color(PartState::On, PartColor::Mark);
        const ImVec4 borderCol = detail::MixColor(restBorder, onBorder, dot01);
        const float bw = std::max(1.f, borderW);
        draw->AddCircle(center, outerR - bw * .5f, ImGui::GetColorU32(borderCol), 24, bw);
    }

    if (dot01 > .01f) {
        const float dotR = std::clamp(r.Var(PartVar::MarkWidth) * .5f, 2.f, outerR - 2.f) * dot01;
        ImVec4 markCol = r.Color(res.disabled ? PartState::Disabled : PartState::On, PartColor::Mark);
        if (!res.disabled && !detail::CurrentIsDark()) {
            const ImVec4 textInk = r.Color(PartState::Rest, PartColor::Text);
            markCol = detail::MixToContrast(markCol, textInk, 0.f, 3.0f, {fill});
        }
        draw->AddCircleFilled(center, dotR, ImGui::GetColorU32(markCol), 20);
    }

    if (res.focus01 > 0.f)
        DrawFocusRing(draw, boxMin, boxMax, outerR, res.focus01, r.Var(PartVar::FocusRing));

    if (!visible.empty()) {
        const ImVec4 textCol =
            r.Color(res.disabled ? PartState::Disabled : (active ? PartState::On : PartState::Rest), PartColor::Text);
        const ImVec2 textPos{boxMax.x + gap, pos.y + (height - textSize.y) * .5f};
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, ImGui::GetColorU32(textCol), visible.data(),
                      visible.data() + visible.size());
    }

    return res.pressed;
}

bool RadioButton(const char *label, int *value, int buttonValue) {
    return imkit::RadioButton(label, value, buttonValue, RadioStyle{});
}
bool RadioButton(const char *label, int *value, int buttonValue, const RadioStyle &style) {
    const bool pressed = imkit::RadioButton(label, *value == buttonValue, style);
    if (pressed)
        *value = buttonValue;
    return pressed;
}

bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format,
                 ImGuiSliderFlags flags) {
    return imkit::SliderFloat(label, value, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format,
                 ImGuiSliderFlags flags, const SliderStyle &style) {
    return DrawModernSlider(
        label, value, minimum, maximum, format, "%.3f", flags, style, false, {},
        [](const char *l, float *v, float mn, float mx, const char *fmt, ImGuiSliderFlags fl) {
            return ImGui::SliderFloat(l, v, mn, mx, fmt, fl);
        });
}
bool SliderInt(const char *label, int *value, int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    return imkit::SliderInt(label, value, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderInt(const char *label, int *value, int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
               const SliderStyle &style) {
    return DrawModernSlider(
        label, value, minimum, maximum, format, "%d", flags, style, false, {},
        [](const char *l, int *v, int mn, int mx, const char *fmt, ImGuiSliderFlags fl) {
            return ImGui::SliderInt(l, v, mn, mx, fmt, fl);
        });
}
bool VSliderFloat(const char *label, const ImVec2 &size, float *value, float minimum, float maximum,
                  const char *format, ImGuiSliderFlags flags) {
    return imkit::VSliderFloat(label, size, value, minimum, maximum, format, flags, SliderStyle{});
}
bool VSliderFloat(const char *label, const ImVec2 &size, float *value, float minimum, float maximum,
                  const char *format, ImGuiSliderFlags flags, const SliderStyle &style) {
    return DrawModernSlider(
        label, value, minimum, maximum, format, "%.3f", flags, style, true, size,
        [&](const char *l, float *v, float mn, float mx, const char *fmt, ImGuiSliderFlags fl) {
            return ImGui::VSliderFloat(l, size, v, mn, mx, fmt, fl);
        });
}

bool SliderFloat2(const char *label, float v[2], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags) {
    return imkit::SliderFloat2(label, v, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderFloat2(const char *label, float v[2], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags, const SliderStyle &style) {
    return SliderComponents(label, v, 2, minimum, maximum, format, flags, style,
                            static_cast<bool (*)(const char *, float *, float, float, const char *, ImGuiSliderFlags,
                                                 const SliderStyle &)>(imkit::SliderFloat));
}
bool SliderFloat3(const char *label, float v[3], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags) {
    return imkit::SliderFloat3(label, v, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderFloat3(const char *label, float v[3], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags, const SliderStyle &style) {
    return SliderComponents(label, v, 3, minimum, maximum, format, flags, style,
                            static_cast<bool (*)(const char *, float *, float, float, const char *, ImGuiSliderFlags,
                                                 const SliderStyle &)>(imkit::SliderFloat));
}
bool SliderFloat4(const char *label, float v[4], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags) {
    return imkit::SliderFloat4(label, v, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderFloat4(const char *label, float v[4], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags, const SliderStyle &style) {
    return SliderComponents(label, v, 4, minimum, maximum, format, flags, style,
                            static_cast<bool (*)(const char *, float *, float, float, const char *, ImGuiSliderFlags,
                                                 const SliderStyle &)>(imkit::SliderFloat));
}
bool SliderInt2(const char *label, int v[2], int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    return imkit::SliderInt2(label, v, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderInt2(const char *label, int v[2], int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
                const SliderStyle &style) {
    return SliderComponents(label, v, 2, minimum, maximum, format, flags, style,
                            static_cast<bool (*)(const char *, int *, int, int, const char *, ImGuiSliderFlags,
                                                 const SliderStyle &)>(imkit::SliderInt));
}
bool SliderInt3(const char *label, int v[3], int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    return imkit::SliderInt3(label, v, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderInt3(const char *label, int v[3], int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
                const SliderStyle &style) {
    return SliderComponents(label, v, 3, minimum, maximum, format, flags, style,
                            static_cast<bool (*)(const char *, int *, int, int, const char *, ImGuiSliderFlags,
                                                 const SliderStyle &)>(imkit::SliderInt));
}
bool SliderInt4(const char *label, int v[4], int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    return imkit::SliderInt4(label, v, minimum, maximum, format, flags, SliderStyle{});
}
bool SliderInt4(const char *label, int v[4], int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
                const SliderStyle &style) {
    return SliderComponents(label, v, 4, minimum, maximum, format, flags, style,
                            static_cast<bool (*)(const char *, int *, int, int, const char *, ImGuiSliderFlags,
                                                 const SliderStyle &)>(imkit::SliderInt));
}

bool DragFloat(const char *label, float *value, float speed, float minimum, float maximum, const char *format,
               ImGuiSliderFlags flags) {
    return imkit::DragFloat(label, value, speed, minimum, maximum, format, flags, DragStyle{});
}
bool DragFloat(const char *label, float *value, float speed, float minimum, float maximum, const char *format,
               ImGuiSliderFlags flags, const DragStyle &style) {
    return DrawModernDrag(label, style, [&] {
        return ImGui::DragFloat(label, value, speed, minimum, maximum, format, flags);
    });
}
bool DragInt(const char *label, int *value, float speed, int minimum, int maximum, const char *format,
             ImGuiSliderFlags flags) {
    return imkit::DragInt(label, value, speed, minimum, maximum, format, flags, DragStyle{});
}
bool DragInt(const char *label, int *value, float speed, int minimum, int maximum, const char *format,
             ImGuiSliderFlags flags, const DragStyle &style) {
    return DrawModernDrag(label, style, [&] {
        return ImGui::DragInt(label, value, speed, minimum, maximum, format, flags);
    });
}

bool InputText(const char *label, char *buffer, std::size_t bufferSize, ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback, void *userData) {
    return imkit::InputText(label, buffer, bufferSize, flags, callback, userData, InputStyle{});
}
bool InputText(const char *label, char *buffer, std::size_t bufferSize, ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback, void *userData, const InputStyle &style) {
    return DrawModernInput(label, style, false, [&] {
        return ImGui::InputText(label, buffer, bufferSize, flags, callback, userData);
    });
}
bool InputFloat(const char *label, float *value, float step, float stepFast, const char *format,
                ImGuiInputTextFlags flags) {
    return imkit::InputFloat(label, value, step, stepFast, format, flags, InputStyle{});
}
bool InputFloat(const char *label, float *value, float step, float stepFast, const char *format,
                ImGuiInputTextFlags flags, const InputStyle &style) {
    return DrawModernInput(label, style, step > 0.f, [&] {
        return ImGui::InputFloat(label, value, step, stepFast, format, flags);
    });
}
bool InputInt(const char *label, int *value, int step, int stepFast, ImGuiInputTextFlags flags) {
    return imkit::InputInt(label, value, step, stepFast, flags, InputStyle{});
}
bool InputInt(const char *label, int *value, int step, int stepFast, ImGuiInputTextFlags flags,
              const InputStyle &style) {
    return DrawModernInput(label, style, step > 0, [&] {
        return ImGui::InputInt(label, value, step, stepFast, flags);
    });
}
} // namespace imkit
