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
    // CPU data is embedded: no image loader, file lookup or GPU dependency is needed.
    // A renderer uploads each returned RGBA image, then calls icons.SetTexture(size, texture).
    imkit::IconAtlas icons;
    for (int size : imkit::IconPixelSizes) {
        const auto atlas = imkit::GetIconAtlasPixels(size);
        if (atlas.rgba.empty())
            return 1;
    }
    if (imkit::GetIconCatalog().size() != 120)
        return 1;
    auto theme = imkit::MakePrecisionTheme(imkit::ColorScheme::Dark);
    imkit::ApplyTheme(theme);
    ImGui::NewFrame();
    {
        imkit::ThemeScope scope(theme);
        if (imkit::Begin("Settings")) {
            bool enabled = true;
            imkit::Toggle("Enabled", &enabled, {&theme});
            imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
            imkit::Icon(icons, imkit::IconId::Settings); // Reserved space until host binds texture.
        }
        imkit::End();
    }
    ImGui::Render();
    ImGui::DestroyContext();
    return 0;
}
