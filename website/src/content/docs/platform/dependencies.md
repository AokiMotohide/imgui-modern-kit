---
title: "依存関係"
description: "ImKitはDear ImGuiの上に作られています。このページでは、ImKitが何に依存するか、固定しているリビジョン、CMakeがそれをどう解決するかをまとめます。"
---

ImKitは[Dear ImGui](https://github.com/ocornut/imgui)の上に作られています。このページでは、ImKitが何に依存するか、固定しているリビジョン、CMakeがそれをどう解決するかをまとめます。

## Dear ImGuiの上に作られています

Dear ImGuiがなければ、ImKitは生まれませんでした。Omar Cornut氏と、このプロジェクトのすべての貢献者、スポンサーの皆さまに感謝します。ImKitは、その成果の上に重ねる層であり、代わりになるものではありません。

| 疑問 | 答え |
| --- | --- |
| ImKitはDear ImGuiを改変しますか。 | いいえ。公開APIだけを使い、`imgui_internal.h`は使わず、パッチも配布しません。 |
| ImKitはDear ImGuiをビルドしますか。 | いいえ。Dear ImGuiのターゲットはアプリが用意します。Galleryは、上流のソースを無改変のままビルドします。 |
| どのリビジョンですか。 | `docking`ブランチの、固定した1つのコミットです。Dear ImGuiのバージョンや、ドッキング、ビューポート、動的テクスチャへの対応が合わないときは、`version.h`がビルドを止めます。 |

## 固定している依存

| 依存 | 入手元 | バージョン | コミット | ライセンス | 用途 |
| --- | --- | --- | --- | --- | --- |
| Dear ImGui | [ocornut/imgui](https://github.com/ocornut/imgui) | `1.93.0 WIP docking` | `367b2c24f399988ddafc0bb4628da0106bcc09be` | MIT | 公開ウィジェットの型とAPI。Galleryでは、公式のGLFW/OpenGL3とGLFW/Metalのバックエンドも使います。 |
| GLFW | [glfw/glfw](https://github.com/glfw/glfw) | `3.5.1` | `d9d6f0f1f967807ffade6598ea9a631ebaf37a56` | zlib/libpng | Galleryのウィンドウ、入力、OpenGLコンテキストだけ |

ライセンスの全文とハッシュは[THIRD_PARTY_NOTICES.md](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/THIRD_PARTY_NOTICES.md)にあります。

## CMakeでの解決のしかた

| ビルドの形 | 動作 |
| --- | --- |
| `add_subdirectory`で組み込む | すでにビルドしてあるDear ImGuiのターゲットを、`IMKIT_IMGUI_TARGET`に指定します。何もダウンロードせず、インクルードディレクトリとリンク要件は、そのターゲットを通してImKitに届きます。 |
| 単体ビルド（Galleryあり） | CMakeが、固定コミットのDear ImGuiとGLFWを取得します。 |
| 単体ビルド（Galleryなし） | CMakeが取得するのはDear ImGuiだけです。GLFWは取得しません。 |

- Windows版のGalleryは、`FindOpenGL`でOpenGLを探します。macOS版のGalleryは、システムのMetal、MetalKit、Cocoaフレームワークをリンクします。
- CMakeは3.20以降が必要です。ImKitが使うプリセットの書式、`FetchContent_MakeAvailable`、ターゲットのエイリアスを、この版が備えているためです。

## 互換性

ImKit 4.xは、上記のDear ImGuiのリビジョンだけに対応します。アプリと、リンクするすべてのImKitライブラリを、同じヘッダーでビルドしてください。3.xのオブジェクトと4.xの構造体は混ぜられません。

| パッケージ | 対応環境 |
| --- | --- |
| Windows | x64とArm64、MSVC |
| macOS | 15以降。arm64、x86_64、Universal 2、Apple Clang（署名・公証なし） |

パッケージには、ImKitのライブラリ、Gallery、Node Editor Gallery、英語と日本語の文書、`LICENSE`、第三者の通知が入っています。Dear ImGuiは入っていません。

## 任意の素材とツール

| 項目 | 役割 |
| --- | --- |
| Inter 4.1、Noto Sans JP 2.004 | SIL Open Font License 1.1の任意フォントです。`imkit_copy_font_assets`が、実行ファイルの隣にコピーします。アプリが自分のアトラスへ読み込みます。 |
| Pillow、FFmpeg | 文書用のメディアを作るときだけ使います。リンク、インストール、再配布はしません。 |

## アプリと並べてImKitを開発する場合

ImKitを固定したGitサブモジュールとして使いながら、アプリの隣でImKitも開発するときは、自分で持つCMakeのキャッシュ変数を用意し、サブモジュール、またはその明示したパスのどちらかを`add_subdirectory`に渡します。この切り替えは、プロジェクトファイルとリリースの一覧に含めません。どちらの経路でも、ImKitを追加する前に、`IMKIT_IMGUI_TARGET`を設定します。
