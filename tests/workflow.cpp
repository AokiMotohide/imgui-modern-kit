#include <imkit/editor_canvas.h>
#include <imkit/shell.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include "../examples/gallery/allocation_probe.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
namespace {
int failures=0,allocations=0;bool measuring=false;
void Check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL %s\n",what);++failures;}}
bool Near(double a,double b){return std::abs(a-b)<1e-8;}
void* Allocate(std::size_t n,void*){if(measuring)++allocations;return std::malloc(n);}
void Free(void* p,void*){std::free(p);}
}
int main(){
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
#endif
    using namespace imkit;using namespace imkit::editor;
    CanvasState canvas;FitImage(canvas,{400,200},{200,200},ImageScaleMode::Fit);
    Check(Near(canvas.scale.x,.5)&&Near(canvas.origin.y,-100),"fit aspect and letterbox");
    auto p=FromScreen({75,80},canvas,{});ZoomAt(canvas,{75,80},{2,2});auto q=FromScreen({75,80},canvas,{});
    Check(Near(p.x,q.x)&&Near(p.y,q.y),"zoom anchor invariant");
    FitImage(canvas,{400,200},{200,200},ImageScaleMode::Fill);Check(Near(canvas.scale.x,1)&&Near(canvas.origin.x,100),"fill crop");
    canvas.origin={999,-999};ClampImage(canvas,{400,200},{200,200});Check(Near(canvas.origin.x,200)&&Near(canvas.origin.y,0),"clamp edges");
    auto n=PixelToNormalized({200,50},{400,200});auto px=NormalizedToPixel(n,{400,200});Check(Near(n.x,.5)&&Near(n.y,.25)&&Near(px.x,200),"normalized roundtrip");
    FitImage(canvas,{0,0},{0,0},ImageScaleMode::Fit);Check(canvas.scale.x==1&&!ImageGeometryValid({1,1},{0,0}),"zero geometry");
    Check(!ImageGeometryValid({std::numeric_limits<double>::quiet_NaN(),2},{2,2}),"nonfinite geometry");
    std::array<FeedbackView,5> notices{{{1,"old","",FeedbackKind::Info,0,0},{2,"expired","",FeedbackKind::Error,4,9},{1,"new","",FeedbackKind::Success,0,1},{3,"priority","",FeedbackKind::Warning,0,3},{4,"equal","",FeedbackKind::Info,0,1}}};
    std::array<std::size_t,4> order{};
    auto count=SelectNotifications(notices,5,order,3);Check(count==3&&order[0]==3&&order[1]==2&&order[2]==4,"update expiry priority stable order");
    Check(SelectNotifications(notices,5,{},4)==0&&SelectNotifications({},5,order,4)==0,"empty notification scratch");
    RequestBuffer noRequests;Check(!noRequests.Push(1)&&noRequests.overflow,"request overflow");
    TileEventBuffer noEvents;Check(!noEvents.Push({})&&noEvents.overflow,"tile overflow");
    ImGui::SetAllocatorFunctions(Allocate,Free);
    for(int generation=0;generation<2;++generation){
        auto* context=ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1000,800};io.DeltaTime=1.f/60;io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
        unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
        auto theme=MakeTheme(ColorScheme::Dark,ContrastMode::HighContrast,Density::Comfortable);ApplyTheme(theme);
        std::array<accessibility::SemanticNode,256> nodes{};std::array<accessibility::ActionRequest,16> pending{};
        accessibility::ActionQueue queue(pending);accessibility::AccessibilityFrame semantics(nodes,&queue);
        ComponentOptions options{&theme,nullptr,&semantics};
        auto frame=[&](auto draw){io.AddFocusEvent(true);ImGui::NewFrame();semantics.Begin(generation);ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({900,740});ImGui::Begin("fixture",nullptr,ImGuiWindowFlags_NoSavedSettings);draw();ImGui::End();ImGui::Render();Check(semantics.Tree().Validate(),"valid semantic tree");};
        std::array<StepItem,4> steps{{{1,"Same","Ready"},{2,"Same","Blocked",false,true,true},{3,"Long label without collisions","Description",true,true,false,FeedbackKind::Warning},{4,"Error","Description",false,true,false,FeedbackKind::Error}}};
        {
            std::array<StepGroup,2> groups{{{10,"First",0,2},{11,"Second",2,2}}};
            StableId request=0;
            auto draw=[&]{request=GroupedStepNavigator("grouped",groups,steps,4,options);};
            frame(draw);frame(draw);
            auto first=semantics.Tree().nodes[0], active=semantics.Tree().nodes[1];
            Check(semantics.Tree().nodes.size()==6 && active.state.selected,"groups and all steps visible");
            queue.Push({active.id,accessibility::SemanticAction::Press});frame(draw);Check(!request,"active group retains current step");
            queue.Push({first.id,accessibility::SemanticAction::Press});frame(draw);Check(request==1,"group selects first step");
            auto blocked=semantics.Tree().nodes[3];
            queue.Push({blocked.id,accessibility::SemanticAction::Press});frame(draw);Check(!request,"grouped disabled step rejected");
            IconAtlas atlas;for(int size:IconPixelSizes)atlas.SetTexture(size,ImTextureRef((ImTextureID)1));
            std::array<IconToolbarItem,3> actions{{{1,IconId::Play,"Play","",true},{2,IconId::Stop,"Stop","",false,true,true},{3,IconId::Reset,"Reset"}}};
            auto toolbar=[&]{ImGui::BeginChild("narrow",{50,200});request=IconToolbar("icons",atlas,actions,options);ImGui::EndChild();};
            frame(toolbar);frame(toolbar);
            Check(semantics.Tree().nodes.size()==3,"icon toolbar semantics");
            if(semantics.Tree().nodes.size()<3) return 1;
            Check(semantics.Tree().nodes[2].minimum.y>semantics.Tree().nodes[0].minimum.y,"narrow toolbar wraps");
            Check(semantics.Tree().nodes[0].state.selected,"icon selection exposed");
            Check(semantics.Tree().nodes[1].state.mixed,"mixed icon state exposed");
            queue.Push({semantics.Tree().nodes[1].id,accessibility::SemanticAction::Press});frame(toolbar);Check(!request,"disabled icon rejected");
        }
        StepNavigatorState nav;StableId selected=0;
        auto drawSteps=[&]{if(auto request=StepNavigator("steps",steps,selected,nav,{},options))selected=request;};
        for(int i=0;i<3;++i)frame(drawSteps);
        auto node=semantics.Tree().nodes.front();auto disabled=semantics.Tree().nodes[1];
        Check(node.id!=disabled.id&&disabled.state.disabled,"same labels distinct IDs and disabled state");
        queue.Push({disabled.id,accessibility::SemanticAction::Press});frame(drawSteps);Check(!selected,"disabled action rejected");
        queue.Push({node.id,accessibility::SemanticAction::Press});frame(drawSteps);Check(selected==1,"selection request");
        nav.focused=1;nav.focusPending=true;frame(drawSteps);frame(drawSteps);
        io.AddKeyEvent(ImGuiKey_RightArrow,true);frame(drawSteps);io.AddKeyEvent(ImGuiKey_RightArrow,false);frame(drawSteps);frame(drawSteps);
        Check(nav.focused==3,"arrow skips disabled");
        io.AddKeyEvent(ImGuiKey_Space,true);frame(drawSteps);io.AddKeyEvent(ImGuiKey_Space,false);frame(drawSteps);frame(drawSteps);frame(drawSteps);
        if(selected!=3){std::fprintf(stderr,"keyboard selected=%llu focused=%llu\n",static_cast<unsigned long long>(selected),static_cast<unsigned long long>(nav.focused));for(auto& item:semantics.Tree().nodes)std::fprintf(stderr,"node=%.*s focused=%d\n",int(item.name.size()),item.name.data(),item.state.focused);}
        Check(selected==3,"keyboard activation");
        auto click=semantics.Tree().nodes.front();
        io.AddMousePosEvent((click.minimum.x+click.maximum.x)*.5f,(click.minimum.y+click.maximum.y)*.5f);frame(drawSteps);
        io.AddMouseButtonEvent(0,true);frame(drawSteps);io.AddMouseButtonEvent(0,false);frame(drawSteps);Check(selected==1,"mouse selection");
        bool dismissed=false;auto feedback=[&]{dismissed=NotificationCard(notices[2],5,options);};
        frame(feedback);auto alert=semantics.Tree().nodes.back();queue.Push({alert.id,accessibility::SemanticAction::Dismiss});frame(feedback);Check(dismissed,"dismiss request");
        std::array<TileSize,2> sizes{{{1,1},{2,150}}};std::array<PreviewTileView,2> tiles{};tiles[0].id=1;tiles[0].title="First";tiles[0].status=PreviewState::Loading;tiles[1].id=2;tiles[1].title="Second";tiles[1].status=PreviewState::Offline;
        std::array<TileEvent,16> eventStorage{};TileStripState tileState;
        std::size_t tileEventCount=0;
        std::size_t tileEventCapacity=eventStorage.size();
        auto drawTiles=[&]{TileEventBuffer events{std::span(eventStorage).first(tileEventCapacity)};ResizableTileStrip("tiles",tiles,sizes,0,tileState,events,{Orientation::Horizontal,120,{300,220}},options);tileEventCount=events.count;};
        frame(drawTiles);Check(sizes[0].extent==120,"minimum tile extent");
        for(int i=0;i<3;++i)frame(drawTiles);
        auto resizeNode=semantics.Tree().nodes[0];for(auto& item:semantics.Tree().nodes)if(item.name=="Resize preview")resizeNode=item;
        queue.Push({resizeNode.id,accessibility::SemanticAction::Increment});frame(drawTiles);
        Check(sizes[0].extent>120&&Near(sizes[0].extent+sizes[1].extent,270)&&tileEventCount==3,"accessible resize transaction and total");
        frame(drawTiles);frame(drawTiles);
        for(auto& item:semantics.Tree().nodes)if(item.name=="Resize preview")resizeNode=item;
        const float originalFirst=sizes[0].extent,originalSecond=sizes[1].extent;
        float mouseX=(resizeNode.minimum.x+resizeNode.maximum.x)*.5f,mouseY=(resizeNode.minimum.y+resizeNode.maximum.y)*.5f;
        io.AddMousePosEvent(mouseX,mouseY);frame(drawTiles);io.AddMouseButtonEvent(0,true);frame(drawTiles);
        io.AddMousePosEvent(mouseX+8,mouseY);frame(drawTiles);Check(sizes[0].extent>originalFirst,"mouse splitter resize");
        tileEventCapacity=0;io.AddKeyEvent(ImGuiKey_Escape,true);frame(drawTiles);
        Check(Near(sizes[0].extent,originalFirst)&&Near(sizes[1].extent,originalSecond),"Escape restores adjacent sizes");
        Check(tileState.cancelPending,"full event buffer retains cancellation");
        tileEventCapacity=eventStorage.size();io.AddKeyEvent(ImGuiKey_Escape,false);io.AddMouseButtonEvent(0,false);frame(drawTiles);
        Check(tileEventCount==1&&eventStorage[0].action==TileAction::ResizeCancel&&!tileState.cancelPending,"cancel terminal retry after release");
        auto titleNode=semantics.Tree().nodes.front();queue.Push({titleNode.id,accessibility::SemanticAction::Select});frame(drawTiles);
        Check(tileEventCount==1&&eventStorage[0].action==TileAction::Select,"tile select request");
        queue.Push({titleNode.id,accessibility::SemanticAction::Select});
        frame([&]{ImGui::BeginDisabled();drawTiles();ImGui::EndDisabled();});Check(tileEventCount==0,"parent-disabled tile semantic request rejected");
        ImageViewportState imageState;ImageView image;image.pixels={640,360};
        auto imageFrame=[&]{ZoomToolbar("zoom",imageState,options);auto view=BeginImageViewport("image",image,imageState,{300,220},theme);Point points[]={{0,0},{640,360}};DrawOverlay(view,imageState.canvas,{OverlayShape::Rectangle,points},theme);EndImageViewport();};
        frame(imageFrame);Check(imageState.canvas.scale.x>0&&imageState.canvas.scale.x==imageState.canvas.scale.y,"viewport uniform fit missing texture");
        DialogState progressState;ToolbarState toolbar;std::array<imkit::Command,2> commands{{{1,"First"},{2,"Second"}}};bool open=true;
        progressState.open=true;
        auto modal=[&]{imkit::Progress("modal",ProgressView{.5f,"Stage","Working",true},ProgressPresentation::Modal,progressState,{},options);};
        frame(modal);frame(modal);progressState.open=false;frame(modal);frame(modal);
        std::array<StableId,4> dismissStorage{};
        frame([&]{RequestBuffer requests{dismissStorage};ToastRegion("toast",notices,5,order,requests,{2,220},options);});
        Check(semantics.Tree().nodes.size()==4,"toast display maximum");
        for(auto& item:semantics.Tree().nodes)Check(item.minimum.x>=0&&item.maximum.x<=io.DisplaySize.x&&item.maximum.y<=io.DisplaySize.y,"toast within viewport");
        StableId executed=0;commands[1].disabled=true;
        auto overflowToolbar=[&]{ImGui::BeginChild("narrow-toolbar",{35,100});executed=ResponsiveToolbar("commands",toolbar,commands,ToolbarOptions{},options);ImGui::EndChild();};
        frame(overflowToolbar);frame(overflowToolbar);
        auto more=semantics.Tree().nodes.front();queue.Push({more.id,accessibility::SemanticAction::Press});frame(overflowToolbar);frame(overflowToolbar);
        auto menuDisabled=semantics.Tree().nodes.back();queue.Push({menuDisabled.id,accessibility::SemanticAction::Press});frame(overflowToolbar);Check(!executed,"overflow disabled command rejected");
        io.AddKeyEvent(ImGuiKey_Escape,true);frame(overflowToolbar);io.AddKeyEvent(ImGuiKey_Escape,false);frame(overflowToolbar);
        CommandPaletteState palette;palette.open=true;
        auto paletteFrame=[&]{executed=CommandPalette("palette",palette,commands,options);};
        frame(paletteFrame);frame(paletteFrame);
        for(auto& item:semantics.Tree().nodes)if(item.role==accessibility::SemanticRole::MenuItem&&!item.state.disabled)queue.Push({item.id,accessibility::SemanticAction::Press});
        frame(paletteFrame);Check(executed==1&&!palette.open,"palette semantic execution closes popup");
        commands[1].disabled=false;
        auto composed=[&]{
            StepNavigatorOptions small;small.size={30,90};StepNavigator("narrow",steps,1,nav,small,options);
            NavigationRail("rail",steps,1,nav,{150,80},options);FilterChip("chip","Filter",true,options);
            InlineAlert("inline",notices[2],options);PersistentBanner("banner",notices[2],options);
            EmptyState("empty",StateView{"Empty","Description","Create"},options);
            UnavailableState("offline",StateView{"Offline","Description"},options);RetryState("retry",StateView{"Error","Description","Retry"},options);
            imkit::Progress("inline-progress",ProgressView{.5f,"Stage","Working",true},ProgressPresentation::Inline,progressState,{},options);
            ResponsiveToolbar("toolbar",toolbar,commands,ToolbarOptions{},options);
            SectionHeader("section","Section",open,options);MultiSelectionBar("selected",2,commands,toolbar,options);
            HelpCallout("help",StateView{"Help","Description"},options);ValidationSummary("issues",steps,options);
            AppBarView appView{"ImKit","Project","Workflow","Ready",true,FeedbackKind::Success,commands};
            AppBar("app",appView,toolbar,options);
            WorkspaceHeader("workspace",WorkspaceHeaderView{"PHASE","Workspace","Description","Ready",FeedbackKind::Success,commands},toolbar,options);
            InspectorSection("inspector","Inspector","Selected item",open,options);
            AdvancedSection("advanced","Advanced",open,options);
            BottomActionBar("bottom",BottomActionBarView{"Ready",FeedbackKind::Success,commands},toolbar,options);
            ThemePickerState picker;ThemePreset preset=ThemePreset::PrecisionDark;ThemePicker("theme",&preset,picker,options);
            DiagnosticsDrawerState diagnostics{true};if(BeginDiagnosticsDrawer("diagnostics","Diagnostics",diagnostics,{0,80},options)){ImGui::TextUnformatted("No issues");EndDiagnosticsDrawer();}
        };
        for(int i=0;i<8;++i)frame(composed);
        measuring=true;gallery::CountAllocations(true);for(int i=0;i<8;++i)frame(composed);gallery::CountAllocations(false);measuring=false;
        Check(gallery::AllocationCount()==0,"steady C++ allocation bounded");
        Check(allocations==0,"steady ImGui allocation bounded");
        frame([&]{StepNavigator("empty",{},0,nav,{},options);ImageView invalid;auto v=BeginImageViewport("invalid",invalid,imageState,{1,1},theme);EndImageViewport();});
        ImGui::DestroyContext(context);
    }
    std::printf("workflow failures=%d allocations=%d\n",failures,allocations);return failures?1:0;
}
