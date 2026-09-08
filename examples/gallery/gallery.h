#pragma once

namespace imkit::gallery {

struct ColumnState {
    int buttonClicks = 0;
    bool checked = false;
    float value = 0.5F;
    char text[64] = "Sample text";
    bool selected = false;
};

struct GalleryState {
    ColumnState standard;
    ColumnState wrapper;
    bool showDearImGuiDemo = false;
};

void Show(GalleryState& state);

}  // namespace imkit::gallery

