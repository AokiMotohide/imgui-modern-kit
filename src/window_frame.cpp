#include <imkit/window_frame.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>

namespace imkit {
namespace {
ImVec4 Mix(ImVec4 a, ImVec4 b, float amount) {
    return {a.x + (b.x - a.x) * amount, a.y + (b.y - a.y) * amount,
            a.z + (b.z - a.z) * amount, a.w + (b.w - a.w) * amount};
}
ImU32 Color(ImVec4 value) { return ImGui::ColorConvertFloat4ToU32(value); }
WindowFrameRect TakeRight(float& right, float width, float height, bool visible) {
    if (!visible) return {};
    width = std::clamp(width, 0.f, std::max(0.f, right));
    WindowFrameRect result{{right - width, 0}, {right, height}};
    right -= width;
    return result;
}
void CaptionGlyph(ImDrawList* draw, const WindowFrameRect& rect, int index, bool maximized,
                  ImU32 ink, float scale) {
    const ImVec2 center{(rect.min.x + rect.max.x) * .5f, (rect.min.y + rect.max.y) * .5f};
    const float d = 4.f * scale, stroke = std::max(1.f, scale);
    if (index == 0) draw->AddLine({center.x - d, center.y}, {center.x + d, center.y}, ink, stroke);
    if (index == 1) {
        if (maximized) {
            const std::array<ImVec2, 3> back{{{center.x - 2 * scale, center.y - 5 * scale},
                                              {center.x + 5 * scale, center.y - 5 * scale},
                                              {center.x + 5 * scale, center.y + 2 * scale}}};
            draw->AddPolyline(back.data(), static_cast<int>(back.size()), ink, 0, stroke);
        }
        draw->AddRect({center.x - d, center.y - d}, {center.x + d, center.y + d}, ink, 0, 0, stroke);
    }
    if (index == 2) {
        draw->AddLine({center.x - d, center.y - d}, {center.x + d, center.y + d}, ink, stroke);
        draw->AddLine({center.x - d, center.y + d}, {center.x + d, center.y - d}, ink, stroke);
    }
}
} // namespace

WindowFrameStyle MakeWindowFrameStyle(WindowFramePreset preset, const Theme& theme) {
    WindowFrameStyle style;
    const auto& semantic = theme.semantic;
    style.activeBackground = semantic.surface;
    style.inactiveBackground = Mix(semantic.surface, semantic.canvas, .35f);
    style.border = semantic.border;
    style.titleText = semantic.text;
    style.auxiliaryText = semantic.textSecondary;
    style.icon = semantic.accent;
    style.buttonText = semantic.text;
    style.buttonHover = semantic.control.hover;
    style.buttonPressed = semantic.control.pressed;
    style.closeButtonHover = theme.colors.destructive;
    style.closeButtonPressed = Mix(theme.colors.destructive, semantic.canvas, .18f);
    switch (preset) {
    case WindowFramePreset::Native:
        style.metrics.height = 0;
        style.features = {false, false, false, false, false, false, false, false};
        break;
    case WindowFramePreset::Studio:
        style.metrics = {32, 32, 4, 12, 46, 1};
        style.features = {true, false, true, false, false, true, true, true};
        break;
    case WindowFramePreset::Workspace:
        style.metrics = {36, 40, 6, 12, 46, 1};
        style.features = {true, true, true, true, true, true, true, true};
        break;
    case WindowFramePreset::Tool:
        style.metrics = {28, 0, 8, 8, 40, 1};
        style.features = {false, false, true, false, false, false, false, true};
        break;
    }
    return style;
}

WindowFrameLayout LayoutWindowFrame(float widthPixels, const WindowFrameStyle& style,
                                    const WindowFrameState& state) {
    WindowFrameLayout layout;
    layout.scale = std::max(.01f, std::isfinite(state.dpiScale) ? state.dpiScale : 1.f);
    const auto& metrics = style.metrics;
    const float height = std::max(0.f, metrics.height * layout.scale);
    const float button = std::max(0.f, metrics.buttonWidth * layout.scale);
    layout.titleBar = {{0, 0}, {std::max(0.f, widthPixels), height}};
    float left = std::max(0.f, state.leadingSystemAreaDip * layout.scale);
    if (style.features.icon && metrics.iconAreaWidth > 0) {
        layout.icon = {{left, 0}, {left + metrics.iconAreaWidth * layout.scale, height}};
        left = layout.icon.max.x;
    }
    float right = layout.titleBar.max.x;
    if (!state.systemCaptionButtons) {
        layout.close = TakeRight(right, button, height, style.features.close);
        layout.maximizeRestore = TakeRight(right, button, height, style.features.maximizeRestore);
        layout.minimize = TakeRight(right, button, height, style.features.minimize);
    }
    const float innerLeft = std::min(right, left + std::max(0.f, metrics.titlePaddingLeft) * layout.scale);
    const float innerRight = std::max(innerLeft, right - std::max(0.f, metrics.titlePaddingRight) * layout.scale);
    float cursor = innerLeft;
    if (style.features.applicationName) {
        const float width = std::min(160.f * layout.scale, std::max(0.f, (innerRight - cursor) * .3f));
        layout.applicationName = {{cursor, 0}, {cursor + width, height}};
        cursor += width;
    }
    if (style.features.unsavedIndicator) {
        layout.unsavedIndicator = {{cursor, 0}, {std::min(innerRight, cursor + 16.f * layout.scale), height}};
        cursor = layout.unsavedIndicator.max.x;
    }
    float workspaceWidth = 0;
    if (style.features.workspaceSwitcher)
        workspaceWidth = std::min(180.f * layout.scale, std::max(0.f, innerRight - cursor) * .35f);
    layout.workspaceSwitcher = {{innerRight - workspaceWidth, 0}, {innerRight, height}};
    layout.projectName = {{cursor, 0}, {std::max(cursor, innerRight - workspaceWidth), height}};
    return layout;
}

std::string ElideWindowFrameTitle(std::string_view title, float widthPixels, ImFont* font, float fontSize) {
    if (!font || widthPixels <= 0 || fontSize <= 0) return {};
    const auto measure = [&](std::string_view value) {
        return font->CalcTextSizeA(fontSize, FLT_MAX, 0, value.data(), value.data() + value.size()).x;
    };
    if (measure(title) <= widthPixels) return std::string(title);
    constexpr std::string_view suffix = "...";
    if (measure(suffix) > widthPixels) return {};
    std::string result(title);
    while (!result.empty()) {
        std::size_t start = result.size() - 1;
        while (start && (static_cast<unsigned char>(result[start]) & 0xc0) == 0x80) --start;
        result.resize(start);
        if (measure(result) + measure(suffix) <= widthPixels) break;
    }
    return result + std::string(suffix);
}

WindowFrameResult DrawWindowFrame(const WindowFrameStyle& style, const WindowFrameContent& content,
                                  const WindowFrameLayout& layout, const WindowFrameState& state) {
    WindowFrameResult result{layout, state.pendingEvent};
    if (layout.titleBar.Height() <= 0) return result;
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    draw->AddRectFilled(layout.titleBar.min, layout.titleBar.max,
                        Color(state.active ? style.activeBackground : style.inactiveBackground));
    const float border = std::max(0.f, style.metrics.borderWidth * layout.scale);
    if (border > 0) draw->AddLine({0, layout.titleBar.max.y - border * .5f},
                                  {layout.titleBar.max.x, layout.titleBar.max.y - border * .5f},
                                  Color(style.border), border);
    if (style.features.icon && layout.icon.Width() > 0) {
        const ImVec2 center{(layout.icon.min.x + layout.icon.max.x) * .5f,
                            (layout.icon.min.y + layout.icon.max.y) * .5f};
        const float d = std::min(layout.icon.Width(), layout.icon.Height()) * .2f;
        draw->AddRect({center.x - d, center.y - d}, {center.x + d, center.y + d}, Color(style.icon),
                      2 * layout.scale, 0, std::max(1.f, 1.5f * layout.scale));
        draw->AddLine({center.x - d * .55f, center.y + d * .1f},
                      {center.x + d * .55f, center.y - d * .55f}, Color(style.icon),
                      std::max(1.f, 1.5f * layout.scale));
    }
    ImFont* font = ImGui::GetFont();
    const float fontSize = std::max(1.f, std::min(14.f * layout.scale, layout.titleBar.Height() - 4.f * layout.scale));
    const auto drawText = [&](const WindowFrameRect& rect, std::string_view text, ImVec4 color) {
        if (rect.Width() <= 0 || text.empty()) return;
        const auto shown = ElideWindowFrameTitle(text, rect.Width() - 6.f * layout.scale, font, fontSize);
        draw->PushClipRect(rect.min, rect.max, true);
        draw->AddText(font, fontSize, {rect.min.x, rect.min.y + (rect.Height() - fontSize) * .5f}, Color(color), shown.c_str());
        draw->PopClipRect();
    };
    if (style.features.applicationName) drawText(layout.applicationName, content.applicationName, style.auxiliaryText);
    if (style.features.projectName) drawText(layout.projectName, content.projectName, state.active ? style.titleText : style.auxiliaryText);
    if (style.features.unsavedIndicator && content.unsaved && layout.unsavedIndicator.Width() > 0) {
        const ImVec2 center{(layout.unsavedIndicator.min.x + layout.unsavedIndicator.max.x) * .5f,
                            (layout.unsavedIndicator.min.y + layout.unsavedIndicator.max.y) * .5f};
        draw->AddCircleFilled(center, 2.5f * layout.scale, Color(style.auxiliaryText));
    }
    if (style.features.workspaceSwitcher && !content.workspaces.empty()) {
        const auto selected = std::min(content.selectedWorkspace, content.workspaces.size() - 1);
        drawText(layout.workspaceSwitcher, content.workspaces[selected], style.auxiliaryText);
        const float x = layout.workspaceSwitcher.max.x - 8.f * layout.scale;
        const float y = (layout.workspaceSwitcher.min.y + layout.workspaceSwitcher.max.y) * .5f;
        draw->AddTriangleFilled({x - 3.f * layout.scale, y - 1.f * layout.scale},
                                {x + 3.f * layout.scale, y - 1.f * layout.scale},
                                {x, y + 2.f * layout.scale}, Color(style.auxiliaryText));
    }
    const std::array<WindowFrameRect, 3> buttons{layout.minimize, layout.maximizeRestore, layout.close};
    for (int index = 0; index < 3; ++index) {
        if (buttons[index].Width() <= 0) continue;
        if (state.hoveredButton == index) {
            const bool down = state.pressedButton == index;
            const ImVec4 fill = index == 2 ? (down ? style.closeButtonPressed : style.closeButtonHover)
                                           : (down ? style.buttonPressed : style.buttonHover);
            draw->AddRectFilled(buttons[index].min, buttons[index].max, Color(fill));
        }
        CaptionGlyph(draw, buttons[index], index, state.maximized, Color(style.buttonText), layout.scale);
    }
    if (style.features.workspaceSwitcher && layout.workspaceSwitcher.Width() > 0 && !content.workspaces.empty()) {
        ImGui::SetNextWindowPos(layout.workspaceSwitcher.min);
        ImGui::SetNextWindowSize({layout.workspaceSwitcher.Width(), layout.workspaceSwitcher.Height()});
        ImGui::SetNextWindowBgAlpha(0);
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus;
        if (ImGui::Begin("##imkit_window_frame_workspace", nullptr, flags)) {
            const auto index = std::min(content.selectedWorkspace, content.workspaces.size() - 1);
            const std::string preview(content.workspaces[index]);
            ImGui::SetCursorPos({0, 0});
            ImGui::SetNextItemWidth(layout.workspaceSwitcher.Width());
            if (ImGui::BeginCombo("##workspace", preview.c_str(), ImGuiComboFlags_HeightSmall)) {
                for (std::size_t candidate = 0; candidate < content.workspaces.size(); ++candidate) {
                    const std::string label(content.workspaces[candidate]);
                    ImGui::PushID(static_cast<int>(candidate));
                    if (ImGui::Selectable(label.c_str(), candidate == index))
                        result.event = {WindowFrameEventType::WorkspaceSelected, WindowFrameOperation::None, candidate};
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
        }
        ImGui::End();
    }
    return result;
}

WindowFrameContrast ValidateWindowFrameContrast(const WindowFrameStyle& style) {
    WindowFrameContrast result;
    result.activeTitle = ContrastRatio(style.titleText, style.activeBackground);
    result.inactiveTitle = ContrastRatio(style.auxiliaryText, style.inactiveBackground);
    result.auxiliary = ContrastRatio(style.auxiliaryText, style.activeBackground);
    result.icon = ContrastRatio(style.icon, style.activeBackground);
    result.button = std::min(ContrastRatio(style.buttonText, style.buttonHover),
                             ContrastRatio(style.buttonText, style.buttonPressed));
    result.closeButton = std::min(ContrastRatio(style.buttonText, style.closeButtonHover),
                                  ContrastRatio(style.buttonText, style.closeButtonPressed));
    result.valid = result.activeTitle >= 4.5f && result.inactiveTitle >= 4.5f &&
                   result.auxiliary >= 4.5f && result.icon >= 3.f &&
                   result.button >= 3.f && result.closeButton >= 3.f;
    return result;
}

} // namespace imkit
