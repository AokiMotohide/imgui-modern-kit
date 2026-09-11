# Public window frame / 公開ウィンドウ枠

`imkit/window_frame.h` is a cross-platform, value-based drawing and layout API. The host owns the Dear ImGui context, backend, renderer, fonts, theme, window, selected preset, edited style and persistence. `MakeWindowFrameStyle(preset, theme)` returns a complete copy; changing the Theme never mutates an existing frame style. Applications may edit every returned color, metric and feature directly.

`imkit/window_frame.h`はOS非依存の値型描画・配置APIです。Dear ImGui Context、backend、renderer、font、Theme、アプリウィンドウ、選択中preset、編集済みStyle、永続化はホストが所有します。`MakeWindowFrameStyle(preset, theme)`は完全なコピーを返し、Theme変更が既存Styleを暗黙に変更することはありません。生成後は全色・寸法・featureを直接編集できます。

| Preset | Default / 既定 |
|---|---|
| `Native` | Zero-height layout; no custom non-client drawing / 高さ0、独自非クライアント描画なし |
| `Studio` | Compact production-tool title, icon and three caption operations / 制作ツール向け簡潔タイトル |
| `Workspace` | Application, project, unsaved state and workspace selector / アプリ・project・未保存・workspace切替 |
| `Tool` | Short utility-window title and close operation / 小窓向けタイトルと閉じる操作 |

```cpp
auto style = imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Workspace, theme);
style.metrics.height = 38.0f;
style.features.workspaceSwitcher = true;
const std::string_view workspaces[] = {"Edit", "Color", "Deliver"};
imkit::WindowFrameContent content{"My App", "Project A", true, workspaces, selectedWorkspace};
auto state = platformAdapter.State();
auto layout = imkit::LayoutWindowFrame(windowWidthPixels, style, state);
platformAdapter.SetLayout(layout);
auto result = imkit::DrawWindowFrame(style, content, layout, state);
```

`WindowFrameContent` uses non-owning `string_view` and `span` values. Keep their storage valid through the draw call. `WindowFrameResult` reports an operation or workspace selection; the host applies it. The library does not retain content, style, selection or a current Theme. Contrast validation reports ratios and warnings but never rejects a color.

`WindowFrameContent`の`string_view`と`span`は非所有です。描画呼出しの終了まで参照先を保持してください。`WindowFrameResult`が操作またはworkspace選択eventを返し、適用するのはホストです。ライブラリはcontent、Style、選択、current Themeを保持しません。コントラスト検証は比率と警告を返しますが、色入力を拒否しません。

## API inventory / API一覧

| API | Contract / 契約 |
|---|---|
| `MakeWindowFrameStyle` | Complete Theme-derived host-owned value / Theme由来の完全なホスト所有値 |
| `LayoutWindowFrame` | DIP metrics to pixel rectangles; reserves platform leading area / DIPからpixel領域を計算しOS領域を予約 |
| `DrawWindowFrame` | Draws through the current context and returns one typed event / current Contextへ描画し型付きeventを返す |
| `ElideWindowFrameTitle` | UTF-8 boundary-safe ellipsis / UTF-8境界を壊さない省略 |
| `ValidateWindowFrameContrast` | Informational ratios; no input rejection / 警告用比率、入力拒否なし |
| `WindowFrameWin32Adapter` | Borrowed `HWND`, explicit Attach/Detach, subclass-based operations / 借用`HWND`と明示Attach/Detach |
| `WindowFrameMacOSAdapter` | Borrowed `NSWindow` as `void*`, transparent full-size title content / 借用`NSWindow`と透明full-size title content |

## Windows Gallery Frame Lab

Build and start the Windows Gallery, then choose **Frame Lab**. The four presets switch at runtime. Selecting `Native` detaches `imkit::window_frame_win32`; other presets attach it and use the public drawing API. Each preset keeps its own edited copy. Theme color regeneration preserves edited metrics/features, while preset reset restores the complete preset. The page edits every color, metric and feature, displays contrast warnings, and copies a reproducing C++ snippet. Clipboard access is Gallery-only.

Windows Galleryを起動して**Frame Lab**を選びます。4 presetを実行中に切り替え、`Native`では`imkit::window_frame_win32`を解除し、それ以外ではAttachして公開描画APIを使います。presetごとに編集値を保持します。Themeからの色再生成は寸法・featureを保持し、preset resetは完全な既定値へ戻します。全色・寸法・feature、コントラスト警告、再現用C++ snippetのコピーを提供し、ClipboardはGalleryだけが扱います。

```powershell
cmake -S . -B build/window-frame-public-debug
cmake --build build/window-frame-public-debug --config Debug --target imkit_gallery --parallel
.\build\window-frame-public-debug\catalog\Debug\imkit_gallery.exe
.\build\window-frame-public-debug\catalog\Debug\imkit_gallery.exe --verify-window-frame --width 1100 --height 760 --output out/window-frame-public
```

Existing automated capture and coordinate verification keeps `Native` by default. Normal interactive startup uses `Studio`. The Win32 adapter uses `SetWindowSubclass`; it preserves resize edges/corners, caption drag and double-click, system menu, Alt+Space, Alt+F4 and standard minimize/maximize/restore/close routing. Its maximize hit region returns `HTMAXBUTTON` for Windows 11 Snap Layouts. Close sends the standard close request and never destroys the window directly.

既存の自動capture・座標検証は`Native`を既定に維持し、通常の対話起動は`Studio`です。Win32 adapterは`SetWindowSubclass`を使い、四辺・四隅resize、caption drag／double-click、system menu、Alt+Space、Alt+F4、標準の最小化・最大化／復元・終了経路を維持します。最大化領域はWindows 11 Snap Layoutsのため`HTMAXBUTTON`を返します。Closeは標準終了要求を送り、直接windowを破棄しません。

## macOS demo / macOSデモ

The Windows-only Gallery is not ported. On Apple only, build the lightweight GLFW/OpenGL/Dear ImGui demo.
Windows専用Galleryは移植しません。Apple上だけで軽量なGLFW/OpenGL/Dear ImGuiデモを構成します。

```bash
cmake -S . -B build/macos-window-frame \
  -DIMKIT_BUILD_GALLERY=OFF \
  -DIMKIT_BUILD_WINDOW_FRAME_MACOS=ON \
  -DIMKIT_BUILD_WINDOW_FRAME_DEMO_MACOS=ON
cmake --build build/macos-window-frame --target imkit_window_frame_demo_macos
./build/macos-window-frame/imkit_window_frame_demo_macos
```

`imkit::window_frame_macos` borrows the Cocoa window. For custom presets it requests a transparent title bar and full-size content view, reserves the traffic-light area, and leaves traffic-light buttons, dragging, full screen, minimize and zoom to Cocoa. It does not draw duplicate close/maximize buttons. Native restores the original title configuration. This implementation has not been built or operated on a real Mac in this work; traffic-light, drag/full-screen and macOS input behavior remain unverified.

`imkit::window_frame_macos`はCocoa windowを借用します。custom presetでは透明title barとfull-size content viewを要求し、traffic-light領域を予約します。traffic-light button、drag、full screen、最小化、拡大はCocoaへ委譲し、閉じる／最大化buttonを重複描画しません。Nativeで元のtitle設定へ戻します。今回は実Macでbuild・操作しておらず、traffic-light、drag/full-screen、macOS入力は未検証です。

Physical 100%/200% DPI, mixed-DPI monitor movement, screen readers, Release and distribution acceptance are also outside the current verification. Synthetic hit tests and Debug builds do not establish those categories.

物理DPI 100%／200%、異なるDPI monitor間移動、screen reader、Release、配布受け入れも今回の検証外です。合成hit testとDebug buildを、それらの合格とは扱いません。
