# AGENTS.md

このリポジトリ固有の規則だけを書く。進め方、確認の上限、止める条件、Git、報告は、ユーザー共通の指示ファイルに従う。製品契約は `docs/architecture/architecture.md`、検証範囲は `docs/reference/validation.md` を正本とする。

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

Claude Code・Codex・Antigravity 共通。`fast-build` スキルと食い違う所はこの節に従う。

- 入口は `tools/Invoke-SharedBuild.ps1` だけ。出力先は `build/windows-debug`（Arm64 は `build/windows-arm64`）だけで、`build/` 直下の他のフォルダーは使わない・増やさない。
- `Invoke-SharedBuild.ps1` は MSBuild の並列オプション（`UseMultiToolTask=true` / `EnforceProcessCountAcrossBuilds=true`）と最小ログ（`/nologo /v:minimal`）を自動付与する。`-Target` 省略や `-TestRegex` なしの `-Tests` は誤爆防止のためエラーになる（全テストは明示時のみ `-AllTests`）。
- 変更パスから最小 target を決めるため、原則として `python tools/affected_targets.py <変更パス…>` を使う（または `-ChangedFiles <変更パス…> -BuildAndTest`）。
- 複数ファイル変更や Gallery 編集中の素早い構文・型確認には `-CompileOnly <cppパス…>` を使う（リンク抜きで数秒で終わる）。
- `imkit_gallery` と `imkit_node_editor_gallery` は依存分離済み。Node Editor 連携画面を確認するときだけ `-Target imkit_gallery,imkit_node_editor_gallery` の両方を指定する。

### 手順（最小算出 → ビルド1回 + テスト1回）

1. 編集をすべて終えてから、`python tools/affected_targets.py <自分が変えたパス…>` で対象 target と `-TestRegex` を算出する（または `-ChangedFiles` を渡す）。
2. 算出した target を全部まとめて 1 回でビルドし、成功後に同じ `-TestRegex` でテストを 1 回実行する。
3. 失敗したら、出たエラーを全部直してから、必要なら `-CompileOnly` でコンパイルエラーを潰し、同じコマンドを 1 回だけやり直す。ソースを変えずに同じ target を再ビルドしない。

```powershell
# 例1: 変更パスから自動で最小ビルド + テストを実行
& 'C:\aokiDev\imgui-modern-kit\tools\Invoke-SharedBuild.ps1' -ChangedFiles src/theme.cpp -BuildAndTest

# 例2: 手動で最小 target をまとめてビルド + テスト
& 'C:\aokiDev\imgui-modern-kit\tools\Invoke-SharedBuild.ps1' -Target imkit_theme_test,imkit_api_compile
& 'C:\aokiDev\imgui-modern-kit\tools\Invoke-SharedBuild.ps1' -Tests -TestRegex '^imkit\.(theme|api_compile)$'

# 例3: 編集中のリンク抜き高速構文チェック
& 'C:\aokiDev\imgui-modern-kit\tools\Invoke-SharedBuild.ps1' -CompileOnly examples/gallery/charts_page.cpp
```

- `imkit_gallery` は、Gallery を変えたときと capture が要るときだけビルドする。ライブラリだけの変更では作らない。
- 文書・規則・コメント・`website/` だけの変更ではビルドしない（`python tools/check_docs.py` のみ）。
- `ALL_BUILD`、`clean`、`Rebuild`、全テスト、成功後の確認ビルドはしない。
- Arm64（`-Preset windows-arm64`）、Release、install、package、SDK consumer は、依頼されたときだけ実行する。

## ビルド時間短縮の設計ルール（恒久ガード）

- **公開ヘッダー (`include/imkit/`)**: 既存利用者の互換性を最優先しつつ、新規・編集時は前方宣言を優先し、不要な STL / 他モジュール `#include` や重い `inline` 実装を足さない。
- **内部実装 (`src/`)**: 共通ヘルパーは `*_internal.h` に宣言だけを置き、実装本体や `static` 配列バッファは `*_internal.cpp` / `src/show/common.cpp` へ分離する。巨大な静的データ配列は `imgui.h` 非依存の独立 `*_data.cpp`（例：`src/icons_data.cpp`）へ分離する。1 ファイルが 1,500〜2,000 行を超える肥大化を避け、責務単位で分割する。
- **Gallery (`examples/gallery/`)**: 各ページ実装は `gallery_page_host.h` とそのページに必要な最小ヘッダーだけを `#include` し、`gallery.h` や他ページヘッダーを増やさない。
- **テスト (`tests/`)**: 単体テストは対象モジュールの個別ヘッダーを直接 `#include` し、アンブレラヘッダー (`imkit/imkit.h`, `imkit/show.h`) を足さない（`*_api_compile.cpp`・`context_smoke.cpp`・`design_system.cpp` は除く）。`*_api_compile` ターゲットにはヘッダー自己完結性検証のため PCH を付けない。
- **CMake と SDK 境界**: 新規ターゲットには `/Z7` (`imkit_configure_target_build`) を適用する。PCH (`IMKIT_ENABLE_PCH`) 等の開発専用高速化はトップレベル開発ビルド・`windows-debug` プリセットに閉じ、外部利用者の `add_subdirectory`・Release SDK（`imkit` 本体の再パック含む）へ影響させない。
- **構造変更時のチェック**: `tests/`・`examples/gallery/`・`src/*internal.h`・CMake を変更したときは `python tools/check_build_hygiene.py` を 1 回実行する。

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
