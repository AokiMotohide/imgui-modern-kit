// This file implements or exercises the deprecated spellings on purpose.
#define IMKIT_DEPRECATED(message)
#include <imkit/patterns.h>
#include "progress_internal.h"
#include "draw_internal.h"
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
    ImGui::PushID(id);
    const auto& style=ImGui::GetStyle();
    const float icon=ImGui::GetFontSize()*.9f,lead=icon+style.ItemInnerSpacing.x;
    ImGui::SetNextItemWidth(std::max(1.f,ImGui::GetContentRegionAvail().x));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{style.FramePadding.x+lead,style.FramePadding.y});
    bool changed=ImGui::InputTextWithHint("##search",Text(o,"search","Search"),text,capacity);
    ImGui::PopStyleVar();
    Node(o,Text(o,"search","Search"),accessibility::SemanticRole::TextField);
    const ImVec2 minimum=ImGui::GetItemRectMin(),maximum=ImGui::GetItemRectMax();
    auto* draw=ImGui::GetWindowDrawList();
    const ImU32 muted=ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImVec2 lens{minimum.x+style.FramePadding.x+icon*.42f,(minimum.y+maximum.y)*.5f-icon*.06f};
    draw->AddCircle(lens,icon*.3f,muted,16,1.5f);
    draw->AddLine({lens.x+icon*.21f,lens.y+icon*.21f},{lens.x+icon*.42f,lens.y+icon*.42f},muted,1.5f);
    if(*text) {
        const float size=maximum.y-minimum.y;
        ImGui::SetCursorScreenPos({maximum.x-size,minimum.y});
        const bool clear=ImGui::InvisibleButton("##clear",{size,size});
        const ImVec2 c{maximum.x-size*.5f,(minimum.y+maximum.y)*.5f};
        if(ImGui::IsItemHovered()) draw->AddCircleFilled(c,size*.3f,ImGui::GetColorU32(ImGuiCol_Button));
        detail::DrawCross(draw,c,size*.2f,ImGui::GetColorU32(ImGui::IsItemHovered()?ImGuiCol_Text:ImGuiCol_TextDisabled),1.5f);
        if(ImGui::IsItemHovered()||ImGui::IsItemFocused()) ImGui::SetTooltip("%s",Text(o,"clear_search","Clear search"));
        if(clear) {text[0]=0;changed=true;}
    }
    ImGui::PopID();return changed;
}
StableId CommandPalette(const char* id,CommandPaletteState& s,std::span<const Command> commands,ComponentOptions o) {
    if(s.open && !ImGui::IsPopupOpen(id)) ImGui::OpenPopup(id);
    if(!s.open) return 0;
    ImGui::SetNextWindowSize({std::max(1.f,std::min(560.f,ImGui::GetMainViewport()->WorkSize.x-24)),0},ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints({1,1},{std::max(1.f,ImGui::GetMainViewport()->WorkSize.x),std::max(1.f,ImGui::GetMainViewport()->WorkSize.y)});
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
            if(o.accessibility) {
                accessibility::SemanticNode node;node.id=ImGui::GetItemID();node.parent=o.parent;node.name=c.label;
                node.description=c.disabled?c.disabledReason:c.shortcut;node.role=accessibility::SemanticRole::MenuItem;
                node.state.selected=index==s.focused;node.actions=accessibility::SemanticAction::Press|accessibility::SemanticAction::Focus;
                accessibility::AnnotateLastItem(*o.accessibility,node);
                if(o.accessibility->Take(node.id,accessibility::SemanticAction::Press)&&!c.disabled)hit=true;
                if(o.accessibility->Take(node.id,accessibility::SemanticAction::Focus)&&!c.disabled){s.focused=index;ImGui::SetKeyboardFocusHere(-1);ImGui::SetNavCursorVisible(true);}
            }
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
        if(visual && ImGui::GetContentRegionAvail().x>width+20) {
            ImGui::SameLine(0,0);const auto p=ImGui::GetCursorScreenPos();const float size=ImGui::GetFontSize();
            ImGui::Dummy({size,ImGui::GetFrameHeight()});
            detail::DrawChevron(ImGui::GetWindowDrawList(),{p.x+size*.5f,p.y+ImGui::GetFrameHeight()*.5f},size*.55f,ImGui::GetColorU32(ImGuiCol_TextDisabled),
                                o.locale && o.locale->direction==TextDirection::RTL?ImGuiDir_Left:ImGuiDir_Right,1.4f);
            ImGui::SameLine(0,0);
        }
        ImGui::PushID(i); if(ActionButton(labels[i],ActionVariant::Ghost,{},o)) result=i; ImGui::PopID();
    } ImGui::PopID(); return result;
}
SplitButtonResult SplitButton(const char* id,const char* label,ComponentOptions o) {
    ImGui::PushID(id);auto result=SplitButtonResult::None;
    const auto& style=ImGui::GetStyle();
    const float height=ImGui::GetFrameHeight(),menuWidth=height*.85f;
    const float labelWidth=ImGui::CalcTextSize(label,nullptr,true).x+style.FramePadding.x*2;
    const ImVec2 origin=ImGui::GetCursorScreenPos();
    const ImVec4 fill=o.theme?o.theme->colors.accent:ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    const ImVec4 ink=o.theme?o.theme->colors.onAccent:ImVec4(1,1,1,1);
    const float rounding=style.FrameRounding;
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin,{origin.x+labelWidth+menuWidth,origin.y+height},ImGui::GetColorU32(fill),rounding);
    auto part=[&](const char* partId,ImVec2 size,ImDrawFlags corners,ImVec2 at,const char* name,accessibility::SemanticAction action) {
        ImGui::SetCursorScreenPos(at);
        bool hit=ImGui::InvisibleButton(partId,size);
        if(ImGui::IsItemFocused()) ImGui::SetNavCursorVisible(true);
        if(ImGui::IsItemHovered()||ImGui::IsItemActive())
            draw->AddRectFilled(at,{at.x+size.x,at.y+size.y},ImGui::GetColorU32(ImVec4(ink.x,ink.y,ink.z,ImGui::IsItemActive()?.28f:.14f)),rounding,corners);
        if(ImGui::IsItemFocused()) draw->AddRect(at,{at.x+size.x,at.y+size.y},ImGui::GetColorU32(ImGuiCol_NavCursor),rounding,corners,2.f);
        if(o.accessibility) {
            accessibility::SemanticNode node;node.id=ImGui::GetItemID();node.parent=o.parent;node.name=name;
            node.role=accessibility::SemanticRole::Button;node.actions=accessibility::SemanticAction::Press|accessibility::SemanticAction::Focus;
            hit=o.accessibility->Take(node.id,action)||hit;
            accessibility::AnnotateLastItem(*o.accessibility,node);
        }
        return hit;
    };
    if(part("##primary",{labelWidth,height},ImDrawFlags_RoundCornersLeft,origin,label,accessibility::SemanticAction::Press)) result=SplitButtonResult::Primary;
    draw->AddText({origin.x+style.FramePadding.x,origin.y+(height-ImGui::GetFontSize())*.5f},ImGui::GetColorU32(ink),label,std::strstr(label,"##"));
    const ImVec2 menuAt{origin.x+labelWidth,origin.y};
    if(part("##menu",{menuWidth,height},ImDrawFlags_RoundCornersRight,menuAt,Text(o,"menu","Menu"),accessibility::SemanticAction::Press)) result=SplitButtonResult::Menu;
    draw->AddLine({menuAt.x,origin.y+height*.25f},{menuAt.x,origin.y+height*.75f},ImGui::GetColorU32(ImVec4(ink.x,ink.y,ink.z,.4f)));
    detail::DrawChevron(draw,{menuAt.x+menuWidth*.5f,origin.y+height*.5f},ImGui::GetFontSize()*.6f,ImGui::GetColorU32(ink),ImGuiDir_Down,1.6f);
    ImGui::SetCursorScreenPos(origin);ImGui::Dummy({labelWidth+menuWidth,height});
    ImGui::PopID();return result;
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
    const float pad=ImGui::GetFontSize()*1.3f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{pad,pad});
    bool visible=ImGui::BeginPopupModal(id,nullptr,ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();
    if(visible) {
        if(o.theme) OverlayDecoration(*o.theme,o.animation);
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
        ImGui::Dummy({0,ImGui::GetStyle().ItemSpacing.y});
        const auto* cancel=Text(o,"cancel","Cancel");const auto* accept=Text(o,"accept","Accept");
        const float footer=ImGui::CalcTextSize(cancel).x+ImGui::CalcTextSize(accept).x+ImGui::GetStyle().FramePadding.x*4+ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(),ImGui::GetWindowContentRegionMax().x-footer));
        if(ActionButton(cancel,ActionVariant::Secondary,{},o)) result=DialogResult::Cancel;
        ImGui::SameLine(); if(ActionButton(accept,ActionVariant::Primary,{},o)) result=DialogResult::Accept;
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
    const ImVec4 error=o.theme?o.theme->semantic.error:ImVec4(.8f,.2f,.25f,1);
    ImGui::TextWrapped("%s",f.label);
    if(f.required) {ImGui::SameLine(0,2);ImGui::TextColored(error,"*");}
    if(*f.hint) {
        ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s",f.hint);ImGui::PopStyleColor();
    }
    if(*f.validation) {
        const float size=ImGui::GetFontSize()*.85f;const auto p=ImGui::GetCursorScreenPos();
        auto* draw=ImGui::GetWindowDrawList();
        const ImVec2 c{p.x+size*.5f,p.y+ImGui::GetTextLineHeight()*.5f};
        draw->AddCircleFilled(c,size*.5f,ImGui::GetColorU32(error));
        draw->AddLine({c.x,c.y-size*.2f},{c.x,c.y+size*.04f},ImGui::GetColorU32(ImGuiCol_WindowBg),1.6f);
        draw->AddCircleFilled({c.x,c.y+size*.22f},1.f,ImGui::GetColorU32(ImGuiCol_WindowBg));
        ImGui::Dummy({size,ImGui::GetTextLineHeight()});ImGui::SameLine(0,ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::PushStyleColor(ImGuiCol_Text,error);ImGui::TextWrapped("%s",f.validation);ImGui::PopStyleColor();
    }
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);return true;
}
void EndFormField() { ImGui::EndGroup(); ImGui::PopID(); }
namespace {
ImVec4 ProgressStatusColor(ProgressStatus status,ComponentOptions o) {
    if(o.theme) {
        switch(status) {
        case ProgressStatus::Succeeded:return o.theme->semantic.success;
        case ProgressStatus::Failed:return o.theme->semantic.error;
        case ProgressStatus::Paused:return o.theme->semantic.warning;
        case ProgressStatus::Cancelled:return o.theme->semantic.textDisabled;
        case ProgressStatus::Queued:return o.theme->semantic.textSecondary;
        default:return o.theme->semantic.accent;
        }
    }
    switch(status) {
    case ProgressStatus::Failed:return ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram);
    case ProgressStatus::Paused:return ImGui::GetStyleColorVec4(ImGuiCol_NavHighlight);
    case ProgressStatus::Cancelled:
    case ProgressStatus::Queued:return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    default:return ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    }
}
}
namespace {
bool MotionAllowed(ComponentOptions o) {
    return !o.theme||(o.theme->motion.enabled&&!o.theme->motion.reducedMotion);
}
ImVec4 TextColor(ComponentOptions o) { return o.theme?o.theme->semantic.text:ImGui::GetStyleColorVec4(ImGuiCol_Text); }
ImVec4 SecondaryColor(ComponentOptions o) { return o.theme?o.theme->semantic.textSecondary:ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled); }
float ControlRadius(ComponentOptions o) { return o.theme?o.theme->radius.control:ImGui::GetStyle().FrameRounding; }
// Light moving highlight clipped to [minimum, maximum]; used by progress fills and skeletons.
void DrawSheen(ImDrawList* draw,ImVec2 minimum,ImVec2 maximum,float phase,ImVec4 color) {
    const float width=maximum.x-minimum.x,band=std::max((maximum.y-minimum.y)*4.f,width*.4f);
    const float x=minimum.x-band+phase*(width+band);
    const ImU32 clear=ImGui::ColorConvertFloat4ToU32(detail::WithAlpha(color,0.f)),lit=ImGui::ColorConvertFloat4ToU32(color);
    draw->PushClipRect(minimum,maximum,true);
    draw->AddRectFilledMultiColor({x,minimum.y},{x+band*.5f,maximum.y},clear,lit,lit,clear);
    draw->AddRectFilledMultiColor({x+band*.5f,minimum.y},{x+band,maximum.y},lit,clear,clear,lit);
    draw->PopClipRect();
}
}
namespace {
void ProgressTrackImpl(const char* id,float fraction,ProgressStatus status,ComponentOptions o,const ProgressTrackView* thumbView) {
    const bool available=std::isfinite(fraction)&&fraction>=0.f;
    const float value=available?std::clamp(fraction,0.f,1.f):0.f;
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float height=detail::ProgressTrackBarHeight();
    const float radius=height*.5f;
    const ImVec2 position=ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    ImGui::Dummy({width,height});
    auto* draw=ImGui::GetWindowDrawList();
    const Motion motion=o.theme?o.theme->motion:Motion{};
    const bool animate=MotionAllowed(o);
    const ImVec4 track=detail::WithAlpha(TextColor(o),.13f);
    const ImVec4 foreground=ProgressStatusColor(status,o);
    draw->AddRectFilled(position,{position.x+width,position.y+height},ImGui::GetColorU32(track),radius,ImDrawFlags_RoundCornersAll);
    if(available) {
        float shown=value;
        // Values tween so that coarse host updates do not look like jumps.
        if(o.animation) shown=o.animation->Update(ImGui::GetID("progress-value"),value,ImGui::GetIO().DeltaTime,
                                                  std::max(motion.controlSeconds*1.5f,.05f),ImGui::GetFrameCount(),animate,motion.easing);
        const float fill=std::max(height,width*shown);
        if(shown>0.f) {
            const ImVec2 end{position.x+fill,position.y+height};
            draw->AddRectFilled(position,end,ImGui::GetColorU32(foreground),radius,ImDrawFlags_RoundCornersAll);
            if(status==ProgressStatus::Running&&animate&&fill>height*3.f)
                DrawSheen(draw,position,end,static_cast<float>(std::fmod(std::max(0.0,ImGui::GetTime()),1.8)/1.8),{1,1,1,.30f});
        }
        if(thumbView&&thumbView->thumb) {
            // An empty bar still shows the thumb, at the start of the track.
            ProgressThumbInfo info;
            info.draw=draw;info.trackMin=position;info.trackMax={position.x+width,position.y+height};
            info.head={position.x+(shown>0.f?fill:0.f),position.y+height*.5f};
            info.fraction=shown;info.status=status;info.animate=animate;
            thumbView->thumb(info,thumbView->thumbUser);
        }
    } else if(status==ProgressStatus::Running) {
        const float segment=std::min(width,std::max(height*3.f,width*.28f));
        // Ping-pong with easing at both ends so the segment never snaps back.
        float phase=.5f;
        if(animate) {
            const float t=static_cast<float>(std::fmod(std::max(0.0,ImGui::GetTime()),2.2)/2.2);
            const float triangle=t<.5f?t*2.f:2.f-t*2.f;
            phase=triangle*triangle*(3.f-2.f*triangle);
        }
        const float left=position.x+phase*(width-segment);
        draw->AddRectFilled({left,position.y},{left+segment,position.y+height},ImGui::GetColorU32(foreground),radius,ImDrawFlags_RoundCornersAll);
        if(thumbView&&thumbView->thumb) {
            ProgressThumbInfo info;
            info.draw=draw;info.trackMin=position;info.trackMax={position.x+width,position.y+height};
            info.head={left+segment*.5f,position.y+height*.5f};
            info.fraction=-1.f;info.status=status;info.animate=animate;
            thumbView->thumb(info,thumbView->thumbUser);
        }
    }
    detail::DrawOutline(draw,position,{position.x+width,position.y+height},radius);
    if(o.accessibility) {
        accessibility::SemanticNode node;
        node.id=ImGui::GetID("progress-track");node.parent=o.parent;
        node.name=Text(o,"progress","Progress");node.value=detail::ProgressStatusLabel(status,o);
        node.role=accessibility::SemanticRole::Progress;
        node.state.readOnly=true;node.state.busy=status==ProgressStatus::Running&&!available;
        node.state.invalid=status==ProgressStatus::Failed;
        if(available) node.numericValue=value;
        node.minimumValue=0;node.maximumValue=1;
        accessibility::AnnotateLastItem(*o.accessibility,node);
    }
    ImGui::PopID();
}
}
void ProgressTrack(const char* id,float fraction,ProgressStatus status,ComponentOptions o) {
    ProgressTrackImpl(id,fraction,status,o,nullptr);
}
void ProgressTrack(const char* id,const ProgressTrackView& view,ComponentOptions o) {
    const bool available=std::isfinite(view.fraction)&&view.fraction>=0.f;
    const char* status=detail::ProgressStatusLabel(view.status,o);
    const ImVec2 top=ImGui::GetCursorScreenPos();
    ImGui::PushID(id);ImGui::BeginGroup();
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const auto header=detail::MakeProgressHeader(view,o);
    const float gap=detail::ProgressHeaderGap(),spacing=ImGui::GetStyle().ItemSpacing.y;
    if(header.any) {
        const ImVec2 origin=ImGui::GetCursorScreenPos();
        const float headerHeight=detail::ProgressHeaderHeight(header,width);
        ImGui::Dummy({width,headerHeight});
        auto* draw=ImGui::GetWindowDrawList();
        if(*header.left)
            draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),origin,ImGui::GetColorU32(TextColor(o)),
                          header.left,nullptr,detail::ProgressHeaderLeftWidth(header,width));
        if(*header.right) {
            const float right=ImGui::CalcTextSize(header.right).x;
            draw->AddText({origin.x+width-right,origin.y},ImGui::GetColorU32(SecondaryColor(o)),header.right);
        }
        // The layout reserved ItemSpacing below the header; replace it with the tighter gap.
        ImGui::SetCursorScreenPos({origin.x,origin.y+headerHeight+gap});
    }
    auto trackOptions=o;trackOptions.accessibility=nullptr;
    if(view.thumb&&view.thumbHeight>0.f) {
        // Leave room above the track for the host's drawing, without the usual item spacing.
        const ImVec2 origin=ImGui::GetCursorScreenPos();
        ImGui::Dummy({width,view.thumbHeight});
        ImGui::SetCursorScreenPos({origin.x,origin.y+view.thumbHeight});
    }
    ProgressTrackImpl("track",view.fraction,view.status,trackOptions,&view);
    if(*detail::SafeProgressText(view.supplemental)) {
        ImGui::SetCursorScreenPos({top.x,ImGui::GetItemRectMax().y+gap});
        ImGui::PushStyleColor(ImGuiCol_Text,SecondaryColor(o));
        ImGui::TextWrapped("%s",view.supplemental);ImGui::PopStyleColor();
    }
    (void)spacing;
    ImGui::EndGroup();
    const ImVec2 bottom=ImGui::GetItemRectMax();
    if(o.accessibility) {
        accessibility::SemanticNode node;
        node.id=ImGui::GetID("progress");node.parent=o.parent;node.name=detail::SafeProgressText(view.label);
        node.description=detail::SafeProgressText(view.supplemental);node.value=status;
        node.role=accessibility::SemanticRole::Progress;node.state.readOnly=true;
        node.state.busy=view.status==ProgressStatus::Running&&!available;
        node.state.invalid=view.status==ProgressStatus::Failed;
        if(available){node.numericValue=std::clamp(view.fraction,0.f,1.f);node.minimumValue=0;node.maximumValue=1;}
        node.minimum=top;node.maximum=bottom;
        o.accessibility->Add(node);
    }
    ImGui::PopID();
}
void Progress(const char* id,float fraction,const char* label,ComponentOptions o) {
    ProgressTrackView view;view.fraction=fraction;view.status=ProgressStatus::Running;view.label=label?label:"";
    ImGui::PushID(id);const StableId semanticId=ImGui::GetID(id);ImGui::PopID();
    const ImVec2 minimum=ImGui::GetCursorScreenPos();
    auto trackOptions=o;trackOptions.accessibility=nullptr;
    ProgressTrack(id,view,trackOptions);
    if(o.accessibility) {
        accessibility::SemanticNode node;node.id=semanticId;node.parent=o.parent;node.name=view.label;
        node.role=accessibility::SemanticRole::Progress;node.state.readOnly=true;
        node.state.busy=!std::isfinite(fraction)||fraction<0.f;
        if(std::isfinite(fraction)&&fraction>=0.f) {node.numericValue=std::clamp(fraction,0.f,1.f);node.minimumValue=0;node.maximumValue=1;}
        node.minimum=minimum;node.maximum=ImGui::GetItemRectMax();
        o.accessibility->Add(node);
    }
}
void Spinner(const char* id,ComponentOptions o) {
    const float size=ImGui::GetFrameHeight();const ImVec2 p=ImGui::GetCursorScreenPos();
    const ImGuiID semanticId=ImGui::GetID(id);
    ImGui::PushID(id);ImGui::Dummy({size,size});
    const bool animate=MotionAllowed(o);
    const float t=animate?static_cast<float>(std::fmod(std::max(0.0,ImGui::GetTime()),1000.0)):0.f;
    // The arc grows and shrinks while it rotates so the motion never looks mechanical.
    const float start=t*5.2f,sweep=animate?1.1f+3.4f*(.5f+.5f*std::sin(t*3.1f)):4.2f;
    auto* draw=ImGui::GetWindowDrawList();
    const ImU32 color=ImGui::GetColorU32(o.theme?o.theme->semantic.accent:ImGui::GetStyleColorVec4(ImGuiCol_Text));
    const float stroke=std::max(2.f,size*.09f),radius=size*.3f;
    const ImVec2 center{p.x+size*.5f,p.y+size*.5f};
    draw->AddCircle(center,radius,ImGui::GetColorU32(detail::WithAlpha(TextColor(o),.12f)),32,stroke);
    draw->PathArcTo(center,radius,start,start+sweep,24);
    draw->PathStroke(color,ImDrawFlags_None,stroke);
    for(float angle:{start,start+sweep})
        draw->AddCircleFilled({center.x+std::cos(angle)*radius,center.y+std::sin(angle)*radius},stroke*.5f,color);
    Node(o,Text(o,"loading","Loading"),accessibility::SemanticRole::Status,semanticId);
    ImGui::PopID();
}
void Skeleton(const char* id,ImVec2 size,ComponentOptions o) {
    auto p=ImGui::GetCursorScreenPos(); ImGui::Dummy(size);
    const ImVec2 end{p.x+size.x,p.y+size.y};
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(p,end,ImGui::GetColorU32(detail::WithAlpha(TextColor(o),.09f)),ControlRadius(o));
    if(MotionAllowed(o))
        DrawSheen(draw,p,end,static_cast<float>(std::fmod(std::max(0.0,ImGui::GetTime()),1.6)/1.6),detail::WithAlpha(TextColor(o),.07f));
    Node(o,Text(o,"loading","Loading"),accessibility::SemanticRole::Status,ImGui::GetID(id));
}
void EmptyState(const char* message) {
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s",message);ImGui::PopStyleColor();
}
void LoadingState(const char* message,ComponentOptions o) { Spinner("loading",o); ImGui::SameLine(); ImGui::AlignTextToFramePadding(); ImGui::TextWrapped("%s",message); }
bool ErrorState(const char* message,ComponentOptions o) {
    const float size=ImGui::GetFontSize();const auto p=ImGui::GetCursorScreenPos();
    const ImVec4 error=o.theme?o.theme->semantic.error:ImVec4(.8f,.2f,.25f,1);
    ImGui::Dummy({size,size});
    auto* draw=ImGui::GetWindowDrawList();
    const ImVec2 c{p.x+size*.5f,p.y+size*.5f};
    draw->AddCircleFilled(c,size*.5f,ImGui::GetColorU32(detail::WithAlpha(error,.16f)));
    draw->AddLine({c.x,c.y-size*.2f},{c.x,c.y+size*.05f},ImGui::GetColorU32(error),1.8f);
    draw->AddCircleFilled({c.x,c.y+size*.22f},1.1f,ImGui::GetColorU32(error));
    ImGui::SameLine();ImGui::TextWrapped("%s",message);
    return ActionButton(Text(o,"retry","Retry"),ActionVariant::Secondary,{},o);
}
const char* ToastRegion(std::span<const Notification> notifications,double now,ComponentOptions o) {
    const char* result=nullptr; for(auto& n:notifications) if(NotificationCard(n,now,o)) result=n.id; return result;
}
bool Pagination(const char* id,int& page,int count,ComponentOptions o) {
    ImGui::PushID(id);int before=page;count=std::max(0,count);page=std::clamp(page,0,std::max(0,count-1));
    const float side=ImGui::GetFrameHeight();
    auto step=[&](const char* name,ImGuiDir direction,bool disabled,const char* tooltip) {
        ImGui::BeginDisabled(disabled);
        ImGui::PushStyleColor(ImGuiCol_Button,{0,0,0,0});ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
        const bool hit=ImGui::Button(name,{side,side});
        ImGui::PopStyleVar();ImGui::PopStyleColor();
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        detail::DrawChevron(ImGui::GetWindowDrawList(),{(a.x+b.x)*.5f,(a.y+b.y)*.5f},side*.34f,ImGui::GetColorU32(ImGuiCol_Text),direction);
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)||ImGui::IsItemFocused()) ImGui::SetTooltip("%s",tooltip);
        Node(o,tooltip,accessibility::SemanticRole::Button);
        ImGui::EndDisabled();return hit;
    };
    if(step("##previous",ImGuiDir_Left,page==0,Text(o,"previous","Previous"))) --page;
    // Window of pages: first, last and the neighbours of the current page; gaps become an ellipsis.
    int last=-1;
    for(int i=0;i<count;++i) {
        const bool shown=i==0||i==count-1||std::abs(i-page)<=1||(page<3&&i<5)||(page>count-4&&i>count-6);
        if(!shown) continue;
        ImGui::SameLine();
        if(last>=0&&i-last>1) {
            ImGui::AlignTextToFramePadding();ImGui::TextDisabled("...");ImGui::SameLine();
        }
        char label[24];std::snprintf(label,sizeof(label),"%d",i+1);
        ImGui::PushID(i);
        auto number=o;
        if(ActionButton(label,i==page?ActionVariant::Primary:ActionVariant::Ghost,{std::max(side,ImGui::CalcTextSize(label).x+ImGui::GetStyle().FramePadding.x*2),0},number)) page=i;
        ImGui::PopID();last=i;
    }
    if(!count) {ImGui::SameLine();ImGui::AlignTextToFramePadding();ImGui::TextDisabled("0 / 0");}
    ImGui::SameLine();
    if(step("##next",ImGuiDir_Right,page>=count-1,Text(o,"next","Next"))) ++page;
    ImGui::PopID();return page!=before;
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
                        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,{0,.5f}); // Centre cell text in the row.
                        bool hit=ImGui::Selectable(label,selected,ImGuiSelectableFlags_AllowDoubleClick,{0,ImGui::GetFrameHeight()});
                        ImGui::PopStyleVar();
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
