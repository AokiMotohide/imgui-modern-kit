#!/usr/bin/env python3
"""Generate ImKit 3.2 reference declarations directly from the public headers."""
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]
GROUPS={
 'workflow.h':(['WorkspaceTab','ChoiceItem','ChoiceGroupOptions','HierarchyRowView','HierarchyRowAction'],['WorkspaceTabs','ChoiceGroup','HierarchyGroupHeader','HierarchyRow','BeginInspectorCard','EndInspectorCard','SettingToggleRow']),
 'components.h':(['CompactActionRowOptions','CompactActionRowRequest'],['DragVector3WithUnit','CompactActionRow']),
 'toast.h':(['ToastPosition','ToastPhase','ToastEventKind','ToastView','ToastEvent','ToastEventBuffer','ToastViewportOptions','ToastViewportState'],['UpdateToastViewport','ToastViewport']),
 'window_frame.h':(['WindowFrameContent'],[]),
}
def declarations():
 entries=[]
 for header,(types,functions) in GROUPS.items():
  source=(ROOT/'include/imkit'/header).read_text(encoding='utf-8')
  for name in types:
   match=re.search(r'(?:struct|enum class) '+re.escape(name)+r'\b[^;{]*\{',source)
   if not match: raise ValueError(name)
   begin=match.start();end=match.end();depth=1
   while depth:
    if source[end]=='{':depth+=1
    if source[end]=='}':depth-=1
    end+=1
   entries.append(dict(header='imkit/'+header,name=name,kind='type',declaration=source[begin:end+1]))
  for name in functions:
   match=re.search(r'(?:bool|void|StableId|HierarchyRowAction|CompactActionRowRequest) '+name+r'\([^;]+;',source)
   if not match: raise ValueError(name)
   entries.append(dict(header='imkit/'+header,name=name,kind='function',declaration=match.group()))
 return entries
def main():
 entries=declarations()
 folder=ROOT/'docs/reference'
 (folder/'v3.2-api.json').write_text(json.dumps(entries,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 for locale,name in [('en','v3.2-api.md'),('ja','3.2追加API.md')]:
  text=('# ImKit 3.2 API reference\n\n[日本語](3.2追加API.md)\n\n' if locale=='en' else '# ImKit 3.2 追加API\n\n[English](v3.2-api.md)\n\n')
  text+=('Declarations below are extracted from the public headers, including defaults and return types. Re-run `python tools/generate_v32_api.py` after editing a covered declaration.\n\n## State and lifetime\n\n' if locale=='en' else '以下は公開ヘッダーから抽出した宣言です。既定値と戻り値の型を含みます。対象の宣言を変更したときは `python tools/generate_v32_api.py` を実行します。\n\n## 状態と寿命\n\n')
  text+=('The application owns all state, values, IDs, strings, spans, icon atlases and textures. Borrowed views must stay valid during each call; textures must also remain valid until the submitted draw commands finish. Accessibility text must also survive publication of the host semantic frame. Keep each state instance associated with its context and destroy scopes before the context.\n\n`WorkspaceTabs` and `ChoiceGroup` return a requested stable ID (zero means no request). `HierarchyRow` returns an action; `HierarchyGroupHeader` edits the supplied open value and writes the optional action flag. `SettingToggleRow` returns a toggle request: the host must flip its own value. `CompactActionRow` returns `None`, `Primary` or `Settings`; the host handles each operation. `DragVector3WithUnit` edits the supplied three floats and returns whether any axis changed. Disabled controls do not issue edit requests.\n\nAlways call `EndInspectorCard`, even when `BeginInspectorCard` returns false. This differs from `BeginSettingRow`, whose End is called only after a true return.\n\nToast strings are borrowed; state stores IDs and timing, not strings or callbacks. Process the bounded `ToastEventBuffer`, then remove dismissed/expired IDs in the host. Reset buffer count/overflow for each frame and call `ToastViewport` once per viewport per frame with monotonic host time. Keep the same ID when changing Loading to Message/Success. See the [toast guide](../components/toasts.md) for duration, pause and overflow rules.\n\n## Binary compatibility\n\n`WindowFrameContent` now has an `iconTexture` member. Recompile applications and every linked ImKit static library against the 3.2 headers. Do not mix 3.1 objects with 3.2 structures. The Dear ImGui revision and matching configuration requirement are unchanged.\n\n' if locale=='en' else '状態、値、ID、文字列、span、アイコンatlas、textureはアプリが所有します。参照先は各呼び出し中に有効に保ちます。textureは送信した描画コマンドが完了するまで保持します。アクセシビリティ用の文字列はホストのsemantic frameを公開する間も保持します。状態は使用するContextごとに管理し、scopeをContextより先に破棄します。\n\n`WorkspaceTabs` と `ChoiceGroup` は選択要求のIDを返します。0は要求なしです。`HierarchyRow` は操作種別を返し、`HierarchyGroupHeader` は渡された開閉値を更新して任意の操作フラグを書き込みます。`SettingToggleRow` は切り替え要求を返すので、アプリ側で値を反転します。`CompactActionRow` の `None`、`Primary`、`Settings` に応じてアプリが処理します。`DragVector3WithUnit` は渡された3要素を編集し、いずれかが変化したかを返します。disabledの操作から編集要求は発生しません。\n\n`BeginInspectorCard` がfalseでも必ず `EndInspectorCard` を呼びます。`BeginSettingRow` のEndはtrueの場合だけ必要です。\n\nトーストの文字列は参照で、状態にはIDと時間を保持します。容量付き `ToastEventBuffer` を処理してから、削除・期限切れIDをアプリのqueueから取り除きます。countとoverflowを毎フレーム初期化し、単調増加するホスト時刻で `ToastViewport` をviewportごとに1回呼びます。LoadingからMessage/Successへ切り替える際もIDを維持します。期限、停止、overflowの規則は[トーストガイド](../components/トースト.md)に記載しています。\n\n## バイナリ互換性\n\n`WindowFrameContent` に `iconTexture` が追加されています。3.2のヘッダーを使い、アプリとリンクするImKit静的ライブラリを再コンパイルしてください。3.1のobjectと3.2の構造体を混在させないでください。Dear ImGuiの対応revisionと設定一致の要件は変わりません。\n\n')
  for entry in entries:
   text+=f'## {entry["name"]}\n\nHeader: `<{entry["header"]}>`\n\n```cpp\n{entry["declaration"]}\n```\n\n'
  (folder/name).write_text(text.rstrip()+'\n',encoding='utf-8')
if __name__=='__main__':main()
