---
title: "テーマ"
description: "ImKitには、名前の付いた13個のプリセットと、それを調整したり、独自に作ったりする関数があります。Themeは、アプリが持つただの値で、ApplyThemeが、現在のDear ImGuiのContextに書き込みます。"
---

ImKitには、名前の付いた13個のプリセットと、それを調整したり、独自に作ったりする関数があります。`Theme`は、アプリが持つただの値で、`ApplyTheme`が、現在のDear ImGuiのContextに書き込みます。

![Galleryでテーマを選ぶと、すべての部品の配色が変わる](/imgui-modern-kit/docs-images/v4-themes-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-themes.gif)

## 内蔵のプリセット

`ThemePresets()`は、プリセットを、安定した表示順で返します。各`ThemePresetInfo`は、enumの値、保存用の小文字の安定ID（例: `"precision-dark"`）、英語の表示名、LightかDarkかの区分を持ちます。

| 区分 | プリセット | 特徴 |
| --- | --- | --- |
| Dark | `PrecisionDark` | 標準のダークテーマ。落ち着いたグレーに、青のアクセント |
| | `Graphite` | 黒に近い深いグレーに、ミントのアクセント |
| | `Midnight` | 深いネイビーに、ラベンダーブルーのアクセント |
| | `Ocean` | 暗い青緑に、シアンのアクセント |
| | `Forest` | 深い緑に、緑のアクセント |
| | `Violet` | 濃い紫に、紫のアクセント |
| | `Slate` | 青みがかったグレーの背景と面 |
| | `HighContrastDark` | 見やすさのために、コントラストを高めたもの |
| Light | `PrecisionLight` | 標準のライトテーマ。白と淡いグレー |
| | `WarmSand` | 砂のようなベージュに、茶色のアクセント |
| | `Rose` | 淡いピンクに、ローズのアクセント |
| | `Solar` | 暖かいクリーム色に、オリーブゴールドのアクセント |
| | `HighContrastLight` | 見やすさのために、コントラストを高めたもの |

`PrecisionLight`、`PrecisionDark`、`Slate`、高コントラストの2種は、手で調整した配色です。残りの8種は、背景、面、文字、アクセントの4色から、`MakeBrandTheme`で作っています。同じ関数でアプリに何ができるかの例にもなります（[自分の世界観を作る](../make-your-own-look/)を参照）。

## テーマを選び、復元する

保存するのは、enumの値ではなく、安定したIDです。復元には、`ThemePresetFromId`を使います。未知のIDや、大文字小文字が違うIDには、`std::nullopt`を返すので、代わりの扱いは、アプリが決められます。

```cpp
#include <imkit/theme.h>

// プリセットから
imkit::Theme theme = imkit::MakeTheme(imkit::ThemePreset::Forest);

// 保存したIDから。見つからなければ代わりを使う
const auto preset = imkit::ThemePresetFromId(savedPresetId)
    .value_or(imkit::ThemePreset::PrecisionDark);
imkit::Theme current = imkit::MakeTheme(preset);
```

`MakePrecisionTheme(Light/Dark)`は、ソース互換のために残してあり、同じPrecisionの値を返します。

## 調整する

```cpp
// アクセント色を変える。ボタン、フォーカスの枠、選択が連動する。
imkit::SetAccent(theme, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
```

| フィールド | 役割 |
| --- | --- |
| `semantic`、`typography`、`radius`、`stroke`、`elevation`、`opacity` | 正となる値です。これを編集し、`ResolveTheme(theme)`を呼びます。 |
| `colors`、`metrics` | `ResolveTheme`と`SetDensity`が作り直す派生値です。直接編集しても、上書きされます。 |
| `layout` | 任意の寸法の上書きです。コントロールの高さ、間隔、余白、Checkbox、Toggle、バッジ、ダイアログの比率が対象です。密度のあとに適用します。未指定の項目は、密度の値のままなので、何も設定しないテーマの見た目は変わりません。 |
| `motion`、`editor` | 必要なときに、明示的に編集します。 |

`SetAccent`は、アクセント、フォーカスの色、アクセント上の文字色、選択を更新します。任意の編集に対して、コントラストを自動で補正することはありません。

同梱のプリセットは、通常の文字を、背景、面、入力、浮いた面に対して4.5:1以上に、補助の文字を3:1以上に、アクセントと破壊的操作の前景の組を4.5:1以上に保っています。独自のテーマは、`ValidateContrast`で確認してください。

## `ApplyTheme`がすること

`ApplyTheme`は、現在のContextの`ImGuiStyle`を、`Alpha`と`FontScaleDpi`を除いて、すべて置き換えます。調整は、`ImGuiStyle`ではなく`Theme`で行ってください。`ImGuiStyle`を直接編集しても、次の`ApplyTheme`で上書きされるためです。

| 項目 | 規則 |
| --- | --- |
| スケール | `ApplyTheme(theme, scale)`は、スケール前の値からスタイルを作り直すので、繰り返し呼んでも、サイズは累積しません。範囲は50%から250%で、既定は125%です（`ThemeScaleMinimum`、`ThemeScaleDefault`、`ThemeScaleMaximum`）。モニターのDPIへの対応は、アプリの役目です。 |
| スコープ | `ThemeScope`は、ブロックの間だけテーマを適用し、終わると、前のスタイルとフォントに戻します。同じ有効なContext上で、入れ子にできます。 |
| フォント | `FontSet`は、借用ポインターを持ちます。フレームの前に、グリフをアトラスへ読み込みます。ImKitは、OSのフォントを探さず、IMEのコールバックも提供しません。 |
| 所有 | `Theme`は、コピーできる値です。ImKitは、現在のテーマの登録を持たず、ファイルも書きません。プリセットのID、または調整済みの値全体を、アプリ自身の設定に保存し、必要なときに適用し直します。 |
