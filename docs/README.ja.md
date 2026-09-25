# ImKit ドキュメント

[English](README.md) · [プロジェクト概要](../README.ja.md)

対象読者、英日ページ、API・Gallery・ソース・package収録先の一覧は[文書カタログ](documentation-catalog.ja.md)を参照してください。module別の実例は[学習マップとrecipe](examples-recipes.ja.md)にまとめています。

このページは、目的に合う文書を選ぶための入口です。APIの契約は参照文書を確認し、検証記録では確認した時点と範囲を確認してください。

## 利用を始める

| 目的 | 文書 |
|---|---|
| 既存Dear ImGuiアプリへImKitを追加する | [導入ガイド](getting-started.ja.md) |
| ホストとライブラリの所有権、Theme、module選択を理解する | [利用ガイド](guide.ja.md)、[Architecture](architecture.md) |
| 再利用部品で一般的な設定画面を組み立てる | [コンポーネントとrecipe](components.ja.md) |
| 実際に操作できる例を見る | [Galleryガイド](gallery.ja.md) |
| Themeを選び、調整して適用する | [Theme](themes.ja.md) |
| 構成や実行時の問題を調べる | [トラブルシューティング](troubleshooting.ja.md) |
| v2利用者が更新する | [v3移行ガイド](migration-v3.ja.md) |
| Galleryの画面、source、API、次に読む文書を調べる | [学習マップとrecipe](examples-recipes.ja.md) |

## 個別moduleを統合する

| module | ガイド | 契約・詳細資料 |
|---|---|---|
| Node Editor | [Node Editor統合](node-editor.ja.md) | [生成API一覧](node-editor-api.json)、[recipe](examples-recipes.ja.md) |
| Workflow・data component | [Workflow component](workflow-components.ja.md) | [Shell component](shell-components.ja.md)、[recipe](examples-recipes.ja.md) |
| Editor Core・Video・CG | [Editor Suite](editor-suite.ja.md) | [Editor API](editor-api.ja.md)、[Timeline編集](timeline-editing.ja.md)、[recipe](examples-recipes.ja.md) |
| Window frame | [Gallery WindowFrameガイド](gallery-window-frame.ja.md) | [WindowFrame API一覧](window-frame-api-inventory.json)、[recipe](examples-recipes.ja.md) |
| Icon | [Icon一覧](icons.ja.md) | [recipe](examples-recipes.ja.md) |

## 契約やAPIを確認する

| 確認したいこと | 基準となる文書 |
|---|---|
| ImKitが所有するもの、ホストに残るもの | [Architecture](architecture.ja.md) |
| 必須Dear ImGui revisionと外部package | [Dependencies](dependencies.ja.md) |
| 公開するDear ImGui関数とoverload | [公開API対応表](api-coverage.ja.md)、[API inventory](api-inventory.json) |
| build・操作で確認した範囲と未検証事項 | [Validation](validation.ja.md) |
| widget catalogの対象 | [Widget inventory](widget-inventory.ja.md) |
| design-system tokenとAPI | [Design system](design-system.ja.md)、[Design-system API一覧](design-system-api.json) |

所有権の基準は`architecture.ja.md`、検証証拠の基準は`validation.ja.md`です。Validationは特定の時点・revisionに対する記録です。過去の合格を後続checkoutの証明として扱わず、native OS入力、accessibility、外部host、実機の受入範囲も個別に確認してください。

## 保守用メモと設計記録

以下は設計判断、提案、checklist、review経緯を残す資料です。利用者向けAPI契約ではありません。ImKit統合時は公開headerと上記の契約文書を参照してください。

過去のreleaseやcheckoutを対象にした記録も含みます。そこにある状態表は履歴であり、現在のTODO一覧や後続公開の許可ではありません。記載されたbaselineと日付を確認し、現行の動作は公開契約文書で照合してください。

- [Design proposal](design-proposals.md) · [Design refinement](design-refinements.md)
- [開発状況](development-status.md) · [Editor更新記録](editor-refresh.md) · [Editor検証記録](editor-validation.md)
- [Editor実装checklist](editor-implementation-checklist.md) · [Node Editor review](node-editor-review.md)
- [GitHub profile原稿](github-profile.md)
- [Performance evidence](evidence/)

## 文書構成の考え方

広く使われるGUI projectの文書構成を参考にしています。Dear ImGuiは導入からFAQ・実例・Demoへ案内し、Qtは学習tutorialとexampleをAPI referenceから分けています。ImKitも目的別の導線を作りつつ、ホスト所有とimmediate-mode統合の契約を明記します。[Dear ImGui](https://github.com/ocornut/imgui) · [Qt Examples and Tutorials](https://doc.qt.io/qt-6/qtexamplesandtutorials.html)
