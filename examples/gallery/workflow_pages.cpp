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
        StepNavigatorOptions layout;layout.size={0,ImGui::GetFrameHeight()*2.6f};
        if(auto id=StepNavigator("workflow",items,selected,steps,layout,o))selected=id;
        Record(host,"workflow-steps");
        apply(ResponsiveToolbar("commands",toolbar,commands,ToolbarOptions{},o));
        if(FilterChip("filter",japanese?"選択のみ":"Selected only",chip,o))chip=!chip;Record(host,"workflow-chip");
        if(ImGui::BeginTable("workspace",2,ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Navigation",ImGuiTableColumnFlags_WidthFixed,180);ImGui::TableSetupColumn("Canvas",ImGuiTableColumnFlags_WidthStretch);
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
}
