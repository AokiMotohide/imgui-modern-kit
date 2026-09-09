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

The native Gallery verifier checks actual OpenGL indexed cubes and sphere, depth
ordering, 64-bit ID picking, background picking, FBO resize, resource deletion and
reinitialization. It drives Timeline move/end trim/split, CG X-axis gizmo, curve key
movement and UV vertex movement through public Dear ImGui IO. It also checks shared
object selection. Renderer observed: NVIDIA GeForce RTX 3090 Ti, OpenGL 3.3.0,
NVIDIA 616.56, Dear ImGui 1.92.9b.
実Galleryでcube/sphere、depth、64bit picking、背景、resize、削除/再初期化を確認します。
公開IOでTimeline移動/終端trim/split、CG X軸gizmo、curve key移動、UV vertex移動を検証します。
GPUはRTX 3090 Ti、OpenGL 3.3.0、NVIDIA 616.56、ImGui 1.92.9bです。

```powershell
ctest --test-dir build/windows-debug -C Debug -R "imkit.(editor_core|video|cg|editor_api_compile)" --output-on-failure
build/windows-debug/catalog/Release/imkit_gallery.exe --verify-editors --capture-editors --output out/editor-release
```

`--capture-editors` captures Editor Core and light/dark Video/CG workspaces, including
150% scale, from the real backbuffer. Artifacts under `out/` are uncommitted.
The performance fixture builds 256 tracks, 100096 clips and 100000 keys, warms 20
frames, then measures 180 panning frames at 1920×1440 with swap interval zero.
Its CPU wall time includes UI construction, GL submission and swap. It reports
visible query/clip counts and separate C++ `new` and ImGui allocator counts; driver
and operating-system allocations are outside these counters.
captureは実backbufferで、out/配下は非コミットです。性能fixtureは256 track・100096 clip・
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
