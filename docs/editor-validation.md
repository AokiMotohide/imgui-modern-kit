# Editor validation / Editor検証

Full Editor Suite 1.0 acceptance is **incomplete**. Tests below verify only their
named contracts. They do not certify all requested editor workflows.
Editor Suite 1.0全体の受入は**未完了**です。以下は記載した契約だけを検証します。

CPU fixtures cover frame/tick conversion, NTSC drop-frame minute boundaries and
negative pre-roll, fixed-buffer transactions and revision cancellation, snap,
cursor-centered zoom, polygon inclusion, curve interpolation and handle modes,
constrained trim/roll/slide/slip/split, PCM buckets, meter hold, scopes, projection
and UV transforms. `imkit.editor_api_compile` exercises the public targets within
a CPU ImGui frame; an external consumer compiles and links the same fixture.
CPU fixtureは時間境界、整数event、cancel、snap、canvas、curve、clip編集計算、PCM/meter/scope、
投影、UVを検証します。API fixtureはCPU ImGui frame、外部consumerは独立ホストtargetで確認します。

Historical native Gallery runs checked OpenGL indexed cubes and sphere, depth
ordering, 64-bit ID picking, background picking, FBO resize, resource deletion and
reinitialization. It drives Timeline move/end trim/split, CG X-axis gizmo, curve key
movement and UV vertex movement through public Dear ImGui IO. It also checks shared
object selection. Renderer observed: NVIDIA GeForce RTX 3090 Ti, OpenGL 3.3.0,
NVIDIA 616.56, Dear ImGui 1.92.9b.
実Galleryでcube/sphere、depth、64bit picking、背景、resize、削除/再初期化を確認します。
公開IOでTimeline移動/終端trim/split、CG X軸gizmo、curve key移動、UV vertex移動を検証します。
過去の実行環境はRTX 3090 Ti、OpenGL 3.3.0、NVIDIA 616.56、ImGui 1.92.9bでした。

```powershell
ctest --test-dir build/windows-debug -C Debug -R "imkit.(editor_core|video|cg|editor_api_compile)" --output-on-failure
build/windows-debug/catalog/Release/imkit_gallery.exe --verify-editors --capture-editors --output out/editor-release
```

`--capture-editors` captures Editor Core and light/dark Video/CG workspaces, including
150% scale, from the real backbuffer. Artifacts under `out/` are uncommitted.
The earlier pan-only fixture built 256 tracks, 100096 clips and 100000 keys, warmed 20
frames, then measured 180 panning frames at 1920×1440 with swap interval zero.
Its CPU wall time includes UI construction, GL submission and swap. It reports
visible query/clip counts and separate C++ `new` and ImGui allocator counts; driver
and operating-system allocations are outside these counters.
captureは実backbufferで、out/配下は非コミットです。過去のpan専用fixtureは256 track・100096 clip・
100000 key、20 frame warm-up後180 pan frameを1920×1440・swap interval 0で計測します。
CPU wall timeにはUI構築・GL投入・swapを含みます。C++ newとImGui allocatorは別計数し、
driver/OS内部allocationは計数対象外です。

Native OS/IME, media decode/playback, real project integration, complete editor
workflow coverage and distribution/SDK-package acceptance have not been performed.
native OS/IME・実media decode/再生・実project統合・全編集workflow・配布SDK受入は未実施です。

## Recorded Release result / Release実測

2026-09-09, Windows x64 MSVC Release, RTX 3090 Ti: **CPU frame P95 3.3495 ms**
for 180 panning frames. Maximum visible queries: **9**; visible clips: **40**.
Tracked C++ new allocations: **0**; ImGui allocations: **0** after warm-up.
This passes the 16.7 ms target for this pan fixture only, not every editor operation.
Raw logs and captures are under `out/editor-release/`.
2026-09-09のRelease pan fixtureはP95 **3.3495 ms**、最大**9 query / 40可視clip**、
C++ new／ImGui allocationはともに**0**でした。このpan fixtureは16.7 ms目標を満たしますが、
全編集操作の性能合格を意味しません。実ログ・captureはout/editor-release/配下です。

## Normal transform regression / normal変換の回帰確認

2026-09-09 Debug: `imkit.cg` checks DrawList triangle color against a known
inverse-transpose normal under rotation and nonuniform scale. The native
`--verify-editors` runner reads the OpenGL color texture for the same transform
and checks the expected Lambert result and exact object ID. Both passed, together
with the existing GPU lifecycle and representative public-IO checks.
Evidence: `out/editor-normal-debug/editors-interaction.txt`.
Debugのimkit.cgで回転・非等方scale後のDrawListの色を既知のnormalから計算した期待値と比較し、
実Galleryで同じ変換のOpenGL textureを読み戻してLambertの期待値とobject IDを確認しました。
両方合格し、既存のGPU lifecycleと代表公開IO操作も通過しました。native OS/IME確認ではありません。

Transaction regressions additionally cover retained Commit/Cancel on overflow,
rejecting a value update after a pending Commit, and preventing Cancel-to-Commit
conversion. Public Timeline calls with a one-event buffer verify that two-clip
termination emits no partial batch, and that release and revision cancellation
complete on a later call with sufficient capacity. Debug Core/Video tests and the
existing Gallery interaction runner passed (`out/editor-transaction-debug/`).
transaction回帰はoverflow後の終端保持、Commit待機中の値変更拒否、CancelのCommit化防止を検証します。
公開Timelineへ容量1のbufferを渡し、2 clipの終了を部分送信せず、容量回復後にrelease・revision cancelを
まとめて完了することを確認しました。Debug Core/Videoテストと既存Gallery操作は合格です。

## Color controls / 色操作

Debug `imkit.video` verifies RGB waveform channel/column placement, rejects partial
RGB buffers, and drives all three wheels through public ImGui IO. It checks RGB
proposals, deferred host Commit and revision Cancel. `imkit.editor_api_compile`
and the incremental external `imkit_editor_consumer` target compile the new overloads.
The focused native `--verify-color` runner applies all three wheel commits to Gallery
host values and captures light/dark backbuffers (`out/editor-color-debug/`).
DebugのvideoテストはRGB waveformのchannel/横位置、不完全bufferの拒否、3つのwheelの公開IO操作、
RGB提案値・host Commit・revision Cancelを確認します。API fixtureと外部consumerの増分ビルドも成功しました。
専用の--verify-colorは3つのwheelをGalleryのhost値へ適用し、light/darkの実backbufferを保存します。
画像でwheel・RGB paradeの描画を確認しました。native OS/IME・色管理の検証ではありません。

Audio focused tests cover native fader and pan public-IO Commit, explicit nonadjacent
property IDs, mute control events, locked controls, and nonfinite first PCM sample
sanitization. Debug video/API fixtures pass. The Gallery Video backbuffer includes
PCM min/max clip waveforms and the connected mixer/stereo meters; artifacts are in
`out/editor-audio-debug/`. No media playback/recording engine or OS audio device is tested.
音声のfocused testはfader/panの公開IO Commit、隣接しない明示ID、mute event、locked操作、PCM先頭の非有限値を
確認し、Debug video/API fixtureは成功しました。GalleryのVideo backbufferでPCM波形・mixer・stereo meterを確認します。
media再生/録音engine・OS audio deviceの検証ではありません。

Debug CG tests verify orthographic projected-size changes, view-relative pan,
six axis alignments, camera-view navigation protection, and public IO wheel/axis
gizmo clicks. The non-owning host camera is checked through BeginViewport.
The API fixture covers NavigateCamera/AlignCamera. These checks do not certify
all transform-gizmo or camera-overlay requirements.
Debug CGテストはorthographic投影倍率・view相対pan・6軸alignment・camera表示のnavigation保護と、
公開IOのwheel/gizmoクリックを確認します。BeginViewport経由でホストcamera参照も確認しました。
API fixtureは新しいcamera関数を呼びます。全transform gizmo・camera overlayの合格を意味しません。

Variable track layout tests check the queried visible pixel interval, expanded and
collapsed extents, and public-IO collapse/source-patch events. Debug Video and API
fixtures passed. The native editor verifier also passed Timeline move/end-trim/split
and the existing shared selection, curve and UV paths using the indexed Gallery
track layout (`out/editor-track-layout-debug/`). This is not full track-role/icon
acceptance or native OS input evidence.
可変track配置のテストは可視pixel区間query、展開・折り畳み高、公開IOのcollapse/source eventを確認します。
Debug Video/API fixtureと、累積高さqueryへ切り替えたGalleryでの移動・終端trim・split・選択・Curve・UVの
既存操作は成功しました。全track role・icon受入やnative OS入力の検証ではありません。


## Saved six-operation Release benchmark / 保存済み6操作測定

The saved `out/editor-benchmark-related-edits-fixed/editor-performance.csv` and `editor-performance-context.txt` contain the following results. They predate the latest camera, icon and UI changes and are not final acceptance of the current HEAD. The independent `--benchmark-editors` mode measures 20 warm-up and 180 sample frames per operation at 1920×1440, 256 tracks, 100096 clips and 100000 keys. Input uses public ImGui IO in a hidden native GL window with vsync off. `Host::Frame` wall time includes host event application, preview rendering, ImGui, GL submission and swap. Continuous-drag terminal frames are measured separately.

保存済みCSVとcontextから転記した結果です。最新のcamera・icon・UI変更より前の測定であり、現在のHEADの最終受入ではありません。1920×1440、256 track、100096 clip、100000 keyで、各操作20 warm-up＋180測定frameです。非表示native GL windowへ公開IOを注入し、vsyncを無効にしています。測定境界は編集適用・preview・ImGui・GL発行・swapを含むHost::Frameです。連続dragの終端frameは別に測定します。

| Operation / 操作 | P95 ms | Max ms | Terminal ms |
|---|---:|---:|---:|
| Pan | 1.6822 | 2.2616 | 1.9811 |
| Zoom | 1.7300 | 2.9294 | 0.9631 |
| Selection | 1.7890 | 2.4931 | 1.0157 |
| Clip drag | 1.8730 | 2.4208 | 9.2084 |
| Clip end trim | 1.6781 | 2.3307 | 8.3003 |
| Inline keyframe drag | 1.7585 | 2.1366 | 9.3105 |

All six saved interactions passed their state-change checks and the 16.7 ms P95 target. Each recorded at most seven visible queries, 30 clips, eight keys and six track rows, with zero measured steady C++ new and ImGui allocations. Key counts include returned clip-local editing spans and Curve neighbors, excluding full borrowed evaluation channels. Driver/OS allocations and the separate terminal frame are outside the steady allocation result. Native OS/IME is not covered. The latest GPU attempt failed GLFW/WGL context creation; final GPU capture and Release measurement remain outstanding rather than inferred from these earlier results.

保存された6操作はいずれも状態変化確認とP95 16.7 ms目標に合格しています。各操作の最大値は7 query・30 clip・8 key・6 track行で、定常C++ new／ImGui allocationは0です。key数はclip内編集spanとCurveの隣接keyを数え、非所有の全評価channelは除きます。driver／OS allocationと別測定の終端frameは定常allocation結果に含めません。native OS／IME確認ではありません。直近のGPU試行はGLFW/WGL context作成に失敗しており、最終GPU captureとRelease測定は未実施です。

An unchanged Move/TrimStart/TrimEnd/Ripple/Roll/Slip/Slide Commit preserves the host revision and bypasses index rebuilding. The headless host-model verifier covers all seven kinds. The benchmark clears selection before each operation and validates the expected selected StableId, so an idle frame with a preexisting selection cannot pass its selection check.

値が変わらないMove／TrimStart／TrimEnd／Ripple／Roll／Slip／SlideのCommitでは、ホストrevisionと索引を維持します。7種をheadlessのホスト適用検証で確認しました。benchmarkは操作ごとにselectionを空にし、期待する選択先StableIdを確認するため、既存selectionを残しただけの無操作フレームは選択検証に合格しません。


An earlier incremental external Debug `imkit_editor_consumer` build and CPU-frame execution covered the Timeline host atlas and five edit-tool IconIds, explicit clip key channel/default, transition picker/edit, envelope point/evaluation, six clip-role palette fields, and Monitor metadata options. The consumer defines its own ImGui target and uses the public module targets through `add_subdirectory`. This is source-consumer evidence; installed-package, Release-consumer and GPU execution are separate gates. The generated `api-inventory.json` inventories native Dear ImGui overloads; custom editor additions are listed in `editor-api.md`.

過去の外部Debug `imkit_editor_consumer`の増分ビルドとCPUフレーム実行で、Timelineのホストatlas・5編集ツールIconId、clipの明示key channel／既定値、transition picker／計算、envelope点／評価、6種clip色、Monitor metadata設定を確認しました。consumerは自身のImGui targetを定義し、`add_subdirectory`経由の公開module targetを使用します。install済みpackage・Release consumer・GPU実行は別gateです。生成`api-inventory.json`はDear ImGui標準overload用であり、独自Editor追加APIは`editor-api.md`に記載しています。
