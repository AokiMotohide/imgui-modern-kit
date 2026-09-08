import pathlib,re,json
root=pathlib.Path(__file__).resolve().parents[1]
header=root/'build/windows-debug/_deps/imkit_imgui_source-src/imgui.h'
text=header.read_text(encoding='utf-8')
body=text[text.index('namespace ImGui\n{'):text.index('// [SECTION] Flags & Enumerations',text.index('namespace ImGui\n{'))]
custom={'Button','Checkbox','SliderFloat','InputText','Selectable','ProgressBar','BeginTabItem','TreeNodeEx','TreeNodeExV'}
excluded_prefix=('CreateContext','DestroyContext','GetCurrentContext','SetCurrentContext','GetIO','GetPlatformIO','NewFrame','EndFrame','Render','GetDrawData','Show','StyleColors','Log','Debug','SetAllocator','GetAllocator','MemAlloc','MemFree','LoadIni','SaveIni','UpdatePlatform','RenderPlatform','DestroyPlatform','FindViewport')
records=[]; names=[]
for line in body.splitlines():
    line=re.sub(r"IM_FMT(?:ARGS|LIST)\(\d+\)","",line.split("//")[0])
    m=re.match(r'\s*IMGUI_API\s+(.+?[\s*&])(\w+)\((.*)\)\s*;',line)
    if not m:
        if line.strip().startswith("IMGUI_API"):raise RuntimeError("Unparsed API declaration: "+line)
        continue
    ret,name,args=m.groups();ret=ret.strip()
    excluded=name.startswith(excluded_prefix)
    # Legacy columns retained as native passthrough, styled through table-adjacent tokens.
    records.append({'name':name,'signature':f'{ret} {name}({args})','included':not excluded,'implementation':'wrapper' if name in custom else 'native alias' if not excluded else 'host / debug responsibility'})
    if not excluded and name not in custom and name not in names:names.append(name)
(root/'include/imkit/native.h').write_text('#pragma once\n#include <imgui.h>\n#if IMGUI_VERSION_NUM != 19291\n#error "ImKit 0.2 requires Dear ImGui 1.92.9b; rebuild with the documented version."\n#endif\n#ifndef IMGUI_HAS_DOCK\n#error "ImKit 0.2 requires the documented docking branch."\n#endif\nnamespace imkit {\n// Exact native overload sets and defaults; visual styling comes from ApplyTheme.\n'+''.join('using ImGui::'+n+';\n' for n in names)+'}\n',encoding='utf-8')
(root/'docs/api-inventory.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(f'{sum(r["included"] for r in records)} included overloads; {len(names)} native names; {len(records)} inventoried declarations')


# Compile/link every included overload using independent declarations carrying
# the exact pinned upstream parameter/default syntax (no wrapper self-test).
prototypes=[];pointers=[];rows=[]
def category(name):
    if re.search(r'^(Drag(Float|Int|Scalar)|V?Slider|Input(Float|Int|Double|Scalar))',name):return 'Numeric / Units'
    if re.search(r'InputText|Color|Image|Plot',name):return 'Input / Media'
    if re.search(r'Tree|Tab|Collapsing',name):return 'Hierarchy / Table'
    if re.search(r'Popup|Tooltip|Menu|Window|Child|Dock|Scroll',name):return 'Overlay / Layout'
    if re.search(r'Text|Button|Checkbox|Radio|Selectable|Combo|ListBox|Progress|Bullet',name):return 'Basic / Selection'
    return 'Shared layout / helpers'
for i,r in enumerate(records):
    sig=r['signature'];name=r['name'];cat=category(name)
    r['category']=cat;r['version']='1.92.9b docking (19291)'
    helper = name.startswith(('Get', 'Set', 'Is', 'Push', 'Pop', 'Calc', 'ColorConvert')) and name not in ('SetTooltip', 'SetTooltipV', 'SetItemTooltip', 'SetItemTooltipV')
    if not r['included']:
        r['implementation'] = 'excluded: host / debug'
        r['visual_scope'] = 'No imkit rendering'
    elif helper:
        r['implementation'] = 'transparent public helper'
        r['visual_scope'] = 'No standalone appearance'
    elif name in ('Selectable', 'BeginTabItem', 'TreeNodeEx', 'TreeNodeExV'):
        r['implementation'] = 'native behavior + public DrawList marker'
        r['visual_scope'] = 'Theme + selection underline'
    else:
        r['implementation'] = 'semantic Theme + native behavior'
        r['visual_scope'] = 'Current themed native rendering'
    r['limitations'] = 'See docs/validation.md; native behavior is not exhaustively retested'

    if r['included']:
        prototypes.append(sig.replace(name+'(',f'overload_{i}(',1)+';')
        pointers.append(f'decltype(&reference::overload_{i}) volatile api_{i} = static_cast<decltype(&reference::overload_{i})>(&imkit::{name});')
    rows.append('| `'+sig.replace('|','\\|')+'` | '+('`imkit::'+name+'`' if r['included'] else 'Excluded')+' | '+r['implementation']+'<br>'+r['visual_scope']+' | '+cat+' | '+('signature compile/link; shared catalog helper' if helper and r['included'] else 'signature compile/link; category GPU capture' if r['included'] else 'host responsibility')+' |')
(root/'tests/api_compile.cpp').write_text('#include <imkit/imkit.h>\nnamespace reference {\n'+'\n'.join(prototypes)+'\n}\n'+'\n'.join(pointers)+'\nint main(){return 0;}\n',encoding='utf-8')
(root/'docs/api-inventory.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(root/'docs/api-coverage.md').write_text('# Public API coverage / 公開API対応表\n\nGenerated from pinned Dear ImGui 1.92.9b docking (19291). Every row is one overload. Native aliases preserve exact defaults, callbacks, flags and Begin/End contracts. ApplyTheme is required for Precision Layers styling. Helpers have no visual output of their own. Every included row requires the pinned version; known limits are in [validation](validation.md). Composite controls are listed separately in [the guide](guide.md#components).\n\n固定版の宣言をoverloadごとに記録。native aliasは標準実装への直接公開で、共通Themeが外観を適用します。非描画補助には単独の外観はありません。全overloadを個別操作したという意味ではなく、署名compile/linkとカテゴリ代表操作を組み合わせて検証します。\n\nSelection/tab wrappers add an underline through the current DrawList without submitting another item. Six original wrapper signatures are retained. Context/frame/renderer/platform, debug tools, logging, allocation and ini persistence are host responsibilities. Obsolete declarations outside the current namespace and internal APIs are excluded.\n\n| Dear ImGui signature | imkit API | Method / 実装 | Catalog / カテゴリ | Evidence scope / 検証範囲 |\n|---|---|---|---|---|\n'+'\n'.join(rows)+'\n',encoding='utf-8')
