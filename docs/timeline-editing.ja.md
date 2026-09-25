# Timeline editing / Timeline編集

[English](timeline-editing.md) · [Editor Suite](editor-suite.ja.md) · [Editor API](editor-api.ja.md)

任意の`TimelineEditingProvider`を使うと、独立fade、cut transition、矩形selection、検証付きtrack間移動が有効になります。未指定なら従来のtransition操作を利用します。provider、借用view、clipboard、Undo履歴はホスト所有です。公開APIは`<imkit/video.h>`と`imkit::video` targetから利用できます。

## 操作例

- clip上端のhandleをdragしてfadeを作成・resizeします。右clickで時間、Linear/Ease in/Ease out、削除を指定します。
- TransitionShelfからDissolveまたはCrossfadeを隣接cutへdragします。bandをresizeし、context menuで時間変更・削除を行います。Dissolveはvideo、Crossfadeはaudio対象です。VideoにはDip to blackもあります。
- 空白timelineをdragして交差clipを矩形選択し、Shiftで追加、Ctrlで反転します。複数選択をdragする場合、providerが全対象を同じtrack offsetで検証します。
- track名をclickして選択し、Shift/Ctrlで複数選択します。選択名を他の名前へdragすると元順で挿入します。Tracks menuから追加・複製・削除し、空でないtrack削除は確認します。

Galleryの **Video** page (`8`) を操作し、[`editor_workspaces.cpp`](../examples/gallery/editor_workspaces.cpp)のsample host実装を参照してください。より広いmodule recipeは[実例一覧](examples-recipes.ja.md)にあります。

### ホスト契約

media decode・playback・capture、clip data、selection、collision policy、revision、event buffer、Undo、保存、workerはホスト所有です。Providerは画面外対象や関連clipを含め、要求された完全な結果を返す必要があります。容量不足、lock、stale revisionを成功扱いせず、編集全体を検証してから適用してください。正確なoverloadとcallbackの寿命は[English API contract](timeline-editing.md)と[Editor API](editor-api.ja.md)を参照してください。

## Event contract / Event契約

編集要求はホスト提供bufferへ出力されます。容量、revision、lock、選択全体を検証してからmodelへ適用します。overflow時に部分成功を仮定しません。

## Verification / 検証

Galleryと自動testはsample provider、公開ImGui IO、合成データを対象にします。native OS入力、実media、利用host側の衝突・Undo統合は個別に受け入れてください。証拠baselineは[Validation](validation.ja.md)にあります。
