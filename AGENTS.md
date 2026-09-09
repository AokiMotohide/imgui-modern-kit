# AGENTS.md

このファイルは必須規則だけを定める。製品契約は `docs/architecture.md`、検証範囲は `docs/validation.md` を正本とする。

## 基本方針

- `imgui-modern-kit` は C++20 / Dear ImGui の静的UIライブラリである。公開ヘッダーは `include/imkit/`、実装は `src/`、テストは `tests/`、開発用Galleryは `examples/gallery/` に置く。
- 依頼範囲だけを変更する。調査・監査・計画・レビュー・説明では実装しない。公開API、ABI、依存、対応Dear ImGui版、CMake、install、packageの大きな変更は事前確認する。
- 開始時とコミット前にブランチ、対象差分、作業ツリーを確認し、ユーザーや他セッションの変更を編集・削除・ステージしない。

## 調査と効率

- 既知の対象ファイル、シンボル、直接依存から始め、失敗または影響根拠がある場合だけ範囲を広げる。全体列挙、全文再読、広い履歴調査を定型化しない。
- 横断的な所在、呼出し経路、所有権、影響範囲は `.codegraph/` の `codegraph explore` を先に使う。既知の局所修正、CMake、文書、設定、単純検索は対象を絞った `rg` または直接確認でよい。
- CodeGraph、`rg`、全文読込で同じ確認を重複しない。`codegraph status` / `sync` は索引の古さが調査結果へ影響する場合だけ実行する。ローカルDBはコミットしない。
- 検索とコマンド出力はディレクトリ、拡張子、シンボル、件数、行数を絞る。同じ失敗コマンドは条件や仮説を変えずに再実行しない。
- ユーザーが明示しない限りサブエージェントを起動しない。並列処理は独立し、結果が重複しない場合だけ使う。

## 実装境界

- ライブラリはDear ImGuiのContext、backend、renderer、font、texture、永続化、workerを所有・初期化しない。Themeと状態の寿命もホストが管理する。
- 公開overload、戻り値、default引数、ID、focus、callback、disabled、Begin/End、clipping、入力編集を維持する。入力widgetを独自挙動で置き換えない。
- 公開APIは `include/imkit/`、実装は `src/` に分離する。非所有参照を所有扱いせず、scopeは対象Contextより先に破棄する。
- 新しい部品は意味token、Dear ImGui公開widget、公開DrawListで構成し、グローバルregistryやホスト固有サービスを導入しない。
- 公開API変更時だけ、API compile fixture、consumer test、API inventory、英日公開文書への影響を確認する。

## ビルドと検証

通常は構成済み `build/windows-debug/` のDebug増分ビルドを使う。

```powershell
cmake --build --preset windows-debug --target <imkit|対象テスト|imkit_gallery> --parallel
ctest --test-dir build/windows-debug -C Debug -R "<対象テスト>" --output-on-failure
```

- 変更した最小targetと直接回帰だけを1回確認する。成功後のno-op再ビルド、同一テスト、全テスト、`ALL_BUILD`、`clean`、`Rebuild` は追加しない。
- 再構成は新規build、CMake・依存変更、構成不整合時だけ行う。Release、install、package、SDK consumerは変更範囲または依頼が必要とする場合だけ実行する。
- 規約、文書、コメントだけの変更ではビルドしない。新規テストやcaptureは、変更した契約または再発リスクを直接検出する場合だけ追加する。
- ビルド前の競合確認は1回とし、同じbuildまたは出力へ書く処理だけを競合とする。長時間処理は固定sleepではなくプロセスと出力更新で追跡する。
- compile/link、公開IO操作、GPU capture、native OS/IME、他アプリ統合、配布検証を区別し、未実施項目を合格扱いしない。

## 文書・資産・Git

- 公開API、導入方法、対応環境、所有権、配布契約、ユーザー可視動作を変えた場合だけ関連文書を更新し、英日版の意味を一致させる。
- `docs/images/` はnative Galleryの実captureだけを使う。再生成可能なbuild、`out/`、ログ、capture一式は依頼なしにコミットしない。
- 第三者のicon、font、画像、コードを変更した場合はprovenance、license、`THIRD_PARTY_NOTICES.md`、配布対象を確認する。
- 完了後は対象ファイルだけをステージし、`git diff --cached --check` 後、日本語でコミットする。ユーザーがコミット不要と指定した場合は除く。push、PR、Issue、Releaseは明示依頼時だけ行う。

## 機密保持

- 回答、文書、コミット、Issue、PR、Release、ログ、生成物に、ユーザーが現在の依頼で対象情報と開示先を個別指定していない他リポジトリ情報を含めない。
- 禁止対象は他リポジトリの名称、パス、存在、進捗、計画、ブランチ、コミット、差分、設計、未公開機能、障害、検証・配布状況と、それらを推測できる表現を含む。過去の許可、包括依頼、閲覧権限は開示許可ではない。
- 他リポジトリを扱うのは、現在の依頼で対象と目的が明示された場合だけとし、情報を本リポジトリや外部公開物へ転記するには開示先の明示も必要とする。

## 報告

- 進捗は開始、重要な判断変更、長時間処理、阻害要因だけを簡潔に伝え、長いログ、全差分、同じ状態の反復を避ける。
- 完了報告は結果、変更ファイル、実施した検証、重要な未実施項目、コミットの有無に限定する。
