---
title: "v3からの移行"
---

**対象:** ImKit 3.xをリンクしているすべてのアプリです。ソースの修正は少なく済みます。ただし、再ビルドは必須で、旧い値を自分で指定しない限り、既定の見た目が変わります。

## 4つの手順

| 手順 | すること | 理由 |
| --- | --- | --- |
| 1 | アプリと、リンクする**すべての**ImKit静的ライブラリを、現在の4.xのヘッダーで再ビルドする。`find_package(imkit 4.1 CONFIG REQUIRED)` を使う。 | 公開構造体がいくつか拡張されたため。3.xのオブジェクトと4.0の構造体を混ぜると、配置の違いで誤読する。Dear ImGuiのrevisionは変わらない。 |
| 2 | 新しい既定値で、アプリを一度見る。そのまま使うか、下の表で旧い値に戻す。 | 角丸、影、動き、コントロールの高さが変わったため。 |
| 3 | コンパイラの警告が出たら、非推奨の綴りを置き換える。 | 4.0でもコンパイルできるが、`[[deprecated]]` が付いたため。 |
| 4 | `imkit::Combo`、`BeginCombo`、`RadioButton`、`Slider*` 系を呼んでいるなら、動作を確かめ直す。 | `ImGui::` への転送ではなく、ImKitのラッパーになったため。 |

## 既定のトークンの変更

| トークン | 3.3.0 | 4.0.0 |
| --- | --- | --- |
| `Radius` 小・コントロール・オーバーレイ | 2 / 4 / 6 | 4 / 8 / 12 |
| `Elevation` raised / overlay | 2 / 5 | 4 / 14（`offset`と`hard`を追加） |
| `Opacity::disabled` | 1 | 0.55 |
| `Metrics::controlHeight`（Comfortable） | 28 | 30 |
| `Metrics::horizontalPadding`（Comfortable） | 10 | 12 |
| `Motion` コントロール・オーバーレイの秒数 | 0.06 / 0.10 | 0.14 / 0.20 |

3.3の見た目を保つには、テーマを作ったあとで旧い値を代入します。`ResolveTheme` は密度からmetricsを計算し直すので、metricsは最後に代入してください。

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
theme.radius = {2, 4, 6};
theme.elevation.raised = 2;
theme.elevation.overlay = 5;
theme.opacity.disabled = 1.f;
theme.motion.controlSeconds = 0.06f;
theme.motion.overlaySeconds = 0.10f;
theme.metrics.controlHeight = 28;
theme.metrics.horizontalPadding = 10;
imkit::ApplyTheme(theme, 1.0f);
```

色つきのプリセット（Graphite、Midnight、Ocean、Forest、Violet、Warm Sand、Rose、Solar）は、4つのブランド色から作る方式に変わりました。そのため、導かれる色は3.xと少し違います。Precision LightとPrecision Darkは、従来のパレットのままです。

## 非推奨になった綴り

| 非推奨 | 置き換え |
| --- | --- |
| `StatusBadge(text, kind, const Theme*)` | `StatusBadge(text, kind, ComponentOptions{&theme})` |
| `NotificationCard(n, now, const Theme*)` | `NotificationCard(n, now, ComponentOptions{&theme})` |
| `ValidationMessage(message, invalid, const Theme*)` | `ValidationMessage(message, invalid, ComponentOptions{&theme})` |
| `EmptyState(message)` | `EmptyState(id, StateView, options)` |
| `Progress(id, fraction, label)` | `Progress(id, ProgressView, ProgressPresentation, ...)` または `ProgressTrack` |
| `ToastRegion(span<Notification>)` | `ToastViewport` |

移行の間は、`IMKIT_DEPRECATED(message)` を空に定義すると、警告を消せます。

## そのほかの動作の変更

- `SemanticRole` に `Slider` と `Meter` が加わりました。`RangeSlider` は、支援技術に `Group` ではなく `Slider` ノードを公開します。
- シェブロン付きのコンボは、項目を編集済みとして記録しません。これまでどおり、戻り値を使ってください。
- `PtzState` の公開フィールドは `speed` だけです。
- `IMKIT_VERSION` は `4.0.0` を返します。3.1〜3.3では `3.0.0` のままでした。

## 確かめ方

- オブジェクトとヘッダーの版の不一致が出ず、`find_package(imkit 4.1)` が解決する。
- `imkit_gallery` の **Before / After** を、自分のアプリの画面と並べて見比べる。
- 自分のテストを非推奨警告つきで実行し、警告を1つずつ修正に変える。

## 4.0から4.1へ

4.1では、公開構造体にメンバーが加わり、ノードエディタの既定の見た目が変わります。ほかの既定値は変わりません。コントロールの寸法、色、動き、Showモジュールは4.0と同じに描かれ、新しいフィールドの既定値は、従来の動作のままです。

| 手順 | すること | 理由 |
| --- | --- | --- |
| 1 | アプリと、リンクする**すべての**ImKitスタティックライブラリを、4.1のヘッダーで再ビルドします。`find_package(imkit 4.1 CONFIG REQUIRED)` を指定してください。 | `Theme`、`ThemeScope`、`Metrics`、`PtzView`、`ProgressTrackView`、ノードエディタの構造体は、末尾にメンバーが加わりました。4.0でビルドしたオブジェクトと4.1の構造体を混ぜると、配置が合いません。`4.0` を指定した要求は、4.1にはマッチしなくなります。 |
| 2 | ノードエディタを使っているなら、一度見た目を確かめます。新しい見た目を使うか、下の値で元に戻します。 | 既定の見た目が変わりました。 |

ノードエディタの既定値の変更:

| 4.1の既定 | 4.0に戻すには |
| --- | --- |
| 配線は出力ソケットの型の色。型の色がなければアクセント | `style.linkColor = LinkColorSource::Accent` |
| 時間も進捗もない `Idle` のノードでは、ステータス行を隠す | `style.status = StatusDisplay::Always` |
| `NodeKind::Reroute` は16単位のノットで、2つのソケットを中心に置く | なし |
| 未接続の必須入力に印を付ける | なし |
| 配線のラベルはピルの中に描き、リサイズのつまみはズームに合わせて大きさが変わる | なし |

全体の一覧は、[変更履歴](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/CHANGELOG.md)と[4.1追加API](../../api/v4-1/)にあります。
