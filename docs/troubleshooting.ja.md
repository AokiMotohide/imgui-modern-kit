# トラブルシューティング

[English](troubleshooting.md)

## `IMKIT_IMGUI_TARGET is required`

`add_subdirectory`より先にホストのDear ImGui CMake targetを作り、その正確なtarget名を`IMKIT_IMGUI_TARGET`へ設定します。埋め込み先へ別のImGui本体を暗黙に追加しないための制約です。

## ImGui版をheaderに拒否される

固定した対応revisionを使ってください。style fieldと公開signatureが変わるため、guardを緩めるだけではsource互換・ABI互換の確認になりません。

## Installed SDKをlinkできない

x64、MSVC toolset、Debug/Release CRT、Dear ImGui revision、compile definition、`imconfig.h`を確認します。binary設定が1つでも異なる場合はソース導入を使います。

## Themeは表示されるが日本語が欠ける

Themeはfontを読み込みません。必要glyphをホストのfont atlasへ追加し、文字入力にはplatform IME callbackを用意します。Galleryのfallback fontは任意のホスト資産であり、library本体の動作ではありません。

## Themeを再適用すると寸法が大きくなる

application倍率は`ApplyTheme`または`ThemeScope`だけへ渡し、`Theme::metrics`を事前に拡大しないでください。ImKitは保存された未拡大metricsからstyleを生成します。

## Gallery executableを上書きできない

実行中の`imkit_gallery.exe`を閉じ、同じincremental targetを1回buildします。Windowsでは実行中のexeをlinkerが置換できません。

## Screenshotだけで組み込み完了と判断できない

[Validation](validation.md)でcompile/link、公開IO、GPU capture、installed consumer、native application受け入れを区別してください。
