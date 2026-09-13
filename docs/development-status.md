# Development status / 開発状況

Precision Layers provides the pinned Dear ImGui public GUI API through the generated overload inventory, twelve named semantic themes and explicit composite controls. Context, renderer, fonts, frame lifecycle and edited data remain host-owned. The product Gallery calls the shipped library and separates stable controls from evolving editor examples.

Precision Layersは固定版の公開GUI API、12種類の名前付きsemantic theme、汎用合成部品を提供します。Context・renderer・font・frame・編集値はホスト所有です。製品Galleryは配布ライブラリを呼び出し、安定部品と更新中のEditor実例を分離します。

- [API coverage / 対応表](api-coverage.md): exact overloads, implementation and exclusions.
- [Validation / 検証](validation.md): Debug/Release, ownership, public IO interactions, actual GPU captures and relocated SDK consumer.
- [Getting started / English](getting-started.md) / [導入ガイド / 日本語](getting-started.ja.md): source integration and installed SDK.
- [Themes / English](themes.md) / [テーマ / 日本語](themes.ja.md): presets, font/DPI and lifetime contracts.
- [v3 changelog](../CHANGELOG.md): Windows/macOS targets, package matrix, migration notes and verification boundaries.

The supported baseline is Dear ImGui 1.93.0 WIP docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`. The build matrix covers Windows 10/11 x64, Windows 11 Arm64 and macOS 15+ arm64/x86_64. Windows x64 automation is locally verified; the other architecture jobs and Apple Silicon native acceptance remain pending as recorded in [Validation](validation.md).

対応基準はDear ImGui 1.93.0 WIP docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`です。build matrixはWindows 10/11 x64、Windows 11 Arm64、macOS 15以降arm64/x86_64を対象とします。Windows x64の自動検証はローカル合格済みで、他architectureのjobとApple Silicon実機受入は[検証記録](validation.md)記載の未実施項目です。
