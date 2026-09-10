# Editor Suite 1.0 validation / 検証結果

Validated on 2026-09-10: Windows x64, MSVC 19.51 (v145), Dear ImGui 1.92.9b-docking,
NVIDIA GeForce RTX 3090 Ti, OpenGL 3.3 NVIDIA 616.56. Tests use public ImGui IO;
they are not native OS/IME automation.
2026-09-10、Windows x64・MSVC 19.51・Dear ImGui 1.92.9b-docking・RTX 3090 Ti・
OpenGL 3.3 NVIDIA 616.56で確認しました。入力は公開ImGui IOで、native OS/IME自動操作ではありません。

## Build and contracts / ビルドと契約

Debug and Release passed the seven CPU/API/icon/context fixtures. After scoped changes,
only affected Core/Video/CG/API/icon targets were rerun. Source consumption with an
external host ImGui target passed in Debug; an installed 1.0 Release SDK consumer
compiled, linked and ran the full editor API fixture. The installed package contains
all five static libraries, headers, notices, 1236 icon PNGs and six atlases; originals
are intentionally excluded while prompts/provenance remain available.

Debug/Releaseの7つのCPU・API・icon・Context fixtureが合格しました。局所修正後は影響targetだけを
再確認しています。独立したホストImGui targetを持つDebug source consumerと、install済み1.0 Release
SDKのEditor API consumerはcompile/link/実行に合格しました。SDKは5静的library・header・notice・
1236 PNG・6 atlasを含み、原画を除外してprompt/provenanceを保持します。

Core fixtures cover timecode/snap, retained terminal events, revision/capacity handling,
property/asset actions and multi-key curve operations. Video covers edit calculations,
related members/locks, transition source handles, caption/key/envelope and audio/color
controls. CG covers affine orientation/pivot, gizmo transactions, hierarchy, animation
strips and all UV selection units. `--verify-inspector-model` additionally checks host
Commit/Cancel application, array reorder, RGB curves, marker edits, ripple deletion,
caption insertion and selection/property restrictions without opening a GL context.

Coreは時間・snap・終端保持・revision/容量・property/asset・複数curve編集、Videoは編集計算・関連対象/
lock・transition・caption/key/envelope・audio/color、CGはアフィン変換/pivot・gizmo・階層・strip・UVを
検証します。`--verify-inspector-model`ではGL Contextなしでホストの確定/取消、配列、RGB curve、marker、
ripple削除、caption追加、選択・property制限も確認します。

```powershell
ctest --test-dir build/windows-debug -C Debug -R '^imkit.(editor_core|video|cg|editor_api_compile|icons|api_compile|context_smoke)$' --output-on-failure
ctest --test-dir build/windows-debug -C Release -R '^imkit.(editor_core|video|cg|editor_api_compile|icons|api_compile|context_smoke)$' --output-on-failure
build/windows-debug/catalog/Release/imkit_gallery.exe --verify-inspector-model
```

## GPU and native captures / 実GPUとcapture

The native verifier passed indexed cube/sphere rendering, depth and full 64-bit ID
picking, background picks, FBO resize and invalid-resize preservation, Shutdown and
reinitialization, rotated/nonuniform/sheared mesh normals and lighting. Public IO also
moved/trimmed/split timeline clips, transformed CG/UV selections and moved curve keys.
Icon mouse/keyboard/disabled/tint checks and three color-wheel host commits passed.

実GPUでcube/sphere・depth・64bit picking・背景ID・FBO resize・不正resize時の保持・Shutdown/
再初期化・回転/非均等scale/shear後のnormalとlightingを確認しました。公開IOによるclip移動/trim/split、
CG/UV変換、curve key移動、iconのmouse/keyboard/disabled/tint、3色wheelのホスト確定も合格しました。

Light/dark Core/Video/CG and icon captures include Japanese and 150% examples. All 86
added glyphs were inspected in the native catalog at 16px and 150%, including scrolling
to its end. Color curves have a dedicated scrolled-pane capture. Capture scroll input
is placed outside embedded canvases so it does not unintentionally zoom their content.
NVIDIA display selection via `--monitor 0` resolved the earlier WGL startup failure in
this multi-adapter session; no display-driver or OS configuration was changed.

Core/Video/CG/Iconをlight/dark・日本語・150%の代表画面でcaptureしました。追加86 glyphはnative一覧の
16px・150%で末尾まで確認し、色補正curveにも専用captureを保存しました。capture時は埋込みcanvasの外へ
scroll入力を送り、意図せず拡大率を変えないようにしています。複数adapter環境での従来のWGL起動失敗は
`--monitor 0`によるNVIDIA画面指定で解消し、driver/OS設定は変更していません。

```powershell
build/windows-debug/catalog/Release/imkit_gallery.exe --list-monitors
build/windows-debug/catalog/Release/imkit_gallery.exe --monitor 0 --verify-editors --capture-editors --output out/editor-gpu
build/windows-debug/catalog/Release/imkit_gallery.exe --monitor 0 --capture --page 6 --icon-search Editor --output out/editor-icons
```

## Release performance / Release性能

1920x1440; 256 tracks, 100096 clips, 100000 keys; vsync off. Each operation used 20
warm-up and 180 measured frames. The measured boundary is Host::Frame wall time,
including host apply, preview, ImGui, GL submission and swap. All interactions passed
their state-change checks and the P95 target of 16.7 ms.

1920x1440、256 track・100096 clip・100000 key、vsync無効です。各操作20 frame warm-up後に180 frameを
測定しました。Host::Frame全体（ホスト適用・preview・ImGui・GL送信・swap）を含み、全操作の状態変化確認と
P95 16.7 ms目標に合格しました。

| Operation | P95 ms | Max ms | C++ new / ImGui allocations |
|---|---:|---:|---:|
| pan | 1.6981 | 2.8342 | 0 / 0 |
| zoom | 1.7868 | 2.8660 | 0 / 0 |
| selection | 1.8210 | 5.4273 | 0 / 0 |
| clip_drag | 1.7946 | 2.6771 | 0 / 0 |
| clip_trim | 1.8727 | 3.1109 | 0 / 0 |
| keyframe_drag | 1.7302 | 3.1683 | 0 / 0 |

Maximum returned work: 7 queries, 30 clips, 8 keys and 6 track rows. Steady allocations
exclude driver/OS internals and the separate terminal frame. Terminal clip drag/trim/key
drag measured 8.9656/10.8067/12.1895 ms. Full borrowed evaluation channels are excluded
from returned-key counts. [CSV](evidence/editor-performance.csv) and
[measurement context](evidence/editor-performance-context.txt) preserve the raw record.

最大返却量は7 query・30 clip・8 key・6 track行です。定常allocationはdriver/OS内部と別測定の終端frameを
除きます。clip drag/trim/key dragの終端は8.9656/10.8067/12.1895 msです。非所有の全評価channelは返却key数に
含めません。上記CSVと測定条件に原記録を保存しています。

## Unperformed and excluded / 未実施と対象外

Native OS/IME input and real-project integration were not performed. Media decoding,
resampling, full color management, node editing, UV unwrap, IK/simulation/animation
runtime, PBR/shadows and format loading are outside this UI suite. DrawList preview
has no z-buffer; OpenGL state restoration and current-context lifetime belong to the host.
The packaged SDK is Release/x64/MSVC v145 with /MD and a host-provided matching ImGui;
use a source build for other ABI/configuration combinations.

native OS/IME入力と実project統合は未実施です。media decode、resample、本格色管理、node、UV unwrap、
IK/simulation/animation runtime、PBR/shadow、形式loaderは対象外です。DrawListにはz-bufferがなく、GL状態復元と
current Contextの寿命はホスト責任です。SDKはRelease/x64/MSVC v145・/MDと一致するホストImGui向けで、
異なるABI/構成にはsource buildを使用してください。
