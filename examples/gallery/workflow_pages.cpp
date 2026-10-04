#include "workflow_pages.h"
#include "gallery.h"
#include <algorithm>
namespace imkit::gallery {
void WorkflowPages::Show(int page,GalleryState& host) {
    auto& theme=host.theme;semantics.Begin(ImGui::GetFrameCount());
    ComponentOptions o{&theme,nullptr,&semantics};
    ImGui::Checkbox("日本語",&japanese);ImGui::SameLine();ImGui::Checkbox("Disabled",&disabled);
    ImGui::SameLine();ImGui::Checkbox("Vertical",&vertical);
    ImGui::TextWrapped("Host-owned state and request events. Tab / arrows / Space; middle-drag and wheel in the image.");
    StepItem items[]={{1,japanese?"準備":"Prepare","Available",true},{2,japanese?"編集":"Edit","Warning is informational",false,true,false,FeedbackKind::Warning},{3,japanese?"確認":"Review","Inspect the result",false,true,false,FeedbackKind::Error},{4,japanese?"実行不可":"Unavailable","The host disabled this action",false,true,true}};
    imkit::Command commands[]={{1,japanese?"通知を表示":"Notify"},{2,japanese?"コマンド検索":"Search commands"},{3,japanese?"実行不可":"Unavailable","",true,"The host disabled this action"}};
    auto apply=[&](StableId action){if(!action)return;++actions;if(action==1){notice=true;expiresAt=ImGui::GetTime()+15;}if(action==2)palette.open=true;};
    ImGui::BeginDisabled(disabled);
    if(page==15) {
        AppBarView app{"ImKit",japanese?"サンプルプロジェクト":"Sample project",japanese?"編集ワークフロー":"Edit workflow",japanese?"準備完了":"Ready",true,FeedbackKind::Success,commands};
        apply(AppBar("app",app,toolbar,o));
        WorkspaceHeaderView header{"WORKFLOW",japanese?"作業スペース":"Workspace",japanese?"状態と操作はホストが所有します":"The host owns state and actions",japanese?"接続済み":"Connected",FeedbackKind::Success,commands};
        apply(WorkspaceHeader("header",header,toolbar,o));
        const std::array<StepGroup,2> groups{{{101,japanese?"準備":"Prepare",0,2,{.25f,.60f,.95f,1.f}},{102,japanese?"仕上げ":"Deliver",2,2,{.85f,.45f,.70f,1.f}}}};
        if(auto request=GroupedStepNavigator("grouped",groups,items,selected,o))selected=request;
        const std::array<IconToolbarItem,3> iconActions{{{1,IconId::Play,japanese?"再生":"Play","Start playback",true},{2,IconId::Pause,japanese?"一時停止":"Pause","Mixed state",false,true},{3,IconId::Stop,japanese?"停止":"Stop","Unavailable",false,false,true}}};
        IconToolbar("wrapping-icons",host.icons,iconActions,{true,true},o);
        IconActionButton("labeled-action",host.icons,IconId::Reset,japanese?"表示リセット":"Reset view","Restore the default view",ActionVariant::Secondary,o);
        const std::array<ChoiceItem,7> sourceChoices{{
            {701,"USB / HDMI##choice-usb",japanese?"ローカル接続":"Direct capture input",IconId::Usb,{.22f,.53f,.91f,1.f}},
            {702,japanese?"ネットワーク##choice-network":"Network##choice-network",japanese?"LAN経由":"Camera over a network",IconId::Network,{.10f,.70f,.82f,1.f}},
            {703,japanese?"仮想入力##choice-virtual":"Virtual##choice-virtual",japanese?"仮想デバイス":"Virtual device",IconId::WindowMaximize,{.61f,.43f,.82f,1.f}},
            {704,japanese?"産業カメラ##choice-industrial":"Industrial##choice-industrial",japanese?"産業用デバイス":"Industrial camera",IconId::IndustrialCamera,{.86f,.57f,.17f,1.f}},
            {705,japanese?"スマートフォン##choice-mobile":"Mobile##choice-mobile",japanese?"モバイル入力":"Mobile input",IconId::Smartphone,{.18f,.67f,.52f,1.f}},
            {706,japanese?"テスト映像##choice-test":"Test pattern##choice-test",japanese?"テスト信号":"Test signal",IconId::TestPattern,{.52f,.57f,.64f,1.f}},
            {707,japanese?"利用不可##choice-disabled":"Unavailable##choice-disabled",japanese?"この例では選択できません":"Unavailable in this example",IconId::Close,{.62f,.62f,.62f,1.f},true,japanese?"この入力経路は無効です":"This input path is disabled"}}};
        ChoiceGroupOptions choiceLayout;choiceLayout.icons=&host.icons;
        if(auto request=ChoiceGroup("connection-method",japanese?"接続方式を選択":"Choose a connection method",
                                    sourceChoices,choiceSource,choiceLayout,o)) choiceSource=request;
        Record(host,"workflow-choice-group");
        StepNavigatorOptions layout;layout.size={0,ImGui::GetFrameHeight()*2.6f};
        if(auto id=StepNavigator("workflow",items,selected,steps,layout,o))selected=id;
        Record(host,"workflow-steps");
        for(const auto& node:semantics.Tree().nodes) if(node.name==items[2].label)
            host.probes["workflow-review"]={node.minimum,node.maximum};
        apply(ResponsiveToolbar("commands",toolbar,commands,ToolbarOptions{},o));
        if(FilterChip("filter",japanese?"選択のみ":"Selected only",chip,o))chip=!chip;Record(host,"workflow-chip");
        const bool workspaceWide=ImGui::GetContentRegionAvail().x>=650.f;
        if(ImGui::BeginTable("workspace",workspaceWide?2:1,ImGuiTableFlags_SizingStretchProp)) {
            if(workspaceWide) {ImGui::TableSetupColumn("Navigation",ImGuiTableColumnFlags_WidthFixed,180);ImGui::TableSetupColumn("Canvas",ImGuiTableColumnFlags_WidthStretch);}
            ImGui::TableNextRow();ImGui::TableNextColumn();
            if(auto id=NavigationRail("rail",items,selected,rail,{0,360},o))selected=id;
            ImGui::TableNextColumn();
            editor::ZoomToolbar("zoom",image,o);Record(host,"workflow-zoom");
            image.canvas.selectionPath=lassoScratch;
            ImGui::Checkbox("Lasso",&lasso);
            auto view=editor::BeginImageViewport("image",{host.texture,{512,288}},image,{0,360},theme,{true,true,disabled},o);
            host.probes["workflow-canvas"]={view.min,view.max};
            editor::Point box[]={{50,50},{350,220}},dot[]={{180,120}},line[]={{50,240},{230,180},{420,240}};
            editor::DrawOverlay(view,image.canvas,{editor::OverlayShape::Rectangle,box,"",1,true},theme);
            editor::DrawOverlay(view,image.canvas,{editor::OverlayShape::Point,dot,"",1,true,true},theme);
            editor::DrawOverlay(view,image.canvas,{editor::OverlayShape::Circle,dot,"",35},theme);
            editor::DrawOverlay(view,image.canvas,{editor::OverlayShape::Polyline,line},theme);
            editor::DrawOverlay(view,image.canvas,{editor::OverlayShape::Label,dot,japanese?"選択ポイント":"Selected point"},theme);
            editor::SelectablePoint point{10,{180,120}};
            editor::SelectionProvider provider;provider.user=&point;provider.query=[](void* user,editor::Rect){return std::span<const editor::SelectablePoint>(static_cast<editor::SelectablePoint*>(user),1);};
            std::array<editor::Event,16> storage{};editor::EventBuffer events{storage};
            if(!disabled)editor::CanvasSelection(view,image.canvas,provider,selection,events,theme,lasso);
            editor::EndImageViewport();
            editor::StatusBar(japanese?"準備完了":"Ready",selection);
            if(InspectorSection("inspector",japanese?"インスペクター":"Inspector",japanese?"選択中の項目":"Selected item",open,o))
                ImGui::TextUnformatted(japanese?"ホスト所有の設定":"Host-owned properties");
            if(AdvancedSection("advanced",japanese?"詳細設定":"Advanced",advanced,o))
                ImGui::TextDisabled("Low-frequency settings");
            ImGui::EndTable();
        }
        ImGui::SeparatorText(japanese?"画像previewのaspect契約":"Image preview aspect contract");
        const editor::ImageScaleMode comparisonModes[]={editor::ImageScaleMode::Fit,editor::ImageScaleMode::Fill,
            editor::ImageScaleMode::ActualSize,editor::ImageScaleMode::Manual};
        const char* comparisonNames[]={"Fit","Fill","1:1","Manual"};
        if(ImGui::BeginTable("image-mode-comparison",4,ImGuiTableFlags_BordersInnerV|ImGuiTableFlags_SizingStretchSame)) {
            for(int i=0;i<4;++i) {
                ImGui::TableNextColumn();ImGui::PushID(i);ImGui::TextUnformatted(comparisonNames[i]);
                auto &state=imageComparisons[i];state.mode=comparisonModes[i];
                if(i==3&&state.reset) {state.canvas.scale={.65,.65};state.canvas.origin={48,32};}
                auto view=editor::BeginImageViewport("comparison",{host.texture,{512,288}},state,{0,120},theme,{true,true,disabled},o);
                const auto center=editor::NormalizedToPixel({.5,.5},{512,288});
                editor::Point circle[]={center};
                editor::DrawOverlay(view,state.canvas,{editor::OverlayShape::Circle,circle,"",48},theme);
                editor::EndImageViewport();
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        if(auto id=MultiSelectionBar("selection",selection.count,commands,toolbar,o))apply(id);
        apply(BottomActionBar("bottom",BottomActionBarView{japanese?"変更なし":"No pending changes",FeedbackKind::Info,commands},toolbar,o));
        auto preset=static_cast<ThemePreset>(host.presetIndex);
        if(ThemePicker("theme",&preset,themePicker,o)){host.presetIndex=static_cast<int>(preset);host.theme=MakeTheme(preset);}
        ImGui::SeparatorText(japanese?"開閉式の右インスペクター":"Collapsible right inspector");
        const ImVec2 panelAvailable{ImGui::GetContentRegionAvail().x,180};
        const bool panelShortcut=!ImGui::GetIO().WantTextInput&&ImGui::IsKeyPressed(ImGuiKey_N);
        RightSidePanelOptions panelOptions;
        panelOptions.openLabel=japanese?"インスペクターを開く (N)":"Open inspector (N)";
        panelOptions.closeLabel=japanese?"インスペクターを閉じる (N)":"Close inspector (N)";
        panelOptions.resizeLabel=japanese?"インスペクターの幅を変更":"Resize inspector";
        const auto panelLayout=ResolveRightSidePanelLayout(rightPanel,panelAvailable.x,panelShortcut,panelOptions);
        ImGui::BeginChild("right-panel-content",{panelLayout.contentWidth,panelAvailable.y},ImGuiChildFlags_Borders);
        ImGui::TextUnformatted(japanese?"制作ビュー":"Authoring view");
        ImGui::TextDisabled("N");
        ImGui::EndChild();ImGui::SameLine(0,0);
        RightSidePanelHandle("right-panel-handle",rightPanel,panelAvailable,panelOptions,o);
        if(panelLayout.panelVisible){ImGui::SameLine(0,0);ImGui::BeginChild("right-panel-body",{panelLayout.panelWidth,panelAvailable.y},ImGuiChildFlags_Borders);ImGui::TextUnformatted(japanese?"インスペクター":"Inspector");ImGui::TextDisabled(japanese?"状態と幅はホストが所有します":"The host owns open state and width");ImGui::EndChild();}
    } else if(page==16) {
        ImGui::SliderFloat("Progress",&fraction,-1,1);ImGui::SameLine();if(ActionButton("Modal progress",ActionVariant::Secondary,{},o))progressDialog.open=true;
        Record(host,"workflow-modal");
        ProgressView progress{fraction,japanese?"処理中":"Processing",japanese?"ホストが進捗を管理します":"The host owns progress and cancellation",true};
        if(imkit::Progress("inline",progress,ProgressPresentation::Inline,progressDialog,{},o))++actions;
        if(imkit::Progress("overlay",progress,ProgressPresentation::Overlay,progressDialog,{0,130},o))++actions;
        if(imkit::Progress("modal",progress,ProgressPresentation::Modal,progressDialog,{},o)){progressDialog.open=false;++actions;}
        ImGui::SeparatorText(japanese?"円形進捗":"Circular progress");
        ImGui::TextDisabled(japanese?"値、完了、未測定を同じコンポーネントで表示します。":"Known, complete, and unavailable states use the same component.");
        CircularProgress("coverage-total",CircularProgressView{.82f,"41/50",japanese?"全体":"Overall"},{96,7},o);ImGui::SameLine();
        CircularProgress("coverage-one",CircularProgressView{1.f,"10/10","PJ1",FeedbackKind::Success},{58,0},o);ImGui::SameLine();
        CircularProgress("coverage-two",CircularProgressView{.7f,"7/10","PJ2"},{58,0},o);ImGui::SameLine();
        CircularProgress("coverage-unknown",CircularProgressView{-1.f,"","PJ3"},{58,0},o);
        FeedbackView info{10,japanese?"設定を更新しました":"Settings updated","Inline feedback",FeedbackKind::Success,0,0,false};
        InlineAlert("inline-alert",info,o);PersistentBanner("banner",{11,"Review required","Persistent warning",FeedbackKind::Warning,0,0,false},o);
        if(BeginCard("states",{0,260},o)) {
            if(SectionHeader("section",japanese?"状態表示":"States",open,o)) {
                if(EmptyState("empty",StateView{"No items","Add the first item","Create",FeedbackKind::Info,IconId::Folder,&host.icons},o))++actions;
                UnavailableState("offline",StateView{"Unavailable","The host is offline"},o);
                if(RetryState("retry",StateView{"Unable to load","Retry when ready","Retry",FeedbackKind::Error},o))++actions;
            }
        }EndCard();
        HelpCallout("help",StateView{"Help","Use keyboard focus to inspect and activate actions"},o);
        if(ValidationSummary("validation",std::span(items).subspan(1,2),o))++actions;
        if(ActionButton("Diagnostics",ActionVariant::Secondary,{},o)){diagnostics.open=true;diagnostics.focusPending=true;}
        if(BeginDiagnosticsDrawer("drawer",japanese?"診断":"Diagnostics",diagnostics,{0,120},o)){
            ImGui::TextUnformatted(japanese?"問題はありません":"No issues");
            EndDiagnosticsDrawer();
        }
    } else {
        const std::array<WorkspaceTab,3> workspaces{{
            {101, japanese?"編集":"Edit", "Edit items", IconId::Cube},
            {102, japanese?"接続":"Connect", "Route sources", IconId::Connected},
            {103, japanese?"仕上げ":"Review", "Review settings", IconId::Settings}}};
        if(auto request=WorkspaceTabs("workspace-tabs",workspaces,workspace,&host.icons,o))workspace=request;
        ImGui::Spacing();
        const bool componentWide=ImGui::GetContentRegionAvail().x>=850.f;
        if(ImGui::BeginTable("component-layout",componentWide?2:1,ImGuiTableFlags_SizingStretchProp)) {
            if(componentWide) {
                ImGui::TableSetupColumn("Items",ImGuiTableColumnFlags_WidthStretch,.8f);
                ImGui::TableSetupColumn("Settings",ImGuiTableColumnFlags_WidthStretch,1.2f);
            }
            ImGui::TableNextRow();ImGui::TableNextColumn();
            if(HierarchyGroupHeader("group",japanese?"項目":"Items",2,&hierarchyOpen,&host.icons,IconId::Layers,o)) {
                HierarchyRow("first",{1,japanese?"項目 A":"Item A","Visible and editable",IconId::Cube,1,true},&host.icons,o);
                HierarchyRow("second",{2,japanese?"項目 B":"Item B","Hidden and locked",IconId::Image,1,false,false,true},&host.icons,o);
            }
            ImGui::TableNextColumn();
            if(BeginInspectorCard("settings-card",japanese?"基本設定":"Basic settings",
                                  japanese?"関連する操作をまとめて表示":"Related controls stay together",
                                  &host.icons,IconId::Settings,o)) {
                if(SettingToggleRow("enabled",japanese?"有効":"Enabled",
                                    japanese?"現在の項目を使用":"Use this item",settingEnabled,false,"",o))
                    settingEnabled=!settingEnabled;
                ImGui::DragFloat(japanese?"強さ":"Strength",&position[0],.01f,0.f,10.f,"%.2f");
            }
            EndInspectorCard();ImGui::EndTable();
        }
        editor::PreviewTileView tiles[5];
        const char* names[]={"Preview A","Loading","Empty","Offline","Error"};
        for(int i=0;i<5;++i){tiles[i].id=i+1;tiles[i].title=names[i];tiles[i].detail="Host-owned texture and state";tiles[i].actions=commands;tiles[i].disabled=disabled;}
        tiles[0].image={host.texture,{512,288}};tiles[1].status=editor::PreviewState::Loading;tiles[2].status=editor::PreviewState::Empty;
        tiles[3].status=editor::PreviewState::Offline;tiles[4].status=editor::PreviewState::Error;
        std::array<editor::TileEvent,32> storage{};editor::TileEventBuffer events{storage};
        editor::ResizableTileStrip("strip",tiles,sizes,tileSelected,strip,events,{vertical?Orientation::Vertical:Orientation::Horizontal,180,{0,480}},o);
        for(std::size_t i=0;i<events.count;++i){auto& e=storage[i];if(e.action==editor::TileAction::Select||e.action==editor::TileAction::Open)tileSelected=e.tile;if(e.action==editor::TileAction::Command)apply(e.command);}
        ImGui::Text("Selected: %llu | events: %zu",static_cast<unsigned long long>(tileSelected),events.count);
    }
    ImGui::EndDisabled();
    if(auto id=CommandPalette("Commands",palette,commands,o))apply(id);
    if(ImGui::Button("Show notification")){notice=true;expiresAt=ImGui::GetTime()+15;}Record(host,"workflow-notify");
    ImGui::SameLine();ImGui::Text("Actions: %d",actions);
    FeedbackView toast{99,japanese?"更新が完了しました":"Update complete",japanese?"通知はホストが管理します":"Dismiss returns a request to the host",FeedbackKind::Success,expiresAt,toastPriority};
    std::array<std::size_t,4> scratch{};std::array<StableId,4> dismissIds{};RequestBuffer dismiss{dismissIds};
    if(notice)ToastRegion("notifications",{&toast,1},ImGui::GetTime(),scratch,dismiss,{},o);
    if(dismiss.count)notice=false;
}
void WorkflowPages::Workbench(GalleryState& host) {
    ComponentOptions o{&host.theme,&host.animation};
    const WorkspaceTab tabs[]={{101,"Compose","Arrange your workspace",IconId::Layers},{102,"Inspect","Edit selected items",IconId::Settings},{103,"Deliver","Prepare output",IconId::Export}};
    std::array<accessibility::SemanticNode,16> tabNodes{};
    accessibility::AccessibilityFrame tabSemantics{tabNodes};tabSemantics.Begin(ImGui::GetFrameCount());
    auto tabOptions=o;tabOptions.accessibility=&tabSemantics;
    if(auto request=WorkspaceTabs("workbench-tabs",tabs,workspace,&host.icons,tabOptions)) workspace=request;
    Record(host,"workbench-tabs");
    for(const auto& node:tabSemantics.Tree().nodes) for(int i=0;i<3;++i)
        if(node.name==tabs[i].label) host.probes["workbench-tab-"+std::to_string(i)]={node.minimum,node.maximum};
    const bool wide=ImGui::GetContentRegionAvail().x>=850.f*std::max(1.f,ImGui::GetStyle().FontScaleDpi);
    if(ImGui::BeginTable("workbench",wide?3:1,ImGuiTableFlags_SizingStretchProp)) {
        if(wide) {
            ImGui::TableSetupColumn("Hierarchy",ImGuiTableColumnFlags_WidthStretch,.8f);
            ImGui::TableSetupColumn("Preview",ImGuiTableColumnFlags_WidthStretch,1.4f);
            ImGui::TableSetupColumn("Inspector",ImGuiTableColumnFlags_WidthStretch,1.f);
        }
        ImGui::TableNextColumn();
        bool add=false;
        const auto hierarchyStart=ImGui::GetCursorScreenPos();
        const bool openHierarchy=HierarchyGroupHeader("scene","Workspace",3,&hierarchyOpen,&host.icons,IconId::Layers,o,&add,"Add item");
        host.probes["workbench-hierarchy"]={hierarchyStart,{hierarchyStart.x+ImGui::GetFrameHeight(),hierarchyStart.y+ImGui::GetFrameHeight()}};
        if(openHierarchy) {
            const char* names[]={"Main image","Overlay","Reference"};
            const IconId icons[]={IconId::Image,IconId::Layers,IconId::Camera};
            for(int i=0;i<3;++i) {
                const auto rowStart=ImGui::GetCursorScreenPos();
                const float rowWidth=ImGui::GetContentRegionAvail().x;
                const auto request=HierarchyRow(names[i],{static_cast<StableId>(i+1),names[i],i==0?"1920 x 1080":"Host-owned item",icons[i],0,selected==static_cast<StableId>(i+1),visible[i],locked[i]},&host.icons,o);
                host.probes[std::string("workbench-row-")+std::to_string(i)]={rowStart,{rowStart.x+std::max(20.f,rowWidth-ImGui::GetFrameHeight()*4),rowStart.y+ImGui::GetFrameHeight()}};
                switch(request) {
                case HierarchyRowAction::Select:selected=i+1;break;
                case HierarchyRowAction::ToggleVisibility:visible[i]=!visible[i];break;
                case HierarchyRowAction::ToggleLock:locked[i]=!locked[i];break;
                case HierarchyRowAction::More:workSettings=true;break;
                default:break;
                }
            }
        }
        if(add) ++actions;
        ImGui::TextDisabled("Requests are applied by this app.");
        ImGui::TableNextColumn();
        if(BeginInspectorCard("preview-card",workspace==103?"Delivery preview":"Live preview","A host texture in the shared workspace",&host.icons,IconId::Image,o)) {
            const float width=ImGui::GetContentRegionAvail().x;
            const auto view=editor::BeginImageViewport("workbench-preview",{host.texture,{512,288}},image,{width,width*9.f/16.f},host.theme,{},o);
            auto* draw=ImGui::GetWindowDrawList();
            draw->AddRectFilled(view.min,view.max,IM_COL32(22,35,41,255));
            editor::DrawGrid(view,image.canvas,{32,32},host.theme);
            const ImVec2 center{(view.min.x+view.max.x)*.5f+position[0]*8.f,(view.min.y+view.max.y)*.5f-position[1]*8.f};
            const float radius=(view.max.y-view.min.y)*.30f;
            const ImVec2 shape[]={{center.x,center.y-radius},{center.x+radius,center.y-radius*.35f},{center.x+radius*.70f,center.y+radius},{center.x-radius*.65f,center.y+radius*.68f},{center.x-radius,center.y-radius*.30f}};
            if(visible[0]) {draw->AddConvexPolyFilled(shape,5,IM_COL32(169,231,203,255));draw->AddPolyline(shape,5,IM_COL32(230,255,244,255),ImDrawFlags_Closed,2.f);}
            if(visible[1]) {draw->AddCircle(center,radius*1.35f,IM_COL32(144,185,255,255),64,2.f);draw->AddLine({center.x-radius*1.5f,center.y},{center.x+radius*1.5f,center.y},IM_COL32(144,185,255,140),1.f);}
            draw->AddText({view.min.x+12,view.max.y-ImGui::GetFontSize()-12},IM_COL32(169,231,203,255),"SAMPLE COMPOSITION / HOST DRAW LIST");
            editor::EndImageViewport();
            Record(host,"workbench-preview");
            ImGui::TextWrapped("Select an item, edit its settings, then apply. The library returns requests; the application owns the result.");
            CompactActionRowOptions row;
            row.statusText=actions?"Changes applied":"Ready to apply";row.statusKind=StatusKind::Success;
            row.primaryLabel="Apply changes";row.components=o;
            const auto actionStart=ImGui::GetCursorScreenPos();
            const float primaryX=actionStart.x+ImGui::CalcTextSize(row.statusText).x+24+ImGui::GetStyle().ItemSpacing.x;
            const auto request=CompactActionRow("workbench-actions",row);
            Record(host,"workbench-actions");
            host.probes["workbench-apply"]={{primaryX,actionStart.y},{primaryX+ImGui::CalcTextSize(row.primaryLabel).x+ImGui::GetStyle().FramePadding.x*2,actionStart.y+ImGui::GetFrameHeight()}};
            if(request==CompactActionRowRequest::Primary) ++actions;
            if(request==CompactActionRowRequest::Settings) workSettings=!workSettings;
            ImGui::Text("Applied: %d",actions);
        }
        EndInspectorCard();
        ImGui::TableNextColumn();
        if(BeginInspectorCard("item-settings","Item settings","Changes update application values",&host.icons,IconId::Settings,o)) {
            const auto toggleStart=ImGui::GetCursorScreenPos();
            const float toggleRight=toggleStart.x+ImGui::GetContentRegionAvail().x;
            if(SettingToggleRow("workbench-enabled","Enable item","Include this item in output",settingEnabled,false,"",o)) settingEnabled=!settingEnabled;
            host.probes["workbench-enabled"]={{toggleRight-ImGui::GetFrameHeight()*2.4f,toggleStart.y},{toggleRight-ImGui::GetFrameHeight()*1.4f,toggleStart.y+ImGui::GetFrameHeight()}};
            ImGui::SetNextItemWidth(-1);
            DragVector3WithUnit("Position",position,"m",.05f,-10.f,10.f,"%.2f");
            Record(host,"workbench-position");
            const ChoiceItem choices[]={{701,"USB","Direct connection",IconId::Usb},{702,"Network","Remote source",IconId::Network},{703,"Mobile","Portable source",IconId::Smartphone},{704,"Camera","Unavailable in this demo",IconId::IndustrialCamera,{},true,"Connect a device in your host app"}};
            std::array<accessibility::SemanticNode,16> choiceNodes{};
            accessibility::AccessibilityFrame choiceSemantics{choiceNodes};choiceSemantics.Begin(ImGui::GetFrameCount());
            auto choiceOptions=o;choiceOptions.accessibility=&choiceSemantics;
            if(auto request=ChoiceGroup("workbench-source","Input source",choices,choiceSource,{2,&host.icons},choiceOptions)) choiceSource=request;
            for(const auto& node:choiceSemantics.Tree().nodes) if(node.name=="Mobile") host.probes["workbench-mobile"]={node.minimum,node.maximum};
            Record(host,"workbench-source");
            if(workSettings) ImGui::TextWrapped("Host settings opened. No global service or persistence is introduced.");
        }
        EndInspectorCard();
        ImGui::EndTable();
    }
    ImGui::SeparatorText("Integration route");
    ImGui::TextWrapped("include/imkit/workflow.h + components.h | Guide: docs/components/workflow-components.md");
}

}
