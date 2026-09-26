# Editor Suite 2.0 — interaction design and migration / 操作設計と移行

Editor 2.0 updates the reusable editor controls and their native Gallery host. Media
processing, edited data, undo, resources and workers remain host-owned. The version
change requires rebuilding all consumers; it does not upgrade Dear ImGui or add a
backend dependency.

Editor 2.0は編集部品とnative Galleryを更新します。素材処理・編集データ・Undo・リソース・
workerの所有者はホストです。利用側の再ビルドが必要です。Dear ImGui版とbackend依存は変更しません。

## Design decisions / 設計判断

- CapCut: place keyframe access next to the selected clip's properties. Reference:
  [desktop keyframes](https://www.capcut.com/help/keyframes-in-capcut-pc).
- Blender: distinguish translation arrows, rotation rings and scale squares; expose
  orientation and pivot, and share animation selection. References:
  [transform gizmos, archived 2.90 manual](https://docs.blender.org/manual/en/2.90/scene_layout/object/editing/transform/control/gizmos.html),
  [Timeline](https://docs.blender.org/manual/en/latest/editors/timeline.html).
- DaVinci Resolve: make edit boundaries discoverable, retain audio peaks when zoomed
  out, and expose an overview next to detailed editing. References:
  [Edit](https://www.blackmagicdesign.com/jp/products/davinciresolve/edit),
  [Cut](https://www.blackmagicdesign.com/products/davinciresolve/cut).

CapCutの選択対象に近いキー操作、Blenderの形状による変換操作の区別、Resolveの編集境界・
波形・全体位置の把握を採用しました。配色や画面の複製ではありません。BlenderのGizmo資料は
旧版の基本操作のみを参照しています。各製品の実アプリ操作比較は行っていません。

The old Gallery drew gizmos at committed positions while meshes used proposed
positions. Waveforms distributed one vertical line per bucket over a whole clip,
without source-time metadata. Fixed tool rows and the catalog heading consumed
available editor space. These findings motivate the state and layout changes.

旧Galleryではメッシュの提案値とGizmoの確定位置が一致せず、波形は素材時間情報なしに
クリップ幅へ割り当てられていました。横一列のツールと大きなカタログ見出しも編集面積を
圧迫していました。今回の状態・レイアウト変更はこれらの問題に対応します。

## Gizmo contract / Gizmo契約

`TransformGizmo` captures the start pivot, basis, camera and viewport in
`ViewportState::gesture*`. Keep the host pivot stable during the gesture and increment
revision for external model changes. Moving the drawing never changes the input
reference. An externally changed pivot cancels the gesture.

`TransformGizmo`は開始ピボット・基底・カメラ・Viewportを`gesture*`に保存します。
操作中はホストのピボットを固定し、外部モデル変更時はrevisionを進めてください。
描画位置の移動は入力の基準を変えません。外部ピボット変更は取消になります。

Use `cg::PreviewTransform(object, state)` for the uncommitted transform used by
meshes and Inspector. Submit the gizmo before dependent overlays. The Gallery renders
the GL texture after UI event processing and before ImGui draw submission.

メッシュ・Inspectorの未確定変換は`cg::PreviewTransform(object, state)`から取得します。
Gizmo処理の後に従属overlayを描画してください。GalleryはUIイベント処理後、ImGui描画送信前に
GL textureを描画します。Context・GL関数・textureの所有権は変わりません。

Apply terminal events before drawing dependent overlays on the release frame. Resize
now preserves the GL color texture name, including draw commands already recorded in
that frame. Gallery camera/light helpers are wireframes in CG and omitted in Program.

リリースframeはterminalイベントを適用してから従属overlayを描きます。GLのResizeは同じ
color texture名のstorageを更新し、そのframeに記録済みの描画参照を維持します。
Galleryのカメラ・ライト補助はCGではワイヤ、Programでは非表示です。

## Source-time waveforms / 素材時間に対応する波形

Set `ClipView::audioSource` and `audioChannels`, then provide
`TimelineProvider::waveform`. `WaveformQuery` contains source ID, channel, visible
source Tick range and requested pixel count. `WaveformView` returns borrowed uniform
min/max buckets, their actual covered Tick range, and Pending/Ready/Error status.
The returned data must remain valid until the next query. Cache and multiresolution
storage remain in the host; bound returned work to the requested resolution.

`audioSource`と`audioChannels`を設定し、`TimelineProvider::waveform`を渡します。
queryは素材ID・チャンネル・可視素材Tick範囲・要求ピクセル数です。viewは等間隔min/max bucket、
実際の素材Tick範囲、Pending/Ready/Errorを返します。返却メモリは次のqueryまで有効にし、
キャッシュと解像度別データはホストで保持してください。要求解像度に応じて返却量を制限します。

```cpp
provider.waveform = {&host, [](void* user, const imkit::video::WaveformQuery& q) {
    return static_cast<Host*>(user)->QueryWaveform(q);
}};
clip.audioSource = sourceId;
clip.audioChannels = 2;
timeline.waveformOptions = {1.0f, true, true};
```

`WaveformPixel` aggregates peaks across a source interval; `DrawWaveform` draws a
clipped pixel envelope. Display gain changes visualization only. Pending and Error
are distinct from Ready silence. Positive finite clip speed maps time as
`sourceIn + (timelineTime - start) * speed`. Legacy `waveform`/`audioBuckets` remain
available as normalized whole-clip views; migrate to the provider for trim-aware audio.

`WaveformPixel`は素材範囲のピークを集約し、`DrawWaveform`はclipされた包絡を描画します。
表示倍率は音量に影響しません。Pending／ErrorとReadyの無音は別の状態です。
正の有限速度では`sourceIn + (timelineTime - start) * speed`で素材時間へ変換します。
旧spanはクリップ全体へ正規化した互換表示として残します。トリム対応にはproviderへ移行してください。

## Editing and history / 編集と履歴

`EventBuffer::PushBatch` validates phase/revision consistency and reserves capacity
before writing any member. Existing transactions retain Begin/Update/Commit/Cancel.
The host must validate all affected locks and constraints before mutation.

`PushBatch`はphase/revisionの一致と全件分の容量を確認してから書き込みます。
既存のBegin/Update/Commit/Cancelは維持します。適用前のlockと制約の一括検証はホストの責任です。

```cpp
// One logical operation; keep all members for retry when capacity is insufficient.
if (events.PushBatch(operationEvents)) {
    // Host validates the entire batch, then applies every member atomically.
    host.ValidateAndApply(operationEvents);
}
```

Rebuild all targets against the 2.0 headers and use `find_package(imkit 2.0 CONFIG)`.
The appended source commands require sufficient host binding storage; use the count
returned by `MakeBindings`. Persist logical model data, never raw widget-state bytes.

全targetを2.0ヘッダーで再ビルドし、`find_package(imkit 2.0 CONFIG)`を指定します。
追加した素材Commandを収容できるbinding容量を確保し、`MakeBindings`の返却件数を使ってください。
永続化するのは編集モデルであり、widget状態の生バイト列ではありません。

The Gallery stores up to 16 host model snapshots, one per accepted commit frame.
Navigation and cancellation create no undo record. Undo/Redo restores model data,
selections and borrowed views, and advances revision. This bounded example is not a
library-owned undo service. Large production documents may use host delta histories.

Galleryは受理した確定frameごとにホストモデルを最大16件保存します。移動表示と取消は履歴を
作りません。Undo/Redoはデータ・選択・非所有viewを復元しrevisionを進めます。
これはライブラリ所有の履歴サービスではなく、ホスト実装例です。大規模な実製品では差分履歴も使えます。

Source placement uses an explicit track and source In/Out. F9 inserts, F10 overwrites,
and Shift+F9 appends. Insert shifts targeted tracks; overwrite operates on the chosen
track. Locked or out-of-scope linked members reject the complete request. The source
is a deterministic Gallery fixture, not decoded media. Property channels can be opened
from Inspector and edited in the shared graph/dope-sheet views.

素材配置は対象trackと素材In/Outを明示します。F9は挿入、F10は上書き、Shift+F9は末尾追加です。
挿入はtarget trackを移動し、上書きは指定trackに適用します。lockや範囲外のリンク対象があれば
全体を拒否します。素材はGalleryの決定的な例であり、動画をdecodeしていません。
Inspectorからプロパティチャンネルを開き、Graph／Dope Sheetで編集できます。

## Native layout and verification / native配置と検証

Editor pages use a compact catalog header. Video defaults to Program, with Source and
Compare modes and optional details. Horizontal dividers resize the upper/lower areas;
vertical dividers resize side areas. CG tools use a vertical rail and overlay popover.
The overview navigates time without changing edit data; Fit selection uses complete
host selection. Capture accepts `--width`, `--height` and `--japanese`.

Editorページはコンパクトな見出しを使います。VideoはProgramが初期表示で、Source・Compareと
詳細パネルを切り替えられます。上下・左右の境界はドラッグで変更できます。
CGは縦の変換ツールと表示Popoverを使います。全体バーの移動は編集データを変えず、
選択Fitはホストの完全な選択を使います。captureは`--width`・`--height`・`--japanese`に対応します。

Validation records for 1.0 remain historical in `editor-validation.md`; they do not
certify 2.0. Current results are recorded in the implementation completion report.
Public ImGui IO, GPU captures, native OS/IME and real-project integration are separate.

`editor-validation.md`の1.0検証は過去の記録であり、2.0の合格を意味しません。
今回の結果は実装完了報告に記録します。公開ImGui IO・GPU capture・native OS/IME・実project統合は
別の検証区分です。

Animation strips optionally receive `StripOptions::selection`, `time` and `snap`.
Their absolute gesture delta avoids accumulated snapping error. Small editor windows
use tighter control padding; font scaling is retained, and auxiliary panes can be shown
explicitly. Overview edges resize the visible time interval.

Animation Stripsは任意で`selection`・`time`・`snap`を受け取ります。操作開始からの絶対差分を
使い、スナップの累積誤差を防ぎます。小画面では文字倍率を保ったまま余白を詰め、補助ペインを
明示的に表示できます。全体バーの端点ドラッグは可視時間範囲を変更します。

## Recorded verification, 2026-09-10 / 今回の検証結果

| Scope / 範囲 | Result / 結果 |
|---|---|
| Debug | Core, Video, CG and editor API tests passed; Gallery compiled/linked / 対象4テスト合格、Galleryビルド成功 |
| Host model | 117 checks passed, including source placement, lock rejection and undo/redo / 素材配置・lock拒否・履歴を含む117項目合格 |
| Native renderer + public IO | 101 checks passed; 8 combinations of perspective/orthographic, World/Local and single/multiple selection test movement, stationary preview, Escape, exact commit and Undo / 8条件で移動・静止・取消・確定・Undoを確認 |
| Consumer | Independent source Debug and installed Release editor consumer compile/link/run passed / 独立source Debug・installed Release consumer成功 |
| Visual | Native captures at 1280×720 and 1920×1080, English/Japanese, Light/Dark, 100%/150%; drag capture included / 2解像度・2言語・2配色・2倍率とドラッグ途中をcapture |

Release, 100,000-item fixture, 180 measured frames per operation:

| Operation / 操作 | P95 ms |
|---|---:|
| Pan | 1.6466 |
| Zoom | 1.4498 |
| Selection / 選択 | 1.4570 |
| Clip drag / 移動 | 1.9560 |
| Clip trim / トリム | 1.7285 |
| Key move / キー移動 | 1.8476 |

All six remain below 16.7 ms, with zero C++/ImGui allocations in the measured frames.
This is the existing active-interaction benchmark, not a bound on snapshot commit
latency, media processing or other hosts. The Gallery's bounded snapshot history is a
reference implementation; large production histories should use host-specific deltas.

6操作ともP95 16.7 ms以下で、計測frameのC++／ImGui allocationは0です。
既存の操作中benchmarkであり、snapshot確定・素材処理・利用側全体の遅延上限ではありません。
Galleryの上限付きsnapshot履歴は実装例です。大規模な製品の履歴にはホスト固有の差分管理を使えます。

Generated evidence: `out/editor-refresh-io-v4/`, `out/editor-refresh-host-v2.txt`,
`out/editor-refresh-performance-v3/editor-performance.csv`, and
`out/editor-refresh-final-{720,1080}-{en,ja}/`. Captures verify appearance, while the
public-IO checks verify the listed operations. Native OS/IME, exhaustive combinations
of property workflows and real-project integration remain unverified. No media decoder
or playback engine was added. Build artifacts and captures are not published.

上記は生成されたローカル証拠の保存先です。captureは外観、公開IO試験は記載した操作を検証します。
native OS/IME、プロパティ編集の全組合せ、実project統合は未検証です。
decode・再生エンジンは追加していません。ビルド成果物とcaptureは公開していません。
