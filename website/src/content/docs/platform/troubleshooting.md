---
title: "トラブルシューティング"
description: "症状を探し、原因と対処を読みます。"
---

症状を探し、原因と対処を読みます。

## ビルドとリンク

| 症状 | 原因 | 対処 |
| --- | --- | --- |
| CMakeが`IMKIT_IMGUI_TARGET is required`で止まる | `add_subdirectory`の前に、Dear ImGuiのターゲットがないか、名前が違う | 先にDear ImGuiのCMakeターゲットを作り、その正確な名前を`IMKIT_IMGUI_TARGET`に設定します。ImKitは、組み込み先のプロジェクトに、2つ目のDear ImGuiを追加することはありません。 |
| ヘッダーが、Dear ImGuiのバージョンについてのエラーで止まる | Dear ImGuiのバージョン、またはドッキング、ビューポート、動的テクスチャへの対応が合っていない | 固定した`docking`のコミットを使います（[依存関係](../dependencies/)を参照）。ガードを外しても、スタイルのフィールドと公開シグネチャが違うので、ほかのリビジョンと互換にはなりません。 |
| CMakeが、SDKのABIの確認を求めて止まる | インストール済みのSDKは、`IMKIT_SDK_ABI_CONFIRMED=ON`が必要 | アーキテクチャ、MSVCのツールセット、DebugかReleaseのCRT、Dear ImGuiのリビジョン、コンパイル定義、`imconfig.h`を確認してから、フラグを設定します。1つでも違うときは、ソースからビルドします。 |
| Galleryのビルドが、`imkit_gallery.exe`を上書きできない | Galleryが実行中で、Windowsは、実行中の実行ファイルをリンカーが置き換えるのを許さない | Galleryを終了して、同じ差分ビルドをもう一度行います。 |

## 実行時

```cpp
// アサート: "Backend does not support ImGuiBackendFlags_RendererHasTextures, and font atlas is not built!"
```

| 症状 | 原因 | 対処 |
| --- | --- | --- |
| レンダラーのバックエンドがないプログラムで、`NewFrame`に上のアサートが出る | フォントアトラスを、だれも構築していない | `io.Fonts->GetTexDataAsRGBA32(...)`を1回呼びます。[最初のアプリを作る](../../tutorials/first-app/)のとおりです。実際のバックエンドは、これを代わりに行います。 |
| `ThemeScope`の破棄時にアサートが出る | スコープがContextより長く生きたか、別のContextで破棄された | すべてのスコープを、`DestroyContext()`の前に、作ったContextで終えます。 |
| テーマは正しいが、日本語が表示されない | テーマはフォントを読み込まないので、アトラスに日本語のグリフがない | フォントアトラスにグリフの範囲を加え、テキスト入力にIMEが必要なら、プラットフォームのIMEのコールバックも用意します。Galleryの代替フォントは、任意の素材で、ライブラリの機能ではありません。 |
| テーマを適用するたびに、サイズが大きくなる | `Theme::metrics`を自分で拡大し、さらにスケールも渡している | アプリのスケールは、`ApplyTheme(theme, scale)`か`ThemeScope(theme, scale)`だけに渡します。ImKitは、いつも、スケール前の値からスタイルを作り直します。 |
| `ImGuiStyle`に加えた変更が消える | `ApplyTheme`は、`Alpha`と`FontScaleDpi`を除いて、スタイル全体を置き換える | 変更は、[テーマ](../../features/themes/)の説明のとおり、`Theme`の側で行います。 |
| 部品は描かれるが、クリックしても何も起きない | 返された結果は要求で、アプリのコードが適用していない | 戻り値、または返された要求を読み、アプリ自身のデータを更新します。 |

## 結果の確かめ方

スクリーンショットだけでは、組み込みが動くことの証明になりません。[検証記録](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/reference/検証記録.md)は、コンパイルとリンクの確認、公開IOによる操作、GPUキャプチャ、インストール済みの利用側、ネイティブのアプリでの受け入れを分けているので、ある主張が、どれに基づくかを判断できます。
