#pragma once
#include <imkit/icons.h>
namespace node_gallery {
inline bool Action(const char *id, const imkit::IconAtlas &atlas, imkit::IconId icon, const char *label) {
    imkit::IconOptions options;
    options.size = ImGui::GetFontSize();
    return imkit::IconButton(id, atlas, icon, label, options);
}
} // namespace node_gallery
