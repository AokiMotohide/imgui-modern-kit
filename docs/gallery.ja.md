# Gallery ガイド

[English](gallery.md) · [README](../README.ja.md)

Windows向けのGalleryは、初めて使う人への案内と公開APIの実例を兼ねています。利用者のアプリと同じImKitライブラリを使い、デモ専用の代替widgetは使っていません。

## 初回訪問のための導線

1. **Start** でImKitとアプリ側の責任範囲を説明し、目的に合った画面へ案内します。
2. **Compare** でDear ImGui標準部品とImKitの比較画面を開きます。ウィンドウは移動・サイズ変更でき、どちらの列からも同じ値を変更できます。
3. **Components、themes、icons** の各ページで部品の動作や配色を試せます。アイコンのC++カタログは284種・7サイズです。リポジトリには16カテゴリの透過PNG原画が238点あり、READMEでは全カタログとは別に、大きなタイルで紹介しています。
4. **Workflow、timeline、Frame Lab** では、任意で使える画面部品やEditor向け機能を紹介します。scene、Undo履歴、rendererはImKitではなくアプリ側が管理します。

Start画面から **Components: Basic** (page 0)、**Icons** (page 6、テーマは上部のAppearanceから変更)、**Generic Workspace** (page 15)、**Video** (page 8)へ移動できます。[実例と実装例](examples-recipes.ja.md)では、画面、公開ヘッダー、CMakeターゲット、実装ファイル、所有範囲、関連ガイドを対応付けています。各Startカードの移動先はGalleryの検証処理で確認しています。

比較対象は、公開Dear ImGui APIから直接描画した標準部品（`StyleColorsDark`）です。見た目と操作方法の例であり、性能、OS入力、アクセシビリティを比較するものではありません。ImKit側ではアプリが管理する`Theme`、scale、状態、animationを使います。グローバルな登録機構やGallery専用の第三者製素材は追加していません。

## Buildと実行

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

macOS なら `macos-universal` preset（Release、arm64/x86_64、Universal 2）を使います。

```bash
cmake --preset macos-universal
cmake --build --preset macos-universal --target imkit_gallery --parallel
# .app は build/macos-universal の出力ディレクトリに生成されます。
```

Windows向け配布zipには`imkit_gallery.exe`、必要な`design-assets` directory、本プロジェクトのlicense、第三者noticeを含めます。serviceやユーザー設定は追加せず、ImKitを組み込むアプリのruntime依存も増やしません。

通常起動では、ImKit GalleryとDear ImGui公式Demo Windowを、移動・サイズ変更が可能な2つのウィンドウとして
同じキャンバスに表示します。広い画面では左右に並べ、通常のDear ImGuiと同じドッキング操作ができます。
Galleryには選択中のImKit themeを適用し、公式Demoは比較できるようDear ImGui標準styleのまま表示します。
公式DemoはGallery上部の**Dear ImGui Demo**チェックから閉じたり、再表示したりできます。

## 検証と再生成可能なcapture

Gallery runnerは公開Dear ImGui IOを使い、OpenGL backbufferを撮影します。次のコマンドで文書用の画面を再生成できます。OSの入力操作、IME、画面読み上げ、実機DPIの自動検証ではありません。

    $gallery = './build/windows-debug/catalog/Debug/imkit_gallery.exe'
    & $gallery --verify-comparison --output out/comparison
    $routes = @('overview', 'comparison', 'components', 'icons', 'icon-artwork', 'themes', 'workflow', 'preview-contract', 'timeline')
    foreach ($route in $routes) {
        & $gallery --capture-demo $route --width 1440 --height 810 --output out/readme
    }
    $node = './build/windows-debug/catalog/Debug/imkit_node_editor_gallery.exe'
    & $node --capture-gif out/readme/node-editor --width 1440 --height 810

各画面の撮影結果はnative backbufferからout/readme以下に出力されます。`icon-artwork` はリポジトリ内のPNG原画をWindows Imaging Componentで読み込み、Gallery上にタイル表示してからOpenGLのbackbufferをcaptureします。リポジトリのルートから実行してください。撮影時は1440×810にし、README画像へ変換するとき960×540に縮小します。Node Editorは指定した撮影サイズで56枚出力します。READMEに載せる10本は、画面ごとのframe数を指定して生成します。

    $routes = @(
        @{name='overview'; file='v3-overview.gif'; frames=60; colors=128; dither='floyd-steinberg'},
        @{name='comparison'; file='v3-comparison.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='components'; file='v3-components.gif'; frames=52; colors=128; dither='floyd-steinberg'},
        @{name='icons'; file='v3-icons.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='icon-artwork'; file='v3-icon-artwork.gif'; frames=60; colors=128; dither='floyd-steinberg'},
        @{name='themes'; file='v3-theme-comparison.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='workflow'; file='v3-workflow-progress.gif'; frames=64; colors=96; dither='floyd-steinberg'},
        @{name='preview-contract'; file='v3-preview-contract.gif'; frames=50; colors=128; dither='floyd-steinberg'},
        @{name='timeline'; file='v3-timeline.gif'; frames=64; colors=128; dither='floyd-steinberg'},
        @{name='node-editor'; file='v3-node-editor.gif'; frames=56; colors=96; dither='none'}
    )
    foreach ($route in $routes) {
        python tools/build_readme_gif.py "out/readme/$($route.name)" "docs/images/$($route.file)" --expected-frames $route.frames --width 960 --height 540 --fps 8 --colors $route.colors --dither $route.dither
    }

変換スクリプトは同じ16:9のframeを960×540に縮小し、8 fpsのGIFを生成します。2 MiBを超えるGIFは出力しません。Node Editorは容量を抑えるため、96色のpaletteを使い、ditheringを無効にしています。ほかの画面には表の設定を使います。READMEに掲載するGIF10本は、合計9 MiB以内に保ちます。Pillowは文書画像の再生成だけに使い、ImKitの依存には加えません。

## 出典と配布境界

`docs/images/v3-*.gif`はGalleryまたはNode Editor companionから生成します。リリース用showcase MP4も同じnative frame列から作ります。既成画像、第三者製品の画面、外部のicon・font・UI実装・media assetは含みません。Dear ImGui、GLFW、任意fontにはそれぞれのlicenseが適用され、詳細は[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)に記載しています。検証の証拠と対象外は[検証記録](validation.md)、Frame Labのplatform adapter境界は[公開window frame](gallery-window-frame.md)を参照してください。
