#pragma once

#include <imkit/theme.h>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace imkit {

enum class WindowFramePreset { Native, Studio, Workspace, Tool };

struct WindowFrameMetrics {
    float height = 32.f;
    float iconAreaWidth = 32.f;
    float titlePaddingLeft = 4.f;
    float titlePaddingRight = 12.f;
    float buttonWidth = 46.f;
    float borderWidth = 1.f;
};

struct WindowFrameFeatures {
    bool icon = true;
    bool applicationName = false;
    bool projectName = true;
    bool unsavedIndicator = false;
    bool workspaceSwitcher = false;
    bool minimize = true;
    bool maximizeRestore = true;
    bool close = true;
};

// Complete, host-owned value. Regenerating from a Theme is always explicit.
struct WindowFrameStyle {
    ImVec4 activeBackground{};
    ImVec4 inactiveBackground{};
    ImVec4 border{};
    ImVec4 titleText{};
    ImVec4 auxiliaryText{};
    ImVec4 icon{};
    ImVec4 buttonText{};
    ImVec4 buttonHover{};
    ImVec4 buttonPressed{};
    ImVec4 closeButtonHover{};
    ImVec4 closeButtonPressed{};
    WindowFrameMetrics metrics{};
    WindowFrameFeatures features{};
};

struct WindowFrameContent {
    std::string_view applicationName{};
    std::string_view projectName{};
    bool unsaved = false;
    std::span<const std::string_view> workspaces{};
    std::size_t selectedWorkspace = 0;
};

enum class WindowFrameOperation { None, Minimize, MaximizeRestore, Close, SystemMenu };
enum class WindowFrameEventType { None, Operation, WorkspaceSelected };

struct WindowFrameEvent {
    WindowFrameEventType type = WindowFrameEventType::None;
    WindowFrameOperation operation = WindowFrameOperation::None;
    std::size_t workspace = 0;
};

struct WindowFrameRect {
    ImVec2 min{}, max{};
    [[nodiscard]] bool Contains(ImVec2 point) const noexcept {
        return point.x >= min.x && point.y >= min.y && point.x < max.x && point.y < max.y;
    }
    [[nodiscard]] float Width() const noexcept { return max.x - min.x; }
    [[nodiscard]] float Height() const noexcept { return max.y - min.y; }
};

struct WindowFrameState {
    float dpiScale = 1.f;
    bool active = true;
    bool maximized = false;
    bool systemCaptionButtons = false;
    float leadingSystemAreaDip = 0.f;
    int hoveredButton = -1;
    int pressedButton = -1;
    WindowFrameEvent pendingEvent{};
};

struct WindowFrameLayout {
    WindowFrameRect titleBar{};
    WindowFrameRect icon{};
    WindowFrameRect applicationName{};
    WindowFrameRect projectName{};
    WindowFrameRect unsavedIndicator{};
    WindowFrameRect workspaceSwitcher{};
    WindowFrameRect minimize{};
    WindowFrameRect maximizeRestore{};
    WindowFrameRect close{};
    float scale = 1.f;
};

struct WindowFrameResult {
    WindowFrameLayout layout{};
    WindowFrameEvent event{};
};

struct WindowFrameContrast {
    float activeTitle = 1.f;
    float inactiveTitle = 1.f;
    float auxiliary = 1.f;
    float icon = 1.f;
    float button = 1.f;
    float closeButton = 1.f;
    bool valid = false;
};

[[nodiscard]] WindowFrameStyle MakeWindowFrameStyle(WindowFramePreset preset, const Theme& theme);
[[nodiscard]] WindowFrameLayout LayoutWindowFrame(float widthPixels, const WindowFrameStyle& style,
                                                   const WindowFrameState& state = {});
[[nodiscard]] WindowFrameResult DrawWindowFrame(const WindowFrameStyle& style,
                                                 const WindowFrameContent& content,
                                                 const WindowFrameLayout& layout,
                                                 const WindowFrameState& state = {});
[[nodiscard]] std::string ElideWindowFrameTitle(std::string_view title, float widthPixels,
                                                ImFont* font, float fontSize);
[[nodiscard]] WindowFrameContrast ValidateWindowFrameContrast(const WindowFrameStyle& style);

} // namespace imkit
