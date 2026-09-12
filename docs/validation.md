# Validation / 検証

## Gallery comparison and documentation captures / Gallery比較と文書capture（2026-09-12）

The Debug native Gallery build passed `--verify-comparison`: Default Dear ImGui and ImKit controls changed the same host-owned value, temporary comparison styles restored after a frame, and closing/reopening removed then restored the submitted controls. The runner captured 120 overview frames and 80 frames each for comparison, themes, workflow and timeline at native `960×540`; the checked-in GIF encoder accepted all five outputs under its 8 MiB limit. Representative native frames were visually inspected.

Debug native Galleryで`--verify-comparison`が成功しました。Default Dear ImGuiとImKitのcontrolは同じホスト所有値を変更し、比較用の一時styleはframe後に復元され、close/reopenでcontrolが消えて再表示されました。概要120 frame、比較・theme・workflow・timeline各80 frameをnative `960×540`でcaptureし、commitする5本のGIFは8MiB上限付きencoderを通過しました。代表native frameを目視確認しました。

The runner sends public Dear ImGui IO and reads a real OpenGL backbuffer. It does not establish native OS/IME input, assistive-technology operation, physical-DPI, performance or external-host acceptance. The released Gallery executable is separately built, staged and inspected for its DLL imports; its archive does not bundle the Microsoft Visual C++ Redistributable.

runnerは公開Dear ImGui IOを送り、実OpenGL backbufferを読みます。native OS/IME入力、支援技術、実機DPI、性能、外部ホストでの受け入れは確認しません。公開Gallery実行ファイルは別途Release build・stage・DLL import確認を行い、Microsoft Visual C++ Redistributableをarchiveに同梱しません。

## Public window frame / 公開ウィンドウ枠（2026-09-11）

The isolated `build/window-frame-public-debug` tree passed the Debug public API unit test and compile fixture, the independent source consumer build, the minimal Gallery build, and `--verify-window-frame`. The verifier covered all four presets, Theme-derived color regeneration versus complete preset reset, metric and feature layout effects, UTF-8 elision, Win32 caption/client/edge hit tests including `HTMAXBUTTON`, maximize/work-area containment, restore, minimize and the GLFW close-request path. Generated captures and reports under `out/window-frame-public/` are not committed.

隔離した`build/window-frame-public-debug`で、Debug公開API unit test／compile fixture、独立source consumer build、最小Gallery build、`--verify-window-frame`が成功しました。枠検証は4 preset、Theme由来の色再生成と完全preset resetの区別、寸法・featureのlayout反映、UTF-8省略、`HTMAXBUTTON`を含むWin32 caption／client／edge hit test、最大化時のwork area、復元、最小化、GLFW終了要求経路を対象にしました。`out/window-frame-public/`の生成capture・reportはcommitしません。

No real Mac was available. macOS compilation, launch, traffic-light controls, drag/full-screen/minimize/zoom and native input are unverified. Physical 100%/200% DPI, mixed-DPI monitor movement, screen readers, Release, installed-package and distribution acceptance were also not run. Debug compilation and synthetic Win32 messages do not establish those categories.

実Macは使用していません。macOS build・起動、traffic-light、drag／full-screen／最小化／拡大、native入力は未検証です。物理DPI 100%／200%、異なるDPI monitor間移動、screen reader、Release、installed package、配布受け入れも未実施です。Debug compileと合成Win32 messageを、それらの合格とは扱いません。

## Generic workflow extension / 汎用部品拡張（2026-09-11）

- Debug workflow tests passed: notification replacement/expiry/priority/capacity,
  Step mouse/keyboard/disabled IDs, dismissal, tile selection/resize/cancel and parent
  disabled semantics, overflow commands, palette actions, modal Begin/End, empty
  geometry, image fit/fill/clamp/zoom anchor, and two context lifetimes.
- Warmed composed controls produced zero measured C++ and ImGui allocations.
- Native API, design-system and Editor Core direct regressions passed. New API fixture
  passed in Debug/Release; source and relocated SDK consumers compiled, linked and ran.
- Debug/Release Gallery built. Public IO exercised filter click and cursor zoom.
  `out/workflow-final/` contains 38 native GPU captures; representative workspace,
  feedback, tile and narrow Japanese frames were visually inspected.
- Debug/Release install includes both new headers. The optional Win32 accessibility
  Release library was built because the existing install manifest requires it.

Debugでは通知更新・期限・優先度・容量、Stepのclick／keyboard／disabled／ID分離、dismiss、
Tile選択・resize・取消・親disabled、overflow command、Palette、modalのBegin/End、
空geometry、fit／fill／clamp／zoom中心、2回のContext生成を検証しました。
定常frameのC++／ImGui allocationは0でした。既存native API・design system・Editor Coreの
直接回帰、新規APIのDebug／Release fixture、source／relocated SDK consumerのcompile・link・
実行も合格しています。Debug／Release Galleryをbuildし、公開IOによるfilter click・cursor zoomと
38枚のnative GPU captureを取得、代表的なworkspace・feedback・Tile・狭幅日本語を目視確認しました。
Debug／Release installに新規headerを収録し、既存installが要求する任意Win32 accessibilityの
Release libraryもbuildしました。

These checks do not cover every state combination, native OS/IME input, real
screen-reader operation, external application integration or release publication.
No claim is made that pre-existing Editor Suite acceptance is complete.

全状態組合せ、native OS/IME、実スクリーンリーダー、他アプリ統合、Release公開は未実施です。
既存Editor Suiteの全受け入れ項目を今回の結果で完了扱いにはしません。

Design-system foundation verification and remaining implementation: [design-system.md](design-system.md#recorded-verification--今回の検証). This is separate from the Editor Suite 2.0 record below.

デザインシステム基盤の検証・残る実装は[刷新文書](design-system.md)を参照してください。以下のEditor Suite 2.0記録とは別の検証です。

Editor Suite 2.0 results and boundaries: [Editor refresh](editor-refresh.md#recorded-verification-2026-09-10--今回の検証結果).
Editor Suite 2.0の今回の結果と検証範囲は[刷新記録](editor-refresh.md)を参照してください。以下の従来記録とは区別します。

This record distinguishes compile/link, public-IO interaction, GPU appearance and distribution. Passing native-API signature coverage does not mean every native behavior was exhaustively retested.

compile/link、公開IO操作、GPU外観、配布を区別します。署名確認を、標準APIの全動作を個別に再検証した証拠としては扱いません。

| Gate | Evidence / 確認内容 |
|---|---|
| Public API | 365 included overloads; independent function-pointer signature references compile and link against the pinned header; excluded functions are enumerated |
| Debug / Release | Library and production catalog compile/link on Windows x64, MSVC 19.51.36256 / v145 |
| Ownership | Two independent contexts; non-cumulative scale; nested ThemeScope style/font restoration; surrounding disabled alpha retained |
| Theme presets | 12 stable unique IDs; complete finite palettes; Light/Dark metadata; legacy Precision values; normal/muted/semantic contrast thresholds |
| Motion | Intermediate value, endpoint, disabled immediate result, pruning and generation reset |
| External target | Separate consumer creates its own ImGui target; no core/backend sources in imkit; no GLFW/OpenGL target leakage |
| Actions | Pointer/model edits, disabled action, switch, mixed-to-checked, radio, same-label IDs, programmatic focus, Space, Tab/Shift-Tab |
| Numeric | Slider edit, Ctrl-click drag direct input, exact 64-bit value beyond float integer precision, range drag and ordering |
| Input / Media | UTF-8 editing, edit callback, resize callback, multiline, validation edit, image button and color picker input; real color/image/plot rendering |
| Hierarchy | Tree expansion, table row selection, independent inline action, descending sort, scrolling with frozen headers, tab switch/reorder/close |
| Overlay | Popup open/Escape, menu selection, modal blocking, nested combo, modal Escape/cancellation and launcher focus restoration |
| Composites | Segmented selection, disabled search result, filtered result selection |
| Visual | Product Home, searchable navigation, 12 presets, component categories and advanced examples; Japanese glyphs, representative 1.5 scale, modal; real OpenGL backbuffers |
| Installed SDK | Debug and Release consumer compile/link/run from a relocated prefix; normalized archives contain no Windows absolute paths |
| Distribution | Source/SDK manifests identify the source commit and ABI; release attachments and SHA256SUMS are the authoritative published artifacts |

The public-IO integration log is `out/catalog/interaction.txt`; renderer details are in `out/catalog/capture-info.txt`. These are generated outputs. Representative images are checked into `docs/images/` for documentation. Full captures are available as release evidence.

The README animation is generated from 120 native 960×540 backbuffer frames. `tools/build_readme_gif.py` enforces the frame count, dimensions and 8 MiB limit; it does not validate native OS input.

公開IOログは`out/catalog/interaction.txt`、renderer情報は`out/catalog/capture-info.txt`に生成します。文書用の代表画像は`docs/images/`、全カテゴリ画像はReleaseの検証用archiveに収録します。

README animationは960×540のnative backbuffer 120枚から生成します。`tools/build_readme_gif.py`はframe数、寸法、8MiB上限を確認しますが、native OS入力の検証ではありません。

## Limits / 制約

- Baseline: Dear ImGui **1.92.9b docking**, default ABI types, Windows x64/MSVC. Other versions and platforms are not verified or implicitly compatible.
- Native OS/IME input, physical devices and integration into another application are not tested by this runner.
- Reordering/resizing/drag-drop preserve exact native APIs; exhaustive combinations are not reimplemented or exhaustively retested.
- Native wrappers are immediate. Optional explicit animation affects custom decoration only; fonts/renderer/state ownership stays with the host.
- Input units are separate labels. Native numeric text layout and parsing are retained, including native centering for drags/sliders and native text editing behavior.
- Japanese coverage is limited to the supplied font. Japanese headings use Regular; arbitrary Unicode/emoji coverage is not claimed.
- Screenshot state after interaction can differ from initial values; no synthetic reference image is used as evidence.

対応版・環境以外、native OS/IME、実機、他アプリへの導入は未検証です。標準のreorder/resize/drag-dropの全組合せを再試験していません。日本語の字形範囲、通常weight、数値の標準レイアウトを明示し、未実施の受け入れ確認を合格扱いしません。

## Editor Suite 1.0

See [Editor validation](editor-validation.md) for CPU/public IO, source and installed
consumers, native GPU, captures and the six-operation Release performance result.
EditorのCPU・公開IO・source/SDK consumer・実GPU・capture・6操作のRelease性能は上記を参照してください。
Native OS/IME and real-project integration remain separate, unperformed categories.
native OS/IMEと実project統合は別区分で、未実施です。
