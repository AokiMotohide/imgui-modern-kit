# AGENTS.md

このリポジトリ固有の規則だけを書く。進め方、確認の上限、止める条件、Git、報告は、ユーザー共通の指示ファイルに従う。製品契約は `docs/architecture.md`、検証範囲は `docs/validation.md` を正本とする。

## 基本方針

- `imgui-modern-kit` は C++20 / Dear ImGui の静的 UI ライブラリ。公開ヘッダーは `include/imkit/`、実装は `src/`、テストは `tests/`、開発用 Gallery は `examples/gallery/` に置く。
- 公開API、ABI、依存、対応する Dear ImGui の版、CMake、install、package の大きな変更は、実装の前に確認する。
- PR、Issue、Release の作成は、依頼されたときだけ行う。

## 実装境界

- 複数の制作ツールで再利用できる UI だけを本リポジトリへ置く。利用製品に固有の型、用語、renderer adapter、保存処理はホスト側へ残す。
- ライブラリは Dear ImGui の Context、backend、renderer、font、texture、永続化、worker を所有・初期化しない。Theme と状態の寿命もホストが管理する。
- 公開 overload、戻り値、default 引数、ID、focus、callback、disabled、Begin/End、clipping、入力編集を維持する。入力 widget を独自の挙動で置き換えない。
- 非所有の参照を所有扱いしない。scope は対象 Context より先に破棄する。
- 新しい部品は、意味 token、Dear ImGui の公開 widget、公開 DrawList で構成する。グローバル registry やホスト固有のサービスを導入しない。
- 公開API を変えたときだけ、API compile fixture、consumer test、API inventory、英日の公開文書を合わせて更新する。

## ビルド

- ビルド出力は `C:\aokiDev\imgui-modern-kit\build` の1か所（`build/windows-debug`、Arm64 は `build/windows-arm64`）。
- 入口は `tools/Invoke-SharedBuild.ps1`（共有ラッパーを呼ぶ）。

```powershell
& 'C:\aokiDev\imgui-modern-kit\tools\Invoke-SharedBuild.ps1' -Target '<imkit|対象テスト|imkit_gallery>'
& 'C:\aokiDev\imgui-modern-kit\tools\Invoke-SharedBuild.ps1' -Tests -TestRegex '<対象テスト>'
```

- 構成が要るときは `-Configure`、Arm64 は `-Preset windows-arm64` を付ける。
- 変更した最小 target だけをビルドする。`ALL_BUILD`、`clean`、`Rebuild`、全テストは使わない。Release、install、package、SDK consumer は、変更範囲か依頼が必要とするときだけ実行する。

## 文書と資産

- 公開API、導入方法、対応環境、所有権、配布契約、ユーザーに見える動作を変えたときだけ、関連文書を更新する。英語版と日本語版は同じ構成・同じ意味にし、片方を「詳細は他方を参照」で済ませない。
- 文書は短く構造化する（表、箇条書き、見出し、コードブロック）。同じ但し書きを繰り返さない。
- 文書を変えたときだけ、`python tools/check_docs.py` を1回実行する。
- `docs/images/` は native Gallery の実 capture だけを使う。build、`out/`、ログ、capture 一式はコミットしない。
- 第三者の icon、font、画像、コードを変えたら、provenance、license、`THIRD_PARTY_NOTICES.md`、配布対象を更新する。

## 機密保持

- 回答、文書、コミット、Issue、PR、Release、ログ、生成物に、ユーザーが現在の依頼で対象情報と開示先を個別指定していない他リポジトリ情報を含めない。
- 禁止対象は他リポジトリの名称、パス、存在、進捗、計画、ブランチ、コミット、差分、設計、未公開機能、障害、検証・配布状況と、それらを推測できる表現を含む。過去の許可、包括依頼、閲覧権限は開示許可ではない。
- 他リポジトリを扱うのは、現在の依頼で対象と目的が明示された場合だけとし、情報を本リポジトリや外部公開物へ転記するには開示先の明示も必要とする。
