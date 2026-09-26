# ImKit 文書

[English](README.md) · [プロジェクト概要](../README.md)

ImKit の文書は、確立した GUI プロジェクト（Dear ImGui、GTK、Qt、egui、Flutter）の構図に倣って、3 段階で整理されています：

1. **Learn（学習）** — 心智モデル：所有、Dear ImGui のフレーム、テーマング、モジュール配置。
2. **Build（作り上げる）** — 実行可能な codelab：初回のテーマ付きウィンドウから、ノードエディター、タイムライン、カスタムコンポーネントまで。
3. **Reference and verify（参照・検証）** — モジュール契約の権威的参照、API 一覧、日付付き検証のエビデンス。

完全な二言語ページマップ、ターゲット読者、ソース/API 対応、パッケージ経路については [文書カタログ](documentation-catalog.md) を参照。実用的なモジュール毎の例については [例とレシピ](examples-recipes.md) を参照。

## 心智モデルを学習する

| 読む | 答えられること |
|---|---|
| [ImKit の仕組み](how-it-works.md) | 誰が何を所有するか、Dear ImGui のフレーム、ImKit の位置づけ |
| [アーキテクチャ](architecture.md) | 所有モデル、モジュール配置、公開契約の権威的な参照 |
| [デザインシステム](design-system.md) | デザイントークン、テーマ、アイコン API |
| [依存関係](dependencies.md) | 必要な Dear ImGui リビジョンと外部パッケージ |
| [ユーザーガイド](guide.md) | ホストとライブラリの分離と最初の呼び出しの短い導入 |

## 作り上げる（codelab）

あなたのアプリに移植できる、ステップバイステップのホスト所有のビルド。各 codelab は、完全な契約を定義する下の参照ページとセットになります。

- [ImKit アプリの作り方](build-first-app.md) — コンテキスト、テーマ、最初のテーマ付きコントロール。
- [設定画面の作り方](build-settings-screen.md) — ホスト状態、設定行、保存/検証/通知。
- [ノードエディターの作り方](build-node-editor.md) — グラフスナップショット投入、編集リクエスト返却。
- [タイムラインエディターの作り方](build-timeline.md) — トラックス、クリップ、フェード、トランジション、リップル移動。
- [カスタムコンポーネントの作成](custom-component.md) — `ThemeScope`、セマンティックカラー、報告。

## ImKit の導入（目的別）

| 目的 | 読む |
|---|---|
| 既存の Dear ImGui アプリに ImKit を追加 | [導入ガイド](getting-started.md) |
| ホストとライブラリーの所有、テーマ、モジュール選択を理解する | [ユーザーガイド](guide.md)、[アーキテクチャ](architecture.md) |
| 再利用可能なコントロールで一般的な設定インターフェースを作る | [コンポーネントとレシピ](components.md) |
| ライブの例を探索する | [Gallery ガイド](gallery.md) |
| テーマの選択、カスタマイズ、適用 | [テーマ](themes.md) |
| 設定やランタイムの問題を診断する | [トラブルシューティング](troubleshooting.md) |
| 既存の v2 コンサマーをアップグレード | [v3 ミグレーション](migration-v3.md) |
| タスクに対する Gallery ページ、ソース、API、次の読書を検索 | [学習マップとレシピ](examples-recipes.md) |

## 特定のモジュールを統合する

| モジュール | ガイド | 契約またはより深い参照 |
|---|---|---|
| Node Editor | [ノードエディター連携](node-editor.md) | [自動生成 API 一覧](node-editor-api.json), [レシピ](examples-recipes.md) |
| Workflow とデータコンポーネント | [ワークフローコンポーネント](workflow-components.md) | [シェルコンポーネント](shell-components.md), [レシピ](examples-recipes.md) |
| Editor Core, Video, CG | [エディタースイート](editor-suite.md) | [エディター API](editor-api.md), [タイムライン編集](timeline-editing.md), [レシピ](examples-recipes.md) |
| ウィンドウフレーム | [Gallery WindowFrame ガイド](gallery-window-frame.md) | [WindowFrame API 一覧](window-frame-api-inventory.json), [レシピ](examples-recipes.md) |
| アイコン | [アイコン参照](icons.md) | [レシピ](examples-recipes.md) |

## 主張を検証する、または API を調べる

| 質問 | 権威的な参照 |
|---|---|
| ImKit は何を所有し、ホストに何を残すか？ | [アーキテクチャ](architecture.md) |
| どの Dear ImGui リビジョンと外部パッケージが必要か？ | [依存関係](dependencies.md) |
| どの Dear ImGui 関数とオーバーロードが公開されているか？ | [公開 API カバーレージ](api-coverage.md), [API 一覧](api-inventory.json) |
| 何が構築され、何が未検証か？ | [検証](validation.md) |
| ウジジェットカタログは何をカバーするか？ | [ウジジェット一覧](widget-inventory.md) |
| デザインシステムのトークンと API は何か？ | [デザインシステム](design-system.md), [デザインシステム API 一覧](design-system-api.json) |

`architecture.md` と `validation.md` は所有とエビデンスの権威的な参照です。検証は時間・リビジョン特定です：記録された合格は自動的に後のチェックアウトを記述したり、ネイティブ OS の入力、アクセシビリティ、外部ホスト、実機の受け入れを確立したりしません。

## 保守用メモと設計記録

これらのページはプロジェクトの決定、提案、チェックリスト、レビューコンテキストを保持します。これらは消費者 API の約束ではありません。ImKit を統合するときは、上記の公開ヘッダと契約参照を使用してください。

一部の記録は以前のリリースやチェックアウトを記述しています。それらのステータステーブルは歴史的スナップショットであり、現在の TODO リストや公開承認ではありません。その基線と日付を使用し、現在の動作は公開契約ページに対して確認してください。

- [デザイン提案](design-proposals.md) · [デザイン改善](design-refinements.md)
- [開発状況](development-status.md) · [エディターリフレッシュ](editor-refresh.md) · [エディター検証](editor-validation.md)
- [エディター実装チェックリスト](editor-implementation-checklist.md) · [ノードエディターレビュー](node-editor-review.md)
- [GitHub プロフィール用コピー](github-profile.md)
- [パフォーマンスのエビデンス](evidence/)

## 文書のアプローチ

情報の流れは確立した GUI プロジェクトのパターンを踏まえています：Dear ImGui はセットアップからバックエンド特定の例とライブデモへリードします；GTK はビルド可能な最初のアプリケーションから始まります；Qt はチュートリアル/例を API 参照と分離します；egui は簡潔な例とインタラクティブデモをセットにします。ImKit は、彼らのコードや表現をコピーせずに、ホスト所有の即時モード契約にこれらの構造選択を適用しています。[Dear ImGui Getting Started](https://github.com/ocornut/imgui/wiki/Getting-Started)、[GTK Getting Started](https://docs.gtk.org/gtk4/getting_started.html)、[Qt ドキュメントカテゴリー](https://doc.qt.io/qt-6/qdoc-categories.html)、[egui](https://docs.rs/egui/latest/egui/) を参照。
