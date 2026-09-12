# imgui-modern-kit

[English](README.md) · [導入ガイド](docs/getting-started.ja.md) · [Gallery ガイド](docs/gallery.ja.md) · [変更履歴](CHANGELOG.md)

## ✨ 開発ツールにも、プロダクトと同等のデザイン品質を

**ImKit v2.2.0** は、Dear ImGui向けのC++20デザインレイヤーです。一貫した外観、再利用可能なUIコンポーネント、用途に応じたテーマ、動的生成アイコン、カスタマイズ可能なエディタインターフェースを提供しながらも、Context、レンダラー、データ、ワークフローの所有権はホスト側に保持します。

MITライセンス · 静的ライブラリ · Windows x64/MSVCで検証済み · Dear ImGui `v1.92.9b-docking` 基準

> 🪟 **まずはギャラリーをお試しください。** [Windows x64 Gallery](https://github.com/AokiMotohide/imgui-modern-kit/releases/download/v2.2.0/imkit-2.2.0-gallery-windows-x64.zip)をダウンロードし、解凍後に`imkit_gallery.exe`を実行してください。インストーラーやアプリケーションコードの書き換えは不要です。

## 🎞 ネイティブギャラリーの動作デモ

以下のGIFアニメーションは、すべてネイティブギャラリーからキャプチャしたものです。最初の2つは機能の違いを分かりやすく示しています。移動・リサイズが可能な **Compare** ウィンドウでは、Dear ImGui標準UIとImKitを並べ、双方から*ホスト側の同じ値*をリアルタイムに編集・同期できます。

| | |
|---|---|
| ![案内付きGallery Start](docs/images/gallery-overview.gif)<br>**迷わず最初の画面から試せる**<br>空のデモ画面ではなく、機能比較、コンポーネント一覧、ワークフロー、エディタの実用例へとスムーズに案内します。 | ![Default Dear ImGuiとImKitのライブ比較](docs/images/gallery-comparison.gif)<br>**同じ操作を並べて比較**<br>標準の`StyleColorsDark`ウィジェットとImKitコントロールが、1つの共有値を同時に更新します。 |
| ![Theme paletteの遷移](docs/images/gallery-themes.gif)<br>**12種類の完全なテーマを切り替え**<br>実行中のUIでカラーパレットや状態に応じた表示の変化を確認できます。 | ![生成iconの検索と選択](docs/images/gallery-icons.gif)<br>**284個の生成アイコンを検索**<br>7種類のピクセルサイズと1,988個のアトラスバリエーションにより、大規模なカタログを実用的に扱えます。 |
| ![Workflow状態のfeedback](docs/images/gallery-workflow.gif)<br>**実際のワークフローを直感的に表示**<br>特定のフレームワークに依存せず、ホスト側のリクエスト、通知、状態のフィードバックを表示します。 | ![Timeline操作](docs/images/gallery-timeline.gif)<br>**編集インターフェースへの拡張**<br>ギャラリーのサンプルとして、タイムライン操作やUndo（取り消し）の動作を確認できます。 |

これらは見た目だけ再現したモックアップではなく、実際のOpenGLバックバッファからキャプチャした映像です。視覚的構造と一貫性のある操作感を示しています。なお、パフォーマンス、OSネイティブのIME入力、アクセシビリティのベンチマークを目的とするものではありません。比較画面のためにサードパーティ製のUIコードやアセットを追加・複製していません。

## 🚀 継続して進化するデザインシステム

ImKitは一度限りの見た目の更新にとどまらず、バージョンリリースを通して進化し続けます。ギャラリー、再生成可能なGIF、SDKマニフェスト、日英のドキュメントを同時に更新するため、導入前にネイティブ実行ファイルで改善内容を確認できます。現在の開発状況と変更履歴は[Releases](https://github.com/AokiMotohide/imgui-modern-kit/releases)および[変更履歴](CHANGELOG.md)をご確認ください。

## 30秒で価値を確認する

ソースからギャラリーをビルドします。

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
