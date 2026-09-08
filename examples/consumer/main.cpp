#include <imkit/imkit.h>
int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.DisplaySize = {640, 480};
    io.DeltaTime = 1.f / 60;
    io.IniFilename = nullptr;
    io.Fonts->AddFontDefault();
    unsigned char *pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = imkit::MakePrecisionTheme(imkit::ColorScheme::Dark);
    imkit::ApplyTheme(theme);
    ImGui::NewFrame();
    {
        imkit::ThemeScope scope(theme);
        if (imkit::Begin("Settings")) {
            bool enabled = true;
            imkit::Toggle("Enabled", &enabled, {&theme});
            imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
        }
        imkit::End();
    }
    ImGui::Render();
    ImGui::DestroyContext();
    return 0;
}
