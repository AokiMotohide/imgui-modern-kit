#pragma once
#include <imkit/imkit.h>

namespace imkit::gallery {
struct GalleryState;

// The Gallery's front page: the same calls before and after a theme, themes to wear, what is in the box and the code to
// start with. It only reads and writes Gallery state; nothing here is a library feature.
struct StartPage {
    // Before / after specimen. Both sides edit these values.
    int style = 1; // 0 = Dear ImGui default, 1 = ImKit theme.
    float level = .62f;
    bool enabled = true;
    int quality = 1, applied = 0;
    char name[48] = "Display Console";

    void Show(GalleryState &s);
};
} // namespace imkit::gallery
