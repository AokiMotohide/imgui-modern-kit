# Gallery ガイド

[English](gallery.md) · [README](../README.ja.md)

Windows native Galleryは、初見向けの案内アプリと公開APIの実例を兼ねます。consumerがlinkするImKitと同じlibraryを使い、demoだけに存在する代替widgetを隠しません。

## 初回訪問のための導線

1. **Start** でホスト所有権の境界を説明し、目的別の画面へ直接案内します。
2. **Compare** で、移動・resize可能なDefault Dear ImGui / ImKit windowを開きます。両列は同じホスト所有値を更新するため、片方の操作がもう片方にも反映されます。
3. **Components、themes、icons** で、検索可能なnative specimen、palette編集、生成iconを確認できます。
4. **Workflow、timeline、Frame Lab** では、任意の合成部品とEditor向けsurfaceを扱います。ただしホストのscene、Undo、rendererをImKitが所有するとは主張しません。

比較のbaselineは公開Dear ImGui APIだけです（`StyleColorsDark`と直接widget）。視覚と操作契約の実例であり、性能、OS入力、accessibilityを比較するものではありません。ImKit列はホスト所有の`Theme`、scale、状態、animationを借用します。global registryやGallery専用の第三者assetは導入していません。

## Buildと実行

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

公開するWindows archiveには`imkit_gallery.exe`、必要な`design-assets` directory、本プロジェクトのlicense、第三者noticeを含めます。serviceをinstallせず、ユーザー設定も作成せず、ImKit consumerへruntime依存を追加しません。

## 検証と再生成可能なcapture

Gallery runnerは公開Dear ImGui IOと実OpenGL backbufferを使用します。決定的なwidget契約と文書用の外観確認には使えますが、native OS/IME、screen reader、実機DPIのautomationでは**ありません**。

```powershell
$gallery = './build/windows-debug/catalog/Debug/imkit_gallery.exe'
& $gallery --verify-comparison --output out/comparison
& $gallery --capture-readme --width 960 --height 540 --output out/readme
foreach ($demo in 'comparison', 'themes', 'workflow', 'timeline') {
    & $gallery --capture-demo $demo --width 960 --height 540 --output out/gifs
}
```

`--verify-comparison`は、DefaultとImKitのcontrolが共有ホスト所有状態を更新すること、一時styleがframe後に復元されること、close/reopen時にcontrolが消え、再表示されることを確認します。

`--capture-readme`は120 frameを出力します。各`--capture-demo`は80 frameを出力します。Start／比較／Components、共有値の編集、paletteとpreset遷移、workflow feedback、timeline操作を扱います。すべてnative `960×540` backbufferです。

```powershell
python tools/build_readme_gif.py out/readme/readme-frames docs/images/gallery-overview.gif
foreach ($demo in 'comparison', 'themes', 'workflow', 'timeline') {
    python tools/build_readme_gif.py (Join-Path out/gifs $demo) (Join-Path docs/images ("gallery-$demo.gif")) --expected-frames 80
}
```

encoderはframe数・寸法の不一致、8MiBを超えるGIFを拒否します。Pillowは文書生成専用であり、ImKitからlink・installしません。

## 出典と配布境界

commitした`docs/images/gallery-*.gif`はGalleryから生成します。v2.1の比較とGIFに、stock image、第三者icon・font・UI実装・media assetを追加していません。Dear ImGui、GLFW、任意fontには既存のlicenseが適用され、noticeは[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)にあります。証拠と対象外は[検証記録](validation.md)、Frame Labのplatform adapter境界は[公開window frame](gallery-window-frame.md)を参照してください。
