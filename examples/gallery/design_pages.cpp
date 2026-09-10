#include "design_pages.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <numeric>
namespace imkit::gallery {
void DesignPages::Prepare() {
    if(!rows.empty()) return;
    rows.resize(100000); order.resize(rows.size());
    for(int i=0;i<100000;++i) { std::snprintf(rows[i].name.data(),64,"Record %06d",i+1); std::snprintf(rows[i].value.data(),64,"%d",i); }
    std::iota(order.begin(),order.end(),0);
}
void DesignPages::Reindex() {
    order.clear();
    for(int i=0;i<static_cast<int>(rows.size());++i) if((!tree || i%10==0 || !collapsed[i/10]) && (filter.empty() || std::strstr(rows[i].name.data(),filter.c_str()))) order.push_back(i);
    std::sort(order.begin(),order.end(),[this](int a,int b){int compare=std::strcmp(rows[a].name.data(),rows[b].name.data()); return descending?compare>0:compare<0;});
}
void DesignPages::Show(int section,Theme& theme) {
    Prepare(); queried=0; semantics.Begin(ImGui::GetFrameCount());
    locale.languageTag=language==1?"ja":language==2?"en-XA":"en";
    locale.direction=language==2?TextDirection::RTL:TextDirection::LTR;
    ComponentOptions options{&theme,nullptr,&semantics,0,&locale};
    if(section==10) {
        ImGui::PushFont(nullptr,theme.typography.title); ImGui::TextUnformatted("Foundations"); ImGui::PopFont();
        ImGui::TextWrapped("Semantic colors, independent input density, typography and reduced motion.");
        const char* densities[]={"Compact 24px","Comfortable 28px","Touch 44px"};
        if(ImGui::Combo("Density",&density,densities,3)) SetDensity(theme,static_cast<Density>(density));
        const char* contrasts[]={"Standard","High contrast"};
        if(ImGui::Combo("Contrast",&contrast,contrasts,2)) { auto fonts=theme.fonts; theme=MakeTheme(theme.scheme,static_cast<ContrastMode>(contrast),static_cast<Density>(density)); theme.fonts=fonts; }
        ImGui::Checkbox("Reduced motion",&theme.motion.reducedMotion);
        ImGui::Text("Contrast: %s",ValidateContrast(theme)?"PASS (text 4.5:1 / controls 3:1)":"FAIL");
        for(auto size:{theme.typography.caption,theme.typography.body,theme.typography.heading,theme.typography.title}) {
            ImGui::PushFont(nullptr,size); ImGui::TextUnformatted("Typography / 読みやすい文字階層"); ImGui::PopFont();
        }
        ActionButton("Primary",ActionVariant::Primary,{},options); ImGui::SameLine(); ActionButton("Secondary",ActionVariant::Secondary,{},options);
        ImGui::BeginDisabled(); ActionButton("Disabled",ActionVariant::Secondary,{},options); ImGui::EndDisabled();
        LoadingState("Loading preview",options);
    } else if(section==13) {
        ImGui::TextUnformatted("Accessibility");
        if(ActionButton("Count action",ActionVariant::Primary,{},options)) ++clicks;
        Toggle("Host-owned switch",&toast,options);
        ImGui::Text("Actions applied: %d",clicks);
        for(auto& n:semantics.Tree().nodes) ImGui::Text("%llu | role %d | %.*s | focus=%d disabled=%d actions=%u",n.id,int(n.role),int(n.name.size()),n.name.data(),n.state.focused,n.state.disabled,unsigned(n.actions));
        ImGui::TextWrapped("Tab / Shift+Tab: focus. Space: activate. UI Automation adapter is an optional host integration; this page shows the frame tree.");
    } else {
        ImGui::TextUnformatted(section==11?"Components":section==12?"Patterns":"Responsive");
        const char* languages[]={"English","日本語","Pseudo RTL"}; ImGui::Combo("Language",&language,languages,3);
        const char* crumbs[]={"Workspace","Documents","Current record"}; Breadcrumbs("path",crumbs,options);
        const Command commands[]={{1,"New record","Ctrl+N"},{2,"Command palette","Ctrl+K"},{3,"Show notification"},{4,"Unavailable","",true,"Waiting for host data"}};
        auto action=ResponsiveToolbar("commands",toolbar,commands,options);
        if(action==1) { ++clicks; toast=true; }
        if(action==2) palette.open=true;
        if(action==3) toast=true;
        if(auto selected=CommandPalette("Commands",palette,commands,options)) { ++clicks; toast=true; (void)selected; }
        DialogLauncher("Open dialog",dialog,options);
        if(AlertDialog("Confirm operation","Apply this operation to the current host data?",dialog,options)==DialogResult::Accept) ++clicks;
        ImGui::Checkbox("Loading",&loading); ImGui::SameLine(); ImGui::Checkbox("Error",&error); ImGui::SameLine(); if(ImGui::Checkbox("Tree",&tree)) Reindex();
        if(loading) LoadingState("Preparing records",options);
        else if(error) { if(ErrorState("The host could not load the records.",options)) error=false; }
        else {
            DataProvider provider; provider.user=this; provider.count=static_cast<int>(order.size());
            provider.query=[](void* ptr,VisibleRange range){ static_cast<DesignPages*>(ptr)->queried+=range.count; };
            provider.id=[](void* ptr,int row)->StableId { auto& s=*static_cast<DesignPages*>(ptr); return row<static_cast<int>(s.order.size())?s.order[row]+1:0; };
            provider.cell=[](void* ptr,int row,int column)->const char* { auto& s=*static_cast<DesignPages*>(ptr); if(row>=static_cast<int>(s.order.size()))return ""; auto& r=s.rows[s.order[row]]; return column==0?r.name.data():r.value.data(); };
            provider.selected=[](void* ptr,StableId id){auto& s=*static_cast<DesignPages*>(ptr); return id && id<=s.rows.size() && s.rows[id-1].selected;};
            if(tree) {
                provider.depth=[](void* ptr,int row){auto& s=*static_cast<DesignPages*>(ptr); return row<static_cast<int>(s.order.size()) && s.order[row]%10?1:0;};
                provider.expandable=[](void* ptr,int row){auto& s=*static_cast<DesignPages*>(ptr); return row<static_cast<int>(s.order.size()) && s.order[row]%10==0;};
                provider.expanded=[](void* ptr,int row){auto& s=*static_cast<DesignPages*>(ptr); return row<static_cast<int>(s.order.size()) && !s.collapsed[s.order[row]/10];};
            }
            provider.apply=[](void* ptr,const DataEvent& e){auto& s=*static_cast<DesignPages*>(ptr);
                if(e.action==DataAction::Expand && e.row && e.row<=s.rows.size()) {s.collapsed[(e.row-1)/10]=!s.collapsed[(e.row-1)/10]; s.Reindex();}
                else if(e.action==DataAction::Filter) {s.filter=e.value;s.Reindex();}
                else if(e.action==DataAction::Sort) {s.descending=e.descending;s.Reindex();}
                else if(e.action==DataAction::Edit && e.row && e.row<=s.rows.size()) {auto& r=s.rows[e.row-1]; auto& text=e.column==1?r.name:r.value; auto n=std::min(e.value.size(),text.size()-1);std::memcpy(text.data(),e.value.data(),n);text[n]=0;}
                else if(e.action==DataAction::Select) {if(!e.control)for(auto& r:s.rows)r.selected=false; for(int i=e.first;i<=e.last && i<static_cast<int>(s.order.size());++i) s.rows[s.order[i]].selected=true;}
                else if(e.action==DataAction::Context) s.toast=true;
            };
            const DataColumn columns[]={{1,language==1?"名前":"Name",260,true},{2,language==1?"値":"Value",160,true}};
            if(tree) TreeDataGrid("records",table,provider,columns,320,options); else DataTable("records",table,provider,columns,320,options);
            ImGui::Text("100,000 host records | visible queried: %d | actions: %d",queried,clicks);
        }
        if(toast) { const Notification n{"updated","Host operation applied",StatusKind::Success,0}; if(ToastRegion({&n,1},ImGui::GetTime(),options)) toast=false; }
    }
}
}
