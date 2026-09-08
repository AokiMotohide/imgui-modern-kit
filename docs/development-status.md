# Development status / 開発状況

Precision Layers 0.2.0 covers the pinned Dear ImGui public GUI API through 365 overloads, semantic light/dark themes and explicit composite controls. Context, renderer, fonts, frame lifecycle and edited data remain host-owned. The six-category production catalog calls the shipped library.

Precision Layers 0.2.0は固定版の公開GUI API 365 overload、light/darkテーマ、汎用合成部品を提供します。Context・renderer・font・frame・編集値はホスト所有です。6カテゴリのカタログは配布ライブラリを呼び出します。

- [API coverage / 対応表](api-coverage.md): exact overloads, implementation and exclusions.
- [Validation / 検証](validation.md): Debug/Release, ownership, public IO interactions, actual GPU captures and relocated SDK consumer.
- [User guide / English](guide.md) / [利用ガイド / 日本語](guide.ja.md): source integration, theme/font/DPI, installed SDK and lifetime contracts.
- [Release v0.2.0](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v0.2.0): source, Windows x64 Debug/Release SDK, manifest, checksums and capture evidence.

Supported baseline is Dear ImGui 1.92.9b docking on Windows x64/MSVC v145. Native OS/IME acceptance, other platforms, older forks and integration into another application are not verified. Earlier design-comparison documents are historical experiments, not alternative supported themes.

対応基準はDear ImGui 1.92.9b docking、Windows x64/MSVC v145です。native OS/IME、他OS、旧fork、他アプリへの導入は未検証です。以前の比較文書は実験の履歴であり、複数の正式テーマを提供するものではありません。
