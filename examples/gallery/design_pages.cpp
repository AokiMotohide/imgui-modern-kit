// Shows the legacy ToastRegion next to its replacement on purpose.
#define IMKIT_DEPRECATED(message)
#include "design_pages.h"
#include "gallery.h"
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
void DesignPages::Show(int section,GalleryState& host) {
    auto& theme=host.theme;
    Prepare(); queried=0; semantics.Begin(ImGui::GetFrameCount());
    locale.languageTag=language==1?"ja":language==2?"en-XA":"en";
    locale.direction=language==2?TextDirection::RTL:TextDirection::LTR;
    ComponentOptions options{&theme,&host.animation,&semantics,0,&locale};
    const bool ja=language==1;
    if(section==10) {
        if(BeginGrid(host,"foundations-grid")) {
            const bool tokens=BeginSection(host,"Tokens","Density, contrast and motion");
            if(tokens) {
                const char* densities[]={"Compact 24px","Comfortable 30px","Touch 44px"};
                if(imkit::Combo("Density",&density,densities,3)) SetDensity(theme,static_cast<Density>(density));
                const char* contrasts[]={"Standard","High contrast"};
                if(imkit::Combo("Contrast",&contrast,contrasts,2)) { auto fonts=theme.fonts; theme=MakeTheme(theme.scheme,static_cast<ContrastMode>(contrast),static_cast<Density>(density)); theme.fonts=fonts; }
                imkit::Checkbox("Reduced motion",&theme.motion.reducedMotion);
                ImGui::Text("Contrast: %s",ValidateContrast(theme)?"PASS (text 4.5:1 / controls 3:1)":"FAIL");
            }
            EndSection(tokens);
            const bool type=BeginSection(host,"Typography","Four steps for a clear hierarchy");
            if(type) for(auto size:{theme.typography.caption,theme.typography.body,theme.typography.heading,theme.typography.title}) {
                ImGui::PushFont(nullptr,size); ImGui::TextUnformatted("Typography / 読みやすい文字階層"); ImGui::PopFont();
            }
            EndSection(type);
            const bool states=BeginSection(host,"Buttons and states","Primary, secondary, disabled and loading");
            if(states) {
                ActionButton("Primary",ActionVariant::Primary,{},options); ImGui::SameLine(); ActionButton("Secondary",ActionVariant::Secondary,{},options);
                ImGui::BeginDisabled(); ActionButton("Disabled",ActionVariant::Secondary,{},options); ImGui::EndDisabled();
                LoadingState("Loading preview",options);
            }
            EndSection(states);
            const bool radii=BeginSection(host,"Shape and depth","Radius tiers and the overlay shadow");
            if(radii) {
                const ImVec2 origin=ImGui::GetCursorScreenPos();
                const float unit=ImGui::GetFontSize()*2.6f;
                ImGui::Dummy({unit*5.4f,unit*1.2f});
                auto* draw=ImGui::GetWindowDrawList();
                const float tiers[]={theme.radius.subtle,theme.radius.control,theme.radius.overlay};
                const char* names[]={"subtle","control","overlay"};
                for(int i=0;i<3;++i) {
                    const ImVec2 a{origin.x+i*unit*1.8f,origin.y},b{a.x+unit*1.5f,a.y+unit*1.2f};
                    draw->AddRectFilled(a,b,ImGui::GetColorU32(theme.semantic.surfaceRaised),tiers[i]);
                    draw->AddRect(a,b,ImGui::GetColorU32(ImGuiCol_Border),tiers[i]);
                    const ImVec2 labelSize=ImGui::CalcTextSize(names[i]);
                    draw->AddText({(a.x+b.x-labelSize.x)*.5f,(a.y+b.y-labelSize.y)*.5f},ImGui::GetColorU32(ImGuiCol_Text),names[i]);
                }
                ImGui::TextDisabled("Radius %.0f / %.0f / %.0f px",tiers[0],tiers[1],tiers[2]);
                ImGui::Spacing();
                // An overlay surface next to a flat card: the shadow is what separates them.
                ImGui::Dummy({0,theme.elevation.overlay*.5f});
                ImGui::SetCursorPosX(ImGui::GetCursorPosX()+theme.elevation.overlay*.5f);
                ImGui::PushStyleColor(ImGuiCol_ChildBg,theme.semantic.surfaceRaised);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,theme.radius.overlay);
                if(ImGui::BeginChild("overlay-sample",{unit*4.f,unit*1.1f},ImGuiChildFlags_Borders)) {
                    OverlayDecoration(theme,&host.animation);
                    ImGui::TextUnformatted("Overlay surface");
                    ImGui::TextDisabled("Shadow %.0f px",theme.elevation.overlay);
                }
                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                ImGui::Dummy({0,theme.elevation.overlay*.5f});
            }
            EndSection(radii);
            EndGrid();
        }
        return;
    }
    if(section==13) {
        if(BeginGrid(host,"a11y-grid")) {
            const bool controls=BeginSection(host,"Controls","Host-owned state exposed to assistive technology");
            if(controls) {
                if(ActionButton("Count action",ActionVariant::Primary,{},options)) ++clicks;
                Toggle("Host-owned switch",&toast,options);
                ImGui::Text("Actions applied: %d",clicks);
                ImGui::TextWrapped("Tab / Shift+Tab: focus. Space: activate. The UI Automation adapter is an optional host integration; this page shows the frame tree.");
            }
            EndSection(controls);
            const bool tree=BeginSection(host,"Semantic tree","One row per published node");
            if(tree) {
                ImGui::BeginChild("a11y-tree",{0,260*host.scale},ImGuiChildFlags_Borders);
                for(auto& n:semantics.Tree().nodes) ImGui::Text("%llu | role %d | %.*s | focus=%d disabled=%d actions=%u",n.id,int(n.role),int(n.name.size()),n.name.data(),n.state.focused,n.state.disabled,unsigned(n.actions));
                ImGui::EndChild();
            }
            EndSection(tree);
            EndGrid();
        }
        return;
    }
    if(BeginGrid(host,"design-grid")) {
        if(section==11) {
            const bool status=BeginSection(host,ja?"機器の状態":"Device status","Status and the actions that belong to it");
            if(status) {
                CompactActionRowOptions row;
                row.statusText=ja?(sampleStreaming?"受信中":"停止"):(sampleStreaming?"Streaming":"Stopped");
                row.statusKind=sampleStreaming?StatusKind::Success:StatusKind::Neutral;
                row.primaryLabel=ja?(sampleStreaming?"停止":"接続"):(sampleStreaming?"Stop":"Connect");
                row.primaryVariant=sampleStreaming?ActionVariant::SubtleDestructive:ActionVariant::Primary;
                row.settingsLabel=ja?"設定":"Settings";
                row.components=options;
                switch(CompactActionRow("sample-camera",row)) {
                case CompactActionRowRequest::Primary: sampleStreaming=!sampleStreaming; break;
                case CompactActionRowRequest::Settings: ++settingsRequests; break;
                case CompactActionRowRequest::None: break;
                }
                if(settingsRequests) ImGui::TextDisabled(ja?"設定要求: %d":"Settings requests: %d",settingsRequests);
                ImGui::Spacing();
                const char* languages[]={"English","日本語","Pseudo RTL"}; imkit::Combo("Language",&language,languages,3);
            }
            EndSection(status);
            const bool inputs=BeginSection(host,"Inputs","Search, form field, split button and pagination");
            if(inputs) {
                SearchField("search",searchText.data(),searchText.size(),options);
                FormFieldInfo info{"Workspace name","Shown in the title bar",formValue[0]?"":"A name is required",true};
                if(BeginFormField("workspace-name",info,options)) ImGui::InputText("##value",formValue.data(),formValue.size());
                EndFormField();
                ImGui::Spacing();
                if(SplitButton("export","Export",options)==SplitButtonResult::Primary) ++clicks;
                ImGui::SameLine();
                Pagination("pages",pageNumber,12,options);
            }
            EndSection(inputs);
        } else {
            const bool navigation=BeginSection(host,"Navigation and commands","Breadcrumbs and toolbars");
            if(navigation) {
                const char* languages[]={"English","日本語","Pseudo RTL"}; imkit::Combo("Language",&language,languages,3);
                const char* crumbs[]={"Workspace","Documents","Current record"}; Breadcrumbs("path",crumbs,options);
                const Command commands[]={{1,"New record","Ctrl+N"},{2,"Command palette","Ctrl+K"},{3,"Show notification"},{4,"Unavailable","",true,"Waiting for host data"}};
                auto action=section==14?ResponsiveToolbar("commands",toolbar,commands,options):Toolbar("commands",toolbar,commands,options);
                if(action==1) { ++clicks; toast=true; }
                if(action==2) palette.open=true;
                if(action==3) toast=true;
                if(auto selected=CommandPalette("Commands",palette,commands,options)) { ++clicks; toast=true; (void)selected; }
            }
            EndSection(navigation);
            const bool dialogs=BeginSection(host,"Dialogs","Focus is restored when a dialog closes");
            if(dialogs) {
                DialogLauncher("Open dialog",dialog,options);
                if(AlertDialog("Confirm operation","Apply this operation to the current host data?",dialog,options)==DialogResult::Accept) ++clicks;
                ImGui::TextDisabled("Applied: %d",clicks);
            }
            EndSection(dialogs);
        }
        const bool data=BeginSection(host,"Data table","100,000 host rows; only the visible range is queried");
        if(data) {
            imkit::Checkbox("Loading",&loading); ImGui::SameLine(); imkit::Checkbox("Error",&error); ImGui::SameLine(); if(imkit::Checkbox("Tree",&tree)) Reindex();
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
                const DataColumn columns[]={{1,ja?"名前":"Name",260,true},{2,ja?"値":"Value",160,true}};
                if(tree) TreeDataGrid("records",table,provider,columns,320,options); else DataTable("records",table,provider,columns,320,options);
                ImGui::TextDisabled("100,000 host records | visible queried: %d | actions: %d",queried,clicks);
            }
        }
        EndSection(data);
        EndGrid();
    }
    if(toast) { const Notification n{"updated","Host operation applied",StatusKind::Success,0}; if(ToastRegion({&n,1},ImGui::GetTime(),options)) toast=false; }
}
}
