#include <imkit/shell.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>

namespace imkit {
namespace {
StatusKind ToStatusKind(FeedbackKind kind) {
    switch(kind) {
    case FeedbackKind::Success:return StatusKind::Success;
    case FeedbackKind::Warning:return StatusKind::Warning;
    case FeedbackKind::Error:return StatusKind::Error;
    default:return StatusKind::Neutral;
    }
}

bool Contains(const char* text,const char* query) {
    if(!query||!*query)return true;
    std::string value=text?text:"",needle=query;
    auto fold=[](unsigned char c){return c<128?static_cast<char>(std::tolower(c)):static_cast<char>(c);};
    std::transform(value.begin(),value.end(),value.begin(),fold);
    std::transform(needle.begin(),needle.end(),needle.begin(),fold);
    return value.find(needle)!=std::string::npos;
}

void DrawStatus(const char* status,FeedbackKind kind,const Theme* theme) {
    if(status&&*status)StatusBadge(status,ToStatusKind(kind),theme);
}

float FiniteOr(float value,float fallback) {
    return std::isfinite(value)?value:fallback;
}

RightSidePanelOptions Sanitize(RightSidePanelOptions options) {
    options.minimumWidth=std::max(1.0f,FiniteOr(options.minimumWidth,240.0f));
    options.minimumContentWidth=std::max(1.0f,FiniteOr(options.minimumContentWidth,160.0f));
    options.maximumWidthRatio=std::clamp(FiniteOr(options.maximumWidthRatio,0.55f),0.05f,0.95f);
    options.splitterWidth=std::max(1.0f,FiniteOr(options.splitterWidth,6.0f));
    options.railWidth=std::max(ImGui::GetFrameHeight(),FiniteOr(options.railWidth,24.0f));
    options.keyboardStep=std::max(1.0f,FiniteOr(options.keyboardStep,16.0f));
    return options;
}

void ClampPanelWidth(RightSidePanelState& state,float availableWidth,
                     const RightSidePanelOptions& options) {
    availableWidth=std::max(1.0f,FiniteOr(availableWidth,1.0f));
    const float fixed=options.railWidth+(state.open?options.splitterWidth:0.0f);
    const float room=std::max(1.0f,availableWidth-fixed-options.minimumContentWidth);
    const float maximum=std::max(1.0f,std::min(availableWidth*options.maximumWidthRatio,room));
    const float minimum=std::min(options.minimumWidth,maximum);
    state.width=std::clamp(FiniteOr(state.width,minimum),minimum,maximum);
}
}

StableId AppBar(const char* id,const AppBarView& view,ToolbarState& state,ComponentOptions options) {
    ImGui::PushID(id);
    const auto& style=ImGui::GetStyle();
    const float height=ImGui::GetFrameHeight()+style.WindowPadding.y*2.f;
    StableId action=0;
    if(ImGui::BeginChild("app-bar",{0,height},ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar)) {
        if(view.product&&*view.product)ImGui::TextUnformatted(view.product);
        if(view.document&&*view.document){ImGui::SameLine();ImGui::TextDisabled("/");ImGui::SameLine();ImGui::TextUnformatted(view.document);}
        if(view.dirty){ImGui::SameLine();StatusBadge("Modified",StatusKind::Warning,options.theme);}
        if(view.context&&*view.context){ImGui::SameLine();ImGui::TextDisabled("%s",view.context);}
        if(view.status&&*view.status){ImGui::SameLine();DrawStatus(view.status,view.statusKind,options.theme);}
        if(!view.actions.empty()){
            ImGui::SameLine();
            action=ResponsiveToolbar("actions",state,view.actions,ToolbarOptions{},options);
        }
    }
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::PopID();
    return action;
}

StableId WorkspaceHeader(const char* id,const WorkspaceHeaderView& view,ToolbarState& state,ComponentOptions options) {
    ImGui::PushID(id);
    StableId action=0;
    if(view.eyebrow&&*view.eyebrow)ImGui::TextDisabled("%s",view.eyebrow);
    ImGui::TextUnformatted(view.title&&*view.title?view.title:"Workspace");
    if(view.status&&*view.status){ImGui::SameLine();DrawStatus(view.status,view.statusKind,options.theme);}
    if(!view.actions.empty()){
        ImGui::SameLine();
        action=ResponsiveToolbar("actions",state,view.actions,ToolbarOptions{},options);
    }
    if(view.description&&*view.description)ImGui::TextDisabled("%s",view.description);
    ImGui::Separator();
    ImGui::PopID();
    return action;
}

bool InspectorSection(const char* id,const char* label,const char* description,bool& open,ComponentOptions options) {
    const bool visible=SectionHeader(id,label,open,options);
    if(visible&&description&&*description)ImGui::TextDisabled("%s",description);
    return visible;
}

bool AdvancedSection(const char* id,const char* label,bool& open,ComponentOptions options) {
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    const bool visible=SectionHeader(id,label,open,options);
    ImGui::PopStyleColor();
    return visible;
}

StableId BottomActionBar(const char* id,const BottomActionBarView& view,ToolbarState& state,ComponentOptions options) {
    ImGui::PushID(id);
    ImGui::Separator();
    DrawStatus(view.status,view.statusKind,options.theme);
    StableId action=0;
    if(!view.actions.empty()){
        if(view.status&&*view.status)ImGui::SameLine();
        action=ResponsiveToolbar("actions",state,view.actions,ToolbarOptions{},options);
    }
    ImGui::PopID();
    return action;
}

RightSidePanelLayout ResolveRightSidePanelLayout(
    RightSidePanelState& state,float availableWidth,bool toggleRequested,
    RightSidePanelOptions options) {
    options=Sanitize(options);
    availableWidth=std::max(1.0f,FiniteOr(availableWidth,1.0f));
    if(toggleRequested)state.open=!state.open;
    ClampPanelWidth(state,availableWidth,options);
    RightSidePanelLayout layout;
    layout.panelVisible=state.open;
    layout.handleWidth=options.railWidth+(state.open?options.splitterWidth:0.0f);
    layout.panelWidth=state.open?state.width:0.0f;
    layout.contentWidth=std::max(1.0f,availableWidth-layout.handleWidth-layout.panelWidth);
    return layout;
}

bool RightSidePanelHandle(const char* id,RightSidePanelState& state,
                          ImVec2 available,RightSidePanelOptions options,
                          ComponentOptions components) {
    options=Sanitize(options);
    available.x=std::max(1.0f,FiniteOr(available.x,1.0f));
    available.y=std::max(1.0f,FiniteOr(available.y,1.0f));
    ClampPanelWidth(state,available.x,options);
    bool changed=false;
    ImGui::PushID(id);
    if(state.open) {
        ImGui::InvisibleButton("splitter",{options.splitterWidth,available.y});
        const bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
        if(ImGui::IsItemHovered()||ImGui::IsItemActive())ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if(!disabled&&ImGui::IsItemActive()&&ImGui::GetIO().MouseDelta.x!=0.0f) {
            state.width-=ImGui::GetIO().MouseDelta.x;changed=true;
        }
        if(!disabled&&ImGui::IsItemFocused()) {
            if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow)){state.width+=options.keyboardStep;changed=true;}
            if(ImGui::IsKeyPressed(ImGuiKey_RightArrow)){state.width-=options.keyboardStep;changed=true;}
        }
        if(components.accessibility) {
            using namespace accessibility;
            const auto item=ImGui::GetItemID();
            if(!disabled&&components.accessibility->Take(item,SemanticAction::Increment)){state.width+=options.keyboardStep;changed=true;}
            if(!disabled&&components.accessibility->Take(item,SemanticAction::Decrement)){state.width-=options.keyboardStep;changed=true;}
            if(!disabled&&components.accessibility->Take(item,SemanticAction::Focus)){ImGui::SetKeyboardFocusHere(-1);ImGui::SetNavCursorVisible(true);}
            SemanticNode node;node.id=item;node.parent=components.parent;node.role=SemanticRole::Button;
            node.name=options.resizeLabel?options.resizeLabel:"Resize inspector";node.state.disabled=disabled;
            node.actions=SemanticAction::Increment|SemanticAction::Decrement|SemanticAction::Focus;
            AnnotateLastItem(*components.accessibility,node);
        }
        ClampPanelWidth(state,available.x,options);
        ImGui::SameLine(0.0f,0.0f);
    }
    if(ImGui::BeginChild("rail",{options.railWidth,available.y},ImGuiChildFlags_Borders,
                         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)) {
        const char* label=state.open?options.closeLabel:options.openLabel;
        if(IconButton("toggle",state.open?ImGuiDir_Right:ImGuiDir_Left,
                      label?label:"Inspector",components)) {
            state.open=!state.open;changed=true;
        }
    }
    ImGui::EndChild();
    ImGui::PopID();
    return changed;
}

bool BeginDiagnosticsDrawer(const char* id,const char* title,DiagnosticsDrawerState& state,ImVec2 size,ComponentOptions options) {
    if(!state.open)return false;
    ImGui::PushID(id);
    if(state.focusPending){ImGui::SetNextWindowFocus();state.focusPending=false;}
    if(!ImGui::BeginChild("diagnostics",size,ImGuiChildFlags_Borders)){
        ImGui::EndChild();ImGui::PopID();return false;
    }
    ImGui::TextUnformatted(title&&*title?title:"Diagnostics");
    ImGui::SameLine();
    if(ActionButton("Close",ActionVariant::Ghost,{},options))state.open=false;
    ImGui::Separator();
    return true;
}

void EndDiagnosticsDrawer(){ImGui::EndChild();ImGui::PopID();}

bool ThemePicker(const char* id,ThemePreset* selected,ThemePickerState& state,ComponentOptions options) {
    if(!selected)return false;
    const auto presets=ThemePresets();
    const auto current=static_cast<std::size_t>(*selected);
    const char* preview=current<presets.size()?presets[current].displayName.data():"";
    bool changed=false;
    ImGui::PushID(id);
    if(ImGui::BeginCombo("Theme",preview)){
        ImGui::InputTextWithHint("##search",options.locale?options.locale->Text("search","Search..."):"Search...",state.search,sizeof(state.search));
        for(std::size_t i=0;i<presets.size();++i){
            if(!Contains(presets[i].displayName.data(),state.search)&&!Contains(presets[i].id.data(),state.search))continue;
            const bool active=i==current;
            ImGui::PushID(static_cast<int>(i));
            if(ImGui::Selectable(presets[i].displayName.data(),active)){
                *selected=presets[i].preset;state.focused=static_cast<int>(i);changed=!active;
            }
            if(active)ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return changed;
}
} // namespace imkit
