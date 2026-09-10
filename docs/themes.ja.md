# テーマとカスタマイズ

[English](themes.md)

## 名前付きpreset

`ThemePresets()`は固定順の12項目を返します。各`ThemePresetInfo`にはenum値、ホスト側保存用の安定した小文字ID、英語表示名、Light/Dark区分があります。

| Light | Dark |
|---|---|
| Precision Light、Warm Sand、Rose、Solar、High Contrast Light | Precision Dark、Graphite、Midnight、Ocean、Forest、Violet、High Contrast Dark |

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::Forest);
```

`MakePrecisionTheme(Light/Dark)`は互換維持され、従来と同じPrecision Light/Dark値を生成します。

## 配色変更

`SetAccent`はaccent、focus、on-accent文字、selectionを更新します。それ以外は`Theme::colors`、`metrics`、`motion`、`editor`を明示的に編集します。任意編集した色のコントラストは自動補正しません。

同梱presetは通常文字とcanvas・surface・input・raisedの間で4.5:1以上、muted文字で3:1以上、accentとdestructiveの前景組合せで4.5:1以上を検証します。

## 所有と保存

`Theme`はコピー可能なホスト所有値です。ImKitはcurrent-theme registryを持たず、ファイルへ保存しません。ホストの設定modelへpreset IDまたはカスタマイズ済み値を保存し、明示的に復元・再適用します。

`FontSet`は非所有参照です。frame開始前にホストのatlasへglyphを読み込んでください。OS font探索やIME callbackは提供しません。

## 倍率とscope

`ApplyTheme(theme, scale)`は未拡大metricsから毎回styleを作るため、繰り返し適用しても寸法は累積しません。`ThemeScope`は同じ生存中Context上で入れ子にでき、破棄時にstyleとfontを復元します。
