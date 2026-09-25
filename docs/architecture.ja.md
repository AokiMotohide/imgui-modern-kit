# Architecture / 設計

[English](architecture.md) · [文書カタログ](documentation-catalog.ja.md)

### 所有境界

ImKitはDear ImGui上で再利用できるUIを提供する静的ライブラリです。Dear ImGui Context・backend・renderer・font atlas・texture・アプリケーションdata・保存・Undo・workerを初期化・所有しません。描画APIはホストが開始したframe内で呼びます。

公開wrapperはDear ImGuiのBegin/End、focus、callback、disabled、clipping、ID、入力編集の契約を維持します。独自部品は標準widgetと公開DrawListで構成し、入力処理そのものを置き換えません。

## v3 platform境界

coreはplatform・renderer非依存です。公式Dear ImGui backendをlinkするのはGallery hostだけです。optional previewは明示的に作成し、渡されたdevice/Context上で動作します。accessibility adapterもsnapshotをコピーし、型付きactionをホストへ返します。

### module構成

| module | 主な責務 | 所有者・制約 |
|---|---|---|
| `imkit` | Theme、native API、widget、composition | host-owned値と非所有viewを受け取る |
| `node_editor` | graph snapshot表示と有限容量のedit request | graph保存、評価、Undo、revision受理はホスト |
| Editor Core / Video / CG | 編集UI、provider query、event生成 | scene/media data、selection、Undo、workerはホスト |
| Preview | 明示的に初期化するoptional offscreen描画 | GPU resourceを所有する場合もdevice、Context、submissionはホスト |
| WindowFrame | frame配置計算、描画、型付きoperation返却 | native windowとOS操作はホスト。OS adapterは別target |

基本`imkit`はEditor Coreへ依存しません。各moduleのtargetとheaderは[Editor Suite](editor-suite.ja.md)および[実例recipe](examples-recipes.ja.md)を参照してください。

### Themeと状態

Themeは`MakeTheme()`が返す値としてホストが保持し、グローバルregistryや選択状態はありません。FontSetは非所有参照です。AnimationStateもホストが寿命を管理し、Contextより先に関連scopeを終了してください。

## Extension policy / 拡張方針

Dear ImGuiの固定版公開宣言と比較してoverload、default、戻り値、callbackの意味を維持します。新しい版への対応はversion guardを緩めるだけで完了せず、adapter・style・font契約とcompile fixtureを確認します。寸法や状態容量はboundedにし、利用アプリ固有の型やserviceをlibraryへ導入しません。

## Editor moduleの所有権

Node Editorはgraph snapshotを借用して有限容量requestを返します。graph保存、評価、revision受理、Undoはホスト所有です。Editor Core・Video・CGはprovider viewからUIを構成しますが、scene/media dataやselectionを保持しません。詳細は[Editor Suite](editor-suite.ja.md)を参照してください。

履歴付きの設計記録は[保守用文書一覧](documentation-catalog.ja.md)から参照できます。現行契約は公開headerと英語版の[architecture](architecture.md)を基準にしてください。
