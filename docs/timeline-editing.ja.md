# Timeline編集

[English](timeline-editing.md)

Timeline の任意の `TimelineEditingProvider` が独立したクリップフェード、cut transition、矩形選択、検証付きのトラック間移動を有効にします。provider を指定しなければ、従来の transition コントロールも利用できます。provider、借用 view、クリップボード、Undo 履歴はホスト所有です。

## 入力

- クリップの上端ハンドルをドラッグすると、ゼロからフェードの作成・リサイズができます。ハンドルを右クリックすると、長さ、Linear/Ease in/Ease out、削除を指定できます。
- `TransitionShelf` から Dissolve または Crossfade を隣接 cut へドラッグします。band をドラッグしてリサイズし、context menu で長さを変更・削除できます。Dissolve は video、Crossfade は audio を対象にします。provider の media handle 制限が初期 1 秒の長さおよび以降の編集を上限にします。video は shelf と種類メニューで Dip to black（`TransitionKind::Fade`）も対応します。
- 空の timeline 領域をドラッグすると交差するクリップを選択できます。Shift で追加、Ctrl で切り替え。選択済みのクリップをドラッグすると選択集合を維持します。到達先 callback はすべてのメンバーを同じ track offset でマッピングし、`canMove` が完全集合を検証します。
- track 名をクリックして行を選択できます。Shift で追加、Ctrl で切り替え。選択中の名前を別の名前にドラッグすると、元の順序でその前の行に選択行を挿入します。Tracks メニューで track の追加・複製・削除ができます。中身がある削除は確認します。名の上半分・下半分は该行の前後への挿入受け入れで、ホストが `trackAfter` を提供する場合は最終行の下半分は末尾追加になります。
- Ctrl+wheel はポインタ基準でズーム、Shift+wheel は横方向スクロール、wheel は縦方向スクロール。+/− は表示中心でズーム。Fit、Fit selection、overview range のハンドルが明示的な導航を提供します。アクティブな gesture はビューポート端でスクロールします。
- Edit clips メニューは Copy/Cut/Paste を要求します。Paste は再生ヘッドとアクティブな track を基準に、相対時間と track offset を保持します。ホストは Insert または Overwrite を実装し、互換しない到達先は原子的に拒否します。

## イベント契約

`ClipFades` はクリップを対象にします：`first/last` は tick 単位の in/out 長さ、`x/y` は `FadeCurve` の値です。`CutTransition` は左クリップを対象にします：`parent` は右クリップ、`first` は全長、`x` は `TransitionKind`。長さがゼロなら transition を削除します。cut は隣接を維持し、同じ unlock された track 上で、両側の source media に十分な余地が必要です。`EvaluateFade` はクリップ局所のフェード利得を評価し、media のデコードや audio/video 効果の適用はしません。

`TrackEdit` は `offset=TrackAction`、`parent=挿入先より前の track`（ゼロは末尾追加）、追加時は `x=TrackKind` を使います。`Clipboard` は `offset=ClipboardAction`、`first=再生ヘッド`、`parent=到達先 track`、`x=PlacementMode` を使います。

公開構造体は末尾フィールドを追加します。利用側は再ビルドしてください。以前コンパイルされた構造体とのバイナリ互換は保証しません。既存の enum 値と関数シグネチャの意味は維持されます。backend や ImGui 版の変更はありません。

## 検証

Windows x64 の Debug で検証済みです：video 公開 IO テスト、Editor Core、CG、アイコン、API compile fixture。独立したホスト ImGui consumer もコンパイル・リンク・実行に成功しました。Gallery の `--verify-timeline-model` と既存の inspector モデル確認は合格です。`--verify-timeline-ui` は native OpenGL/公開 IO でのフェード作成、プレビュー/コミット、Undo、複数トラック選択、末尾追加/並べ替えに合格しました。100k クリップの fixture は可視行/クリップの問い合わせを維持（検査したフレームごとに 64 行以下・1000 クリップ以下）。238 アイコンが 6 サイズの alpha/atlas 整合性を合格しました。native の明暗 capture（新規 3D セットを含む 16/20/24 ピクセル）を目視確認しました。

生成されたログ・capture は `out/timeline-ui/` にあり、コミットしません。本次の変更に対して Release/配布、実素材処理、native OS/IME、他アプリ統合は実行していません。

実装と検証の結果はタスクの完了報告に別記しています。公開 IO 確認は native OS/IME、実素材処理、他アプリへの組込みを証明しません。
