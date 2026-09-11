#include <imkit/window_frame.h>

#include <array>
#include <cassert>

int main() {
    const auto theme = imkit::MakeTheme(imkit::ThemePreset::Graphite);
    const std::array presets{imkit::WindowFramePreset::Native, imkit::WindowFramePreset::Studio,
                             imkit::WindowFramePreset::Workspace, imkit::WindowFramePreset::Tool};
    for (const auto preset : presets) {
        const auto style = imkit::MakeWindowFrameStyle(preset, theme);
        const auto layout = imkit::LayoutWindowFrame(1280, style);
        if (preset == imkit::WindowFramePreset::Native) assert(layout.titleBar.Height() == 0);
        else assert(layout.titleBar.Height() == style.metrics.height);
    }
    auto style = imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Workspace, theme);
    imkit::WindowFrameState state;
    state.dpiScale = 2;
    auto layout = imkit::LayoutWindowFrame(1600, style, state);
    assert(layout.titleBar.Height() == style.metrics.height * 2);
    assert(layout.applicationName.Width() > 0 && layout.projectName.Width() > 0);
    assert(layout.workspaceSwitcher.Width() > 0 && layout.close.Width() == style.metrics.buttonWidth * 2);
    style.features.close = false;
    assert(imkit::LayoutWindowFrame(1600, style, state).close.Width() == 0);
    style.metrics.buttonWidth = 55;
    assert(imkit::LayoutWindowFrame(1600, style, state).maximizeRestore.Width() == 110);

    ImGuiContext* context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = {640, 480};
    io.DeltaTime = 1.f / 60.f;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    ImGui::NewFrame();
    const auto elided = imkit::ElideWindowFrameTitle("Window / 長いUTF-8タイトル", 70, ImGui::GetFont(), 13);
    assert(elided.ends_with("...") && elided.find('\0') == std::string::npos);
    style.activeBackground = style.titleText;
    assert(!imkit::ValidateWindowFrameContrast(style).valid);
    ImGui::EndFrame();
    ImGui::DestroyContext(context);
    return 0;
}
