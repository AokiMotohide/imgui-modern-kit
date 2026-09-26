# Design system / Design system

[English](design-system.md) · [Theme](themes.ja.md) · [文書カタログ](documentation-catalog.ja.md)

このページは、公開API、Theme token、accessibility frame、locale、patternの設計と統合制約を日本語でまとめます。全API表・型・callbackの正本は[英語版design-system reference](design-system.md)です。公開headerとAPI compile fixtureを併せて確認してください。

## Design / 設計

semantic tokenからstyleを導出し、host providerでdataを供給する方針です。locale callback、native widget、明示accessibility stateを利用し、global localeやhost serviceを導入しません。参考にした設計原則と出典は[英語版](design-system.md#design)に記載されています。

## APIとmigration

`MakeTheme(scheme, contrast, density)`は色scheme、contrast、densityからTheme値を返します。`SetDensity`は文字サイズを変えずにcontrol寸法を切り替えます。semantic tokenを編集した後は`ResolveTheme`で描画用palette/metricsを導出してください。legacy `colors`/`metrics`とsemantic tokenを個別に二重編集しません。font pointerをload/freeするのはホストです。

`motion.reducedMotion`はcomponent animationを抑制します。`ValidateContrast`は宣言したsurface/control stateに対する検査で、任意の背景画像や全合成状態を保証しません。実OS screen reader受け入れは別途必要です。

Accessibilityについては`AccessibilityFrame`、`ActionQueue`、storage、generation、描画前後のpublicationをホストが管理します。公開label/spanはpublication期間だけ借用します。capacity overflow、stale requestの破棄、同期もホスト責務です。

`LocaleContext`はhost callbackを通じて翻訳・数値・日時表示・layout directionを提供します。擬似RTLは文字の双方向整形や全UIの完全RTLを意味しません。

## Components / 部品

`patterns.h`はCommandPalette、SearchField、Breadcrumbs、Dialog、Menu、FormField、Progress、ToastRegion、Pagination、VirtualList、DataTable、TreeDataGrid等を公開します。application data、Undo、保存、workerはホスト所有です。

providerはsort/filter/展開後のvisible indexを供給し、必要なrangeだけをqueryに応じます。安定したnonzero IDとframe期間有効なtextを返します。大きなtreeの全走査を毎frame要求しません。

## Remaining implementation / 残る実装

英語版のこの見出し以下は、包括的な将来仕様に対する当時の作業状況です。現在の未実装一覧として利用せず、各公開APIと現行testを確認してください。

## Recorded verification / 記録された検証

英語版は2026-09-10時点のdesign system検証履歴です。build/captureの範囲と未確認の受入条件は[Validation](validation.ja.md)で確認してください。履歴を後続revisionの証明として扱わないでください。

### Theme・Accessibility・Locale

`MakeTheme`のscheme、contrast、density、semantic tokenについては上記の[APIとmigration](#apiとmigration)を参照してください。正確なbuffer・callback・署名条件は[英語版API](design-system.md)を参照してください。
