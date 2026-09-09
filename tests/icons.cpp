#include <imkit/imkit.h>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <cstdlib>

namespace {
int failures = 0;
void Check(bool ok, const char *message) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", message);
        ++failures;
    }
}
}
int main() {
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    const auto catalog = imkit::GetIconCatalog();
    std::set<std::string> names, categories;
    Check(catalog.size() == static_cast<std::size_t>(imkit::IconId::Count), "All public IDs have catalog entries");
    Check(static_cast<int>(imkit::IconId::Add) == 0 && static_cast<int>(imkit::IconId::Target) == 119,
          "Original ID boundaries remain stable");
    for (const auto &item : catalog) {
        Check(names.insert(item.name).second, "Unique icon name");
        categories.insert(item.category);
        Check(imkit::GetIconInfo(item.id) == &item, "Stable ID lookup");
    }
    Check(!categories.empty(), "Catalog categories are present");
    Check(!imkit::GetIconInfo(imkit::IconId::Count), "Invalid ID rejected");
    Check(imkit::GetIconAtlasPixels(17).rgba.empty(), "Unsupported raster size rejected");
    for (int size : imkit::IconPixelSizes) {
        const auto pixels = imkit::GetIconAtlasPixels(size);
        Check(pixels.rgba.size() == static_cast<std::size_t>(pixels.width * pixels.height * 4),
              "Decoded RGBA byte count");
        for (const auto &item : catalog) {
            const auto region = imkit::GetIconRegion(item.id, size);
            Check(region.uv0.x >= 0 && region.uv0.y >= 0 && region.uv1.x <= 1 && region.uv1.y <= 1,
                  "UV inside atlas");
            const int x = static_cast<int>(std::lround(region.uv0.x * pixels.width));
            const int y = static_cast<int>(std::lround(region.uv0.y * pixels.height));
            int opaque = 0, transparent = 0;
            for (int row = 0; row < size; ++row)
                for (int col = 0; col < size; ++col) {
                    const auto offset = ((y + row) * pixels.width + x + col) * 4;
                    Check(pixels.rgba[offset] == 255 && pixels.rgba[offset + 1] == 255 &&
                              pixels.rgba[offset + 2] == 255, "White RGB supports tint");
                    opaque += pixels.rgba[offset + 3] > 100;
                    transparent += pixels.rgba[offset + 3] == 0;
                }
            Check(opaque > 0 && transparent > 0, "Every glyph has content and transparency");
        }
    }
    auto *context = ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {800, 600};
    io.DeltaTime = 1.f / 60;
    unsigned char *fontPixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&fontPixels, &width, &height);
    // This CPU-only draw-command test uses synthetic texture IDs, including the font atlas.
    // Actual uploads and rendering are exercised by the Gallery verifier.
    io.Fonts->SetTexID(ImTextureID{1});
    imkit::IconAtlas atlas, missing;
    Check(!atlas.SetTexture(17, ImTextureRef(ImTextureID{7})), "Binding rejects unsupported size");
    for (int size : imkit::IconPixelSizes)
        atlas.SetTexture(size, ImTextureRef(static_cast<ImTextureID>(size)));
    for (int frame = 0; frame < 2; ++frame) {
        ImGui::NewFrame();
        ImGui::SetNextWindowSize({700, 500});
        ImGui::Begin("Icons test");
        ImGui::GetStyle().Alpha = .8f;
        ImGui::BeginDisabled();
        const ImVec4 tint{.2f, .4f, .6f, .5f};
        const auto expected = ImGui::GetColorU32(tint);
        imkit::Icon(atlas, imkit::IconId::Search, {20, tint});
        const auto *draw = ImGui::GetWindowDrawList();
        Check(draw->VtxBuffer.back().col == expected, "Tint and disabled alpha applied once");
        ImGui::EndDisabled();
        const int before = draw->VtxBuffer.Size;
        imkit::Icon(missing, imkit::IconId::Add);
        Check(draw->VtxBuffer.Size == before, "Missing binding emits no textured geometry");
        Check(!imkit::IconButton("missing", missing, imkit::IconId::Add, "Add"),
              "Missing binding button cannot activate");
        io.DisplayFramebufferScale = {2, 2};
        imkit::Icon(atlas, imkit::IconId::Search);
        bool found48 = false;
        for (const auto &command : draw->CmdBuffer)
            found48 |= command.GetTexID() == ImTextureID{48};
        Check(found48, "2x framebuffer chooses a sufficient raster level");
        io.DisplayFramebufferScale = {1, 1};
        ImGui::End();
        ImGui::Render();
    }
    atlas.Clear();
    for (const auto &texture : atlas.textures)
        Check(texture.GetTexID() == ImTextureID{}, "Clear unbinds textures");
    ImGui::DestroyContext(context);
    if (!failures)
        std::puts("PASS icon catalogue, embedded pixels, invalid inputs, tint, disabled alpha, DPI and binding lifetime");
    return failures ? 1 : 0;
}
