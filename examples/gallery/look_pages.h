#pragma once
#include <imkit/imkit.h>
#include <filesystem>
#include <string>
#include <vector>

namespace imkit::gallery {
struct GalleryState;

// Pages for building a look: the Theme Studio edits the live theme through the public theme API and shows the text that
// reproduces it; the Skins page shows what a Skin, a backdrop pattern and a nine-slice picture can do.
struct LookPages {
    // Studio controls. They are the source of the theme text, which ParseTheme applies to the Gallery theme.
    BrandPalette brand;
    bool deriveBorder = true;
    int corner = 1;                // CornerStyle, or 4 when the radii are custom.
    float border = 1.f;
    bool hard = false;
    float shadowOffset[2] = {0.f, 3.f};
    ImVec4 shadow{.2f, .1f, .05f, .45f};
    int density = 1, preset = 0;
    bool synced = false;
    // The theme file being edited outside the Gallery.
    char file[260]{};
    bool watch = false;
    std::filesystem::file_time_type fileTime{};
    double lastPoll = 0;
    std::vector<std::filesystem::path> worlds;
    bool worldsSearched = false;
    std::string text, warnings, fileMessage;
    // Skins page.
    BackdropStyle backdrop;
    float nineSliceBorder = 24.f;
    SpriteState sprite;
    int spritePlayback = 0;
    float spriteFps = 12.f;
    bool spriteAnimate = true, progressAuto = true;
    float progressValue = .35f;

    void Studio(GalleryState& s);
    void Skins(GalleryState& s);
    // Reads a theme file onto a clean base theme (Precision Light) and keeps watching it when asked to.
    bool LoadFile(GalleryState& s, const std::filesystem::path& path);
    // Called every frame by the Gallery: reloads the theme file when "Watch" is on and the file changed on disk.
    void Poll(GalleryState& s);

  private:
    void SyncFromTheme(const Theme& theme);
    std::string Compose() const;
    void Apply(GalleryState& s);
    void FindWorlds();
};
} // namespace imkit::gallery
