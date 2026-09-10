# Timeline editing / タイムライン編集

The timeline's optional `TimelineEditingProvider` enables independent clip fades,
cut transitions, rectangular selection and validated track-to-track moves. Without
the provider the legacy transition controls remain available. Providers, borrowed
views, clipboards and undo history are owned by the host.

任意の`TimelineEditingProvider`で独立したフェード、クリップ境界のトランジション、
矩形選択、検証付きトラック間移動を有効にする。未指定時は従来のtransition操作を使う。
provider、参照先データ、クリップボード、Undo履歴はホストが所有する。

## Input / 操作

- Drag either upper clip handle to create or resize a fade, including from zero.
  Right-click the handle for duration, Linear/Ease in/Ease out and removal.
- Drag Dissolve or Crossfade from `TransitionShelf` onto an adjacent cut. Drag the
  band to resize it; its context menu edits the duration or removes it. Dissolve
  targets video and Crossfade targets audio. The provider's media-handle limit
  bounds the initial one-second duration and subsequent edits.
  Video also supports Dip to black (`TransitionKind::Fade`) in the shelf and type menu.
- Drag empty timeline space to select intersecting clips. Shift adds; Ctrl toggles.
  Drag a selected clip to preserve the selection. The destination callback maps
  every member using the same track offset; `canMove` validates the complete set.
- Click track names to select rows. Shift adds and Ctrl toggles. Drag a selected
  name onto another name to insert the selected rows before it, in original order.
  The Tracks menu adds, duplicates and removes tracks. Nonempty removal is confirmed.
  The upper/lower half of a name accepts insertion before/after that row; the last
  row's lower half appends when the host supplies `trackAfter`.
- Ctrl+wheel zooms about the pointer; Shift+wheel scrolls horizontally; wheel scrolls
  vertically. +/- zoom about the view center. Fit, Fit selection and overview range
  handles provide explicit navigation. Active gestures scroll at the viewport edges.
- The Edit clips menu sends Copy/Cut/Paste requests. Paste uses the playhead and the
  active track, preserving relative time and track offsets. The host implements
  Insert or Overwrite and rejects incompatible destinations atomically.

左右上端のハンドルでゼロからフェードを作成・伸縮し、右クリックで長さ・曲線・削除を
指定する。効果一覧から隣接境界へドロップすると共有トランジションを作成する。
映像はDissolve、音声はCrossfadeを使い、初期1秒と編集後の長さを素材余白以内に制限する。
映像ではDip to black（`TransitionKind::Fade`）への種類変更もできる。

空白の矩形ドラッグでクリップを選択し、Shiftで追加、Ctrlで反転する。選択済みクリップ
のドラッグは選択を維持する。トラック名でも独立した複数選択ができ、ドラッグ先の直前へ
元の順序で挿入する。Tracksメニューで追加・複製・削除し、内容のある削除は確認する。
トラック名の上半分は直前、下半分は直後への挿入先となり、`trackAfter`を提供したホストでは
最終行の下半分へドロップして末尾へ移動できる。

Ctrl＋ホイールはポインター基準ズーム、Shift＋ホイールは横移動、通常ホイールは縦移動。
±、全体表示、選択範囲表示、overviewでも表示範囲を調整する。Edit clipsメニューの
コピー・切り取り・貼り付けはホストへ要求を送り、再生ヘッドと対象トラックを基準に
相対配置を保って挿入／上書きする。

## Event contract / イベント契約

`ClipFades` targets a clip: `first/last` are in/out durations in ticks; `x/y` are
`FadeCurve` values. `CutTransition` targets the left clip: `parent` is the right
clip, `first` is total duration and `x` is `TransitionKind`. Zero duration removes
the transition. A cut must remain adjacent, on the same unlocked track, with
enough source media on both sides. `EvaluateFade` evaluates clip-local fade gain;
it does not decode media or apply an audio/video effect.

`TrackEdit` uses `offset=TrackAction`, `parent=insertion-before track` (zero appends)
and `x=TrackKind` when adding. `Clipboard` uses `offset=ClipboardAction`,
`first=playhead`, `parent=destination track`, and `x=PlacementMode`.

`ClipFades`は対象クリップのin/out長を`first/last`、曲線を`x/y`へ格納する。
`CutTransition`は左クリップを対象とし、右IDを`parent`、全長を`first`、種類を`x`へ
格納する。長さゼロは削除。`EvaluateFade`はクリップ内時刻のフェード係数だけを返す。

`TrackEdit`は操作を`offset`、挿入先直前のIDを`parent`、追加する種類を`x`へ格納する。
`Clipboard`は操作を`offset`、再生ヘッドを`first`、対象トラックを`parent`、挿入／上書き
モードを`x`へ格納する。各操作のBegin/Update/Commit/Cancelとrevisionをホストで検証し、
複数対象を一括適用して1回のUndo単位にする。

Public structures gain appended fields. Rebuild consumers; binary compatibility
with previously compiled structures is not promised. Existing enum values and
function signatures retain their meanings. No backend or ImGui version changes.

公開構造体へ末尾フィールドを追加しているため利用側を再ビルドする。旧バイナリとの
構造体互換性は保証しない。既存enum値・関数署名の意味とbackend・ImGui版は維持する。

## Verification / 検証

Verified on Windows x64 in Debug: video public-IO tests, Editor Core, CG, icon and
API compile fixtures; an independent host-ImGui consumer compiled, linked and ran.
The Gallery's `--verify-timeline-model` and existing inspector model checks passed.
`--verify-timeline-ui` passed native OpenGL/public-IO fade creation, preview/commit,
Undo, multi-track selection and append/reorder. Its 100k-clip fixture retained
visible-row/clip queries (under 64 rows and 1000 clips per inspected frame).
All 238 icons passed six-size alpha/atlas consistency checks. Native dark/light
captures were inspected, including the new 3D set at 16, 20 and 24 pixels.

Windows x64 DebugでVideo公開IO、Editor Core、CG、アイコン、API compileを確認した。
独立したホストImGuiを使うconsumerもcompile/link/runに成功した。Galleryのタイムライン
モデル試験と既存Inspectorモデル試験が通過した。native OpenGL上でも公開IOによる
フェード作成・プレビュー・確定・Undo、複数トラック選択と末尾への並べ替えを確認した。
100kクリップ例は検査したフレームで64行未満・1000クリップ未満の可視範囲問い合わせを
維持した。238アイコンの全6サイズの整合性と、16/20/24pxを含むnative明暗captureを確認した。

Generated logs and captures live under `out/timeline-ui/`; they are not committed.
Release/distribution, actual media processing, native OS/IME and external application
integration were not run for this change.
ログ・captureは`out/timeline-ui/`に生成しコミットしない。今回のRelease・配布検証、実素材
処理、native OS/IME、他アプリ統合は未実施。

Implementation and verification results are recorded separately in the task's
completion report. Public IO checks do not establish native OS/IME, real-media
processing or integration in another application.

実装と実行した検証は完了報告で区別する。公開IO試験だけでnative OS/IME、実素材の
処理、他アプリへの組込みを検証済みとはしない。
