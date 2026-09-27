// This file implements or exercises the deprecated spellings on purpose.
#define IMKIT_DEPRECATED(message)
#include <imkit/workflow.h>
#include <imkit/widgets.h>
#include "progress_internal.h"
#include "draw_internal.h"
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
ImVec4 ProgressStatusColor(ProgressStatus status,ComponentOptions o) {
    switch(status) {
    case ProgressStatus::Succeeded:return Color(FeedbackKind::Success,o);
    case ProgressStatus::Failed:return Color(FeedbackKind::Error,o);
    case ProgressStatus::Paused:return Color(FeedbackKind::Warning,o);
    case ProgressStatus::Cancelled:return o.theme?o.theme->semantic.textDisabled:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    case ProgressStatus::Queued:return o.theme?o.theme->semantic.textSecondary:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    default:return Color(FeedbackKind::Info,o);
    }
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
    if(selected) {
        const ImVec4 accent=ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        d->AddRectFilled(a,b,ImGui::GetColorU32(detail::WithAlpha(accent,.14f)),ImGui::GetStyle().FrameRounding);
        d->AddRect(a,b,ImGui::GetColorU32(detail::WithAlpha(accent,.55f)),ImGui::GetStyle().FrameRounding,ImDrawFlags_None,1.f);
    }
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
        const auto fill=Mix(base,accent,active?.18f:.07f);
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
        const auto fill=currentStep?Mix(base,accent,.16f):base;
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
            (icons&&tab.icon!=IconId::Count?ImGui::GetFontSize()+ImGui::GetStyle().ItemInnerSpacing.x:0)+
            ImGui::GetStyle().ItemSpacing.x;
    if(required>ImGui::GetContentRegionAvail().x && tabs.size()>1) {
        const char* preview="";
        for(const auto& tab:tabs)if(tab.id==selected)preview=Safe(tab.label);
        if(imkit::BeginCombo("##workspaces",preview)) {
            for(const auto& tab:tabs) {
                Push(tab.id);ImGui::BeginDisabled(tab.disabled||!tab.id);
                if(ImGui::Selectable(Safe(tab.label),tab.id==selected))request=tab.id;
                Annotate(tab.label,tab.description,accessibility::SemanticRole::Tab,
                         accessibility::SemanticAction::Select,tab.id==selected,o);
                ImGui::EndDisabled();Pop();
            }
            imkit::EndCombo();
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
StableId ChoiceGroup(const char* id,const char* title,std::span<const ChoiceItem> items,
                     StableId current,ChoiceGroupOptions layout,ComponentOptions o) {
    ImGui::PushID(Safe(id));
    const auto titleText=std::string_view(Safe(title));
    StableId request=0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8.f,8.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,ImGui::GetStyle().FrameRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize,1.f);
    const bool visible=ImGui::BeginChild("choice-group-card",{0,0},
        ImGuiChildFlags_Borders|ImGuiChildFlags_AutoResizeY|ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollbar);
    if(visible) {
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(titleText.data(),titleText.data()+titleText.size());
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        const StableId semanticGroupId=ImGui::GetID("##choice-group-semantics");
        const auto available=std::max(1.f,ImGui::GetContentRegionAvail().x);
        const auto& style=ImGui::GetStyle();
        const float height=ImGui::GetFrameHeight();
        const float font=ImGui::GetFontSize();
        const float iconSize=std::min(font,std::max(1.f,height-2.f*style.FramePadding.y));
        const float checkWidth=font*.9f;
        float minimumCell=1.f;
        for(const auto& item:items) {
            const std::string_view label=Safe(item.label);
            const auto end=label.find("##");
            const auto visibleLabel=label.substr(0,end);
            const float textWidth=ImGui::CalcTextSize(visibleLabel.data(),visibleLabel.data()+visibleLabel.size()).x;
            const bool hasIcon=layout.icons&&GetIconInfo(item.icon);
            const float iconWidth=hasIcon?iconSize+style.ItemInnerSpacing.x:0.f;
            minimumCell=std::max(minimumCell,2.f*style.FramePadding.x+iconWidth+textWidth+
                                 style.ItemInnerSpacing.x+checkWidth);
        }
        const int maximumColumns=std::clamp(layout.maximumColumns,1,3);
        int columns=std::max(1,std::min(maximumColumns,static_cast<int>(std::min<std::size_t>(items.size(),3))));
        while(columns>1 && (available-style.ItemSpacing.x*(columns-1))/columns<minimumCell) --columns;
        const float cellWidth=std::max(minimumCell,(available-style.ItemSpacing.x*(columns-1))/columns);
        const float startX=ImGui::GetCursorPosX(),startY=ImGui::GetCursorPosY();
        ComponentOptions itemOptions=o;itemOptions.parent=semanticGroupId;
        for(std::size_t i=0;i<items.size();++i) {
            const auto& item=items[i];
            const std::string_view label=Safe(item.label);
            const auto end=label.find("##");
            const auto visibleLabel=label.substr(0,end);
            const bool hasIcon=layout.icons&&GetIconInfo(item.icon);
            const int column=static_cast<int>(i%static_cast<std::size_t>(columns));
            const int row=static_cast<int>(i/static_cast<std::size_t>(columns));
            ImGui::SetCursorPos({startX+column*(cellWidth+style.ItemSpacing.x),
                                 startY+row*(height+style.ItemSpacing.y)});
            ImGui::PushID("choices");Push(item.id);
            if(!item.id) ImGui::PushID(static_cast<int>(i));
            const bool selected=item.id!=0&&item.id==current;
            ImVec4 accent=item.accent.w>0?item.accent:
                (o.theme?o.theme->semantic.accent:ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
            const auto base=o.theme?o.theme->semantic.control.rest:ImGui::GetStyleColorVec4(ImGuiCol_Button);
            const auto fill=selected?Mix(base,accent,.30f):base;
            ImGui::BeginDisabled(item.disabled||!item.id);
            ImGui::PushStyleColor(ImGuiCol_Button,fill);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,Mix(fill,accent,.18f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,Mix(fill,accent,.32f));
            ImGui::PushStyleColor(ImGuiCol_CheckMark,accent);
            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
            const bool pressed=ImGui::Button(Safe(item.label),{cellWidth,height});
            ImGui::PopStyleColor();
            const auto minimum=ImGui::GetItemRectMin(),maximum=ImGui::GetItemRectMax();
            const bool focused=ImGui::IsItemFocused();
            const bool hovered=ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
            const char* description=Safe(item.description);
            const char* disabledReason=Safe(item.disabledReason);
            const char* semanticDescription=(item.disabled||!item.id)&&*disabledReason?disabledReason:description;
            if(focused) ImGui::SetNavCursorVisible(true);
            if(Annotate(Safe(item.label),semanticDescription,accessibility::SemanticRole::Radio,
                        accessibility::SemanticAction::Press,selected,itemOptions)) request=item.id;
            if(selected) {
                ImGui::GetWindowDrawList()->AddRect(minimum,maximum,ImGui::GetColorU32(accent),
                    style.FrameRounding,ImDrawFlags_None,o.theme?o.theme->stroke.focus:1.5f);
                const float cx=maximum.x-style.FramePadding.x-checkWidth*.58f;
                const float cy=(minimum.y+maximum.y)*.5f;
                const auto checkColor=ImGui::GetColorU32(accent);
                ImGui::GetWindowDrawList()->AddLine({cx-checkWidth*.34f,cy},{cx-checkWidth*.08f,cy+checkWidth*.27f},checkColor,1.7f);
                ImGui::GetWindowDrawList()->AddLine({cx-checkWidth*.08f,cy+checkWidth*.27f},{cx+checkWidth*.38f,cy-checkWidth*.28f},checkColor,1.7f);
            }
            const float textX=minimum.x+style.FramePadding.x+
                (hasIcon?iconSize+style.ItemInnerSpacing.x:0.f);
            if(hasIcon) {
                const auto framebufferScale=ImGui::GetIO().DisplayFramebufferScale;
                const float pixelSize=iconSize*std::max({1.f,framebufferScale.x,framebufferScale.y});
                int atlasIndex=static_cast<int>(IconPixelSizes.size())-1;
                for(int level=0;level<static_cast<int>(IconPixelSizes.size());++level)
                    if(IconPixelSizes[level]>=pixelSize) {atlasIndex=level;break;}
                const auto texture=layout.icons->textures[static_cast<std::size_t>(atlasIndex)];
                if(texture.GetTexID()!=ImTextureID{}) {
                    const auto uv=GetIconRegion(item.icon,IconPixelSizes[atlasIndex]);
                    const ImVec2 pos{minimum.x+style.FramePadding.x,minimum.y+(height-iconSize)*.5f};
                    ImGui::GetWindowDrawList()->AddImage(texture,pos,{pos.x+iconSize,pos.y+iconSize},
                        uv.uv0,uv.uv1,ImGui::GetColorU32(accent));
                }
            }
            const float textY=minimum.y+(height-font)*.5f;
            ImGui::GetWindowDrawList()->AddText({textX,textY},ImGui::GetColorU32(ImGuiCol_Text),
                                                 visibleLabel.data(),visibleLabel.data()+visibleLabel.size());
            if(hovered||focused) {
                if(!visibleLabel.empty()||*description||((item.disabled||!item.id)&&*disabledReason)) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(font*32.f);
                    if(!visibleLabel.empty()) ImGui::TextUnformatted(visibleLabel.data(),visibleLabel.data()+visibleLabel.size());
                    if(*description) ImGui::TextWrapped("%s",description);
                    if((item.disabled||!item.id)&&*disabledReason) ImGui::TextWrapped("%s",disabledReason);
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            if(pressed&&item.id&&!item.disabled) request=item.id;
            ImGui::PopStyleColor(4);
            ImGui::EndDisabled();
            if(!item.id) ImGui::PopID();
            Pop();ImGui::PopID();
        }
        const std::size_t rowCount=(items.size()+static_cast<std::size_t>(columns)-1)/static_cast<std::size_t>(columns);
        if(rowCount) {
            const float contentHeight=rowCount*height+(rowCount-1)*style.ItemSpacing.y;
            ImGui::SetCursorPos({startX,startY+contentHeight});
            ImGui::Dummy({available,0});
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        if(o.accessibility) {
            accessibility::SemanticNode group;group.id=semanticGroupId;group.parent=o.parent;
            group.name=titleText;group.role=accessibility::SemanticRole::Group;
            group.minimum=ImGui::GetItemRectMin();group.maximum=ImGui::GetItemRectMax();
            o.accessibility->Add(group);
        }
    } else {
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return request;
}
bool HierarchyGroupHeader(const char* id,const char* label,int count,bool* open,
                          const IconAtlas* icons,IconId icon,ComponentOptions,
                          bool* actionRequested,const char* actionLabel) {
    ImGui::PushID(id);
    bool local=open?*open:true;
    const auto padding=ImGui::GetStyle().FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{padding.x*.65f,padding.y*.65f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
    if(ImGui::ArrowButton("toggle",local?ImGuiDir_Down:ImGuiDir_Right))local=!local;
    ImGui::SameLine();
    if(icons&&icon!=IconId::Count) {Icon(*icons,icon,{ImGui::GetFontSize()});ImGui::SameLine();}
    char caption[256];std::snprintf(caption,sizeof(caption),"%s (%d)",Safe(label),std::max(0,count));
    const float actionWidth=actionRequested?ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.x:0.f;
    if(ImGui::Selectable(caption,false,0,
                         {actionRequested?std::max(1.f,ImGui::GetContentRegionAvail().x-actionWidth):0.f,
                          ImGui::GetFrameHeight()}))local=!local;
    if(actionRequested) {
        ImGui::SameLine();
        *actionRequested=icons?IconButton("group_action",*icons,IconId::Add,Safe(actionLabel),{ImGui::GetFontSize()})
                              :ImGui::SmallButton(Safe(actionLabel));
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    if(open)*open=local;
    ImGui::PopID();return local;
}
HierarchyRowAction HierarchyRow(const char* id,const HierarchyRowView& row,
                               const IconAtlas* icons,ComponentOptions o) {
    ImGui::PushID(id);Push(row.id);
    HierarchyRowAction result=HierarchyRowAction::None;
    ImGui::BeginDisabled(row.disabled||!row.id);
    const auto padding=ImGui::GetStyle().FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{padding.x*.65f,padding.y*.55f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
    const ImVec2 origin=ImGui::GetCursorScreenPos();
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float height=ImGui::GetFrameHeight();
    const int actionCount=1+int(row.visibilityAvailable)+int(row.lockAvailable);
    const float actionWidth=height*actionCount+
        ImGui::GetStyle().ItemSpacing.x*(actionCount+1);
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
    if(ImGui::Selectable(Safe(row.label),row.selected,ImGuiSelectableFlags_AllowOverlap,{width,height}))
        result=HierarchyRowAction::Select;
    ImGui::PopStyleColor();
    const bool hovered=ImGui::IsItemHovered();
    const bool focused=ImGui::IsItemFocused();
    if(Annotate(row.label,row.detail,accessibility::SemanticRole::TreeItem,
                accessibility::SemanticAction::Select,row.selected,o))result=HierarchyRowAction::Select;
    if((hovered||focused)&&*Safe(row.detail))ImGui::SetTooltip("%s",row.detail);
    const ImVec2 afterRow=ImGui::GetCursorScreenPos();
    ImDrawList* draw=ImGui::GetWindowDrawList();
    const float indent=std::max(0,row.depth)*ImGui::GetFontSize();
    if(row.accentColor.w>0.f)
        draw->AddRectFilled({origin.x+indent,origin.y+3.f},
                            {origin.x+indent+3.f,origin.y+height-3.f},
                            ImGui::ColorConvertFloat4ToU32(row.accentColor),2.f);
    float textX=origin.x+indent+9.f;
    if(icons&&row.icon!=IconId::Count) {
        ImGui::SetCursorScreenPos({textX,origin.y+(height-ImGui::GetFontSize())*.5f});
        Icon(*icons,row.icon,{ImGui::GetFontSize()});
        textX+=ImGui::GetFontSize()+ImGui::GetStyle().ItemInnerSpacing.x;
    }
    const float textEnd=std::max(textX+1.f,origin.x+width-actionWidth);
    const ImVec2 textPos(textX,origin.y+(height-ImGui::GetTextLineHeight())*.5f);
    std::string_view visibleLabel=Safe(row.label);
    if(auto suffix=visibleLabel.find("##");suffix!=std::string_view::npos)
        visibleLabel=visibleLabel.substr(0,suffix);
    float room=std::max(0.f,textEnd-textX);
    const bool ellipsis=ImGui::CalcTextSize(visibleLabel.data(),visibleLabel.data()+visibleLabel.size()).x>room;
    if(ellipsis) {
        room=std::max(0.f,room-ImGui::CalcTextSize("...").x);
        while(!visibleLabel.empty() && ImGui::CalcTextSize(visibleLabel.data(),visibleLabel.data()+visibleLabel.size()).x>room) {
            auto count=visibleLabel.size()-1;
            while(count && (static_cast<unsigned char>(visibleLabel[count])&0xc0)==0x80)--count;
            visibleLabel=visibleLabel.substr(0,count);
        }
    }
    draw->PushClipRect({textX,origin.y},{textEnd,origin.y+height},true);
    draw->AddText(textPos,ImGui::GetColorU32(ImGuiCol_Text),visibleLabel.data(),visibleLabel.data()+visibleLabel.size());
    if(ellipsis)draw->AddText({textX+ImGui::CalcTextSize(visibleLabel.data(),visibleLabel.data()+visibleLabel.size()).x,textPos.y},
                             ImGui::GetColorU32(ImGuiCol_Text),"...");
    draw->PopClipRect();
    float actionX=origin.x+width-actionWidth+ImGui::GetStyle().ItemSpacing.x;
    const auto actionButton=[&](const char* key,IconId icon,const char* text) {
        ImGui::SetCursorScreenPos({actionX,origin.y});
        actionX+=height+ImGui::GetStyle().ItemSpacing.x;
        ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        bool pressed=icons?IconButton(key,*icons,icon,text,{ImGui::GetFontSize()})
                          :ImGui::SmallButton(text);
        ImGui::PopStyleColor(3);
        if(ImGui::IsItemHovered()||ImGui::IsItemFocused())ImGui::SetTooltip("%s",text);
        return pressed;
    };
    if(row.visibilityAvailable&&actionButton("visibility",row.visible?IconId::Eye:IconId::EyeOff,
                    row.visible?Safe(row.visibleLabel):Safe(row.hiddenLabel)))result=HierarchyRowAction::ToggleVisibility;
    if(row.lockAvailable&&actionButton("lock",row.locked?IconId::Lock:IconId::Unlock,
                    row.locked?Safe(row.lockedLabel):Safe(row.unlockedLabel)))result=HierarchyRowAction::ToggleLock;
    if(actionButton("more",IconId::More,Safe(row.moreLabel)))result=HierarchyRowAction::More;
    ImGui::SetCursorScreenPos(afterRow);
    ImGui::Dummy({0.f,0.f});
    ImGui::PopStyleVar(2);
    ImGui::EndDisabled();Pop();ImGui::PopID();return result;
}
bool BeginInspectorCard(const char* id,const char* title,const char* description,
                        const IconAtlas* icons,IconId icon,ComponentOptions) {
    const ImGuiStyle& style=ImGui::GetStyle();
    const ImVec4 surface=Mix(style.Colors[ImGuiCol_WindowBg],style.Colors[ImGuiCol_FrameBg],.68f);
    const ImVec4 border=Mix(surface,style.Colors[ImGuiCol_Text],.13f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,std::max(8.f,style.FrameRounding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{style.WindowPadding.x+5.f,style.WindowPadding.y+4.f});
    ImGui::PushStyleColor(ImGuiCol_ChildBg,surface);
    ImGui::PushStyleColor(ImGuiCol_Border,border);
    const bool visible=ImGui::BeginChild(id,{0,0},
        ImGuiChildFlags_Borders|ImGuiChildFlags_AutoResizeY,ImGuiWindowFlags_NoScrollbar);
    if(visible&&*Safe(title)) {
        const ImVec2 marker=ImGui::GetCursorScreenPos();
        const ImVec4 header=Mix(surface,style.Colors[ImGuiCol_CheckMark],.14f);
        ImGui::GetWindowDrawList()->AddRectFilled(marker,
            {marker.x+ImGui::GetContentRegionAvail().x,marker.y+ImGui::GetTextLineHeight()+8.f},
            ImGui::GetColorU32(header),4.f);
        ImGui::Dummy({4.f,ImGui::GetTextLineHeight()+7.f});
        ImGui::SameLine(0.f,style.ItemSpacing.x+2.f);
        if(icons&&icon!=IconId::Count) {Icon(*icons,icon,{ImGui::GetFontSize()});ImGui::SameLine();}
        ImGui::PushStyleColor(ImGuiCol_Text,Mix(style.Colors[ImGuiCol_Text],style.Colors[ImGuiCol_CheckMark],.13f));
        ImGui::TextUnformatted(Safe(title));
        ImGui::PopStyleColor();
        ImGui::GetWindowDrawList()->AddRectFilled(marker,{marker.x+3.f,marker.y+ImGui::GetTextLineHeight()+7.f},
            ImGui::GetColorU32(ImGuiCol_CheckMark),2.f);
        if(*Safe(description))ImGui::TextDisabled("%s",description);
        ImGui::Spacing();
    }
    return visible;
}
void EndInspectorCard() {ImGui::EndChild();ImGui::PopStyleColor(2);ImGui::PopStyleVar(2);}
bool BeginCollapsibleInspectorCard(const char* id,const char* title,bool* open,
                                   const IconAtlas* icons,IconId icon,ComponentOptions o) {
    ImGui::PushID(id);
    ImGui::SetNextItemOpen(open?*open:true,ImGuiCond_Always);
    const bool expanded=ImGui::CollapsingHeader(Safe(title),ImGuiTreeNodeFlags_SpanAvailWidth);
    if(open)*open=expanded;
    if(!expanded) {ImGui::PopID();return false;}
    BeginInspectorCard("##body","","",icons,icon,o);
    return true;
}
void EndCollapsibleInspectorCard() {EndInspectorCard();ImGui::PopID();}
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
            const float radius=h*.3f;
            ImVec2 center{a.x+pad.x+h*.5f,a.y+h*.5f};
            const ImVec4 tone=Color(item.status,o);
            auto color=ImGui::GetColorU32(tone);
            const bool filled=item.completed||item.id==current;
            const ImVec4 ink=ContrastRatio({1,1,1,1},tone)>=ContrastRatio({0,0,0,1},tone)?ImVec4{1,1,1,1}:ImVec4{0,0,0,1};
            if(filled) draw->AddCircleFilled(center,radius,color,24);
            else draw->AddCircle(center,radius,color,24,o.theme?o.theme->stroke.border:1);
            const auto markColor=filled?ImGui::GetColorU32(ink):ImGui::GetColorU32(ImGuiCol_Text);
            if(item.completed) detail::DrawCheck(draw,center,radius*1.05f,markColor,1.7f);
            else {
                char number[24];std::snprintf(number,sizeof(number),"%zu",i+1);
                const char* mark=item.status==FeedbackKind::Error?"!":item.status==FeedbackKind::Warning?"!":layout.numbers?number:"";
                auto ts=ImGui::CalcTextSize(mark);draw->AddText({center.x-ts.x*.5f,center.y-ts.y*.5f},markColor,mark);
            }
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
    ImGui::PushID(id);
    const auto pad=ImGui::GetStyle().FramePadding;
    const float check=selected?ImGui::GetFontSize()*1.1f:0.f;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,ImGui::GetFrameHeight()*.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{pad.x+check*.5f,pad.y});
    bool hit=LabeledButton(label,"",{ImGui::CalcTextSize(Safe(label)).x+(pad.x+check*.5f)*2+check*.5f,0},selected,o);
    ImGui::PopStyleVar(2);
    if(selected) {
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        detail::DrawCheck(ImGui::GetWindowDrawList(),{a.x+pad.x*.6f+check*.55f,(a.y+b.y)*.5f},ImGui::GetFontSize()*.8f,ImGui::GetColorU32(ImGuiCol_CheckMark),1.7f);
    }
    ImGui::PopID();return hit;
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
    Push(n.id);
    const auto& style=ImGui::GetStyle();
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float pad=style.FramePadding.x,icon=ImGui::GetFontSize()*1.25f,close=n.dismissible?ImGui::GetFrameHeight()*.75f:0.f;
    const float textWidth=std::max(1.f,width-pad*2-icon-style.ItemInnerSpacing.x-close-6.f);
    const float titleHeight=ImGui::CalcTextSize(Safe(n.title),nullptr,false,textWidth).y;
    const float descriptionHeight=*Safe(n.description)?ImGui::CalcTextSize(n.description,nullptr,false,textWidth).y:0.f;
    const float height=std::max({icon,close,titleHeight+descriptionHeight})+style.FramePadding.y*2+(descriptionHeight>0?2.f:0.f);
    const ImVec2 start=ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();
    auto* d=ImGui::GetWindowDrawList();
    const ImVec4 accent=Color(n.kind,o);
    d->AddRectFilled(start,{start.x+width,start.y+height},ImGui::GetColorU32(detail::WithAlpha(accent,.10f)),style.FrameRounding);
    d->AddRectFilled({start.x+5,start.y+8},{start.x+8,start.y+height-8},ImGui::GetColorU32(accent),1.5f);
    const ImVec2 c{start.x+pad+6+icon*.5f,start.y+style.FramePadding.y+icon*.5f};
    d->AddCircleFilled(c,icon*.5f,ImGui::GetColorU32(accent),24);
    const ImVec4 ink=ContrastRatio({1,1,1,1},accent)>=ContrastRatio({0,0,0,1},accent)?ImVec4{1,1,1,1}:ImVec4{0,0,0,1};
    const ImU32 glyph=ImGui::GetColorU32(ink);
    if(n.kind==FeedbackKind::Success) detail::DrawCheck(d,c,icon*.5f,glyph,1.8f);
    else if(n.kind==FeedbackKind::Error) detail::DrawCross(d,c,icon*.34f,glyph,1.8f);
    else {
        const float direction=n.kind==FeedbackKind::Warning?1.f:-1.f;
        d->AddLine({c.x,c.y-icon*.25f*direction},{c.x,c.y+icon*.06f*direction},glyph,1.8f);
        d->AddCircleFilled({c.x,c.y+icon*.25f*direction},1.3f,glyph);
    }
    const float textX=start.x+pad+6+icon+style.ItemInnerSpacing.x;
    d->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{textX,start.y+style.FramePadding.y},ImGui::GetColorU32(ImGuiCol_Text),Safe(n.title),nullptr,textWidth);
    if(descriptionHeight>0)
        d->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{textX,start.y+style.FramePadding.y+titleHeight+2.f},
                   ImGui::GetColorU32(o.theme?o.theme->semantic.textSecondary:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)),n.description,nullptr,textWidth);
    ImGui::Dummy({width,height});
    bool dismiss=false;
    if(n.dismissible) {
        ImGui::SetCursorScreenPos({start.x+width-close-4,start.y+(height-close)*.5f});
        dismiss=ImGui::InvisibleButton("##dismiss",{close,close});
        const ImVec2 cc{start.x+width-close*.5f-4,start.y+height*.5f};
        if(ImGui::IsItemHovered()) d->AddCircleFilled(cc,close*.5f,ImGui::GetColorU32(detail::WithAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Text),.10f)));
        if(ImGui::IsItemFocused()) d->AddCircle(cc,close*.5f,ImGui::GetColorU32(ImGuiCol_NavCursor),24,1.5f);
        detail::DrawCross(d,cc,close*.32f,ImGui::GetColorU32(ImGui::IsItemHovered()?ImGuiCol_Text:ImGuiCol_TextDisabled));
        if(ImGui::IsItemHovered()||ImGui::IsItemFocused()) ImGui::SetTooltip("%s",Text(o,"dismiss","Dismiss"));
        dismiss=Annotate(Text(o,"dismiss","Dismiss"),"",accessibility::SemanticRole::Button,accessibility::SemanticAction::Press,false,o)||dismiss;
    }
    ImGui::SetCursorScreenPos({start.x,start.y+height});
    ImGui::Dummy({0,0});
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
        for(std::size_t i=0;i<count;++i) {if(NotificationCard(items[scratch[i]],now,o)) dismiss.Push(items[scratch[i]].id);}
    }ImGui::End();
}
bool InlineAlert(const char* id,const FeedbackView& item,ComponentOptions o) {ImGui::PushID(id);bool result=NotificationCard(item,0,o);ImGui::PopID();return result;}
bool PersistentBanner(const char* id,const FeedbackView& item,ComponentOptions o) {auto persistent=item;persistent.expiresAt=0;return InlineAlert(id,persistent,o);}
bool EmptyState(const char* id,const StateView& v,ComponentOptions o) {
    ImGui::PushID(id);ImGui::BeginGroup();
    const float available=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float block=std::min(available,ImGui::GetFontSize()*26.f);
    const float offset=(available-block)*.5f;
    const float originX=ImGui::GetCursorPosX();
    auto centred=[&](float width) {ImGui::SetCursorPosX(originX+offset+std::max(0.f,(block-std::min(width,block))*.5f));};
    const ImVec4 accent=Color(v.kind,o);
    if(v.icons && v.icon!=IconId::Count) {
        const float tile=ImGui::GetFontSize()*2.6f;
        centred(tile);const auto p=ImGui::GetCursorScreenPos();ImGui::Dummy({tile,tile});
        ImGui::GetWindowDrawList()->AddRectFilled(p,{p.x+tile,p.y+tile},ImGui::GetColorU32(detail::WithAlpha(accent,.14f)),tile*.3f);
        const float glyph=tile*.54f;
        ImGui::SetCursorScreenPos({p.x+(tile-glyph)*.5f,p.y+(tile-glyph)*.5f});
        ImGui::PushStyleColor(ImGuiCol_Text,accent);Icon(*v.icons,v.icon,{glyph});ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos({p.x,p.y+tile});ImGui::Dummy({0,0});
    }
    centred(ImGui::CalcTextSize(Safe(v.heading),nullptr,false,block).x);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::max(1.f,block-(ImGui::GetCursorPosX()-originX-offset)));
    ImGui::TextUnformatted(Safe(v.heading));ImGui::PopTextWrapPos();
    if(o.accessibility) {accessibility::SemanticNode n;n.id=ImGui::GetID("state");n.parent=o.parent;n.name=Safe(v.heading);n.description=Safe(v.description);n.role=accessibility::SemanticRole::Status;accessibility::AnnotateLastItem(*o.accessibility,n);}
    if(*Safe(v.description)) {
        centred(ImGui::CalcTextSize(v.description,nullptr,false,block).x);
        ImGui::PushStyleColor(ImGuiCol_Text,o.theme?o.theme->semantic.textSecondary:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::max(1.f,block-(ImGui::GetCursorPosX()-originX-offset)));
        ImGui::TextUnformatted(v.description);ImGui::PopTextWrapPos();ImGui::PopStyleColor();
    }
    bool result=false;ImGui::BeginDisabled(v.disabled);
    if(*Safe(v.action)) {
        centred(ImGui::CalcTextSize(v.action).x+ImGui::GetStyle().FramePadding.x*2);
        result=LabeledButton(v.action,"",{},false,o);
    }
    ImGui::EndDisabled();ImGui::EndGroup();ImGui::PopID();return result;
}
bool UnavailableState(const char* id,const StateView& view,ComponentOptions o) {return EmptyState(id,view,o);}
bool RetryState(const char* id,const StateView& view,ComponentOptions o) {return EmptyState(id,view,o);}
void CircularProgress(const char* id,const CircularProgressView& view,CircularProgressOptions layout,ComponentOptions o) {
    constexpr float pi=3.14159265358979323846f;
    constexpr const char* unavailable="\xE2\x80\x94";
    const float diameter=std::max(12.f,layout.diameter>0?layout.diameter:ImGui::GetFrameHeight()*3.f);
    const float stroke=std::clamp(layout.strokeWidth>0?layout.strokeWidth:diameter*.1f,1.f,diameter*.24f);
    const bool available=std::isfinite(view.fraction)&&view.fraction>=0.f;
    float fraction=available?std::clamp(view.fraction,0.f,1.f):0.f;
    const char* value=*Safe(view.value)?view.value:(available?"":unavailable);
    const ImVec4 text=o.theme?o.theme->semantic.text:ImGui::GetStyleColorVec4(ImGuiCol_Text);
    const ImVec4 foreground=available?Color(view.kind,o):(o.theme?o.theme->semantic.textDisabled:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    const bool animate=!o.theme||(o.theme->motion.enabled&&!o.theme->motion.reducedMotion);
    ImGui::PushID(id);ImGui::BeginGroup();
    // Values tween so that coarse host updates do not look like jumps.
    if(available&&o.animation) {
        const Motion motion=o.theme?o.theme->motion:Motion{};
        fraction=o.animation->Update(ImGui::GetID("circular-value"),fraction,ImGui::GetIO().DeltaTime,
                                     std::max(motion.controlSeconds*2.f,.05f),ImGui::GetFrameCount(),animate,motion.easing);
    }
    const ImVec2 top=ImGui::GetCursorScreenPos();ImGui::Dummy({diameter,diameter});
    const ImVec2 center{top.x+diameter*.5f,top.y+diameter*.5f};
    const float outline=detail::IsOutlined(ImGui::GetStyle())?ImGui::GetStyle().FrameBorderSize:0.f;
    const float radius=std::max(1.f,diameter*.5f-stroke*.5f-1.f-outline);
    auto* draw=ImGui::GetWindowDrawList();const int segments=std::clamp(static_cast<int>(diameter*.75f),24,96);
    detail::DrawRingOutline(draw,center,radius,stroke,segments);
    draw->AddCircle(center,radius,ImGui::GetColorU32(detail::WithAlpha(text,.11f)),segments,stroke);
    if(available&&fraction>0.f) {
        const float start=-pi*.5f;
        if(fraction>=.9999f) draw->AddCircle(center,radius,ImGui::GetColorU32(foreground),segments,stroke);
        else detail::DrawRingArc(draw,center,radius,stroke,start,start+2.f*pi*fraction,ImGui::GetColorU32(foreground));
    }
    const ImVec2 valueSize=ImGui::CalcTextSize(value);
    const float innerWidth=std::max(1.f,(radius-stroke*.5f-outline)*1.55f);
    const float textScale=std::min({diameter>=ImGui::GetFontSize()*4.f?1.15f:1.f,
                                  innerWidth/std::max(1.f,valueSize.x),
                                  innerWidth/std::max(1.f,valueSize.y)});
    draw->AddText(ImGui::GetFont(),ImGui::GetFontSize()*textScale,
                  {center.x-valueSize.x*textScale*.5f,center.y-valueSize.y*textScale*.5f},
                  ImGui::GetColorU32(available?text:foreground),value);
    if(*Safe(view.label)) {
        const float width=ImGui::CalcTextSize(view.label).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.f,(diameter-width)*.5f));
        ImGui::PushStyleColor(ImGuiCol_Text,o.theme?o.theme->semantic.textSecondary:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
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
void StageProgress(const char* id,const StageProgressView& view,ComponentOptions o) {
    const auto stages=view.stages;
    if(stages.empty()) return;
    int current=-1;
    for(std::size_t i=0;i<stages.size();++i) if(view.current&&stages[i].id==view.current){current=static_cast<int>(i);break;}
    if(current<0) for(std::size_t i=0;i<stages.size();++i)
        if(stages[i].status==ProgressStatus::Running||stages[i].status==ProgressStatus::Paused){current=static_cast<int>(i);break;}
    if(current<0) for(std::size_t i=0;i<stages.size();++i)
        if(stages[i].status!=ProgressStatus::Succeeded){current=static_cast<int>(i);break;}
    if(current<0) current=static_cast<int>(stages.size()-1);
    ImGui::PushID(id);ImGui::BeginGroup();
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const bool compact=stages.size()>8||width<ImGui::GetFontSize()*5.5f*static_cast<float>(stages.size());
    if(compact) {
        const auto& stage=stages[static_cast<std::size_t>(current)];
        ImGui::PushStyleColor(ImGuiCol_Text,ProgressStatusColor(stage.status,o));
        ImGui::TextWrapped("%s",Safe(stage.label));ImGui::PopStyleColor();
        if(o.accessibility) {
            accessibility::SemanticNode node;node.id=stage.id?stage.id:ImGui::GetItemID();
            node.parent=o.parent;node.name=Safe(stage.label);node.role=accessibility::SemanticRole::Status;
            node.state.selected=true;node.state.busy=stage.status==ProgressStatus::Running;
            node.state.invalid=stage.status==ProgressStatus::Failed;
            accessibility::AnnotateLastItem(*o.accessibility,node);
        }
        ImGui::TextDisabled("%d / %zu",current+1,stages.size());
    } else {
        const ImVec2 origin=ImGui::GetCursorScreenPos();
        const float markerHeight=std::max(12.f,ImGui::GetFrameHeight()*.75f);
        ImGui::Dummy({width,markerHeight});
        auto* draw=ImGui::GetWindowDrawList();
        const float step=width/static_cast<float>(stages.size());
        const float radius=std::min(markerHeight*.4f,ImGui::GetFontSize()*.5f);
        const float centerY=origin.y+markerHeight*.5f;
        const ImVec4 connector=o.theme?o.theme->semantic.border:ImGui::GetStyleColorVec4(ImGuiCol_Border);
        const bool outlined=detail::IsOutlined(ImGui::GetStyle());
        const float outline=ImGui::GetStyle().FrameBorderSize;
        const ImU32 outlineColor=ImGui::GetColorU32(ImGuiCol_Border);
        for(std::size_t i=0;i+1<stages.size();++i) {
            const float x1=origin.x+step*(static_cast<float>(i)+.5f)+radius;
            const float x2=origin.x+step*(static_cast<float>(i)+1.5f)-radius;
            const ImVec4 line=stages[i].status==ProgressStatus::Succeeded?ProgressStatusColor(stages[i].status,o):connector;
            if(outlined) draw->AddLine({x1,centerY},{x2,centerY},outlineColor,3.f+outline*2.f);
            draw->AddLine({x1,centerY},{x2,centerY},ImGui::GetColorU32(line),3.f);
        }
        for(std::size_t i=0;i<stages.size();++i) {
            const auto& stage=stages[i];const float x=origin.x+step*(static_cast<float>(i)+.5f);
            const ImVec4 color=ProgressStatusColor(stage.status,o);
            if(stage.status==ProgressStatus::Queued) draw->AddCircle({x,centerY},radius,ImGui::GetColorU32(color),24,2.f);
            else draw->AddCircleFilled({x,centerY},radius,ImGui::GetColorU32(color),24);
            if(outlined) draw->AddCircle({x,centerY},radius,outlineColor,24,outline);
            if(stage.status==ProgressStatus::Succeeded) {
                const ImVec4 ink=ContrastRatio({1,1,1,1},color)>=ContrastRatio({0,0,0,1},color)?ImVec4{1,1,1,1}:ImVec4{0,0,0,1};
                detail::DrawCheck(draw,{x,centerY},radius*1.1f,ImGui::GetColorU32(ink),1.7f);
            }
            if(static_cast<int>(i)==current) draw->AddCircle({x,centerY},radius+3.5f,ImGui::GetColorU32(detail::WithAlpha(color,.5f)),24,2.f);
        }
        if(ImGui::BeginTable("stage-labels",static_cast<int>(stages.size()),ImGuiTableFlags_SizingStretchSame)) {
            for(std::size_t i=0;i<stages.size();++i) {
                ImGui::TableNextColumn();
                const auto& stage=stages[i];
                ImGui::PushStyleColor(ImGuiCol_Text,ProgressStatusColor(stage.status,o));
                ImGui::TextWrapped("%s",Safe(stage.label));
                ImGui::PopStyleColor();
                if(o.accessibility) {
                    accessibility::SemanticNode node;node.id=stage.id?stage.id:ImGui::GetItemID();
                    node.parent=o.parent;node.name=Safe(stage.label);node.role=accessibility::SemanticRole::Status;
                    node.state.selected=static_cast<int>(i)==current;
                    node.state.busy=stage.status==ProgressStatus::Running;
                    node.state.invalid=stage.status==ProgressStatus::Failed;
                    accessibility::AnnotateLastItem(*o.accessibility,node);
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndGroup();ImGui::PopID();
}
StableId TaskProgressCard(const char* id,const TaskProgressCardView& view,TaskProgressCardOptions layout,ComponentOptions o) {
    const auto& style=ImGui::GetStyle();
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float innerWidth=std::max(1.f,width-style.WindowPadding.x*2.f-style.ChildBorderSize*2.f);
    const float titleWidth=std::max(1.f,innerWidth-ImGui::GetFrameHeight()*.55f-style.ItemSpacing.x);
    float contentHeight=0.f;
    const auto addTextHeight=[&](const char* text,float textWidth) {
        if(*Safe(text)) {
            if(contentHeight>0.f) contentHeight+=style.ItemSpacing.y;
            contentHeight+=ImGui::CalcTextSize(text,nullptr,false,textWidth).y;
        }
    };
    addTextHeight(view.title,titleWidth);
    if(!layout.compact) addTextHeight(view.description,innerWidth);
    ProgressTrackView progress;progress.fraction=view.fraction;progress.status=view.status;
    progress.label=view.stage;progress.supplemental=view.supplemental;
    if(contentHeight>0.f) contentHeight+=style.ItemSpacing.y;
    contentHeight+=detail::ProgressTrackContentHeight(progress,innerWidth,o);
    const bool available=std::isfinite(view.fraction)&&view.fraction>=0.f;
    int operationRows=0;float rowWidth=0.f;
    for(const auto& command:view.operations) {
        if(!command.id||!*Safe(command.label)) continue;
        const float buttonWidth=std::min(innerWidth,ImGui::CalcTextSize(Safe(command.label)).x+style.FramePadding.x*2.f);
        if(rowWidth>0.f&&rowWidth+style.ItemSpacing.x+buttonWidth>innerWidth) {++operationRows;rowWidth=0.f;}
        if(rowWidth>0.f) rowWidth+=style.ItemSpacing.x;
        rowWidth+=buttonWidth;
    }
    if(rowWidth>0.f) ++operationRows;
    if(operationRows) contentHeight+=style.ItemSpacing.y+operationRows*ImGui::GetFrameHeight();
    const float cardHeight=std::max(ImGui::GetFrameHeight()*2.f,contentHeight+style.WindowPadding.y*2.f+style.ChildBorderSize*2.f);
    ImGui::PushID(id);if(view.id) Push(view.id);
    const StableId taskNodeId=view.id?view.id:ImGui::GetID("task-root");
    StableId requested=0;
    const bool visible=BeginCard("card",{0,cardHeight},o);
    if(visible) {
        const ImVec2 cardMinimum=ImGui::GetWindowPos();
        const ImVec2 cardMaximum{cardMinimum.x+ImGui::GetWindowSize().x,cardMinimum.y+ImGui::GetWindowSize().y};
        ImGui::BeginGroup();
        const float marker=std::max(4.f,ImGui::GetFrameHeight()*.28f);
        const ImVec2 markerPosition=ImGui::GetCursorScreenPos();
        ImGui::Dummy({marker,ImGui::GetFontSize()});
        ImGui::GetWindowDrawList()->AddCircleFilled({markerPosition.x+marker*.5f,markerPosition.y+ImGui::GetFontSize()*.5f},
                                                    marker*.5f,ImGui::GetColorU32(ProgressStatusColor(view.status,o)),16);
        ImGui::SameLine();
        ImGui::TextWrapped("%s",Safe(view.title));
        if(!layout.compact&&*Safe(view.description)) ImGui::TextWrapped("%s",view.description);
        auto progressOptions=o;progressOptions.parent=taskNodeId;
        ProgressTrack("progress",progress,progressOptions);
        if(operationRows) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,layout.compact?ImVec2(4.f,2.f):style.FramePadding);
            float used=0.f;
            for(const auto& command:view.operations) {
                if(!command.id||!*Safe(command.label)) continue;
                const float buttonWidth=std::min(innerWidth,ImGui::CalcTextSize(Safe(command.label)).x+ImGui::GetStyle().FramePadding.x*2.f);
                if(used>0.f&&used+style.ItemSpacing.x+buttonWidth>innerWidth) used=0.f;
                else if(used>0.f) {ImGui::SameLine();used+=style.ItemSpacing.x;}
                Push(command.id);
                ImGui::BeginDisabled(command.disabled);
                const ImVec2 buttonSize{buttonWidth,0};
                const bool hit=ImGui::Button("###task-action",buttonSize);
                const ImVec2 minimum=ImGui::GetItemRectMin(),maximum=ImGui::GetItemRectMax();
                const ImVec2 textPosition{minimum.x+ImGui::GetStyle().FramePadding.x,minimum.y+ImGui::GetStyle().FramePadding.y};
                const ImVec2 clipMinimum{minimum.x+ImGui::GetStyle().FramePadding.x,minimum.y};
                const ImVec2 clipMaximum{std::max(clipMinimum.x,maximum.x-ImGui::GetStyle().FramePadding.x),maximum.y};
                auto* draw=ImGui::GetWindowDrawList();
                draw->PushClipRect(clipMinimum,clipMaximum,true);
                draw->AddText(textPosition,ImGui::GetColorU32(command.disabled?ImGuiCol_TextDisabled:ImGuiCol_Text),Safe(command.label));
                draw->PopClipRect();
                if(command.disabled&&ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)&&*Safe(command.disabledReason))
                    ImGui::SetTooltip("%s",command.disabledReason);
                if(hit&&!command.disabled) requested=command.id;
                if(o.accessibility) {
                    accessibility::SemanticNode node;node.id=ImGui::GetItemID();node.parent=taskNodeId;
                    node.name=Safe(command.label);node.description=command.disabled?Safe(command.disabledReason):Safe(command.shortcut);
                    node.role=accessibility::SemanticRole::Button;node.actions=accessibility::SemanticAction::Press;
                    node.state.disabled=command.disabled;
                    if(!command.disabled&&o.accessibility->Take(node.id,accessibility::SemanticAction::Press)) requested=command.id;
                    accessibility::AnnotateLastItem(*o.accessibility,node);
                }
                ImGui::EndDisabled();Pop();
                used+=buttonWidth;
            }
            ImGui::PopStyleVar();
        }
        if(o.accessibility) {
            accessibility::SemanticNode node;node.id=taskNodeId;node.parent=o.parent;
            node.name=Safe(view.title);node.description=Safe(view.description);node.value=Safe(view.stage);
            node.role=accessibility::SemanticRole::Group;
            node.state.busy=view.status==ProgressStatus::Running;
            node.state.invalid=view.status==ProgressStatus::Failed;
            if(available){node.numericValue=std::clamp(view.fraction,0.f,1.f);node.minimumValue=0;node.maximumValue=1;}
            node.minimum=cardMinimum;node.maximum=cardMaximum;
            o.accessibility->Add(node);
        }
        ImGui::EndGroup();
    }
    EndCard();if(view.id) Pop();ImGui::PopID();
    return requested;
}
bool BeginCard(const char* id,ImVec2 size,ComponentOptions o) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg,o.theme?o.theme->semantic.surfaceRaised:ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize,o.theme?o.theme->stroke.border:1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,o.theme?o.theme->radius.overlay:ImGui::GetStyle().FrameRounding+4.f);
    return ImGui::BeginChild(id,size,ImGuiChildFlags_Borders);
}
void EndCard() {ImGui::EndChild();ImGui::PopStyleVar(2);ImGui::PopStyleColor();}
bool SectionHeader(const char* id,const char* label,bool& open,ComponentOptions o) {
    ImGui::PushID(id);
    const auto pad=ImGui::GetStyle().FramePadding;const float lead=ImGui::GetFontSize()*1.1f;
    ImGui::PushStyleColor(ImGuiCol_Button,{0,0,0,0});ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{pad.x+lead,pad.y});
    const bool hit=LabeledButton(label,Text(o,"toggle_section","Expand or collapse"),{std::max(1.f,ImGui::GetContentRegionAvail().x),0},false,o);
    ImGui::PopStyleVar(2);ImGui::PopStyleColor();
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    detail::DrawChevron(ImGui::GetWindowDrawList(),{a.x+pad.x+lead*.35f,(a.y+b.y)*.5f},ImGui::GetFontSize()*.6f,ImGui::GetColorU32(ImGuiCol_TextDisabled),open?ImGuiDir_Down:ImGuiDir_Right,1.5f);
    if(hit) open=!open;
    ImGui::PopID();return open;
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
