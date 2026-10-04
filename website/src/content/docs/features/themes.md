---
title: "テーマ"
---

## 用途

テーマpresetを選び、必要に応じて色や寸法を調整します。Themeは値のcopyで、所有権はアプリ側にあります。

## Galleryの画面

![Gallery内でImKitのテーマを比較する画面。](../../../assets/captures/v3-theme-comparison-poster.png)

Themes — ライト・ダークの配色 · [Galleryの操作映像を開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v3-theme-comparison.gif)

## 最小描画例

```cpp
#include <imkit/theme.h>

imkit::Theme MakeForestTheme() {
    auto theme = imkit::MakeTheme(imkit::ThemePreset::Forest);
    imkit::SetAccent(theme, {0.20f, 0.62f, 0.46f, 1.0f});
    return theme;
}
```

**例の種別:** 完結した関数例です。返されたTheme値はアプリ側で保持し、有効なContext上で`NewFrame`より前に適用します。

## アプリへ組み込む

戻り値はアプリ側で保持し、有効なContext上で`NewFrame`より前に`ApplyTheme(theme, scale)`を呼びます。フォントはアプリ側のatlasへ読み込み、描画で参照する間有効に保ちます。atlasとContextの寿命もアプリ側が管理します。

## 範囲

Themeは外観の値を適用します。Context生成やOS font読み込みは行わず、任意の色変更でcontrastを保証しません。

## 関連APIとガイド

- [Design-system header一覧](../../api/design-system/)
- [Native API一覧](../../api/native/)

---

## 名前付きpreset

`ThemePresets()`は固定順の12項目を返します。各`ThemePresetInfo`にはenum値、ホスト側保存用の安定した小文字ID、英語表示名、Light/Dark区分があります。

| Light | Dark |
|---|---|
| Precision Light、Warm Sand、Rose、Solar、High Contrast Light | Precision Dark、Graphite、Midnight、Ocean、Forest、Violet、High Contrast Dark |

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::Forest);
```

enum順ではなく安定IDを保存し、`ThemePresetFromId()`で復元します。未知IDや大文字小文字が異なるIDは`std::nullopt`となるため、移行とfallbackはホスト側で明示できます。

```cpp
const auto preset = imkit::ThemePresetFromId(savedPresetId)
    .value_or(imkit::ThemePreset::Graphite);
auto theme = imkit::MakeTheme(preset);
```

`MakePrecisionTheme(Light/Dark)`は互換維持され、従来と同じPrecision Light/Dark値を生成します。

## 配色変更

`SetAccent`はaccent、focus、on-accent文字、selectionを更新します。それ以外は`Theme::colors`、`metrics`、`motion`、`editor`を明示的に編集します。任意編集した色のコントラストは自動補正しません。

同梱presetは通常文字とcanvas・surface・input・raisedの間で4.5:1以上、muted文字で3:1以上、accentとdestructiveの前景組合せで4.5:1以上を検証します。

## 所有と保存

`Theme`はコピー可能なホスト所有値です。ImKitはcurrent-theme registryを持たず、ファイルへ保存しません。ホストの設定modelへpreset IDまたはカスタマイズ済み値を保存し、明示的に復元・再適用します。

固定Git submoduleを通常利用しつつホストとImKitを並行開発する場合は、ホスト所有のCMake cache pathで`add_subdirectory`の入力だけを明示的に差し替えます。この絶対pathをproject fileや配布manifestへ保存せず、どちらの経路でもImKit追加前に`IMKIT_IMGUI_TARGET`を設定してください。

`FontSet`は非所有参照です。frame開始前にホストのatlasへglyphを読み込んでください。OS font探索やIME callbackは提供しません。

## 倍率とscope

`ApplyTheme(theme, scale)`は未拡大metricsから毎回styleを作るため、繰り返し適用しても寸法は累積しません。`ThemeScope`は同じ生存中Context上で入れ子にでき、破棄時にstyleとfontを復元します。

共通のアプリ倍率は50%～250%、既定値は125%です。ホストは`ThemeScaleMinimum`、`ThemeScaleDefault`、`ThemeScaleMaximum`で同じ値を参照できます。`ApplyTheme`または`ThemeScope`で倍率を省略すると125%を使い、monitor DPIへの追従は引き続きホストが担います。
