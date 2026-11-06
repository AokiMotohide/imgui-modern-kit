# ImKit

**Dear ImGuiのまま、現代的なUIへ。使い方は、いつもどおり。**

ImKitは、既存のDear ImGuiアプリに、現代的な見た目と使いやすい部品を足すC++20の静的ライブラリです。テーマを1つ足すだけで始められ、フレームのループは変えません。必要なモジュールだけをリンクできます。

[動画を見る](https://aokimotohide.github.io/imgui-modern-kit/#film) · [ダウンロード](https://github.com/AokiMotohide/imgui-modern-kit/releases/latest) · [マニュアル](https://aokimotohide.github.io/imgui-modern-kit/) · [API参照](https://aokimotohide.github.io/imgui-modern-kit/api/) · [English](README.md)

[![ImKitの紹介動画: テーマの前後で同じアプリが変わる](website/public/media/imkit-poster.jpg)](https://aokimotohide.github.io/imgui-modern-kit/#film)

## ImKitでできること

- **見た目だけが新しくなる。** テーマを足すと、角丸、やわらかい影、なめらかな動きが入ります。呼び出しの形も、値の持ち方もそのままです。
- **自分の世界観にできる。** 4色からテーマを作り、角、枠線、影を足せます。部品の枠を自作のスキンに差し替えることもできます。テーマは13種を内蔵しています。
- **部品が揃っている。** 作業画面、タイムライン、ノードエディタ、グラフ、PTZパッド、321個のアイコン。メディアサーバー向けの任意モジュール `show` もあります。
- **アプリの作りは変わらない。** Dear ImGuiのコンテキスト、バックエンド、レンダラー、フォント、データ、Undo履歴、保存処理は、これまでどおりアプリが持ちます。

<img src="docs/images/v4-start.gif" alt="同じ部品が、ImKitのテーマで見た目だけ変わり、続けてGallery全体の配色が切り替わる" width="960">

## Galleryで試す

お使いの環境向けの[v4.1.0パッケージ](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v4.1.0)をダウンロードして展開し、`bin/imkit_gallery.exe`（Windows）または `imkit_gallery.app`（macOS）を起動してください。Node Editor専用のGalleryも入っています。Galleryは、アプリがリンクするのと同じ公開部品だけで作ってあります。

Windowsでソースからビルドする場合は、次のとおりです。

```
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

## 既存アプリへ組み込む

アプリがすでに使っているDear ImGuiのターゲットを、ImKitに教えます。

```cmake
set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

フレームを始める前にテーマを適用し、ふつうのウィンドウの中で部品を呼びます。

```cpp
#include <imkit/imkit.h>

auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
imkit::ApplyTheme(theme, 1.0f); // ImGui::NewFrame() より前

if (imkit::Begin("Display")) {
    static bool enabled = true;
    imkit::Toggle("Enabled", &enabled);
    imkit::ActionButton("Apply", imkit::ActionVariant::Primary);
}
imkit::End();
```

## 作れるもの

| 分野 | 使える部品 | CMakeターゲット | Galleryの画面 |
| --- | --- | --- | --- |
| 見た目 | テーマ、ブランド色、スキン、321個のアイコン | `imkit::imkit` | Theme Studio、Skins、Icons |
| 基本部品 | ボタン、トグル、スライダー、入力、表、トースト、進捗 | `imkit::imkit` | Controls |
| データ | ドーナツ・棒・ゲージ・スパークラインのグラフ、PTZ操作 | `imkit::imkit` | Data |
| 作業画面 | タイムライン、曲線、インスペクター、階層、プレビューモニター | `imkit::editor_suite` | Workspaces |
| ノードグラフ | キャンバス、ソケット、ミニマップ、編集要求 | `imkit::node_editor` | Node Editor Gallery |
| ショー | キューリスト、ミキサー、コントロールサーフェス、LED、スケジュール | `imkit::show` | Show |
| ネイティブ連携 | ウィンドウ枠、OpenGL／Metalのプレビュー、アクセシビリティ連携 | `imkit::window_frame_*`、`imkit::preview_*` | Window frame |

分野ごとの公開ヘッダー、ターゲット、フレーム内で呼ぶ位置は、[実例とレシピ](docs/getting-started/実例とレシピ.md)にまとめてあります。

## 動きを見る

| | |
| --- | --- |
| <img src="docs/images/v4-themes.gif" alt="テーマを選ぶと、Gallery全体の配色が変わる" width="470"><br>テーマを選ぶと、画面全体が着替えます。 | <img src="docs/images/v4-studio.gif" alt="Theme Studioでblueprint、pastel-pop、stickerの見た目を読み込む" width="470"><br>見た目を丸ごと読み込み、角と影も変えられます。 |
| <img src="docs/images/v4-workspace.gif" alt="タブ、階層、設定、選択部品を1画面にまとめた作業画面" width="470"><br>タブ、階層、設定を1画面に。 | <img src="docs/images/v4-show.gif" alt="キューリストとクリップランチャーでライブ演出を動かす" width="470"><br>GOでライブ演出を動かします。 |

## 4.1の新機能

- ノードエディタを作り直しました。ヘッダーの塗り、配線の色、ソケットの記号、ノードの形と状態、プリセット、描画フック、配線中のフィードバック、ソケットのAltクリックとCtrlドラッグが入ります。
- `show::TileMap`。LEDウォールのようなタイル状の表示装置を、ポートごとに色分けし、配線の順序とともに描きます。
- `Theme::layout` で、コントロールの寸法を調整できます。
- スプライトシート（`Sprite`）と、進捗の塗りに乗る絵（`ProgressTrackView::thumb`）。
- カメラが持つ機能だけを描くPTZ操作（`PtzView::capabilities`）。

| | |
| --- | --- |
| <img src="docs/images/v4-node-editor.gif" alt="ノードエディタで配線をドラッグする: 型で色分けした配線、記号つきのソケット、ノードの状態" width="470"><br>ひと目で読めるノードエディタ。 | <img src="docs/images/v4-tilemap.gif" alt="ポートごとに色分けしたLEDウォールのタイルマップ。ケーブルの順序とツールチップ" width="470"><br>LEDウォールを、ポートごとに色分けしたタイルマップで。 |
| <img src="docs/images/v4-sprites.gif" alt="スプライトシートの再生と、進捗バーに乗る絵" width="470"><br>スプライトと、進捗バーに乗る絵。 | |

アプリと、リンクするすべてのImKitライブラリを、4.1のヘッダーで再ビルドしてください。既定の見た目が変わるのはノードエディタだけです。[移行ガイド](docs/reference/v4移行ガイド.md#40から41へ)、[4.1追加API](docs/reference/4.1追加API.md)、[変更履歴](CHANGELOG.md)を確認してください。4.0では、刷新した既定の見た目、ブランドテーマ、グラフ、PTZ、範囲スライダー、`show` モジュールが加わりました。

## 互換性

v4のABIは、Dear ImGui 1.93.0 WIP docking（コミット `367b2c24f399988ddafc0bb4628da0106bcc09be`）を対象にしています。リリースパッケージは、Windows x64とArm64、macOS arm64、x86_64、Universal 2です。macOS版は署名も公証もしていません。自動検査では、実機の入力、ネイティブIME、スクリーンリーダー、複数DPI、他社ホストでの動作は確かめていません。範囲は[検証の範囲](docs/reference/検証記録.md)と[依存関係](docs/architecture/依存関係.md)を参照してください。

## 文書

[文書一覧](docs/目次.md) · [導入ガイド](docs/getting-started/導入ガイド.md) · [部品の使い方](docs/getting-started/利用ガイド.md) · [Galleryガイド](docs/getting-started/ギャラリーガイド.md) · [ノードエディタ](docs/components/ノードエディタ.md) · [アーキテクチャ](docs/architecture/アーキテクチャ.md) · [変更履歴](CHANGELOG.md)

## ライセンス

ImKitはMITライセンスです。Dear ImGui、GLFW、任意のフォントは、それぞれのライセンスを保ちます。動画の音楽、Kevin MacLeod ft. DarkNES「Laserpack」（[incompetech.com](https://incompetech.com)）は、[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)です。リビジョン、ハッシュ、通知は[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)にあります。
