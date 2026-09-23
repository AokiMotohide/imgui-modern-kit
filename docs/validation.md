# Validation / 検証

[日本語](validation.ja.md) · [English](validation.md)

## v3.0 platform matrix / v3.0 platform matrix

| Configuration | Automated gate | Native acceptance |
|---|---|---|
| Windows 10/11 x64 | build, focused tests, Gallery, install consumer, ZIP | required on one Windows x64 machine |
| Windows 11 Arm64 | build, focused tests, Gallery, install consumer, ZIP | architecture-specific manual run is not required |
| macOS 15+ Apple Silicon | build, tests, Metal Gallery, app bundle, ZIP | required on one Apple Silicon Mac |
| macOS 15+ Intel | build, tests, Metal Gallery, app bundle, ZIP | architecture-specific manual run is not required |

PR #8 head `326618c` passed all five GitHub Actions jobs on 2026-09-13:
Windows x64 and Arm64 build/test/package, macOS arm64 and x86_64 build/test/package,
and the Universal 2 Gallery/package job. The arm64 job also launched the native Metal
Gallery smoke path. Signing/notarization steps were skipped because release credentials
were unavailable.

PR #8 head `326618c`は2026-09-13にGitHub Actions全5 jobへ合格しました。
Windows x64／Arm64のbuild・test・package、macOS arm64／x86_64のbuild・test・package、
Universal 2のGallery build・packageが対象です。arm64ではnative Metal Gallery smokeも
成功しました。release資格情報がなかったため、署名・notarization stepは未実施です。

These automated gates do not establish physical Apple Silicon/Intel acceptance,
Japanese IME, Retina or mixed-DPI behavior, traffic-light interaction, VoiceOver,
native pointer/keyboard behavior or external-host integration.

これらの自動gateは、Apple Silicon／Intel実機受入、日本語IME、Retina／mixed-DPI、
traffic-light操作、VoiceOver、native pointer／keyboard、外部host統合の合格を意味しません。

## Node editor access, appearance and collapse / Node editorの導線・外観・折りたたみ（2026-09-12）

The Debug Gallery and Node Editor companion built successfully. `imkit.node_editor`
passed host name/color Undo/Redo, borrowed header styles and repeated collapsed-node
layout checks. The collapse regression reproduced Dear ImGui's cursor-boundary
assertion before the fix. The native `--verify-node-actions` runner passed 60
public-IO clicks on Output monitor, producing 30 collapse/expand transitions.
Native OS mouse/IME input and launcher focus behavior were not verified. The v3 PR
CI subsequently built and packaged the companion on every supported architecture.

Debugの通常GalleryとNode Editor実行ファイルはbuildに成功した。`imkit.node_editor`で
ホスト所有の名前・色のUndo／Redo、借用header style、折りたたみの反復を確認した。
修正前には折りたたみ回帰テストでDear ImGuiのcursor境界assertionを再現した。
native `--verify-node-actions`ではOutput monitorへの公開IOクリック60回、
折りたたみ・展開30回が成功した。native OSマウス／IMEとlauncherの前面化は未検証。
v3 PR CIではその後、全対応architectureでcompanionのbuildとpackageが成功した。

## V3 Gallery and documentation captures / V3 Galleryと文書capture（2026-09-13）

The Debug native Gallery build passed `--verify-comparison`: Default Dear ImGui and ImKit controls changed the same host-owned value, temporary comparison styles restored after a frame, and closing/reopening removed then restored the submitted controls. V3 documentation captures contain 120 overview frames, 80 Node Editor frames, 120 workflow/progress frames, 80 timeline frames and 120 theme/comparison frames at native `960×540`. The checked-in encoder accepted all five GIFs under its 8 MiB limit. The Node Editor sequence deterministically changes host-owned zoom/pan state, adds a visible socket, connects it, and shows inline values, previews and the minimap. The showcase MP4 was encoded from the same 520 native frames.

Debug native Galleryで`--verify-comparison`が成功しました。Default Dear ImGuiとImKitのcontrolは同じホスト所有値を変更し、比較用の一時styleはframe後に復元され、close/reopenでcontrolが消えて再表示されました。V3文書用にoverview 120、Node Editor 80、workflow／progress 120、timeline 80、theme／comparison 120 frameをnative `960×540`でcaptureし、5本のGIFは8MiB上限付きencoderを通過しました。Node Editor列はホスト所有zoom／pan、可視socket追加と接続、inline値、preview、minimapを決定的に変化させます。showcase MP4は同じnative 520 frameから生成しました。

The runner sends public Dear ImGui IO and reads a real OpenGL backbuffer. It does not establish native OS/IME input, assistive-technology operation, physical-DPI, performance or external-host acceptance. The released Gallery executable is separately built, staged and inspected for its DLL imports; its archive does not bundle the Microsoft Visual C++ Redistributable.

runnerは公開Dear ImGui IOを送り、実OpenGL backbufferを読みます。native OS/IME入力、支援技術、実機DPI、性能、外部ホストでの受け入れは確認しません。公開Gallery実行ファイルは別途Release build・stage・DLL import確認を行い、Microsoft Visual C++ Redistributableをarchiveに同梱しません。

## Timeline external-drop preview / Timeline外部drop preview（2026-09-12）

The Debug Video test, Editor API compile fixture and independent source consumer passed.
The focused checks cover the optional exact candidate range callback, the preserved
single delivery callback and source-compatible aggregate initialization. Release,
native OS input and external-host acceptance were not run.

DebugのVideo test、Editor API compile fixture、独立source consumerが合格しました。任意の正確な
候補範囲callback、deliveryが1回だけであること、既存aggregate初期化とのsource互換を確認しました。
Release、native OS入力、外部ホスト受け入れは未実施です。

## Preview placement extension / Preview配置拡張（2026-09-12）

The affected Debug library, API-fixture and Gallery targets built successfully.
`imkit.workflow`, `imkit.workflow_api_compile`, `imkit.video`, `imkit.cg`,
`imkit.editor_api_compile` and `imkit.consumer_build` passed. CPU checks cover
Fit/Fill/Stretch placement, invalid and extreme geometry, ImageViewport mode
recalculation and pan/coordinate behavior, legacy and additive Monitor rendering,
UV cropping and `flipY`, all five preview states, action requests, steady-frame
allocation, and perspective/orthographic projection at 4:3, 16:9, 1:1 and 9:16.

影響するDebug library・API fixture・Gallery targetはbuildに成功しました。
`imkit.workflow`、`imkit.workflow_api_compile`、`imkit.video`、`imkit.cg`、
`imkit.editor_api_compile`、`imkit.consumer_build`が合格しました。CPU検証は
Fit／Fill／Stretch配置、不正・極端geometry、ImageViewport mode再計算とpan／座標変換、
新旧Monitor描画、UV cropと`flipY`、5状態、action要求、定常frame allocation、
Perspective／Orthographicの4:3・16:9・1:1・9:16投影を対象にしました。

The Debug native Gallery verifier passed public-IO interaction and OpenGL backbuffer
captures for the four ImageViewport modes, every combination of four source aspects,
five Monitor states and three placement modes, and uniform CG projection at the four
viewport aspects. The generated evidence under `out/preview-contract-debug/` is not
committed. Representative ImageViewport, portrait Loading Monitor and portrait CG
captures were visually inspected.

Debug native Gallery verifierでは公開IO操作とOpenGL backbuffer captureが合格しました。
ImageViewportの4 mode、4 source aspect×5 Monitor状態×3配置、4 viewport aspectのCG等方投影を
確認しています。`out/preview-contract-debug/`の生成物はcommitせず、ImageViewport、縦長Loading
Monitor、縦長CGの代表captureを目視確認しました。

For v2.2.0, Debug and Release built the affected library, Gallery and API fixtures. The focused
workflow, Video, CG, editor API, public API, Window Frame and accessibility checks passed; source
and staged-SDK consumers configured, built and ran in Debug and Release. The Windows Gallery and
SDK were staged for packaging, including the inspected DLL-import inventory.

v2.2.0では、影響するlibrary、Gallery、API fixtureをDebug／Releaseでbuildしました。workflow、Video、CG、
Editor API、公開API、Window Frame、accessibilityの対象checkが合格し、source consumerとstage済みSDK consumerを
Debug／Releaseでconfigure・build・実行しました。Windows GalleryとSDKは、DLL import一覧を確認してpackage用にstageしています。

Real camera enumeration/capture, media decoding, physical monitor selection or output, native OS/IME
input and external-host integration were not performed. Debug/Release and synthetic Gallery sources
do not establish those categories.

実Camera列挙・capture、media decode、物理monitor選択・出力、native OS/IME入力、外部ホスト統合は未実施です。
Debug／Releaseとsynthetic Gallery sourceを、これらの合格とは扱いません。

## Public window frame / 公開ウィンドウ枠（2026-09-11）

The isolated `build/window-frame-public-debug` tree passed the Debug public API unit test and compile fixture, the independent source consumer build, the minimal Gallery build, and `--verify-window-frame`. The verifier covered all four presets, Theme-derived color regeneration versus complete preset reset, metric and feature layout effects, UTF-8 elision, Win32 caption/client/edge hit tests including `HTMAXBUTTON`, maximize/work-area containment, restore, minimize and the GLFW close-request path. Generated captures and reports under `out/window-frame-public/` are not committed.

隔離した`build/window-frame-public-debug`で、Debug公開API unit test／compile fixture、独立source consumer build、最小Gallery build、`--verify-window-frame`が成功しました。枠検証は4 preset、Theme由来の色再生成と完全preset resetの区別、寸法・featureのlayout反映、UTF-8省略、`HTMAXBUTTON`を含むWin32 caption／client／edge hit test、最大化時のwork area、復元、最小化、GLFW終了要求経路を対象にしました。`out/window-frame-public/`の生成capture・reportはcommitしません。

GitHub-hosted macOS arm64/x86_64 compilation and arm64 automated Gallery smoke now pass.
No interactive physical-Mac acceptance was performed for traffic-light controls,
drag/full-screen/minimize/zoom, native input, 100%/200% or mixed-DPI behavior,
screen readers, signing or notarization. Automated launch does not establish those categories.

GitHub-hosted macOS arm64／x86_64 buildとarm64自動Gallery smokeは成功しました。
traffic-light、drag／full-screen／最小化／拡大、native入力、物理DPI 100%／200%、
mixed-DPI、screen reader、署名、notarizationの対話的な実機受入は未実施です。
自動起動をそれらの合格とは扱いません。

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

Design-system foundation verification and remaining implementation: [design-system.md](design-system.md#recorded-verification-今回の検証). This is separate from the Editor Suite 2.0 record below.

デザインシステム基盤の検証・残る実装は[刷新文書](design-system.md)を参照してください。以下のEditor Suite 2.0記録とは別の検証です。

Editor Suite 2.0 results and boundaries: [Editor refresh](editor-refresh.md#recorded-verification-2026-09-10-今回の検証結果).
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

- Baseline: Dear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`, default ABI types. Supported artifacts are the four v3 matrix configurations above; other combinations require source builds and explicit validation.
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

## Dynamic node sockets / 動的ノードソケット（2026-09-12）

The dedicated Debug node-editor library, API fixture and independent Gallery built
successfully. `imkit.node_editor` and `imkit.node_editor_api_compile` passed. The
same public fixture built and ran against a separate consumer-owned ImGui target.
Native Gallery capture was visually checked; clipped preview and minimap overlap
were corrected. [Review and exact scope](node-editor-review.md) records the P0/P1
findings, focused regressions and remaining boundaries. No capture is committed.

専用Debugのノードライブラリ・公開API fixture・独立Galleryがbuildに成功した。
直接テスト2件が合格し、独自ImGui targetを持つ独立consumerもbuild・実行に成功した。
Native captureを目視し、previewの切れとminimapの重なりを修正した。
P0／P1、直接回帰、未実施境界は上記レビューに記録した。captureはcommitしない。
実機OS／IME・DPI、支援技術、性能、外部ホスト統合、Release・配布検証は未実施。

## Node navigation completion / ノード周辺操作の完了（2026-09-12）

The focused Debug node test and native Gallery build passed. The new public-IO
regression verifies that minimap clicks navigate without editing underlying nodes.
Native captures cover both pages, Comfortable/Touch/Compact density, narrow width,
Japanese labels and High Contrast. Touch preview clipping was corrected by sizing
host views from control metrics; narrow vector controls retain precise tooltips.
The initial minimap cursor-restoration assertion was fixed before the final pass.

対象Debug testとnative Gallery buildが合格した。追加した公開IO回帰ではminimapのクリックが
背後のノードを編集せずviewportを移動することを確認した。両ページ、Comfortable／Touch／Compact、
狭幅、日本語ラベル、High Contrastをnative captureで目視した。Touchのpreview切れを
control寸法に基づくホストView高さで修正し、狭いVector欄は正確な値をtooltipでも表示する。
初回に検出したminimapカーソル復元assertは最終合格前に修正済み。
実機IME・DPI、性能、外部ホスト統合の受入検証とは区別する。
