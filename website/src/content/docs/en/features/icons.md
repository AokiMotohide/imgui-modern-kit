---
title: "Icons"
description: "ImKit ships 321 outline icons in 21 categories."
---

ImKit ships 321 outline icons in 21 categories. The library draws them from texture atlases that your application uploads, so it needs no image decoder, file lookup or renderer of its own.

![The Gallery Icons page: category and name search, sizes, tint and button states](/imgui-modern-kit/docs-images/v4-icons-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-icons.gif)

## Summary

| Item | Value |
| --- | --- |
| Icons | 321 in 21 categories, listed in `assets/icons/catalog.json` |
| Raster sizes | 12, 16, 20, 24, 32, 48 and 64 px, one atlas per size |
| Colour | White glyphs with alpha, so any tint works on light and dark UI |
| Header | `<imkit/imkit.h>` or `<imkit/icons.h>` |
| Owner of GPU textures | Your application |

## Use the icons

Upload one atlas per size, bind each to an `IconAtlas`, then draw inside a normal frame.

```cpp
imkit::IconAtlas icons;                       // keep it with your renderer resources
for (int size : imkit::IconPixelSizes) {
    const auto pixels = imkit::GetIconAtlasPixels(size);
    // Upload pixels.width x pixels.height RGBA8 with straight alpha.
    // Use linear filtering, clamp-to-edge and no mipmaps. Keep the GPU resource alive.
    ImTextureRef texture = UploadRgbaTexture(pixels.width, pixels.height, pixels.rgba);
    icons.SetTexture(size, texture);
}

// In your usual ImGui frame and window:
imkit::Icon(icons, imkit::IconId::Search);                         // 20 logical pixels, current text colour
imkit::Icon(icons, imkit::IconId::Warning,
            {.size = 24, .color = ImVec4(0.9f, 0.6f, 0.1f, 1)});
if (imkit::IconButton("save", icons, imkit::IconId::Save, "Save document")) {
    SaveDocument();
}
if (imkit::IconLabelButton("open", icons, imkit::IconId::FolderOpen, "Open")) {
    OpenDocument();
}
ImGui::BeginDisabled();
imkit::IconButton("delete", icons, imkit::IconId::Delete, "Delete selection");
ImGui::EndDisabled();

// After the last draw call that uses these textures has finished:
icons.Clear();
// Then destroy your GPU textures by your renderer's normal rules.
```

`UploadRgbaTexture`, `SaveDocument` and `OpenDocument` are your functions, not ImKit APIs. `examples/gallery/main.cpp` has a concrete OpenGL upload. An `IconButton(id, ImGuiDir, accessibleLabel)` overload for arrow buttons also exists.

## Behaviour

| Topic | Rule |
| --- | --- |
| Catalog | `GetIconCatalog()` returns stable IDs, English names and categories. `GetIconInfo` returns null for an invalid ID. |
| Atlas data | `GetIconAtlasPixels` accepts exactly 12, 16, 20, 24, 32, 48 or 64. The CPU data is immutable and lives for the whole process. Other sizes return an empty view. |
| UVs | `GetIconRegion` returns the normalized rectangle. Two transparent pixels around each cell stop neighbours bleeding under linear filtering. |
| Size selection | Bind all seven sizes. Drawing picks the smallest level that covers `options.size * max(DisplayFramebufferScale)`, up to 64. Larger physical sizes enlarge the 64 px bitmap and look softer. `options.size` defaults to 20 logical pixels. Scale it with your UI scale. |
| Colour | With no colour set, the icon uses `ImGuiCol_Text`. A tint changes the glyph only. Button text keeps the theme text colour. Style alpha applies, including the disabled state. |
| Buttons | They use native Dear ImGui hit testing, focus, hover, active and disabled behaviour. Give each a stable, non-null `id`. The label is the hover and focus tooltip, and text after `##` is not drawn. It is not an operating-system accessibility bridge. |
| Missing data | An invalid ID or an unbound size draws no glyph. `Icon` still reserves its space, and buttons become disabled instead of doing nothing. |
| Lifetime | Give each renderer or independent context its own `IconAtlas`. Nothing is registered globally. `Clear` only unbinds and deletes no GPU resource. |

`cg::ObjectView::icon` lets a 3D outliner pick any catalog glyph without extending `ObjectKind`.

## Assets and rebuilding

| Location | Content |
| --- | --- |
| `assets/icons/{12,16,20,24,32,48,64}` | 321 transparent PNGs per size, 2,247 in total |
| `assets/icons/atlases` | The seven atlas PNGs |
| `assets/icons/originals`, `assets/icons/prompts` | The 238 source images and the prompts that generated them. They stay in the source tree and are not copied into the SDK. |
| `assets/icons/provenance.json` | Source file hashes |
| `share/imkit/icons` | Where CMake install places the PNGs and atlases |

The raster artwork was generated with an image generation tool, one asset at a time. Its geometry is approximate, not a mathematically identical stroke system or an icon font. The other icons are drawn by code and need no source bitmap. The artwork is part of this repository and uses its MIT license.

Consumers need no generation tool, key or Python. To rebuild the derived files after replacing an original, install Pillow and run `python tools/build_icons.py`. It keeps the source images, normalizes their extent, converts RGB to white without changing the alpha shape, and writes the PNG variants and `src/icons_data.inc`. `python tools/build_icons.py --check` validates every original, hash, image and atlas region.

The first 120 names are frozen in `stable-ids.json`, and new IDs are only appended, so existing IDs keep their order.

## Check it in the Gallery

The Icons page lets you search by category and name, switch between the seven sizes, and try tint and interaction states. The image below shows editor icons at 150% scale.

![Editor icons at 150% scale](/imgui-modern-kit/docs-images/editor-icons-1.0.png)
