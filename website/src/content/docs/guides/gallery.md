---
title: "Galleryガイド"
---

ImKit Galleryは、導入する前にImKitを試せるデスクトップアプリです。アプリが組み込むのと同じ公開ライブラリで作ってあり、画面に出る部品に、デモ専用のものはありません。

**この文書の読み方:** 初めて使うときは「[最初の3分](#最初の3分)」と「[ビルドと起動](#ビルドと起動)」だけで足ります。画面一覧と撮影の節は、必要になったときに引いてください。

## 最初の3分

| 手順 | 操作 | 見えるもの |
| --- | --- | --- |
| 1 | **Start**を開き、表示設定パネルの上にある **Dear ImGui / ImKit** を切り替える。 | 同じ部品が、テーマの前後で並ぶ。どちらも同じ値を編集する。 |
| 2 | **Wear a world** のテーマをクリックする。 | Gallery全体が着替える。 |
| 3 | Lookの **Theme Studio** を開き、`blueprint`・`pastel-pop`・`sticker` のどれかを読み込む。 | 色、角、枠線、影がまとめて変わる。下のテキストが、その見た目を再現するテーマファイルになる。 |
| 4 | **Before / After** を開く。 | 素のDear ImGuiとImKitを横に並べ、ホストが持つ同じ値を編集できる。 |

上部のバーには、画面の検索欄（Enterで先頭の一致を開く）、**Appearance**（テーマ、ダーク、密度、コントラスト、動きの抑制、倍率）、**Compare**、**Node Editor** があります。

## ビルドと起動

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

macOSでは `macos-universal` プリセット（Release、arm64とx86_64）を使います。

```bash
cmake --preset macos-universal
cmake --build --preset macos-universal --target imkit_gallery --parallel
# アプリバンドルは build/macos-universal に作られます。
```

Windows版のリリースアーカイブには、`imkit_gallery.exe`、`design-assets`、ライセンス、第三者通知が入っています。サービスは登録せず、利用者の設定ファイルも作りません。`imkit_gallery --theme-file my.theme` は、起動時にテーマファイルを適用し、以後の編集にも追従します（[自分の世界観を作る](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/architecture/世界観を作る.md)）。

## 画面一覧

ナビゲーションを並べ替えても、画面の番号は変わりません。`--page N` と撮影スクリプトがこの番号を使います。名前は `PageId`（[gallery.h](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.h)）にあります。

| グループ | 画面 | 用途 |
| --- | --- | --- |
| **Overview** | Start · Before / After · Workspace | まず変化を見る。Workspaceはタブ、階層の操作、設定、選択部品を1画面にまとめ、操作の要求はGalleryの状態が反映する。 |
| **Look** | Theme Studio · Skins · Tokens · Icons · Window frame | 4色から見た目を作る、部品の枠を差し替える、320個のアイコンを7サイズで探す、ウィンドウ枠のプリセットを試す。 |
| **Controls** | Basic · Numbers and units · Text, colour, media · Trees and tables · Popups and layout · Composites · Rows and palette · Progress and states · Toasts | 必要な部品を見つけて、呼び出しをコピーする。 |
| **Data** | Charts · PTZ camera | ドーナツ、棒、ゲージ、スパークライン。疑似カメラを操作するパン・チルト・ズームのパッド。 |
| **Workspaces** | Editor core · Video · CG · Workspace shell · Preview tiles | 公開部品で組んだタイムライン、インスペクター、モニター、ギズモ。 |
| **Show** | Controls · Live · Rules · Schedule · Beat · Script · Surface · LED · Images | メディアサーバー向けの操作部品（`imkit::show`）。 |
| **Guidelines** | Patterns · Accessibility · Responsive | ナビゲーションの型、公開するセマンティックツリー、狭い幅の配置を確かめる。 |

各画面の公開ヘッダー、CMakeターゲット、実装ファイル、ホスト側に残る責務は、[実例とレシピ](../examples/)に対応表があります。

## 動画とGIFの撮影

動画とGIFは、実際のGalleryを公開Dear ImGuiの入力（ポインタ移動、押下、ドラッグ、ホイール、文字入力）で操作して録画します。大きなカーソルとクリックの輪は、実際の入力座標に描きます。各テイクは結果を検査します（共有値、作業画面の要求、ノードの接続など）。この検査は、OSネイティブの入力、IME、スクリーンリーダー、物理DPIを確かめるものではありません。

```powershell
& './tools/Invoke-SharedBuild.ps1' -Target imkit_gallery,imkit_node_editor_gallery
python tools/media/capture.py        # out/media/native に可逆圧縮の1080p60、あわせてGIF
python tools/media/film.py --lang both
python tools/media/verify.py
```

リポジトリのルートで、FFmpegをPATHに置いて実行します。動画の合成にはPythonのPillowとNumPyを使います。どれもImKitの実行時依存ではありません。中間ファイルは `out/media` に置き、Gitには入れません。

| 出力 | 規格 |
| --- | --- |
| GIF `docs/images/v4-*.gif` | 960×540、20 fps（必要なら12 fps）、6〜8秒、2 MiB以下。長いテイクは、連続したまま加速する。 |
| 動画 `website/public/media/imkit-4.0-film-{ja,en}.mp4` | 1920×1080、60 fps、約3分、約−14 LUFS。映像と音楽は日英で共通で、テロップだけが違う。 |

## 素材の出所

画像と動画は、GalleryまたはNode Editorの付属アプリから録画しています。既成の映像、第三者の製品画面、アイコン、フォント、UIコードは含みません。Dear ImGui、GLFW、任意のフォントは、それぞれのライセンスを保ちます（[THIRD_PARTY_NOTICES.md](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/THIRD_PARTY_NOTICES.md)）。検証の根拠と範囲外の項目は[検証記録](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/reference/検証記録.md)、Frame Labのプラットフォーム境界は[ウィンドウフレーム](../../features/window-frame/)を参照してください。
