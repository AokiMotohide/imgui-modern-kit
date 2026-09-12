# Development status / 開発状況

Precision Layers provides the pinned Dear ImGui public GUI API through the generated overload inventory, twelve named semantic themes and explicit composite controls. Context, renderer, fonts, frame lifecycle and edited data remain host-owned. The product Gallery calls the shipped library and separates stable controls from evolving editor examples.

Precision Layersは固定版の公開GUI API、12種類の名前付きsemantic theme、汎用合成部品を提供します。Context・renderer・font・frame・編集値はホスト所有です。製品Galleryは配布ライブラリを呼び出し、安定部品と更新中のEditor実例を分離します。

- [API coverage / 対応表](api-coverage.md): exact overloads, implementation and exclusions.
- [Validation / 検証](validation.md): Debug/Release, ownership, public IO interactions, actual GPU captures and relocated SDK consumer.
- [Getting started / English](getting-started.md) / [導入ガイド / 日本語](getting-started.ja.md): source integration and installed SDK.
- [Themes / English](themes.md) / [テーマ / 日本語](themes.ja.md): presets, font/DPI and lifetime contracts.
- [Release v2.2.0](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v2.2.0): source, Windows x64 SDK, Gallery archive, manifest, checksums and capture evidence.

Supported baseline is Dear ImGui 1.92.9b docking on Windows x64/MSVC v145. Native OS/IME acceptance, other platforms, older forks and integration into another application are not verified. Earlier design-comparison documents are historical experiments; the twelve presets documented in [Themes](themes.md) are the supported set.

対応基準はDear ImGui 1.92.9b docking、Windows x64/MSVC v145です。native OS/IME、他OS、旧fork、他アプリへの導入は未検証です。以前の比較文書は実験履歴であり、対応presetは[テーマ文書](themes.ja.md)の12種類です。
