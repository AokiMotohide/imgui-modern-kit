#include <imkit/workflow.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit {
namespace {
const char* Safe(const char* s) { return s?s:""; }
const char* Text(ComponentOptions o,const char* key,const char* fallback) { return o.locale?o.locale->Text(key,fallback):fallback; }
void Push(StableId id) { ImGui::PushID(static_cast<int>(id>>32)); ImGui::PushID(static_cast<int>(id)); }
void Pop() { ImGui::PopID(); ImGui::PopID(); }
ImVec4 Color(FeedbackKind kind,ComponentOptions o) {
    if(o.theme) {
        switch(kind) {
        case FeedbackKind::Success:return o.theme->semantic.success;
        case FeedbackKind::Warning:return o.theme->semantic.warning;
        case FeedbackKind::Error:return o.theme->semantic.error;
        default:return o.theme->semantic.accent;
        }
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
}
ImVec4 Mix(ImVec4 a,ImVec4 b,float t) {
    return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t};
}
ImVec4 Accent(const StepGroup& group,ComponentOptions o) {
    if(group.accent.w>0) return group.accent;
    return o.theme?o.theme->semantic.accent:ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
}
bool Annotate(const char* label,const char* description,accessibility::SemanticRole role,
              accessibility::SemanticAction action,bool selected,ComponentOptions o,bool mixed=false) {
    if(!o.accessibility) return false;
    const auto id=ImGui::GetItemID();
    const bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    bool focus=o.accessibility->Take(id,accessibility::SemanticAction::Focus);
    if(focus && !disabled) ImGui::SetKeyboardFocusHere(-1);
    bool requested=o.accessibility->Take(id,action);
    accessibility::SemanticNode n; n.id=id;n.parent=o.parent;n.name=Safe(label);n.description=Safe(description);
    n.role=role;n.actions=action|accessibility::SemanticAction::Focus;n.state.selected=selected;n.state.mixed=mixed;
    if(auto suffix=n.name.find("##");suffix!=std::string_view::npos)n.name=n.name.substr(0,suffix);
    accessibility::AnnotateLastItem(*o.accessibility,n);
    return requested && !disabled;
}
bool LabeledButton(const char* label,const char* description,ImVec2 size,bool selected,ComponentOptions o,
                   accessibility::SemanticRole role=accessibility::SemanticRole::Button) {
    if(size.x==0)size.x=ImGui::CalcTextSize(Safe(label),nullptr,true).x+2*ImGui::GetStyle().FramePadding.x;
    bool pressed=ImGui::Button("##action",size);
    if(ImGui::IsItemFocused())ImGui::SetNavCursorVisible(true);
    auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    const auto pad=ImGui::GetStyle().FramePadding;
    auto* d=ImGui::GetWindowDrawList();
    d->PushClipRect({a.x+pad.x,a.y},{std::max(a.x+pad.x,b.x-pad.x),b.y},true);
    std::string_view text=Safe(label);if(auto suffix=text.find("##");suffix!=std::string_view::npos)text=text.substr(0,suffix);
    float room=std::max(0.f,b.x-a.x-pad.x*2);
    bool ellipsis=ImGui::CalcTextSize(text.data(),text.data()+text.size()).x>room;
    if(ellipsis) {
        room=std::max(0.f,room-ImGui::CalcTextSize("...").x);
        while(!text.empty() && ImGui::CalcTextSize(text.data(),text.data()+text.size()).x>room) {
            auto count=text.size()-1;while(count && (static_cast<unsigned char>(text[count])&0xc0)==0x80)--count;text=text.substr(0,count);
        }
    }
    ImVec2 pos{a.x+pad.x,a.y+(b.y-a.y-ImGui::GetFontSize())*.5f};
    d->AddText(pos,ImGui::GetColorU32(ImGuiCol_Text),text.data(),text.data()+text.size());
    if(ellipsis)d->AddText({pos.x+ImGui::CalcTextSize(text.data(),text.data()+text.size()).x,pos.y},ImGui::GetColorU32(ImGuiCol_Text),"...");
    d->PopClipRect();
    if(selected) d->AddRect(a,b,ImGui::GetColorU32(ImGuiCol_CheckMark),ImGui::GetStyle().FrameRounding,o.theme?o.theme->stroke.focus:2,ImDrawFlags_None);
    if((ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0)d->AddLine({b.x-pad.x,b.y-pad.y},{b.x-pad.x-ImGui::GetFontSize()*.4f,b.y-pad.y-ImGui::GetFontSize()*.4f},ImGui::GetColorU32(ImGuiCol_TextDisabled));
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)||ImGui::IsItemFocused()) {
        if(!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetNextWindowPos({a.x,b.y});
        ImGui::BeginTooltip();ImGui::PushTextWrapPos(ImGui::GetFontSize()*32.f);ImGui::TextUnformatted(Safe(label));
        if(*Safe(description)) ImGui::TextUnformatted(description);ImGui::PopTextWrapPos();ImGui::EndTooltip();
    }
    return Annotate(label,description,role,accessibility::SemanticAction::Press,selected,o)||pressed;
}
}
StableId GroupedStepNavigator(const char* id, std::span<const StepGroup> groups,
    std::span<const StepItem> items, StableId current, ComponentOptions o) {
    if(items.empty()) return 0;
    ImGui::PushID(id);
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float gap=std::min(ImGui::GetStyle().ItemSpacing.x,width/items.size()*.1f);
    const float cell=width/items.size();
    const float x=ImGui::GetCursorPosX();
    const float y=ImGui::GetCursorPosY();
    const float h=ImGui::GetFrameHeight();
    StableId result=0;
    ImGui::PushID("groups");
    for(const auto& group:groups) {
        if(!group.count || group.first>=items.size()) continue;
        const auto end=std::min(items.size(),group.first+std::min(group.count,items.size()-group.first));
        bool active=false;
        for(auto i=group.first;i<end;++i) active|=items[i].id==current;
        const auto& first=items[group.first];
        const auto accent=Accent(group,o);
        const auto base=o.theme?o.theme->semantic.control.rest:ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const auto fill=Mix(base,accent,active?.34f:.13f);
        ImGui::SetCursorPos({x+cell*group.first,y});Push(group.id);
        ImGui::BeginDisabled(!first.id || first.disabled || !first.available);
        ImGui::PushStyleColor(ImGuiCol_Button,fill);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,Mix(fill,accent,.24f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,Mix(fill,accent,.38f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark,accent);
        if(LabeledButton(group.label,"",{std::max(1.f,cell*(end-group.first)-gap),h},active,o,accessibility::SemanticRole::Tab) && !active)
            result=first.id;
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        if(active) ImGui::GetWindowDrawList()->AddLine({a.x,b.y-1.f},{b.x,b.y-1.f},ImGui::GetColorU32(accent),3.f);
        ImGui::PopStyleColor(4);
        ImGui::EndDisabled();Pop();
    }
    ImGui::PopID();ImGui::PushID("items");
    ImVec2 previousMax{};
    for(std::size_t i=0;i<items.size();++i) {
        const auto& item=items[i];Push(item.id);
        ImVec4 accent=o.theme?o.theme->semantic.accent:ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        for(const auto& group:groups) if(i>=group.first && i<group.first+group.count) {accent=Accent(group,o);break;}
        const bool currentStep=item.id==current;
        const auto base=o.theme?o.theme->semantic.control.rest:ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const auto fill=currentStep?Mix(base,accent,.32f):base;
        ImGui::SetCursorPos({x+cell*i,y+h+ImGui::GetStyle().ItemSpacing.y});
        ImGui::BeginDisabled(!item.id || item.disabled || !item.available);
        ImGui::PushStyleColor(ImGuiCol_Button,fill);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,Mix(fill,accent,.18f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,Mix(fill,accent,.30f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark,accent);
        if(LabeledButton(item.label,item.description,{std::max(1.f,cell-gap),h},currentStep,o,accessibility::SemanticRole::Tab)) result=item.id;
        ImGui::PopStyleColor(4);
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        auto* draw=ImGui::GetWindowDrawList();
        if(i>0) {
            const auto lineColor=items[i-1].completed?accent:(o.theme?o.theme->semantic.border:ImGui::GetStyleColorVec4(ImGuiCol_Border));
            draw->AddLine({previousMax.x,(a.y+b.y)*.5f},{a.x,(a.y+b.y)*.5f},ImGui::GetColorU32(lineColor),items[i-1].completed?3.f:1.f);
        }
        const float marker=std::max(4.f,ImGui::GetFontSize()*.28f);
        const ImVec2 center{b.x-marker-3.f,a.y+marker+3.f};
        if(item.completed) {
            draw->AddCircleFilled(center,marker,ImGui::GetColorU32(accent));
            const auto check=o.theme?o.theme->semantic.onAccent:ImVec4(1,1,1,1);
            draw->AddLine({center.x-marker*.55f,center.y},{center.x-marker*.10f,center.y+marker*.45f},ImGui::GetColorU32(check),1.5f);
            draw->AddLine({center.x-marker*.10f,center.y+marker*.45f},{center.x+marker*.65f,center.y-marker*.45f},ImGui::GetColorU32(check),1.5f);
        } else if(item.status==FeedbackKind::Warning || item.status==FeedbackKind::Error) {
            const auto warning=Color(item.status,o);
            const ImVec2 p0{center.x,center.y-marker},p1{center.x-marker,center.y+marker},p2{center.x+marker,center.y+marker};
            draw->AddTriangleFilled(p0,p1,p2,ImGui::GetColorU32(warning));
            draw->AddLine({center.x,center.y-marker*.35f},{center.x,center.y+marker*.25f},ImGui::GetColorU32(ImGuiCol_Text),1.f);
        }
        previousMax=b;
        ImGui::EndDisabled();Pop();
    }
    ImGui::PopID();
    ImGui::SetCursorPos({x,y+2*(h+ImGui::GetStyle().ItemSpacing.y)});
    ImGui::Dummy({width,0});
    ImGui::PopID();return result;
}
StableId IconToolbar(const char* id,const IconAtlas& atlas,
    std::span<const IconToolbarItem> items,ComponentOptions o) {
    return IconToolbar(id,atlas,items,IconToolbarOptions{},o);
}
StableId IconToolbar(const char* id,const IconAtlas& atlas,
    std::span<const IconToolbarItem> items,IconToolbarOptions layout,ComponentOptions o) {
    ImGui::PushID(id);StableId result=0;
    const float start=ImGui::GetCursorPosX(), right=start+ImGui::GetContentRegionAvail().x;
    const float size=std::max(12.f,ImGui::GetFontSize());
    bool first=true;
    for(const auto& item:items) {
        const auto text=std::string_view(Safe(item.label));
        const auto visible=text.substr(0,text.find("##"));
        const float labelWidth=layout.showLabels && !visible.empty()?ImGui::CalcTextSize(visible.data(),visible.data()+visible.size()).x+ImGui::GetStyle().ItemInnerSpacing.x:0.f;
        const float button=size+labelWidth+2*ImGui::GetStyle().FramePadding.x;
        const float nextX=ImGui::GetItemRectMax().x-ImGui::GetWindowPos().x+ImGui::GetScrollX()+ImGui::GetStyle().ItemSpacing.x+button;
        if(!first && (!layout.wrap || nextX<=right)) ImGui::SameLine();
        first=false;Push(item.id);ImGui::BeginDisabled(item.disabled || !item.id);
        if(item.selected) {
            const auto accent=o.theme?o.theme->semantic.accent:ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
            const auto base=o.theme?o.theme->semantic.control.rest:ImGui::GetStyleColorVec4(ImGuiCol_Button);
            const auto fill=Mix(base,accent,.32f);
            ImGui::PushStyleColor(ImGuiCol_Button,fill);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,Mix(fill,accent,.18f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,Mix(fill,accent,.30f));
        }
        const bool pressed=layout.showLabels
            ?IconLabelButton("action",atlas,item.icon,Safe(item.label),{size})
            :IconButton("action",atlas,item.icon,Safe(item.label),{size});
        if(item.selected) ImGui::PopStyleColor(3);
        if(Annotate(item.label,item.description,accessibility::SemanticRole::Button,accessibility::SemanticAction::Press,item.selected,o,item.mixed) || pressed) result=item.id;
        auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        auto* draw=ImGui::GetWindowDrawList();
        if(item.selected) draw->AddRect(a,b,ImGui::GetColorU32(ImGuiCol_CheckMark),ImGui::GetStyle().FrameRounding,2.f,0);
        if(item.mixed) draw->AddLine({a.x+4,b.y-3},{b.x-4,b.y-3},ImGui::GetColorU32(ImGuiCol_TextDisabled),2.f);
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)||ImGui::IsItemFocused()) {
            ImGui::BeginTooltip();ImGui::PushTextWrapPos(ImGui::GetFontSize()*32.f);ImGui::TextUnformatted(Safe(item.label));
            if(*Safe(item.description)) ImGui::TextUnformatted(item.description);
            if(item.mixed) ImGui::TextUnformatted(Text(o,"mixed","Mixed"));
            ImGui::PopTextWrapPos();ImGui::EndTooltip();
        }
        ImGui::EndDisabled();Pop();
    }
    ImGui::PopID();return result;
}
StableId WorkspaceTabs(const char* id,std::span<const WorkspaceTab> tabs,
                       StableId selected,const IconAtlas* icons,ComponentOptions o) {
    ImGui::PushID(id);
    StableId request=0;
    float required=0;
    for(const auto& tab:tabs)
        required+=ImGui::CalcTextSize(Safe(tab.label)).x+ImGui::GetStyle().FramePadding.x*2+
            (icons&&tab.icon!=IconId::Count?ImGui::GetFrameHeight():0)+ImGui::GetStyle().ItemSpacing.x;
    if(required>ImGui::GetContentRegionAvail().x && tabs.size()>1) {
        const char* preview="";
        for(const auto& tab:tabs)if(tab.id==selected)preview=Safe(tab.label);
        if(ImGui::BeginCombo("##workspaces",preview)) {
            for(const auto& tab:tabs) {
                Push(tab.id);ImGui::BeginDisabled(tab.disabled||!tab.id);
                if(ImGui::Selectable(Safe(tab.label),tab.id==selected))request=tab.id;
                Annotate(tab.label,tab.description,accessibility::SemanticRole::Tab,
                         accessibility::SemanticAction::Select,tab.id==selected,o);
                ImGui::EndDisabled();Pop();
            }
            ImGui::EndCombo();
        }
    } else {
        for(std::size_t i=0;i<tabs.size();++i) {
            const auto& tab=tabs[i];
            if(i)ImGui::SameLine(0,ImGui::GetStyle().ItemSpacing.x);
            Push(tab.id);ImGui::BeginDisabled(tab.disabled||!tab.id);
            const bool active=tab.id==selected;
            if(active) {
                ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_Header));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
            }
            const bool hit=icons&&tab.icon!=IconId::Count
                ?IconLabelButton("tab",*icons,tab.icon,Safe(tab.label),{ImGui::GetFontSize()})
                :ImGui::Button(Safe(tab.label));
            if(active)ImGui::PopStyleColor(2);
            if(Annotate(tab.label,tab.description,accessibility::SemanticRole::Tab,
                        accessibility::SemanticAction::Select,active,o)||hit)request=tab.id;
            if(active) {
                auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine({a.x,b.y-1},{b.x,b.y-1},
                    ImGui::GetColorU32(ImGuiCol_CheckMark),2.f);
            }
            if((ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)||ImGui::IsItemFocused())&&*Safe(tab.description))
                ImGui::SetTooltip("%s",tab.description);
            if(ImGui::IsItemFocused()&&tabs.size()>1) {
                int direction=ImGui::IsKeyPressed(ImGuiKey_RightArrow)?1:ImGui::IsKeyPressed(ImGuiKey_LeftArrow)?-1:0;
                for(std::size_t n=1;direction&&n<tabs.size();++n) {
                    const auto next=(static_cast<long long>(i)+direction*static_cast<long long>(n)+static_cast<long long>(tabs.size()))%static_cast<long long>(tabs.size());
                    if(tabs[static_cast<std::size_t>(next)].id&&!tabs[static_cast<std::size_t>(next)].disabled) {
                        request=tabs[static_cast<std::size_t>(next)].id;break;
                    }
                }
            }
            ImGui::EndDisabled();Pop();
        }
    }
    ImGui::PopID();return request;
}
bool HierarchyGroupHeader(const char* id,const char* label,int count,bool* open,
                          const IconAtlas* icons,IconId icon,ComponentOptions) {
    ImGui::PushID(id);
    bool local=open?*open:true;
    if(ImGui::ArrowButton("toggle",local?ImGuiDir_Down:ImGuiDir_Right))local=!local;
    ImGui::SameLine();
    if(icons&&icon!=IconId::Count) {Icon(*icons,icon,{ImGui::GetFontSize()});ImGui::SameLine();}
    char caption[256];std::snprintf(caption,sizeof(caption),"%s (%d)",Safe(label),std::max(0,count));
    if(ImGui::Selectable(caption,false,0,{0,ImGui::GetFrameHeight()}))local=!local;
    if(open)*open=local;
    ImGui::PopID();return local;
}
HierarchyRowAction HierarchyRow(const char* id,const HierarchyRowView& row,
                               const IconAtlas* icons,ComponentOptions o) {
    ImGui::PushID(id);Push(row.id);
    HierarchyRowAction result=HierarchyRowAction::None;
    ImGui::BeginDisabled(row.disabled||!row.id);
    ImGui::Indent(std::max(0,row.depth)*ImGui::GetFontSize());
    if(icons&&row.icon!=IconId::Count) {Icon(*icons,row.icon,{ImGui::GetFontSize()});ImGui::SameLine();}
    const float actions=ImGui::GetFrameHeight()*3+ImGui::GetStyle().ItemSpacing.x*4;
    if(ImGui::Selectable(Safe(row.label),row.selected,ImGuiSelectableFlags_AllowOverlap,
                         {std::max(1.f,ImGui::GetContentRegionAvail().x-actions),ImGui::GetFrameHeight()}))
        result=HierarchyRowAction::Select;
    if(Annotate(row.label,row.detail,accessibility::SemanticRole::TreeItem,
                accessibility::SemanticAction::Select,row.selected,o))result=HierarchyRowAction::Select;
    if((ImGui::IsItemHovered()||ImGui::IsItemFocused())&&*Safe(row.detail))ImGui::SetTooltip("%s",row.detail);
    const auto actionButton=[&](const char* key,IconId icon,const char* text) {
        ImGui::SameLine(0,ImGui::GetStyle().ItemSpacing.x);
        bool pressed=icons?IconButton(key,*icons,icon,text,{ImGui::GetFontSize()})
                          :ImGui::SmallButton(text);
        if(ImGui::IsItemHovered()||ImGui::IsItemFocused())ImGui::SetTooltip("%s",text);
        return pressed;
    };
    if(actionButton("visibility",row.visible?IconId::Eye:IconId::EyeOff,
                    row.visible?Safe(row.visibleLabel):Safe(row.hiddenLabel)))result=HierarchyRowAction::ToggleVisibility;
    if(actionButton("lock",row.locked?IconId::Lock:IconId::Unlock,
                    row.locked?Safe(row.lockedLabel):Safe(row.unlockedLabel)))result=HierarchyRowAction::ToggleLock;
    if(actionButton("more",IconId::More,Safe(row.moreLabel)))result=HierarchyRowAction::More;
    ImGui::Unindent(std::max(0,row.depth)*ImGui::GetFontSize());
    ImGui::EndDisabled();Pop();ImGui::PopID();return result;
}
bool BeginInspectorCard(const char* id,const char* title,const char* description,
                        const IconAtlas* icons,IconId icon,ComponentOptions) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,ImGui::GetStyle().FrameRounding);
    const bool visible=ImGui::BeginChild(id,{0,0},
        ImGuiChildFlags_Borders|ImGuiChildFlags_AutoResizeY,ImGuiWindowFlags_NoScrollbar);
    if(visible) {
        if(icons&&icon!=IconId::Count) {Icon(*icons,icon,{ImGui::GetFontSize()});ImGui::SameLine();}
        ImGui::TextUnformatted(Safe(title));
        if(*Safe(description))ImGui::TextDisabled("%s",description);
        ImGui::Separator();
    }
    return visible;
}
void EndInspectorCard() {ImGui::EndChild();ImGui::PopStyleVar();ImGui::PopStyleColor();}
bool SettingToggleRow(const char* id,const char* label,const char* description,
                      bool current,bool disabled,const char* disabledReason,ComponentOptions o) {
    ImGui::PushID(id);
    bool next=current,changed=false;
    if(ImGui::BeginTable("row",2,ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("label",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("toggle",ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight()*2.4f);
        ImGui::TableNextRow();ImGui::TableNextColumn();
        ImGui::TextUnformatted(Safe(label));
        if(*Safe(description))ImGui::TextDisabled("%s",description);
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(disabled);
        ComponentOptions toggleOptions=o;toggleOptions.accessibility=nullptr;
        changed=Toggle("##value",&next,toggleOptions);
        changed=Annotate(label,description,accessibility::SemanticRole::Toggle,
                         accessibility::SemanticAction::Toggle,current,o)||changed;
        if(disabled&&*Safe(disabledReason)&&ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s",disabledReason);
        ImGui::EndDisabled();ImGui::EndTable();
    }
    ImGui::PopID();return changed;
}
StableId StepNavigator(const char* id,std::span<const StepItem> items,StableId current,
                      StepNavigatorState& state,StepNavigatorOptions layout,ComponentOptions o) {
    ImGui::PushID(id);
    const bool horizontal=layout.orientation==Orientation::Horizontal;
    const float h=std::max(ImGui::GetFrameHeight(),o.theme?o.theme->metrics.controlHeight:0.f);
    const float extent=layout.itemExtent>0?layout.itemExtent:h*6;
    if(layout.size.y==0 && horizontal) layout.size.y=h+ImGui::GetStyle().WindowPadding.y*2+ImGui::GetStyle().ScrollbarSize;
    StableId result=0;
    if(ImGui::BeginChild("steps",layout.size,ImGuiChildFlags_None,horizontal?ImGuiWindowFlags_HorizontalScrollbar:0)) {
        for(std::size_t visual=0;visual<items.size();++visual) {
            auto i=horizontal && o.locale && o.locale->direction==TextDirection::RTL?items.size()-1-visual:visual;
            const auto& item=items[i];
            if(horizontal && visual) ImGui::SameLine();
            Push(item.id); ImGui::BeginDisabled(item.disabled||!item.available||!item.id);
            if(state.focusPending && state.focused==item.id) { ImGui::SetKeyboardFocusHere();state.focusPending=false; }
            auto a=ImGui::GetCursorScreenPos();
            ImVec2 size{horizontal?extent:std::max(1.f,ImGui::GetContentRegionAvail().x),h};
            // Reserve a leading badge without modifying the host's label.
            auto pad=ImGui::GetStyle().FramePadding;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(pad.x+h,pad.y));
            if(LabeledButton(item.label,item.description,size,item.id==current,o,accessibility::SemanticRole::Tab)) result=item.id;
            ImGui::PopStyleVar();
            if(ImGui::IsItemFocused()) {
                state.focused=item.id;
                int direction=0;
                if(ImGui::IsKeyPressed(horizontal?ImGuiKey_RightArrow:ImGuiKey_DownArrow)) direction=1;
                if(ImGui::IsKeyPressed(horizontal?ImGuiKey_LeftArrow:ImGuiKey_UpArrow)) direction=-1;
                if(horizontal && o.locale && o.locale->direction==TextDirection::RTL) direction=-direction;
                if(direction) for(std::size_t n=1;n<items.size();++n) {
                    auto next=static_cast<std::size_t>((static_cast<long long>(i)+direction*static_cast<long long>(n)+items.size())%items.size());
                    if(items[next].available && !items[next].disabled && items[next].id) {state.focused=items[next].id;state.focusPending=true;break;}
                }
                ImGui::SetScrollHereX(); ImGui::SetScrollHereY();
            }
            auto* draw=ImGui::GetWindowDrawList();
            const float radius=h*.28f;
            ImVec2 center{a.x+pad.x+h*.5f,a.y+h*.5f};
            auto color=ImGui::GetColorU32(Color(item.status,o));
            draw->AddCircle(center,radius,color,0,o.theme?o.theme->stroke.border:1);
            char number[24];std::snprintf(number,sizeof(number),"%zu",i+1);
            const char* mark=item.completed?"+":item.status==FeedbackKind::Error?"!":item.status==FeedbackKind::Warning?"!":layout.numbers?number:"";
            auto ts=ImGui::CalcTextSize(mark);draw->AddText({center.x-ts.x*.5f,center.y-ts.y*.5f},ImGui::GetColorU32(ImGuiCol_Text),mark);
            if(layout.icons && item.icon!=IconId::Count) {
                auto cursor=ImGui::GetCursorScreenPos();ImGui::SetCursorScreenPos({center.x-radius,center.y-radius});
                Icon(*layout.icons,item.icon,{radius*2});ImGui::SetCursorScreenPos(cursor);
            }
            ImGui::EndDisabled();Pop();
        }
    }
    ImGui::EndChild();ImGui::PopID();return result;
}
StableId NavigationRail(const char* id,std::span<const StepItem> items,StableId current,StepNavigatorState& state,ImVec2 size,ComponentOptions o) {
    StepNavigatorOptions layout;layout.orientation=Orientation::Vertical;layout.size=size;layout.numbers=false;
    return StepNavigator(id,items,current,state,layout,o);
}
bool FilterChip(const char* id,const char* label,bool selected,ComponentOptions o) {
    ImGui::PushID(id);bool hit=LabeledButton(label,"",{ImGui::CalcTextSize(Safe(label)).x+ImGui::GetStyle().FramePadding.x*2,0},selected,o);ImGui::PopID();return hit;
}
bool RequestBuffer::Push(StableId id) {
    if(count>=storage.size()) {overflow=true;return false;} storage[count++]=id;return true;
}
std::size_t SelectNotifications(std::span<const FeedbackView> items,double now,std::span<std::size_t> scratch,std::size_t maximum) {
    std::size_t count=0,limit=std::min(maximum,scratch.size());
    for(std::size_t i=0;i<items.size();++i) {
        if(!items[i].id || (items[i].expiresAt>0 && now>=items[i].expiresAt)) continue;
        bool replaced=false;for(std::size_t j=i+1;j<items.size();++j) if(items[j].id==items[i].id) {replaced=true;break;}
        if(replaced) continue;
        std::size_t at=0;while(at<count && items[scratch[at]].priority>=items[i].priority) ++at;
        if(at>=limit) continue;
        for(std::size_t j=std::min(count,limit-1);j>at;--j) scratch[j]=scratch[j-1];
        scratch[at]=i;count=std::min(count+1,limit);
    }return count;
}
bool NotificationCard(const FeedbackView& n,double now,ComponentOptions o) {
    if(!n.id || (n.expiresAt>0 && now>=n.expiresAt)) return false;
    Push(n.id);ImGui::BeginGroup();
    auto start=ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_Text,Color(n.kind,o));ImGui::TextWrapped("%s",Safe(n.title));ImGui::PopStyleColor();
    if(*Safe(n.description)) ImGui::TextWrapped("%s",n.description);
    bool dismiss=false;
    if(n.dismissible) dismiss=LabeledButton(Text(o,"dismiss","Dismiss"),"",{},false,o);
    ImGui::EndGroup();
    if(o.accessibility) {
        accessibility::SemanticNode node;node.id=ImGui::GetID("feedback");node.parent=o.parent;
        node.name=Safe(n.title);node.description=Safe(n.description);node.role=accessibility::SemanticRole::Alert;
        node.minimum=start;node.maximum=ImGui::GetItemRectMax();
        node.actions=n.dismissible?accessibility::SemanticAction::Dismiss:accessibility::SemanticAction::None;
        node.state.disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
        if(o.accessibility->Take(node.id,accessibility::SemanticAction::Dismiss) && !node.state.disabled && n.dismissible) dismiss=true;
        o.accessibility->Add(node);
    }
    Pop();return dismiss;
}
void ToastRegion(const char* id,std::span<const FeedbackView> items,double now,std::span<std::size_t> scratch,RequestBuffer& dismiss,ToastOptions layout,ComponentOptions o) {
    auto count=SelectNotifications(items,now,scratch,layout.maximum);if(!count) return;
    auto* viewport=ImGui::GetWindowViewport(); const auto pad=ImGui::GetStyle().WindowPadding;
    float width=std::min(layout.width>0?layout.width:ImGui::GetFontSize()*25,std::max(1.f,viewport->WorkSize.x-pad.x*2));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos({viewport->WorkPos.x+viewport->WorkSize.x-pad.x,viewport->WorkPos.y+pad.y},ImGuiCond_Always,{1,0});
    ImGui::SetNextWindowSizeConstraints({width,0},{width,std::max(1.f,viewport->WorkSize.y-pad.y*2)});
    ImGui::PushID(id);auto scope=ImGui::GetID("toast-window");ImGui::PopID();
    char name[40];std::snprintf(name,sizeof(name),"###imkit-toast-%08x",scope);
    if(ImGui::Begin(name,nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing)) {
        for(std::size_t i=0;i<count;++i) {if(i) ImGui::Separator();if(NotificationCard(items[scratch[i]],now,o)) dismiss.Push(items[scratch[i]].id);}
    }ImGui::End();
}
bool InlineAlert(const char* id,const FeedbackView& item,ComponentOptions o) {ImGui::PushID(id);bool result=NotificationCard(item,0,o);ImGui::PopID();return result;}
bool PersistentBanner(const char* id,const FeedbackView& item,ComponentOptions o) {auto persistent=item;persistent.expiresAt=0;return InlineAlert(id,persistent,o);}
bool EmptyState(const char* id,const StateView& v,ComponentOptions o) {
    ImGui::PushID(id);ImGui::BeginGroup();
    if(v.icons && v.icon!=IconId::Count) {Icon(*v.icons,v.icon,{ImGui::GetFrameHeight()});ImGui::SameLine();}
    ImGui::TextColored(Color(v.kind,o),"%s",Safe(v.heading));
    if(o.accessibility) {accessibility::SemanticNode n;n.id=ImGui::GetID("state");n.parent=o.parent;n.name=Safe(v.heading);n.description=Safe(v.description);n.role=accessibility::SemanticRole::Status;accessibility::AnnotateLastItem(*o.accessibility,n);}
    if(*Safe(v.description)) ImGui::TextWrapped("%s",v.description);
    bool result=false;ImGui::BeginDisabled(v.disabled);
    if(*Safe(v.action)) result=LabeledButton(v.action,"",{},false,o);
    ImGui::EndDisabled();ImGui::EndGroup();ImGui::PopID();return result;
}
bool UnavailableState(const char* id,const StateView& view,ComponentOptions o) {return EmptyState(id,view,o);}
bool RetryState(const char* id,const StateView& view,ComponentOptions o) {return EmptyState(id,view,o);}
void CircularProgress(const char* id,const CircularProgressView& view,CircularProgressOptions layout,ComponentOptions o) {
    constexpr float pi=3.14159265358979323846f;
    constexpr const char* unavailable="\xE2\x80\x94";
    const float diameter=std::max(12.f,layout.diameter>0?layout.diameter:ImGui::GetFrameHeight()*3.f);
    const float stroke=std::clamp(layout.strokeWidth>0?layout.strokeWidth:diameter*.06f,1.f,diameter*.24f);
    const bool available=std::isfinite(view.fraction)&&view.fraction>=0.f;
    const float fraction=available?std::clamp(view.fraction,0.f,1.f):0.f;
    const char* value=*Safe(view.value)?view.value:(available?"":unavailable);
    const ImVec4 track=o.theme?o.theme->semantic.border:ImGui::GetStyleColorVec4(ImGuiCol_Border);
    const ImVec4 foreground=available?Color(view.kind,o):(o.theme?o.theme->semantic.textDisabled:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::PushID(id);ImGui::BeginGroup();
    const ImVec2 top=ImGui::GetCursorScreenPos();ImGui::Dummy({diameter,diameter});
    const ImVec2 center{top.x+diameter*.5f,top.y+diameter*.5f};
    const float radius=std::max(1.f,diameter*.5f-stroke*.5f-1.f);
    auto* draw=ImGui::GetWindowDrawList();const int segments=std::clamp(static_cast<int>(diameter*.75f),24,96);
    draw->AddCircle(center,radius,ImGui::GetColorU32(track),segments,stroke);
    if(available&&fraction>0.f) {
        const float start=-pi*.5f;
        if(fraction>=.9999f) draw->AddCircle(center,radius,ImGui::GetColorU32(foreground),segments,stroke);
        else {
            const float end=start+2.f*pi*fraction;
            draw->PathArcTo(center,radius,start,end,std::max(2,static_cast<int>(segments*fraction)));
            draw->PathStroke(ImGui::GetColorU32(foreground),stroke,0);
            for(float angle:{start,end})
                draw->AddCircleFilled({center.x+std::cos(angle)*radius,center.y+std::sin(angle)*radius},stroke*.5f,ImGui::GetColorU32(foreground));
        }
    }
    const ImVec2 valueSize=ImGui::CalcTextSize(value);
    const float innerWidth=std::max(1.f,(radius-stroke*.5f)*1.55f);
    const float textScale=std::min({diameter>=ImGui::GetFontSize()*4.f?1.15f:1.f,
                                  innerWidth/std::max(1.f,valueSize.x),
                                  innerWidth/std::max(1.f,valueSize.y)});
    draw->AddText(ImGui::GetFont(),ImGui::GetFontSize()*textScale,
                  {center.x-valueSize.x*textScale*.5f,center.y-valueSize.y*textScale*.5f},
                  ImGui::GetColorU32(available?(o.theme?o.theme->semantic.text:ImGui::GetStyleColorVec4(ImGuiCol_Text)):foreground),value);
    if(*Safe(view.label)) {
        const float width=ImGui::CalcTextSize(view.label).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.f,(diameter-width)*.5f));
        ImGui::PushStyleColor(ImGuiCol_Text,o.theme?o.theme->semantic.textDisabled:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(view.label);ImGui::PopStyleColor();
    }
    ImGui::EndGroup();
    if(o.accessibility) {accessibility::SemanticNode n;n.id=ImGui::GetID("progress");n.parent=o.parent;n.name=Safe(view.label);n.value=value;n.role=accessibility::SemanticRole::Progress;n.state.readOnly=true;n.state.invalid=!available;accessibility::AnnotateLastItem(*o.accessibility,n);}
    ImGui::PopID();
}
bool Progress(const char* id,const ProgressView& view,ProgressPresentation presentation,DialogState& state,ImVec2 size,ComponentOptions o) {
    bool visible=true,modal=presentation==ProgressPresentation::Modal,child=presentation==ProgressPresentation::Overlay;
    ImGui::PushID(id);
    if(modal) {
        if(state.open && !ImGui::IsPopupOpen("progress")) ImGui::OpenPopup("progress");
        if(!state.open && ImGui::IsPopupOpen("progress") && ImGui::BeginPopupModal("progress")){ImGui::CloseCurrentPopup();ImGui::EndPopup();}
        visible=state.open && ImGui::BeginPopupModal("progress",nullptr,ImGuiWindowFlags_AlwaysAutoResize);
    } else if(child) visible=ImGui::BeginChild("progress",size,ImGuiChildFlags_Borders);
    bool cancel=false;
    if(visible) {
        if(modal) ImGui::PushItemWidth(ImGui::GetFontSize()*24);
        imkit::Progress("bar",std::isfinite(view.fraction)?view.fraction:-1,Safe(view.stage),o);
        if(*Safe(view.description)) ImGui::TextWrapped("%s",view.description);
        if(view.cancellable) cancel=LabeledButton(Text(o,"cancel","Cancel"),"",{},false,o);
        if(modal) ImGui::PopItemWidth();
    }
    if(modal && visible) ImGui::EndPopup();if(child) ImGui::EndChild();ImGui::PopID();return cancel;
}
bool BeginCard(const char* id,ImVec2 size,ComponentOptions o) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg,o.theme?o.theme->semantic.surfaceRaised:ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize,o.theme?o.theme->stroke.border:1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,o.theme?o.theme->radius.control:ImGui::GetStyle().FrameRounding);
    return ImGui::BeginChild(id,size,ImGuiChildFlags_Borders);
}
void EndCard() {ImGui::EndChild();ImGui::PopStyleVar(2);ImGui::PopStyleColor();}
bool SectionHeader(const char* id,const char* label,bool& open,ComponentOptions o) {
    ImGui::PushID(id);if(LabeledButton(label,Text(o,"toggle_section","Expand or collapse"),{std::max(1.f,ImGui::GetContentRegionAvail().x),0},open,o)) open=!open;ImGui::PopID();return open;
}
StableId MultiSelectionBar(const char* id,std::size_t count,std::span<const Command> actions,ToolbarState& state,ComponentOptions o) {
    if(!count) return 0;ImGui::PushID(id);ImGui::Text("%zu %s",count,Text(o,"selected","selected"));
    auto result=ResponsiveToolbar("actions",state,actions,ToolbarOptions{},o);ImGui::PopID();return result;
}
void HelpCallout(const char* id,const StateView& view,ComponentOptions o) {auto v=view;v.action="";EmptyState(id,v,o);}
StableId ValidationSummary(const char* id,std::span<const StepItem> issues,ComponentOptions o) {
    ImGui::PushID(id);StableId result=0;
    for(auto& issue:issues) {Push(issue.id);ImGui::BeginDisabled(issue.disabled||!issue.available);if(LabeledButton(issue.label,issue.description,{std::max(1.f,ImGui::GetContentRegionAvail().x),0},false,o)) result=issue.id;ImGui::EndDisabled();Pop();}
    ImGui::PopID();return result;
}
StableId ResponsiveToolbar(const char* id,ToolbarState& state,std::span<const Command> commands,ToolbarOptions layout,ComponentOptions o) {
    ImGui::PushID(id);StableId result=0;float available=ImGui::GetContentRegionAvail().x;
    const auto spacing=ImGui::GetStyle().ItemSpacing.x;const float menu=ImGui::GetFrameHeight();
    std::size_t firstHidden=commands.size();
    for(std::size_t i=0;i<commands.size();++i) {
        auto& c=commands[i];float width=layout.iconOnly?menu:ImGui::CalcTextSize(Safe(c.label)).x+2*ImGui::GetStyle().FramePadding.x;
        const float reserve=i+1<commands.size()?menu+spacing:0;
        if(layout.overflow && width+reserve>available) {firstHidden=i;break;}
        if(i) ImGui::SameLine();Push(c.id);ImGui::BeginDisabled(c.disabled);
        bool icon=layout.iconOnly && layout.icons && i<layout.iconsByCommand.size();
        if(icon) {if(IconButton("command",*layout.icons,layout.iconsByCommand[i],Safe(c.label),{menu,{},o.accessibility,o.parent})) result=c.id;}
        else if(LabeledButton(c.label,c.disabled?c.disabledReason:c.shortcut,{std::min(width,std::max(1.f,available)),0},false,o)) result=c.id;
        if(ImGui::IsItemFocused()) state.focused=static_cast<int>(i);
        if(c.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s",Safe(c.disabledReason));
        ImGui::EndDisabled();Pop();available-=width+spacing;
    }
    if(firstHidden<commands.size()) {
        if(firstHidden) ImGui::SameLine();
        if(LabeledButton("...",Text(o,"more_actions","More actions"),{std::max(1.f,std::min(menu,available)),0},false,o)) ImGui::OpenPopup("overflow");
        if(ImGui::BeginPopup("overflow")) {
            for(std::size_t i=firstHidden;i<commands.size();++i) {auto& c=commands[i];Push(c.id);
                bool hit=ImGui::MenuItem(Safe(c.label),Safe(c.shortcut),false,!c.disabled);
                ImGui::BeginDisabled(c.disabled);
                if(Annotate(c.label,c.disabledReason,accessibility::SemanticRole::MenuItem,accessibility::SemanticAction::Press,false,o)) hit=true;
                ImGui::EndDisabled();if(hit) result=c.id;
                if(c.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s",Safe(c.disabledReason));Pop();
            }ImGui::EndPopup();
        }
    }
    ImGui::PopID();return result;
}
}
