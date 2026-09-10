# Modern outline icons

ImKit includes 173 individually ImageGen-generated monochrome icons in 13 categories.
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
  returns null for invalid IDs. The 173 entries follow `assets/icons/catalog.json`.
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

`assets/icons/{16,20,24,32,48,64}` contains 1038 individual transparent PNGs.
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
The catalog now contains 173 originals and 1038 size variants; the original 120 IDs remain stable.
BoxSelectとLassoSelectは個別のImageGen原画から生成し、Viewportの選択modeへ接続しています。
原画173枚・サイズ別PNG1038枚で、既存120 IDの順序と数値は維持しています。

HandPan is an individual built-in ImageGen original with its saved prompt and provenance hash. Timeline uses it for the Hand tool. Native Icon Gallery inspection covers 16px light and 150% dark in `out/hand-pan-native/`. The catalog contains 173 originals, 1038 variants, and six atlases.

HandPanは組込みImageGenによる個別原画で、promptとprovenance hashを保存しています。TimelineのHand toolへ接続し、`out/hand-pan-native/`のnative Icon Galleryで16px lightと150% darkを確認しました。原画173枚・派生PNG1038枚・atlas6枚です。


Razor, RippleEdit, and SlipEdit each have an individual built-in ImageGen original and saved prompt. Timeline connects them to split-at-cursor, ripple trim, and source slip tools. Native Icon Gallery was inspected at 16px light and 150% dark in `out/timeline-edit-icons/`. RollingEdit and SlideEdit were generated successfully on retry, with individual prompts and originals. All seven Timeline tool selectors now use icons when a host atlas is provided. Their 16px light and 150% dark appearance was inspected in `out/roll-slide-icons/`.

Razor・RippleEdit・SlipEditは組込みImageGenの個別原画とpromptを保存し、Timelineのカーソル位置分割・ripple trim・source slipへ接続しました。`out/timeline-edit-icons/`のnative Icon Galleryで16px lightと150% darkを確認しました。RollingEditとSlideEditも再試行で個別原画を生成し、promptを保存しました。ホストatlasがある場合、Timelineの7ツールすべてをアイコンで操作できます。`out/roll-slide-icons/`で16px lightと150% darkを確認しました。

Solo uses a separately generated headphones-and-dot original, saved prompt and provenance hash. It is connected to the Timeline track button/menu. Native 16px light and 150% dark Icon Gallery captures are in `out/solo-icons-native/`; the host Solo toggle passes `--verify-track-controls`. The catalog contains 173 originals, 1038 size variants, six atlases and 798328 embedded RLE bytes. Existing IDs remain stable. Install includes generated assets and prompts while excluding originals, as before.

Soloはheadphoneと中央点を持つ個別生成原画で、promptとprovenance hashを保存し、Timelineのtrackボタン／menuへ接続しました。`out/solo-icons-native/`で16px light・150% darkを確認し、`--verify-track-controls`でホストSolo状態への反映を確認しました。原画173枚、派生PNG1038枚、atlas6枚、埋込みRLE 798328 bytesで、既存IDは維持しています。installは従来どおり原画を除外し、生成資産とpromptを含みます。

SourcePatch is an individual ImageGen original with saved prompt and provenance hash, integrated into track source controls. Track targeting reuses Target rather than adding a synonymous original. Native 16px light and 150% dark captures are in `out/source-patch-icons/`; Source and Target menu changes reach host state in `out/source-target-native/track-controls.txt`. The catalog contains 173 originals, 1038 PNG variants and six atlases.

SourcePatchはpromptとprovenance hashを保存した個別ImageGen原画で、trackの素材patch操作へ接続しました。編集先指定は同義の原画を追加せず既存Targetを再利用します。`out/source-patch-icons/`で16px light・150% darkを確認し、`out/source-target-native/track-controls.txt`でSource／Targetのホスト反映を確認しました。原画173枚、派生PNG1038枚、atlas6枚です。

Six Timeline track headings now use Video, Audio, Text, EffectTrack, AdjustmentTrack and Layers. EffectTrack and AdjustmentTrack are separate ImageGen originals with prompts/hashes; the other four reuse existing IDs. Native 16px light, 150% dark and all six heading roles were inspected in `out/track-kind-icons/` and `out/track-kinds-native/`. There are 173 originals, 1038 PNG variants, six atlases and 798328 embedded RLE bytes.

Timelineの6種類の見出しへVideo、Audio、Text、EffectTrack、AdjustmentTrack、Layersを接続しました。EffectTrackとAdjustmentTrackはprompt／hashを保存した個別ImageGen原画で、他4種類は既存IDを再利用します。`out/track-kind-icons/`と`out/track-kinds-native/`で16px light、150% dark、6種類の見出しを確認しました。原画173枚、派生PNG1038枚、atlas6枚、埋込みRLE 798328 bytesです。

VertexNormals and FaceNormals are separate built-in ImageGen originals with saved prompts
and SHA-256 provenance. Three vertex arrows and one face-center arrow distinguish their
meaning. BeginViewport connects both to real normal overlays with labeled toggles,
active backgrounds and underlines. All six sizes are derived by build_icons.py; originals
remain excluded from install, while generated assets and prompts follow the existing install rules.

VertexNormalsとFaceNormalsは個別のImageGen原画で、promptとSHA-256を保存しています。
頂点の3矢印と面中央の1矢印で意味を区別し、実際の法線overlay切替へ接続しています。
有効状態は背景と下線でも表示します。6サイズは同じ生成処理で派生し、原画を除く既存のinstall規則を維持します。

ObjectOrigin, SelectionOutline, RenderRegion, Passepartout and UnifiedTransform use
individual ImageGen originals and saved prompts. They are connected to the viewport
menu and Unified tool. Six-size derivatives and embedded atlases retain the existing
ID prefix, alpha/tint format and original-excluding install rules.

ObjectOrigin、SelectionOutline、RenderRegion、Passepartout、UnifiedTransformは個別原画と
promptを保存し、Viewportの表示メニューと統合ツールへ接続しました。6サイズ・埋込みatlasを
同じ処理で生成し、既存ID・alpha/tint形式・原画を除くinstall規則を維持しています。

SelectVertex, SelectEdge, SelectFace and SelectIsland are individually generated originals connected to the UV selection menu. The menu uses native selection/focus states and host UTF-8 labels. All six PNG sizes and atlases pass generation validation; native GPU inspection remains pending because GLFW cannot create an OpenGL context on this session.

SelectVertex／SelectEdge／SelectFace／SelectIslandは個別生成した原画で、UVの選択メニューへ接続しました。nativeの選択・focus状態とホスト指定UTF-8ラベルを使用します。PNG全6サイズとatlasの生成検証は合格しました。このセッションではGLFWがOpenGL contextを作成できず、native GPUでの表示確認は未実施です。

UVEditor, UVSeam, UVOverlap and UDIMTiles have individual generated originals, saved prompts and provenance hashes. UVSeam was simplified after the 16px alpha validation rejected its first original. Display toggles control seam/overlap overlays; UDIM mode draws visible tile boundaries and numbers with bounded label density. Native GPU appearance remains unverified in the current session.

UVEditor／UVSeam／UVOverlap／UDIMTilesに個別生成原画・prompt・provenance hashを追加しました。UVSeamは最初の原画が16px alpha検査を通らず、簡略化した原画へ差し替えました。表示切替はシーム・重なりの描画へ接続し、UDIMモードは表示範囲内のタイル境界と番号を密度制限付きで描きます。現在のセッションでのnative GPU表示は未検証です。

Transition, Dissolve, FadeIn, FadeOut, Crossfade and TransitionDuration use individual generated originals and saved prompts/hashes. TransitionPicker displays the appropriate incoming/outgoing fade glyph, and Timeline uses atlas badges plus a duration glyph in the handle tooltip. Badge raster level follows framebuffer scale. Labels remain available without an atlas; native GPU appearance is pending.

Transition／Dissolve／FadeIn／FadeOut／Crossfade／TransitionDurationに個別原画・prompt・hashを追加しました。TransitionPickerは開始・終了に対応したFade glyphを表示し、Timelineはatlas badgeとhandle tooltipの長さglyphを使用します。badgeのraster levelはframebuffer scaleへ追従します。atlas未指定でも文字で操作でき、native GPU表示は未検証です。

SourceMonitor, ProgramMonitor, SafeArea, Guides, MetadataOverlay, TransformBounds, AnchorPoint and ProxyMedia have individual originals and saved prompts/hashes. Gallery monitor headings and MonitorControls use them; CG Safe Frame shares SafeArea, and AssetBrowser/Timeline share ProxyMedia. All six size/atlas generation checks passed; native GPU appearance remains pending.

SourceMonitor／ProgramMonitor／SafeArea／Guides／MetadataOverlay／TransformBounds／AnchorPoint／ProxyMediaの個別原画・prompt・hashを追加しました。GalleryのMonitor見出しとMonitorControlsへ接続し、CG Safe FrameはSafeArea、AssetBrowser／TimelineはProxyMediaを共用します。全6サイズとatlasの生成検査は合格し、native GPU表示は未検証です。

TangentLinked/TangentBroken identify Aligned/Free handle modes. CurveEditor, AnimationTimeline, DopeSheet and AnimationStrip mark Gallery editor roles; StripBlend marks the strip blend setting. Seven individual originals have saved prompts and hashes. The six-size generation checks pass; latest native GPU appearance remains pending.

TangentLinked／TangentBrokenはAligned／Free接線モードへ接続しました。CurveEditor／AnimationTimeline／DopeSheet／AnimationStripはGalleryの役割表示、StripBlendはstripの混合率設定に使います。7原画のprompt・hashを保存し、全6サイズ生成検査は合格しました。最新native GPU表示は未検証です。
