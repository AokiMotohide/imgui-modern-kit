#include <imkit/icons.h>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <vector>

namespace imkit {
namespace {
// Generated from the individually generated source images by tools/build_icons.py.
#include "icons_data.inc"
static_assert(std::size(kCatalog) == static_cast<std::size_t>(IconId::Count));
static_assert(kAtlasColumns * kAtlasRows >= std::size(kCatalog));
int Index(int size) {
    for (int i = 0; i < static_cast<int>(IconPixelSizes.size()); ++i)
        if (IconPixelSizes[i] == size)
            return i;
    return -1;
}
float Size(IconOptions o) {
    return std::isfinite(o.size) && o.size > 0 ? o.size : 20.f;
}
int Level(float size) {
    const auto scale = ImGui::GetIO().DisplayFramebufferScale;
    const float pixels = size * std::max({1.f, scale.x, scale.y});
    for (int i = 0; i < 5; ++i)
        if (IconPixelSizes[i] >= pixels)
            return i;
    return 5;
}
bool Ready(const IconAtlas &atlas, IconId id, int level) {
    return GetIconInfo(id) && atlas.textures[level].GetTexID() != ImTextureID{};
}
ImU32 Color(IconOptions o) {
    return o.color ? ImGui::GetColorU32(*o.color) : ImGui::GetColorU32(ImGuiCol_Text);
}
void Draw(const IconAtlas &atlas, IconId id, int level, ImVec2 pos, float size, ImU32 color) {
    if (!Ready(atlas, id, level))
        return;
    const auto r = GetIconRegion(id, IconPixelSizes[level]);
    ImGui::GetWindowDrawList()->AddImage(atlas.textures[level], pos,
                                       {pos.x + size, pos.y + size}, r.uv0, r.uv1, color);
}
bool ButtonImpl(const char *id, const IconAtlas &atlas, IconId icon, const char *label,
                bool showLabel, IconOptions options) {
    const float size = Size(options);
    const int level = Level(size);
    const auto &style = ImGui::GetStyle();
    const char *text = label ? label : "";
    const std::string_view view(text);
    const auto visibleLength = view.substr(0, view.find("##")).size();
    const ImVec2 textSize = showLabel ? ImGui::CalcTextSize(text, text + visibleLength) : ImVec2{};
    const float gap = showLabel && visibleLength ? style.ItemInnerSpacing.x : 0;
    const ImVec2 extent{size + gap + textSize.x + 2 * style.FramePadding.x,
                        std::max(size, textSize.y) + 2 * style.FramePadding.y};
    ImGui::PushID(id);
    // A visible label remains a complete fallback when the optional texture is
    // unavailable. Icon-only controls stay disabled because their meaning vanishes.
    ImGui::BeginDisabled(!showLabel && !Ready(atlas, icon, level));
    if (showLabel)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
    bool pressed = ImGui::Button(showLabel ? text : "##icon", extent);
    if (showLabel)
        ImGui::PopStyleColor();
    if(options.accessibility) {
        using namespace accessibility;
        SemanticNode n; n.id=ImGui::GetItemID(); n.parent=options.parent; n.role=SemanticRole::Button;
        n.name=view.substr(0,visibleLength); n.actions=SemanticAction::Press|SemanticAction::Focus;
        bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
        bool requested=options.accessibility->Take(n.id,SemanticAction::Press);
        if(options.accessibility->Take(n.id,SemanticAction::Focus) && !disabled) ImGui::SetKeyboardFocusHere(-1);
        pressed=pressed || (requested && !disabled);
        AnnotateLastItem(*options.accessibility,n);
    }
    if(ImGui::IsItemFocused()) ImGui::SetNavCursorVisible(true);
    const auto p = ImGui::GetItemRectMin();
    Draw(atlas, icon, level, {p.x + style.FramePadding.x, p.y + (extent.y - size) / 2},
         size, Color(options));
    if (showLabel)
        ImGui::GetWindowDrawList()->AddText(
            {p.x + style.FramePadding.x + size + gap, p.y + (extent.y - textSize.y) / 2},
            ImGui::GetColorU32(ImGuiCol_Text), text, text + visibleLength);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemFocused())
        ImGui::SetTooltip("%s", text);
    ImGui::EndDisabled();
    ImGui::PopID();
    return pressed;
}
} // namespace
std::span<const IconInfo> GetIconCatalog() { return kCatalog; }
const IconInfo *GetIconInfo(IconId id) {
    const auto index = static_cast<std::size_t>(id);
    return index < std::size(kCatalog) ? &kCatalog[index] : nullptr;
}
IconAtlasPixels GetIconAtlasPixels(int iconPixels) {
    const int index = Index(iconPixels);
    if (index < 0)
        return {};
    // Thread-safe initialization; independent of any ImGui context or GPU.
    static const auto decoded = [] {
        std::array<std::vector<unsigned char>, 7> result;
        for (int i = 0; i < 7; ++i) {
            auto &rgba = result[i];
            const int cell = IconPixelSizes[i] + 4;
            rgba.reserve(cell * cell * kAtlasColumns * kAtlasRows * 4);
            for (std::size_t j = kOffsets[i]; j < kOffsets[i + 1]; j += 2)
                for (int n = 0; n < kAlphaRle[j]; ++n)
                    rgba.insert(rgba.end(), {255, 255, 255, kAlphaRle[j + 1]});
        }
        return result;
    }();
    const int cell = iconPixels + 4;
    return {cell * kAtlasColumns, cell * kAtlasRows, iconPixels, decoded[index]};
}
IconRegion GetIconRegion(IconId id, int iconPixels) {
    if (!GetIconInfo(id) || Index(iconPixels) < 0)
        return {};
    const int cell = iconPixels + 4;
    const int index = static_cast<int>(id);
    const float x = static_cast<float>((index % kAtlasColumns) * cell + 2);
    const float y = static_cast<float>((index / kAtlasColumns) * cell + 2);
    return {{x / (cell * kAtlasColumns), y / (cell * kAtlasRows)},
            {(x + iconPixels) / (cell * kAtlasColumns), (y + iconPixels) / (cell * kAtlasRows)}};
}
bool IconAtlas::SetTexture(int iconPixels, ImTextureRef texture) {
    const int i = Index(iconPixels);
    if (i < 0)
        return false;
    textures[i] = texture;
    return true;
}
void IconAtlas::Clear() { textures = {}; }
void Icon(const IconAtlas &atlas, IconId icon, IconOptions options) {
    const float size = Size(options);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy({size, size});
    Draw(atlas, icon, Level(size), pos, size, Color(options));
}
bool IconButton(const char *id, const IconAtlas &atlas, IconId icon, const char *accessibleLabel,
                IconOptions options) {
    return ButtonImpl(id, atlas, icon, accessibleLabel, false, options);
}
bool IconLabelButton(const char *id, const IconAtlas &atlas, IconId icon, const char *label,
                     IconOptions options) {
    return ButtonImpl(id, atlas, icon, label, true, options);
}
} // namespace imkit
