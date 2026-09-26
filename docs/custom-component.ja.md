# カスタムコンポーネントの作成

[English](custom-component.md) · [文書索引](README.md) · [ImKit の仕組み](how-it-works.md)

ImKit のコンポーネントは Dear ImGui の挙動の上の小さな構成物です：`ThemeScope`、テーマからのセマンティックカラー、ホスト所有の状態、そして返される要求。この形に従えば、ライブラリに触れずに設計システムに合うコントロールを追加できます。このページはルールと、完全で最小のカスタムコントロール（"chip" トグル）を示します。

![Gallery でのテーマ付きパネル](images/gallery-overview.gif)

## コアパターン

すべてのカスタムコンポーネントは同じ 4 つの手順を踏みます：

1. **`ThemeScope` に包む。** コントロールはアクティブなテーマで描画され、ブロック終了時に前のコンテキストへ復元する。
2. **セマンティックカラーを読み、生の hex を使わない。** `theme.semantic.*` から色を取得すれば、ライト/ダーク、コントラスト、カスタムアクセントに自動追従する。
3. **状態はホストで所有する。** コントロールはあなたのモデルが保持する値を読み書きする。ImKit は保持しない。
4. **動くのではなく報告する。** 変更したら `true` を返す要求を返す。変更の意味はあなたのコードが決める。

## 作るもの

- 小さなトグルチップ：ラベル、on/off のドット、クリック。
- テーマを自動的に追従する `ThemeScope` とセマンティックカラー。
- ホストの `bool` とフォーカス用の `StableId`。
- 返されたトグル要求。

## 必要なもの

1. Dear ImGui コンテキストとウィンドウ。
2. `Theme`（`MakeTheme`/`MakePrecisionTheme` 由来）。
3. CMake で `imkit::imkit` ターゲットをリンク（オプションの a11y 手順には `accessibility` も）。

## 状態を所有する

```cpp
#include <imkit/imkit.h>
#include <imkit/accessibility.h>

struct ChipState {
    bool on = false;
    const char *label = "Notifications";
    imkit::StableId id = 1001;   // 0 以外：フォーカス + a11y の同一性
    bool disabled = false;
};
```

`id` はあなたが割り当てる 64 бит安定値。a11y 経路では `ImGui::GetID` を使わない（フレーム揺れに耐える必要がある）。コントロールの生存期間中維持する。

## テーマ化して描画する

コンポーネントは `ThemeScope` + セマンティックカラー + Dear ImGui プライマリです。色が `theme.semantic` から来るので、テーマが変わればチップは自動で再スケインします。

```cpp
bool DrawChip(const ChipState &s,
              const imkit::Theme &theme,
              imkit::ComponentOptions options = {}) {
    imkit::ThemeScope scope(theme);                 // 1. テーマ；終了時に復元

    // 2. セマンティックカラー（自動ライト/ダーク、コントラスト、アクセント）
    ImVec4 bg   = s.on ? theme.semantic.accent : theme.semantic.surfaceRaised;
    ImVec4 ink  = s.disabled ? theme.semantic.textDisabled : theme.semantic.text;
    float r      = theme.metrics.radius;
    ImVec2 size  = { 96.f, theme.metrics.controlHeight };   // テーマのメトリクスから

    bool pressed = !s.disabled && ImGui::InvisibleButton("##chip", size);
    if (pressed) s.on = !s.on;                              // 3. ホスト値が反転

    // 4. テーマカラーだけでピル + ラベルを描画
    ImGuiDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetItemRectMin();
    dl->AddRoundedRect(p, p + size, r,
                       (ImU32)ImGui::ColorImVec4(theme.semantic.border), bg);
    ImGui::SetCursorScreenPos(p + ImVec2(size.x * .36f, size.y * .5f));
    ImGui::PushStyleColorV3(ImGuiCol_Text, ink);
    ImGui::TextUnformatted(s.label);
    ImGui::PopStyleColorV3(1);
    // ドット: dl->AddCircle(p + ImVec2(size.x * .24f, size.y * .5f), 4.f,
    //                     s.on ? theme.semantic.onAccent : theme.semantic.border, 32);

    // 高度: フォーカス、a11y、ローカル化された文字列、モーションのため
    // ここに options.accessibility / options.parent / options.locale /
    // options.animation を読む。
    (void)options;

    return pressed;                    // 5. 報告；意味は呼び出側が決める
}
```

視覚詳細（ピル、ドット、ラベル）はあなたの自由です。重要なのは**すべての色がテーマから来**、**すべての状態がホストのもの**であることです。`theme.semantic.success` に "ready" チップ、`theme.semantic.error` に故障インディケターと置き換えられ、`ThemeScope` は周囲のコンテキストに触れない。

## 動くのではなく報告する

```cpp
// フレーム内で、NewFrame() の後:
if (DrawChip(chip, theme, { .disabledReason = "Locked" })) {
    chip.on ? PushEvent(chip.id, "on") : PushEvent(chip.id, "off"); // あなたのコード
}
```

返された `true` は「ユーザーがクリックした」を意味する。あなたのコードがその意味（永続化、トグル、イベント発火）を決めます。`ComponentOptions` struct はコントロールが必要な追加事項を運ぶ：`theme`、`animation`、`accessibility`、`parent` (StableId)、`locale`、`disabledReason`。

## オプション：アクセシビリティ注釈

大きなコントロールではスクリーンリーダやネイティブ a11y に公開します。一度 `AccessibilityFrame` を構築し、描画したアイテムに注釈を付けてください：

```cpp
imkit::accessibility::AccessibilityFrame frame(storage, actions);   // ホスト所有の storage/queue
frame.Begin(generation);
// ... コンポーネントを描画 ...
frame.Add({
    .id = s.id,
    .parent = parentId,
    .role = imkit::accessibility::SemanticRole::Toggle,
    .name = s.label,
    .value = s.on ? "on" : "off",
    .state = { .checked = s.on, .disabled = s.disabled, .focused = false },
    .actions = imkit::accessibility::SemanticAction::Press,
});
frame.Publish(sink);
```

ノードフィールドはホストがフレームを公開するまで借りられる。直ちに前の ImGui インタムに添付するなら `AnnotateLastItem` を使う。これにより、キーボードフォーカス、スクリーンリーダ、高コントラストモードにカスタムコントロールが参加できます。

## 実行する

- `DrawChip` を Gallery や自分のアプリに落として、フレーム内で呼び出せば、アクティブな `Theme` に自動テーマ化される。
- `tests/design_system.cpp` 風のフィクスチャでは、チップの色が `ThemePresets()` に追従し、`ThemeScope` が前のコンテキストを復元するのをアサートできる。

## 正しく保つためのルール

- **必ず `ThemeScope`。** コンポーネントはアクティブなテーマの外に出ない。
- **セマンティックカラーのみ。** コントロールに `ImVec4{...}` の hex 定数を埋めない。`theme.semantic.*` を読む。
- **状態はホスト所有。** コントロールはあなたの値を書き、要求を返す。永続化、アロケーション、フレーム横断のポインタ保持はしない。
- **安定な `id`。** フォーカスと a11y に 0 以外の `StableId` を使う。`ImGui::GetID` の値を再利用しない。
- **`disabled` でゲートする。** `disabled` がセットされた場合、入力をスキップし、ユーザーに `disabledReason` を表示する。
- **すべてのスコープに対応させる。** `ThemeScope` は RAII ブロック。`Begin`/`End` ペアは false を返しても閉じる。

## 次に読む

- [コンポーネントとレシピ（各コントロール参照）](components.md)
- [デザインシステム](design-system.md)
- [アーキテクチャ](architecture.md)
