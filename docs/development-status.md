# Project status and contract map / 開発状況と契約文書

This page is a stable map of the current ImKit source contract, not a live CI dashboard. The source and CMake project version is 3.0.0. Use the [release page](https://github.com/AokiMotohide/imgui-modern-kit/releases) for published packages and [CHANGELOG](../CHANGELOG.md) for versioned changes. Build and acceptance evidence is recorded separately in [Validation](validation.md), with dates and explicit exclusions.

このページはImKitの現行source契約への案内です。live CI dashboardではありません。sourceとCMakeのproject versionは3.0.0です。公開packageは[Release一覧](https://github.com/AokiMotohide/imgui-modern-kit/releases)、versionごとの変更は[CHANGELOG](../CHANGELOG.md)、buildと受け入れ証拠は確認日と対象外を分けた[Validation](validation.md)で確認してください。

## Product contract / 製品契約

ImKit is a C++20 static UI library for the pinned Dear ImGui 1.93.0 WIP docking revision `367b2c24f399988ddafc0bb4628da0106bcc09be`. It supplies themes, reusable controls, workflow/editor components and optional host-integrated modules. The host owns the Dear ImGui context and frame lifecycle, renderer, fonts, application data, persistence and workers.

ImKitは固定したDear ImGui 1.93.0 WIP docking revision `367b2c24f399988ddafc0bb4628da0106bcc09be`上のC++20静的UI libraryです。Theme、再利用部品、workflow／editor部品、任意のhost統合moduleを提供します。Dear ImGui Contextとframe lifecycle、renderer、font、アプリケーションdata、保存、workerはホストが所有します。

## Use the contract references / 契約の参照先

- [Architecture](architecture.md): ownership, module boundaries and extension rules.
- [Dependencies](dependencies.md): pinned revision, build requirements and third-party assets.
- [Public API coverage](api-coverage.md): Dear ImGui overloads, aliases and exclusions.
- [Getting started](getting-started.md) · [導入ガイド](getting-started.ja.md): source and installed SDK integration.
- [Themes](themes.md) · [テーマ](themes.ja.md): presets, scaling, fonts and lifetime.
- [Gallery](gallery.md) · [Galleryガイド](gallery.ja.md): runnable examples and their evidence boundary.
- [Validation](validation.md): dated checks and what they do not establish.

Validation entries describe the revision and activity named in each section. A later entry can supersede an earlier result; none should be read as proof of physical-device, native OS/IME, accessibility or external-host acceptance unless that activity is explicitly recorded.

各Validation記録はsectionに記したrevisionと作業時点を対象にします。後続entryが過去の状態を更新することがあります。実機、native OS／IME、accessibility、外部hostの受け入れは、該当する実施記録がある範囲だけを確認済みとして扱います。
