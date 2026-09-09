# Modern outline icons

ImKit includes 134 individually ImageGen-generated monochrome icons in 13 categories.
The artwork is distributed with this repository under its MIT license. The original
generations and their prompts are retained in `assets/icons/originals` and
`assets/icons/prompts`; `provenance.json` records source hashes. Original images use
black strokes because that produced cleaner alpha edges. Packaged PNGs and embedded
atlases use white RGB with the generated alpha, so any tint works on light or dark UI.

## Host integration

Include `<imkit/imkit.h>` (or `<imkit/icons.h>`). There is no new renderer dependency,
context creation, image decoder, file lookup, or runtime download in the library.

```cpp
imkit::IconAtlas icons; // Owned alongside your renderer resources.
for (int size : imkit::IconPixelSizes) {
    const auto pixels = imkit::GetIconAtlasPixels(size);
    // Your renderer: upload pixels.width x pixels.height RGBA8, straight alpha.
    // Use linear filtering, clamp-to-edge, no mipmaps. Retain the GPU resource.
    ImTextureRef texture = UploadRgbaTexture(pixels.width, pixels.height, pixels.rgba);
    icons.SetTexture(size, texture);
}

// Within the host's usual ImGui frame/window:
imkit::Icon(icons, imkit::IconId::Search); // 20 logical pixels; current text color.
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

// After the last draw submission using these resources has completed:
icons.Clear();
// Destroy your six GPU resources using the renderer's normal lifetime rules.
```

`UploadRgbaTexture`, `SaveDocument` and `OpenDocument` above are host functions,
not ImKit APIs. See `examples/gallery/main.cpp` for the concrete OpenGL upload.
The existing `IconButton(id, ImGuiDir, accessibleLabel)` overload is unchanged.

## API behavior

- `GetIconCatalog()` returns stable IDs, English names and categories; `GetIconInfo`
  returns null for invalid IDs. The 134 entries follow `assets/icons/catalog.json`.
- `GetIconAtlasPixels` accepts exactly 16, 20, 24, 32, 48 or 64. The returned CPU data
  is immutable with process lifetime; unsupported sizes return an empty view.
- `GetIconRegion` returns the corresponding normalized UV rectangle. Two transparent
  pixels around each atlas cell prevent adjacent glyphs bleeding with linear filtering.
- Upload and bind all six sizes. Rendering selects the smallest level sufficient for
  `options.size * max(DisplayFramebufferScale)`, capped at 64. Sizes above 64 physical
  pixels upscale the bitmap and can look soft. `options.size` defaults to 20; invalid
  or nonpositive values use that default. Application UI scaling should scale this
  logical size too; the Gallery demonstrates it.
- An absent color inherits `ImGuiCol_Text`. Explicit tint affects the glyph; button
  text keeps the theme text color. Both honor style alpha, including disabled state.
- Buttons use native Dear ImGui button hit testing, focus, hover, active and disabled
  behavior. Supply a stable non-null `id`; each control owns an ImGui ID scope.
  Labels provide hover/focus tooltips. This is not an operating-system accessibility
  bridge. Label text after `##` is not drawn.
- An invalid ID or an unbound selected raster level draws no glyph. `Icon` still
  reserves its layout space; buttons are disabled rather than allowing blank actions.
- Each renderer or independently managed context should own an `IconAtlas`. Nothing
  is globally registered. `Clear` only unbinds; it does not delete GPU resources.

## Assets and reproduction

`assets/icons/{16,20,24,32,48,64}` contains 804 individual transparent PNGs.
`assets/icons/atlases` contains six atlas PNGs. All are included by CMake install
under `share/imkit/icons`. Headers and compiled embedded data are installed normally.
Original large images are kept in the source checkout, not copied into the SDK.

The built-in image generation tool was used individually for each asset; no API key
or generation tool is required by consumers. `assets/icons/prompts` records the exact
generation prompts. Generated raster geometry is approximate, not a mathematically
identical stroke system or a vector icon font.

To rebuild derived assets after replacing an original, install Pillow in your authoring
Python environment and run `python tools/build_icons.py`. This preserves source images,
normalizes their visual extent, converts RGB to white without replacing alpha shapes,
and produces PNG variants plus `src/icons_data.inc`. Missing originals are errors.
Run `python tools/build_icons.py --check` for image/atlas validation. Python and Pillow
are not build or runtime dependencies for C++ consumers.

Atlas rows are derived from catalog length. `metadata.json` records dimensions,
categories and asset counts, and the C++ UV calculations use the generated layout.
`stable-ids.json` freezes the original 120 names in order; new IDs must be appended.
The generator checks the complete public enum against the catalog and validates
original hashes, every derived image and every atlas region.

atlasの行数はcatalog件数から算出します。`metadata.json`に寸法・カテゴリ数・資産数を記録し、
C++のUV計算も生成した配置を使用します。`stable-ids.json`は既存120個の順序を固定し、
新規IDは末尾へ追加します。生成処理は公開enumとcatalogの一致、原画hash、全派生画像、
全atlas領域を検査します。原画や派生物の追加なしに件数だけを増やすことはできません。

## Native Gallery

Open the **Icons** page for name/category search, 16–64px sizes, custom tint, copyable
C++ snippets, and live icon-only/label/disabled controls. Use:

```powershell
build/windows-debug/catalog/Debug/imkit_gallery.exe --verify-icons --capture --page 6 --output out/icons
```

This focused run uses public Dear ImGui IO events and captures the real OpenGL
backbuffer at 16px and 20px on both themes. It does not claim native OS input automation.

BoxSelect and LassoSelect are separate ImageGen originals, connected to Viewport selection mode.
The catalog now contains 134 originals and 804 size variants; the original 120 IDs remain stable.
BoxSelectとLassoSelectは個別のImageGen原画から生成し、Viewportの選択modeへ接続しています。
原画134枚・サイズ別PNG804枚で、既存120 IDの順序と数値は維持しています。

HandPan is an individual built-in ImageGen original with its saved prompt and provenance hash. Timeline uses it for the Hand tool. Native Icon Gallery inspection covers 16px light and 150% dark in `out/hand-pan-native/`. The catalog contains 134 originals, 804 variants, and six atlases.

HandPanは組込みImageGenによる個別原画で、promptとprovenance hashを保存しています。TimelineのHand toolへ接続し、`out/hand-pan-native/`のnative Icon Galleryで16px lightと150% darkを確認しました。原画134枚・派生PNG804枚・atlas6枚です。


Razor, RippleEdit, and SlipEdit each have an individual built-in ImageGen original and saved prompt. Timeline connects them to split-at-cursor, ripple trim, and source slip tools. Native Icon Gallery was inspected at 16px light and 150% dark in `out/timeline-edit-icons/`. RollingEdit and SlideEdit were generated successfully on retry, with individual prompts and originals. All seven Timeline tool selectors now use icons when a host atlas is provided. Their 16px light and 150% dark appearance was inspected in `out/roll-slide-icons/`.

Razor・RippleEdit・SlipEditは組込みImageGenの個別原画とpromptを保存し、Timelineのカーソル位置分割・ripple trim・source slipへ接続しました。`out/timeline-edit-icons/`のnative Icon Galleryで16px lightと150% darkを確認しました。RollingEditとSlideEditも再試行で個別原画を生成し、promptを保存しました。ホストatlasがある場合、Timelineの7ツールすべてをアイコンで操作できます。`out/roll-slide-icons/`で16px lightと150% darkを確認しました。

Solo uses a separately generated headphones-and-dot original, saved prompt and provenance hash. It is connected to the Timeline track button/menu. Native 16px light and 150% dark Icon Gallery captures are in `out/solo-icons-native/`; the host Solo toggle passes `--verify-track-controls`. The catalog contains 134 originals, 804 size variants, six atlases and 634926 embedded RLE bytes. Existing IDs remain stable. Install includes generated assets and prompts while excluding originals, as before.

Soloはheadphoneと中央点を持つ個別生成原画で、promptとprovenance hashを保存し、Timelineのtrackボタン／menuへ接続しました。`out/solo-icons-native/`で16px light・150% darkを確認し、`--verify-track-controls`でホストSolo状態への反映を確認しました。原画134枚、派生PNG804枚、atlas6枚、埋込みRLE 634926 bytesで、既存IDは維持しています。installは従来どおり原画を除外し、生成資産とpromptを含みます。
