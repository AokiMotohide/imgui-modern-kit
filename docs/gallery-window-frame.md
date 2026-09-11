# Gallery window frame prototype / Galleryのウィンドウ枠試作

The interactive Windows Gallery starts with a Modern frame and the Graphite theme.
Use **Appearance > Theme** to change all 12 presets, including the title bar.
The title bar has a geometric icon, an elided UTF-8 title, and minimize,
maximize/restore and close buttons. Its 32-DIP height follows the window's DPI,
independently of Gallery's content zoom. Inactive titles use secondary text color.

Windows版Galleryの通常起動はModern枠とGraphiteテーマを使います。
**Appearance > Theme**で12テーマを選択すると、タイトルバーも連動します。
タイトルバーは幾何学アイコン、長さに応じて省略するUTF-8タイトル、最小化・
最大化／復元・閉じるボタンで構成します。高さ32 DIPはウィンドウのDPIに追従し、
Gallery内の表示倍率からは独立します。非アクティブ時は文字の強調を落とします。

```powershell
# Default / 通常起動
.\build\windows-debug\catalog\Debug\imkit_gallery.exe
# Compare the native frame / 標準枠との比較
.\build\windows-debug\catalog\Debug\imkit_gallery.exe --window-frame native
# Explicit Modern frame and a custom title / Modern枠とタイトル指定
.\build\windows-debug\catalog\Debug\imkit_gallery.exe --window-frame modern --window-title "My application"
# Focused verification (opens a window and closes it on completion)
# 専用検証（ウィンドウを表示し、完了時に閉じる）
.\build\windows-debug\catalog\Debug\imkit_gallery.exe --verify-window-frame --width 1100 --height 760 --output out/window-frame
```

Existing automated captures/verifiers retain the native frame unless explicitly
passed `--window-frame modern`. Their content coordinate origin stays unchanged.
Explicitly adding a Modern frame shifts content by the title bar height; existing
coordinate-based verifiers are not guaranteed in that combination.

既存の自動capture・検証は、`--window-frame modern`を明示しない限り標準枠と
既存のコンテンツ座標原点を維持します。Modern枠を明示した場合はタイトルバーの高さ分
コンテンツが移動するため、既存の座標固定検証との組合せは保証しません。

## Ownership and behavior / 所有権と動作

This is Gallery-only code, not an installed public API. `window_frame` draws from
theme and window state and returns the layout and queued action. `window_frame_win32`
borrows the GLFW HWND through an explicitly detached Win32 subclass. It retains
the original window styles and chains messages to the existing backend. Close
reaches GLFW's close request instead of destroying the window directly. The host
continues to own the window, context, renderer, font and theme. No public Theme
fields, library dependencies or SDK packaging change.

Gallery内だけの試作であり、installされる公開APIではありません。描画部品はテーマと
ウィンドウ状態から描画し、領域と操作要求を返します。Windows連携部品はGLFWのHWNDを
借用し、明示的に解除するsubclassで既存backendのメッセージ処理を維持します。
元のウィンドウstyleを残し、閉じる操作は直接破棄せずGLFWの終了要求へ渡します。
ウィンドウ・Context・renderer・font・themeはホスト所有のままです。
公開Theme型、ライブラリ本体の依存、SDK配布構成は変更しません。

The minimum track size is 320 x 200 DIP to keep caption controls accessible.
Drag/double-click, edge/corner resizing, the system menu and Alt+F4 use Windows
behavior. Alt+Space is explicitly forwarded around GLFW's default menu suppression.
The maximize region returns `HTMAXBUTTON` and non-client mouse movement is forwarded
to DWM for Windows 11 Snap Layouts. OS shadow/corner appearance remains OS-dependent.

操作ボタンの領域を確保するため、最小サイズは320×200 DIPです。移動・ダブルクリック、
四辺／四隅のリサイズ、システムメニュー、Alt+F4はWindowsの動作を使います。
Alt+SpaceはGLFWの既定のメニュー抑止を回避してOSへ渡します。最大化領域は
`HTMAXBUTTON`を返し、非クライアント領域のマウス移動をDWMへ渡します。
影や角の外観はOSに依存します。

## Verification record / 検証記録

The focused Debug verifier checks layout and UTF-8 title elision at 100/150/200%,
caption/client/resize hit tests, maximize work-area containment, restore, minimize,
the GLFW close path and backbuffer captures for all 12 themes. Output is under
`out/window-frame/`; generated evidence is not committed.

Debug専用検証は100／150／200％の領域計算とUTF-8タイトル省略、タイトル・ボタン・
リサイズ・コンテンツのヒットテスト、最大化時の作業領域、復元、最小化、GLFW終了経路、
12テーマのbackbuffer captureを確認します。生成物は`out/window-frame/`に置き、
コミットしません。

On 2026-09-11, the isolated Debug Gallery build and focused verifier passed at
actual DPI 144 (150%). Computer Use confirmed maximize by button, restore by
title-bar double-click, title-bar drag, bottom-right resize, minimize/restore,
Alt+Space, Alt+F4, the close button, the Snap Layout hover menu and live switching to a light theme.
The `--window-frame native` comparison launch and existing `--verify` public-IO
regression also passed. These observations do not
certify physical 100/200% DPI, mixed-DPI monitor transitions, all resize edges,
Snap Layout placement, screen readers, detached viewports, other application
integration or Release/distribution acceptance.

2026-09-11、専用ビルド先のDebug Galleryと専用検証が実DPI 144（150％）で成功しました。
Computer Useでボタン最大化、タイトルバーダブルクリック復元、タイトルバー移動、右下
リサイズ、最小化／復元、Alt+Space、Alt+F4、閉じるボタン、スナップのhoverメニュー、
ライトテーマへの即時切り替えを確認しました。`--window-frame native`での比較起動と
既存`--verify`の公開IO回帰検証も成功しました。
実DPI 100／200％、異なるDPIのモニター間移動、全リサイズ辺、スナップ先への配置、
スクリーンリーダー、切り離し窓、他アプリ統合、Release／配布の合格を意味しません。
