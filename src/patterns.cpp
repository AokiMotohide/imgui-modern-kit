#include <imkit/patterns.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cctype>
#include <cstdio>

namespace imkit {
namespace {
const char* Text(ComponentOptions o,const char* key,const char* fallback) { return o.locale?o.locale->Text(key,fallback):fallback; }
bool Match(const char* text,const char* query) {
    if(!*query) return true;
    for(;*text;++text) { auto a=text,b=query;
        while(*a && *b && std::tolower(static_cast<unsigned char>(*a))==std::tolower(static_cast<unsigned char>(*b))) { ++a; ++b; }
        if(!*b) return true;
    } return false;
}
void Node(ComponentOptions o,const char* name,accessibility::SemanticRole role,StableId id=0) {
    if(!o.accessibility) return;
    accessibility::SemanticNode node; node.id=id?id:ImGui::GetItemID(); node.parent=o.parent; node.name=name; node.role=role;
    accessibility::AnnotateLastItem(*o.accessibility,node);
}
void Copy(char* destination,std::size_t capacity,const char* source) { if(capacity) { auto n=std::min(capacity-1,std::strlen(source)); std::memcpy(destination,source,n); destination[n]=0; } }
void Apply(const DataProvider& p,const DataEvent& event) { if(p.apply) p.apply(p.user,event); }
}
bool SearchField(const char* id,char* text,std::size_t capacity,ComponentOptions o) {
    ImGui::PushID(id); ImGui::SetNextItemWidth(std::max(1.f,ImGui::GetContentRegionAvail().x-ImGui::GetFrameHeight()-ImGui::GetStyle().ItemSpacing.x));
    bool changed=ImGui::InputTextWithHint("##search",Text(o,"search","Search"),text,capacity);
    Node(o,Text(o,"search","Search"),accessibility::SemanticRole::TextField);
    ImGui::SameLine(); ImGui::BeginDisabled(!*text);
    if(ActionButton("X##clear",ActionVariant::Ghost,{},o)) { text[0]=0; changed=true; }
    if(ImGui::IsItemHovered() || ImGui::IsItemFocused()) ImGui::SetTooltip("%s",Text(o,"clear_search","Clear search"));
    ImGui::EndDisabled(); ImGui::PopID(); return changed;
}
StableId CommandPalette(const char* id,CommandPaletteState& s,std::span<const Command> commands,ComponentOptions o) {
    if(s.open && !ImGui::IsPopupOpen(id)) ImGui::OpenPopup(id);
    if(!s.open) return 0;
    ImGui::SetNextWindowSize({std::min(560.f,ImGui::GetMainViewport()->WorkSize.x-24),0},ImGuiCond_Appearing);
    StableId result=0;
    if(ImGui::BeginPopupModal(id,&s.open,ImGuiWindowFlags_AlwaysAutoResize)) {
        if(ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        if(SearchField("search",s.search,sizeof(s.search),o)) s.focused=0;
        int count=0; for(auto& c:commands) if(Match(c.label,s.search)) ++count;
        if(ImGui::IsKeyPressed(ImGuiKey_DownArrow)) ++s.focused;
        if(ImGui::IsKeyPressed(ImGuiKey_UpArrow)) --s.focused;
        s.focused=std::clamp(s.focused,0,std::max(0,count-1));
        int index=0;
        for(auto& c:commands) if(Match(c.label,s.search)) {
            ImGui::PushID(static_cast<int>(c.id>>32)); ImGui::PushID(static_cast<int>(c.id));
            ImGui::BeginDisabled(c.disabled);
            bool hit=ImGui::Selectable(c.label,index==s.focused);
            if(!c.disabled && (hit || (index==s.focused && ImGui::IsKeyPressed(ImGuiKey_Enter)))) result=c.id;
            if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemFocused())
                ImGui::SetTooltip("%s",c.disabled?c.disabledReason:c.shortcut);
            ImGui::EndDisabled(); ImGui::PopID(); ImGui::PopID(); ++index;
        }
        if(!count) EmptyState(Text(o,"no_results","No results"));
        if(result || ImGui::IsKeyPressed(ImGuiKey_Escape)) { s.open=false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    } return result;
}
int Breadcrumbs(const char* id,std::span<const char* const> labels,ComponentOptions o) {
    ImGui::PushID(id); int result=-1;
    for(int visual=0;visual<static_cast<int>(labels.size());++visual) {
        int i=o.locale?o.locale->VisualIndex(visual,static_cast<int>(labels.size())):visual;
        auto width=ImGui::CalcTextSize(labels[i]).x+2*ImGui::GetStyle().FramePadding.x;
        if(visual && ImGui::GetContentRegionAvail().x>width+20) { ImGui::SameLine(); ImGui::TextUnformatted(o.locale && o.locale->direction==TextDirection::RTL?"<":">"); ImGui::SameLine(); }
        ImGui::PushID(i); if(ActionButton(labels[i],ActionVariant::Ghost,{},o)) result=i; ImGui::PopID();
    } ImGui::PopID(); return result;
}
SplitButtonResult SplitButton(const char* id,const char* label,ComponentOptions o) {
    ImGui::PushID(id); auto result=SplitButtonResult::None;
    if(ActionButton(label,ActionVariant::Primary,{},o)) result=SplitButtonResult::Primary;
    ImGui::SameLine(0,1); if(IconButton("menu",ImGuiDir_Down,Text(o,"menu","Menu"),o)) result=SplitButtonResult::Menu;
    ImGui::PopID(); return result;
}
StableId ResponsiveToolbar(const char* id,ToolbarState& s,std::span<const Command> commands,ComponentOptions o) {
    const auto parent=o.parent; const auto group=ImGui::GetID(id); auto origin=ImGui::GetCursorScreenPos();
    if(o.accessibility) o.parent=group;
    ImGui::PushID(id); StableId result=0;
    s.focused=std::clamp(s.focused,0,std::max(0,static_cast<int>(commands.size())-1));
    if(!commands.empty() && commands[s.focused].disabled) {
        for(int i=0;i<static_cast<int>(commands.size());++i) if(!commands[i].disabled) {s.focused=i;break;}
    }
    float right=ImGui::GetCursorScreenPos().x+ImGui::GetContentRegionAvail().x;
    for(int visual=0;visual<static_cast<int>(commands.size());++visual) {
        int i=o.locale?o.locale->VisualIndex(visual,static_cast<int>(commands.size())):visual;
        auto& c=commands[i]; auto width=ImGui::CalcTextSize(c.label).x+2*ImGui::GetStyle().FramePadding.x;
        if(visual && ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<right) ImGui::SameLine();
        ImGui::PushID(static_cast<int>(c.id)); ImGui::PushTabStop(i==s.focused);
        if(i==s.focused && s.focusPending) { ImGui::SetKeyboardFocusHere(); s.focusPending=false; }
        ImGui::BeginDisabled(c.disabled);
        if(ActionButton(c.label,ActionVariant::Ghost,{},o)) result=c.id;
        if(ImGui::IsItemFocused()) {
            int next=i;
            if(ImGui::IsKeyPressed(ImGuiKey_RightArrow)) next=(i+1)%static_cast<int>(commands.size());
            if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) next=(i+static_cast<int>(commands.size())-1)%static_cast<int>(commands.size());
            if(ImGui::IsKeyPressed(ImGuiKey_Home)) next=0;
            if(ImGui::IsKeyPressed(ImGuiKey_End)) next=static_cast<int>(commands.size())-1;
            if(next!=i) {
                int direction=(ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_End))?-1:1;
                for(int skip=0;commands[next].disabled && skip<static_cast<int>(commands.size());++skip)
                    next=(next+direction+static_cast<int>(commands.size()))%static_cast<int>(commands.size());
            }
            if(next!=i) { s.focused=next; s.focusPending=true; }
        }
        if(c.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s",c.disabledReason);
        ImGui::EndDisabled(); ImGui::PopTabStop(); ImGui::PopID();
    }
    if(o.accessibility) { accessibility::SemanticNode node; node.id=group; node.parent=parent; node.name=id;
        node.role=accessibility::SemanticRole::Toolbar; node.minimum=origin; node.maximum=ImGui::GetItemRectMax(); o.accessibility->Add(node); }
    ImGui::PopID(); return result;
}
StableId Toolbar(const char* id,ToolbarState& s,std::span<const Command> commands,ComponentOptions o) {
    float available=ImGui::GetContentRegionAvail().x,used=0;
    float overflow=ImGui::CalcTextSize(Text(o,"more","More")).x+2*ImGui::GetStyle().FramePadding.x+ImGui::GetStyle().ItemSpacing.x;
    std::size_t count=0;
    for(;count<commands.size();++count) {
        float width=ImGui::CalcTextSize(commands[count].label).x+2*ImGui::GetStyle().FramePadding.x+ImGui::GetStyle().ItemSpacing.x;
        if(used+width+(count+1<commands.size()?overflow:0)>available) break;
        used+=width;
    }
    ImGui::PushID(id);
    auto result=ResponsiveToolbar("visible",s,commands.first(count),o);
    if(count<commands.size()) {
        if(count) ImGui::SameLine();
        if(ActionButton(Text(o,"more","More"),ActionVariant::Ghost,{},o)) ImGui::OpenPopup("overflow");
        if(auto selected=Menu("overflow",commands.subspan(count),o)) result=selected;
    }
    ImGui::PopID(); return result;
}
bool DialogLauncher(const char* label,DialogState& s,ComponentOptions o) {
    if(s.restoreFocus) { ImGui::SetKeyboardFocusHere(); s.restoreFocus=false; }
    if(ActionButton(label,ActionVariant::Secondary,{},o)) { s.open=true; s.initialFocus=true; return true; } return false;
}
bool BeginDialog(const char* id,DialogState& s,ComponentOptions o) {
    if(s.open && !ImGui::IsPopupOpen(id)) ImGui::OpenPopup(id);
    if(!s.open) return false;
    bool visible=ImGui::BeginPopupModal(id,nullptr,ImGuiWindowFlags_AlwaysAutoResize);
    if(visible) {
        if(ImGui::IsKeyPressed(ImGuiKey_Escape)) { s.open=false; s.restoreFocus=true; ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return false; }
        if(s.initialFocus) { ImGui::SetKeyboardFocusHere(); s.initialFocus=false; }
        if(o.accessibility) { accessibility::SemanticNode n; n.id=ImGui::GetID(id); n.parent=o.parent; n.role=accessibility::SemanticRole::Dialog;
            n.name=id; n.minimum=ImGui::GetWindowPos(); auto size=ImGui::GetWindowSize(); n.maximum={n.minimum.x+size.x,n.minimum.y+size.y}; o.accessibility->Add(n); }
    } return visible;
}
void EndDialog() { ImGui::EndPopup(); }
DialogResult AlertDialog(const char* id,const char* message,DialogState& s,ComponentOptions o) {
    auto result=DialogResult::None;
    if(BeginDialog(id,s,o)) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::min(440.f,ImGui::GetMainViewport()->WorkSize.x-48)); ImGui::TextUnformatted(message); ImGui::PopTextWrapPos();
        if(ActionButton(Text(o,"cancel","Cancel"),ActionVariant::Secondary,{},o)) result=DialogResult::Cancel;
        ImGui::SameLine(); if(ActionButton(Text(o,"accept","Accept"),ActionVariant::Primary,{},o)) result=DialogResult::Accept;
        if(result!=DialogResult::None) { s.open=false; s.restoreFocus=true; ImGui::CloseCurrentPopup(); }
        EndDialog();
    } return result;
}
bool BeginPopover(const char* id) { return ImGui::BeginPopup(id,ImGuiWindowFlags_AlwaysAutoResize); }
void EndPopover() { ImGui::EndPopup(); }
StableId Menu(const char* id,std::span<const Command> commands,ComponentOptions o) {
    StableId result=0; if(BeginPopover(id)) { for(auto& c:commands) {
        ImGui::PushID(static_cast<int>(c.id)); if(ImGui::MenuItem(c.label,c.shortcut,false,!c.disabled)) result=c.id;
        Node(o,c.label,accessibility::SemanticRole::MenuItem); ImGui::PopID();
    } EndPopover(); } return result;
}
bool BeginFormField(const char* id,const FormFieldInfo& f,ComponentOptions o) {
    ImGui::PushID(id); ImGui::BeginGroup();
    ImGui::TextWrapped("%s%s",f.label,f.required?" *":"");
    if(*f.hint) ImGui::TextWrapped("%s",f.hint);
    if(*f.validation) ImGui::TextWrapped("! %s",f.validation);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); (void)o; return true;
}
void EndFormField() { ImGui::EndGroup(); ImGui::PopID(); }
void Progress(const char* id,float fraction,const char* label,ComponentOptions o) {
    ImGui::PushID(id); float value=fraction;
    if(value<0) value=o.theme && o.theme->motion.reducedMotion?.5f:static_cast<float>(std::fmod(ImGui::GetTime(),1.));
    ImGui::ProgressBar(std::clamp(value,0.f,1.f),{-1,0},label); Node(o,label,accessibility::SemanticRole::Progress,ImGui::GetID(id)); ImGui::PopID();
}
void Spinner(const char* id,ComponentOptions o) {
    float h=ImGui::GetFrameHeight(); auto p=ImGui::GetCursorScreenPos(); ImGui::Dummy({h,h});
    auto* draw=ImGui::GetWindowDrawList(); float phase=o.theme && o.theme->motion.reducedMotion?0.f:static_cast<float>(ImGui::GetTime()*4);
    draw->PathArcTo({p.x+h/2,p.y+h/2},h*.3f,phase,phase+4.7f,20); draw->PathStroke(ImGui::GetColorU32(ImGuiCol_Text),2.f,0);
    Node(o,Text(o,"loading","Loading"),accessibility::SemanticRole::Status,ImGui::GetID(id));
}
void Skeleton(const char* id,ImVec2 size,ComponentOptions o) {
    auto p=ImGui::GetCursorScreenPos(); ImGui::Dummy(size);
    ImGui::GetWindowDrawList()->AddRectFilled(p,{p.x+size.x,p.y+size.y},ImGui::GetColorU32(ImGuiCol_FrameBg),ImGui::GetStyle().FrameRounding);
    Node(o,Text(o,"loading","Loading"),accessibility::SemanticRole::Status,ImGui::GetID(id));
}
void EmptyState(const char* message) { ImGui::TextWrapped("%s",message); }
void LoadingState(const char* message,ComponentOptions o) { Spinner("loading",o); ImGui::SameLine(); ImGui::TextWrapped("%s",message); }
bool ErrorState(const char* message,ComponentOptions o) { ImGui::TextWrapped("! %s",message); return ActionButton(Text(o,"retry","Retry"),ActionVariant::Secondary,{},o); }
const char* ToastRegion(std::span<const Notification> notifications,double now,ComponentOptions o) {
    const char* result=nullptr; for(auto& n:notifications) if(NotificationCard(n,now,o.theme)) result=n.id; return result;
}
bool Pagination(const char* id,int& page,int count,ComponentOptions o) {
    ImGui::PushID(id); int before=page; page=std::clamp(page,0,std::max(0,count-1));
    ImGui::BeginDisabled(page==0); if(ActionButton(Text(o,"previous","Previous"),ActionVariant::Ghost,{},o)) --page; ImGui::EndDisabled();
    ImGui::SameLine(); ImGui::Text("%d / %d",count?page+1:0,std::max(0,count)); ImGui::SameLine();
    ImGui::BeginDisabled(page>=count-1); if(ActionButton(Text(o,"next","Next"),ActionVariant::Ghost,{},o)) ++page; ImGui::EndDisabled(); ImGui::PopID(); return page!=before;
}
bool BeginAdaptiveSplitLayout(const char* id,AdaptiveSplitState& s,float minimum,float fraction) {
    ImGui::PushID(id); auto available=ImGui::GetContentRegionAvail(); s.stacked=available.x<minimum*2;
    s.firstVisible=ImGui::BeginChild("first",s.stacked?ImVec2(0,std::max(1.f,available.y*.45f)):ImVec2(std::max(1.f,available.x*std::clamp(fraction,.1f,.9f)),0)); return s.firstVisible;
}
bool NextAdaptiveSplitPane(AdaptiveSplitState& s) { ImGui::EndChild(); if(!s.stacked) ImGui::SameLine(); return ImGui::BeginChild("second",{0,0}); }
void EndAdaptiveSplitLayout() { ImGui::EndChild(); ImGui::PopID(); }
VisibleRange QueryVisibleRange(int count,float scroll,float height,float rowHeight,int overscan) {
    if(count<=0 || !std::isfinite(scroll) || !std::isfinite(height) || !std::isfinite(rowHeight) || rowHeight<=0 || height<=0) return {};
    int first=static_cast<int>(std::clamp(std::floor(double(scroll)/rowHeight)-std::max(0,overscan),0.,double(count)));
    int end=static_cast<int>(std::clamp(std::ceil((double(std::max(0.f,scroll))+height)/rowHeight)+std::max(0,overscan),double(first),double(count)));
    return {first,end-first};
}
StableId VirtualList(const char* id,const ListProvider& p,StableId selected,float height,ComponentOptions o) {
    StableId result=0; if(ImGui::BeginChild(id,{0,height})) {
        if(!p.count) EmptyState(Text(o,"empty","No items"));
        else if(p.id && p.label) { ImGuiListClipper clip; clip.Begin(p.count,ImGui::GetTextLineHeightWithSpacing());
            while(clip.Step()) { if(p.query) p.query(p.user,{clip.DisplayStart,clip.DisplayEnd-clip.DisplayStart});
                for(int i=clip.DisplayStart;i<clip.DisplayEnd;++i) { auto key=p.id(p.user,i); ImGui::PushID(static_cast<int>(key>>32)); ImGui::PushID(static_cast<int>(key));
                    if(ImGui::Selectable(p.label(p.user,i),key==selected)) result=key; ImGui::PopID(); ImGui::PopID(); }
            }
        }
    } ImGui::EndChild(); return result;
}
void DataTable(const char* id,DataTableState& s,const DataProvider& p,std::span<const DataColumn> columns,float height,ComponentOptions o) {
    ImGui::PushID(id);
    if(SearchField("filter",s.filter,sizeof(s.filter),o)) Apply(p,{DataAction::Filter,0,0,0,0,false,false,false,s.filter});
    if(p.count<=0) { EmptyState(Text(o,"empty","No rows")); ImGui::PopID(); return; }
    if(columns.empty() || !p.id || !p.cell) { ImGui::PopID(); return; }
    s.focusedRow=std::clamp(s.focusedRow,0,p.count-1); s.focusedColumn=std::clamp(s.focusedColumn,0,static_cast<int>(columns.size())-1);
    if(ImGui::BeginTable("grid",static_cast<int>(columns.size()),ImGuiTableFlags_Resizable|ImGuiTableFlags_Hideable|ImGuiTableFlags_Sortable|ImGuiTableFlags_ScrollY|ImGuiTableFlags_RowBg,{0,height})) {
        const auto gridID=ImGui::GetID("##semantic-grid");
        if(o.accessibility) { accessibility::SemanticNode node; node.id=gridID; node.parent=o.parent; node.role=accessibility::SemanticRole::Grid; node.name=id;
            node.minimum=ImGui::GetCursorScreenPos(); node.maximum={node.minimum.x+ImGui::GetContentRegionAvail().x,node.minimum.y+height};o.accessibility->Add(node); }
        for(int v=0;v<static_cast<int>(columns.size());++v) { int c=o.locale?o.locale->VisualIndex(v,static_cast<int>(columns.size())):v;
            ImGui::TableSetupColumn(columns[c].label,ImGuiTableColumnFlags_WidthFixed,columns[c].width,static_cast<ImGuiID>(c+1)); }
        ImGui::TableSetupScrollFreeze(0,1); ImGui::TableHeadersRow();
        if(auto sort=ImGui::TableGetSortSpecs();sort && sort->SpecsDirty) {
            for(int i=0;i<sort->SpecsCount;++i) { auto& x=sort->Specs[i]; Apply(p,{DataAction::Sort,0,columns[x.ColumnUserID-1].id,0,0,false,false,x.SortDirection==ImGuiSortDirection_Descending}); }
            sort->SpecsDirty=false;
        }
        ImGuiListClipper clip; clip.Begin(p.count,ImGui::GetFrameHeight()+2*ImGui::GetStyle().CellPadding.y);
        if(s.focusPending) clip.IncludeItemByIndex(s.focusedRow);
        while(clip.Step()) {
            if(p.query) p.query(p.user,{clip.DisplayStart,clip.DisplayEnd-clip.DisplayStart});
            for(int row=clip.DisplayStart;row<clip.DisplayEnd;++row) {
                auto key=p.id(p.user,row); ImGui::PushID(static_cast<int>(key>>32)); ImGui::PushID(static_cast<int>(key));
                ImGui::TableNextRow();
                auto rowID=ImGui::GetID("##semantic-row");
                if(o.accessibility) { accessibility::SemanticNode node;node.id=rowID;node.parent=gridID;node.role=p.depth?accessibility::SemanticRole::TreeItem:accessibility::SemanticRole::Row;
                    node.name=p.cell(p.user,row,0);node.state.selected=p.selected && p.selected(p.user,key);
                    node.state.expandable=p.expandable && p.expandable(p.user,row);node.state.expanded=p.expanded && p.expanded(p.user,row);
                    node.minimum=ImGui::GetCursorScreenPos();node.maximum={node.minimum.x+ImGui::GetContentRegionAvail().x,node.minimum.y+ImGui::GetFrameHeight()};o.accessibility->Add(node); }
                for(int visual=0;visual<static_cast<int>(columns.size());++visual) {
                    if(!ImGui::TableSetColumnIndex(visual)) continue;
                    int c=o.locale?o.locale->VisualIndex(visual,static_cast<int>(columns.size())):visual;
                    ImGui::PushID(c); auto text=p.cell(p.user,row,c);
                    if(c==0 && p.depth) { ImGui::Indent(std::max(0,p.depth(p.user,row))*14.f);
                        if(p.expandable && p.expandable(p.user,row)) { if(ImGui::SmallButton(p.expanded && p.expanded(p.user,row)?"-":"+")) Apply(p,{DataAction::Expand,key}); ImGui::SameLine(); } }
                    if(s.editingRow==key && s.editingColumn==columns[c].id) {
                        if(s.focusEditor) { ImGui::SetKeyboardFocusHere(); s.focusEditor=false; }
                        ImGui::SetNextItemWidth(-1);
                        bool commit=ImGui::InputText("##edit",s.edit,sizeof(s.edit),ImGuiInputTextFlags_EnterReturnsTrue);
                        if(commit) { Apply(p,{DataAction::Edit,key,columns[c].id,0,0,false,false,false,s.edit}); s.editingRow=0; }
                        if(ImGui::IsKeyPressed(ImGuiKey_Escape)) s.editingRow=0;
                    } else {
                        bool selected=p.selected && p.selected(p.user,key);
                        if(s.focusPending && row==s.focusedRow && c==s.focusedColumn) {
                            ImGui::SetKeyboardFocusHere(); ImGui::SetScrollHereY(); s.focusPending=false;
                        }
                        char label[1024]; std::snprintf(label,sizeof(label),"%s###cell",text);
                        bool hit=ImGui::Selectable(label,selected,ImGuiSelectableFlags_AllowDoubleClick,{0,ImGui::GetFrameHeight()});
                        bool focused=ImGui::IsItemFocused();
                        if(hit || focused) { s.focusedRow=row; s.focusedColumn=c; }
                        if(hit) { auto& io=ImGui::GetIO(); if(!io.KeyShift) s.anchor=row;
                            Apply(p,{DataAction::Select,key,columns[c].id,std::min(s.anchor,row),std::max(s.anchor,row),io.KeyCtrl,io.KeyShift}); }
                        if(focused) {
                            int nextRow=row,nextColumn=c;
                            if(ImGui::IsKeyPressed(ImGuiKey_UpArrow)) --nextRow;
                            if(ImGui::IsKeyPressed(ImGuiKey_DownArrow)) ++nextRow;
                            if(ImGui::IsKeyPressed(ImGuiKey_PageUp)) nextRow-=std::max(1,int(height/ImGui::GetFrameHeight())-1);
                            if(ImGui::IsKeyPressed(ImGuiKey_PageDown)) nextRow+=std::max(1,int(height/ImGui::GetFrameHeight())-1);
                            if(ImGui::IsKeyPressed(ImGuiKey_Home)) { if(ImGui::GetIO().KeyCtrl) nextRow=0; nextColumn=0; }
                            if(ImGui::IsKeyPressed(ImGuiKey_End)) { if(ImGui::GetIO().KeyCtrl) nextRow=p.count-1; nextColumn=static_cast<int>(columns.size())-1; }
                            if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) --nextColumn;
                            if(ImGui::IsKeyPressed(ImGuiKey_RightArrow)) ++nextColumn;
                            nextRow=std::clamp(nextRow,0,p.count-1); nextColumn=std::clamp(nextColumn,0,static_cast<int>(columns.size())-1);
                            if(nextRow!=row || nextColumn!=c) {
                                s.focusedRow=nextRow; s.focusedColumn=nextColumn; s.focusPending=true;
                                if(ImGui::GetIO().KeyShift) Apply(p,{DataAction::Select,key,columns[c].id,std::min(s.anchor,nextRow),std::max(s.anchor,nextRow),ImGui::GetIO().KeyCtrl,true});
                            }
                        }
                        if(columns[c].editable && ((hit && ImGui::IsMouseDoubleClicked(0)) || (focused && (ImGui::IsKeyPressed(ImGuiKey_F2) || ImGui::IsKeyPressed(ImGuiKey_Enter))))) {
                            s.editingRow=key; s.editingColumn=columns[c].id; Copy(s.edit,sizeof(s.edit),text); s.focusEditor=true;
                        }
                        if(ImGui::BeginPopupContextItem("context")) { if(ImGui::MenuItem(Text(o,"row_action","Row action"))) Apply(p,{DataAction::Context,key,columns[c].id}); ImGui::EndPopup(); }
                        if(o.accessibility) { accessibility::SemanticNode node; node.id=ImGui::GetItemID();node.parent=rowID;node.name=text;node.role=accessibility::SemanticRole::Cell;node.state.selected=selected;
                            node.actions=accessibility::SemanticAction::Focus|accessibility::SemanticAction::Select;
                            if(o.accessibility->Take(node.id,accessibility::SemanticAction::Focus)) { s.focusedRow=row;s.focusedColumn=c;s.focusPending=true; }
                            if(o.accessibility->Take(node.id,accessibility::SemanticAction::Select)) Apply(p,{DataAction::Select,key,columns[c].id,row,row});
                            accessibility::AnnotateLastItem(*o.accessibility,node); }
                    }
                    if(c==0 && p.depth) ImGui::Unindent(std::max(0,p.depth(p.user,row))*14.f);
                    ImGui::PopID();
                } ImGui::PopID(); ImGui::PopID();
            }
        } ImGui::EndTable();
    } ImGui::PopID();
}
void TreeDataGrid(const char* id,DataTableState& state,const DataProvider& provider,std::span<const DataColumn> columns,float height,ComponentOptions o) { DataTable(id,state,provider,columns,height,o); }
} // namespace imkit
