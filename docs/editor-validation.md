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
