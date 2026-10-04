---
title: "Timeline編集"
---

## 用途

アプリ側のmedia modelに、track、clip、keyframe、編集操作を表示する場合に使います。

## Galleryの画面

![clip、track、編集操作を表示するTimeline画面。](../../../assets/captures/v3-timeline-poster.png)

Timeline — clip、track、編集操作 · [Galleryの操作映像を開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v3-timeline.gif)

## 最小描画例

```cpp
#include <imkit/video.h>
#include <imkit/editor_core.h>

void DrawTimeline(const imkit::video::TimelineProvider& provider,
                  imkit::video::TimelineState& state,
                  imkit::editor::Selection& selection,
                  imkit::editor::EventBuffer& events,
                  const imkit::Theme& theme) {
    imkit::video::Timeline("timeline", provider, state, selection, events, theme);
}
```

**例の種別:** アプリの状態を引数に取る関数例です。provider dataとevent buffer、編集反映はアプリ側が所有します。

## アプリへ組み込む

Begin/Update/Commit/Cancel eventをまとめて処理し、model変更前にrevisionとcollisionを検証します。

## 範囲

Timelineは編集UIです。再生engine、media decoder、project保存機能ではありません。

## 関連APIとガイド

- [Timeline操作契約](./)
- [Editor APIリファレンス](../../api/editor-suite/)

---

任意の`TimelineEditingProvider`を使うと、独立fade、cut transition、矩形selection、検証付きtrack間移動が有効になります。未指定なら従来のtransition操作を利用します。provider、借用view、clipboard、Undo履歴はホスト所有です。公開APIは`<imkit/video.h>`と`imkit::video` targetから利用できます。

## 操作例

- clip上端のhandleをdragしてfadeを作成・resizeします。右clickで時間、Linear/Ease in/Ease out、削除を指定します。
- TransitionShelfからDissolveまたはCrossfadeを隣接cutへdragします。bandをresizeし、context menuで時間変更・削除を行います。Dissolveはvideo、Crossfadeはaudio対象です。VideoにはDip to blackもあります。
- 空白timelineをdragして交差clipを矩形選択し、Shiftで追加、Ctrlで反転します。複数選択をdragする場合、providerが全対象を同じtrack offsetで検証します。
- track名をclickして選択し、Shift/Ctrlで複数選択します。選択名を他の名前へdragすると元順で挿入します。Tracks menuから追加・複製・削除し、空でないtrack削除は確認します。

Galleryの **Video** page (`8`) を操作し、[`editor_workspaces.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/editor_workspaces.cpp)のsample host実装を参照してください。より広いmodule recipeは[実例一覧](../../guides/examples/)にあります。

### ホスト契約

media decode・playback・capture、clip data、selection、collision policy、revision、event buffer、Undo、保存、workerはホスト所有です。Providerは画面外対象や関連clipを含め、要求された完全な結果を返す必要があります。容量不足、lock、stale revisionを成功扱いせず、編集全体を検証してから適用してください。正確なoverloadとcallbackの寿命は[English API contract](../../en/features/timeline/)と[Editor API](../../api/editor-suite/)を参照してください。

## Event contract / Event契約

編集要求はホスト提供bufferへ出力されます。容量、revision、lock、選択全体を検証してからmodelへ適用します。overflow時に部分成功を仮定しません。

## Verification / 検証

Galleryと自動testはsample provider、公開ImGui IO、合成データを対象にします。native OS入力、実media、利用host側の衝突・Undo統合は個別に受け入れてください。証拠baselineは[Validation](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/validation.ja.md)にあります。
