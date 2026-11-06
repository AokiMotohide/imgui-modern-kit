---
title: "カスタムコンポーネントを作る"
description: "ImKitの部品は、小さな部品の組み合わせでできています。操作にはDear ImGuiの項目、色と大きさにはテーマのトークン、呼び出し側が持つ値、そして返り値です。同じ作りで書いた部品は、ライブラリを変えなくても、テーマに従います。"
---

ImKitの部品は、小さな部品の組み合わせでできています。操作にはDear ImGuiの項目、色と大きさにはテーマのトークン、呼び出し側が持つ値、そして返り値です。同じ作りで書いた部品は、ライブラリを変えなくても、テーマに従います。このチュートリアルでは、丸いチップ形のトグル`ChipToggle`を作ります。

![Galleryでテーマを選ぶと、意味の色を読むすべての部品の配色が変わる](/imgui-modern-kit/docs-images/v4-themes-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-themes.gif)

## 基本の形

1. **操作はDear ImGuiに任せる。** `InvisibleButton`のような項目を使います。ホバー、フォーカス、キーボード操作、ID、無効の状態は、標準のまま残ります。
2. **色は`theme.semantic`から読む。** ライトとダークのテーマ、コントラスト、独自のアクセント色に、部品が従います。大きさは、リテラルでなく、テーマが決めるフレームの高さから取ります。
3. **状態は、呼び出し側が持つ。** 部品は、ポインターで値を編集し、保存しません。
4. **結果を返す。** ユーザーが値を変えたときにtrueを返します。その意味は、呼び出し側が決めます。

## 始める前に

Dear ImGuiのContextとウィンドウ、`MakeTheme`で作った`Theme`、リンクした`imkit::imkit`が必要です。

## 手順1: 部品を書く

```cpp
#include <imgui.h>
#include <imkit/imkit.h>

namespace a11y = imkit::accessibility;

// 丸いチップ形のトグル。*onを編集し、ユーザーが変えたフレームでtrueを返す。
bool ChipToggle(const char* label, bool* on, const imkit::Theme& theme,
                a11y::AccessibilityFrame* access = nullptr, a11y::StableId id = 0) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const imkit::SemanticColors& color = theme.semantic;

    // 大きさは、テーマが決めるフレームの高さから求め、密度とスケールに従わせる。
    const float height = ImGui::GetFrameHeight();
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 size{height + textSize.x + style.FramePadding.x, height};

    // 当たり判定、ホバー、フォーカス、無効の動作は、Dear ImGuiに任せる。
    const bool pressed = ImGui::InvisibleButton(label, size);
    if (pressed) *on = !*on;

    if (access) {                                    // 任意: 支援技術に、この項目の内容を伝える
        a11y::SemanticNode node;
        node.id = id;
        node.role = a11y::SemanticRole::Toggle;
        node.name = label;
        node.state.checked = *on;
        node.actions = a11y::SemanticAction::Toggle;
        a11y::AnnotateLastItem(*access, node);
    }

    // 色は、意味トークンだけから取る。
    const ImVec2 min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    const ImVec4 fill = *on ? color.accent : (ImGui::IsItemHovered() ? color.surfaceRaised : color.surface);
    const ImVec4 ink = *on ? color.onAccent : color.text;
    const float round = height * 0.5f;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, ImGui::GetColorU32(fill), round);
    draw->AddRect(min, max, ImGui::GetColorU32(color.border), round);
    draw->AddCircleFilled({min.x + round, min.y + round}, height * 0.14f, ImGui::GetColorU32(ink));
    draw->AddText({min.x + height, min.y + (height - textSize.y) * 0.5f}, ImGui::GetColorU32(ink), label);
    if (ImGui::IsItemFocused())
        draw->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_NavCursor), round, ImDrawFlags_None, 2.0f);
    return pressed;
}
```

| 部分 | 理由 |
| --- | --- |
| `ImGui::GetFrameHeight()` | 高さが、テーマの密度とアプリのスケールに従います |
| `ImGui::InvisibleButton(label, size)` | 項目にID、当たり判定、ホバー、フォーカス、無効の動作を与えます |
| `color.accent`、`onAccent`、`surface`、`surfaceRaised`、`text`、`border` | 意味の色です。テーマを変えると、チップの色も変わります |
| `ImGui::GetColorU32(ImVec4)` | スタイルのアルファを反映し、`BeginDisabled`の中では、チップが暗くなります |
| `ImGui::IsItemFocused()` | キーボードとゲームパッドの操作用に、フォーカスの枠を描きます |

ラベルに`##`を含めてはいけません。IDと違う文字を表示したいときは、呼び出しの前に、表示する文字を自分で組み立てます。

## 手順2: 使う

```cpp
    bool notifications = true;   // アプリが持つ値

    // フレームの中で
    if (ChipToggle("Notifications", &notifications, theme)) {
        SaveNotificationSetting(notifications);   // 意味を決めるのは、アプリのコード
    }
```

1つのパネルだけ別のテーマにするときは、呼び出しを`imkit::ThemeScope`で囲みます。無効にするときは、チップの前に`imkit::BeginDisabled(true)`を呼びます。すると、チップは入力を受け付けず、暗く描かれます。

## 手順3: 支援技術に伝える（任意）

`AnnotateLastItem`は、描いたばかりの項目に、意味のノードを付けます。新しい項目は追加しません。ノードの保管場所はアプリが持ち、フレームをプラットフォームの橋渡しへ公開するのも、アプリです。

```cpp
std::array<imkit::accessibility::SemanticNode, 64> storage;   // アプリが持つ
imkit::accessibility::AccessibilityFrame access(storage);

access.Begin(generation);                                    // 毎フレーム1回、描画の前に
ChipToggle("Notifications", &notifications, theme, &access, 1001);
access.Publish(sink);                                        // 描画のあと、プラットフォームの橋渡しへ
```

各部品には、自分で決めた、0でない`StableId`を付けます。IDがフレームをまたいで変わらないようにするため、`ImGui::GetID`は使いません。ノードの文字は、フレームを公開するまで借用です。

## 守る規則

- 色は`theme.semantic`から、大きさはテーマのフレームから取ります。部品の中に、色のリテラルを書きません。
- 操作はDear ImGuiの項目に任せ、フォーカス、ID、`Begin`/`End`の対応、無効の状態を、標準のまま保ちます。
- 値は、呼び出し側が持ちます。部品は、値の保存、メモリの確保、フレームをまたぐポインターの保持をしません。
- 結果を返し、動作は呼び出し側に任せます。
- アクセシビリティには、変わらない、0でない`StableId`を使います。

## 次に読む

- [基本コンポーネント](../../features/components/): ライブラリ自身の部品です。
- [デザインシステム](../../features/design-system/): トークンの詳細です。
- [アーキテクチャ](../../guide/architecture/)
