#include <imkit/shell.h>

#include <algorithm>
#include <cctype>
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
