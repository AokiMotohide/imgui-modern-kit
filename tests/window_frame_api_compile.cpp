#include <imkit/window_frame.h>
#ifdef _WIN32
#include <imkit/window_frame_win32.h>
#endif
#ifdef __APPLE__
#include <imkit/window_frame_macos.h>
#endif

#include <array>

int main() {
    imkit::Theme theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
    imkit::WindowFrameStyle style = imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Workspace, theme);
    style.activeBackground = {.1f, .1f, .1f, 1};
    style.metrics.height = 36;
    style.features.workspaceSwitcher = true;
    const std::array<std::string_view, 2> workspaces{"Edit", "Output"};
    imkit::WindowFrameContent content{"Application", "Project", true, workspaces, 0};
    imkit::WindowFrameState state{};
    const auto layout = imkit::LayoutWindowFrame(1280, style, state);
    const auto contrast = imkit::ValidateWindowFrameContrast(style);
    const auto title = imkit::ElideWindowFrameTitle("Title", 100, nullptr, 13);
    imkit::WindowFrameEvent event{imkit::WindowFrameEventType::Operation,
                                  imkit::WindowFrameOperation::Close, 0};
    imkit::WindowFrameResult result{layout, event};
    (void)content; (void)contrast; (void)title; (void)result;
#ifdef _WIN32
    imkit::WindowFrameWin32Adapter adapter;
    adapter.SetLayout(layout);
    adapter.Execute(imkit::WindowFrameOperation::None);
#endif
#ifdef __APPLE__
    imkit::WindowFrameMacOSAdapter adapter;
    adapter.Configure(imkit::WindowFramePreset::Native);
    adapter.Execute(imkit::WindowFrameOperation::None);
#endif
    return 0;
}
