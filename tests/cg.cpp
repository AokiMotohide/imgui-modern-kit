#include <imkit/cg.h>
#include <imkit/preview.h>
#include <cmath>
#include <cstdio>
using namespace imkit::cg;
int main() {
    int failures = 0;
    auto check = [&](bool ok, const char *s) {
        if (!ok) {
            ++failures;
            std::fprintf(stderr, "FAIL %s\n", s);
        }
    };
    auto center = Project({}, Camera{}, {0, 0}, {800, 600});
    check(center.visible && center.screen.x == 400 && center.screen.y == 300, "camera center");
    Camera c;
    c.projection = Projection::Orthographic;
    c.yaw = 0;
    c.pitch = 0;
    c.orthographicHeight = 6;
    auto p = Project({1, 1, 0}, c, {0, 0}, {800, 600});
    check(p.visible && p.screen.x == 500 && p.screen.y == 200, "orthographic projection");
    NavigateCamera(c,{}, {},1,600);
    auto zoomed=Project({1,1,0},c,{0,0},{800,600});
    check(std::abs(c.orthographicHeight-5.1)<1e-12 && zoomed.screen.x>p.screen.x && c.distance==6,
          "orthographic navigation zoom changes projected extent");
    AlignCamera(c,Axis::X);
    NavigateCamera(c,{}, {60,0},0,600);
    check(std::abs(c.target.x)<1e-12 && c.target.z>0 && std::abs(c.target.z-.51)<1e-12,
          "pan follows the view right axis and projected scale");
    for (Axis axis:{Axis::X,Axis::Y,Axis::Z})
        for (bool negative:{false,true}) {
            AlignCamera(c,axis,negative);
            auto basis=OrientationBasis(Orientation::View,{},c);
            double facing=axis==Axis::X ? basis.z.x : axis==Axis::Y ? basis.z.y : basis.z.z;
            check(std::abs(facing-(negative ? -1 : 1))<1e-12,"six axis view alignment");
        }
    c.projection=Projection::Camera;
    auto originalCamera=c;
    NavigateCamera(c,{20,10},{20,10},3,600);
    check(c.yaw==originalCamera.yaw && c.distance==originalCamera.distance && c.target.z==originalCamera.target.z,
          "host camera view ignores free viewport navigation");
    c=Camera{}; c.projection=Projection::Orthographic; c.yaw=c.pitch=0;c.orthographicHeight=6;
    auto t = TransformDelta({}, TransformTool::Translate, Axis::X, {1.26, 5, 5}, {}, .5);
    check(t.translation.x == 1.5 && t.translation.y == 0 && t.translation.z == 0, "axis snapped translation");
    Basis b;
    b.x = {0, 1, 0};
    t = TransformDelta({}, TransformTool::Translate, Axis::X, {1, 0, 0}, b);
    check(t.translation.y == 1, "custom basis");
    t = TransformDelta({}, TransformTool::Scale, Axis::Y, {1, 1, 1}, {});
    check(t.scale.x == 1 && t.scale.y == 2, "axis scale");
    auto uv = TransformUV({1, 0}, {0, 0}, {0, 0}, 3.141592653589793 / 2, {1, 1});
    check(std::abs(uv.x) < 1e-12 && std::abs(uv.y - 1) < 1e-12, "UV rotation");
    uv = TransformUV({.13, .37}, {0, 0}, {0, 0}, 0, {2, 2}, .25);
    check(uv.x == .25 && uv.y == .75, "UV scale snap");
    auto *context = ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {800, 600};
    io.DeltaTime = 1.f / 60;
    unsigned char *pixels; int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ImGui::NewFrame();
    ImGui::Begin("Preview normal contract");
    imkit::preview::Vertex vertices[3] = {
        {{-1,-1,0},{1,1,1}}, {{1,-1,0},{1,1,1}}, {{0,1,0},{1,1,1}}};
    std::uint32_t indices[] = {0,1,2};
    imkit::preview::Mesh mesh{1, vertices, indices};
    mesh.transform.scale = {2,1,.5};
    mesh.transform.rotation.z = 3.141592653589793 / 2;
    imkit::preview::Triangle triangles[1];
    check(imkit::preview::DrawListPreview(*ImGui::GetWindowDrawList(), {&mesh,1}, c,
                                         {0,0}, {800,600}, triangles) == 1,
          "DrawList transformed triangle");
    auto expected = static_cast<int>(std::lround(255 * (.25 + .75 * 1.1 / std::sqrt(5.25 * .98))));
    check(std::abs(static_cast<int>(triangles[0].color & 255) - expected) <= 1,
          "DrawList inverse transpose normal under rotation and nonuniform scale");
    ImGui::End();
    ImGui::Render();
    ViewportState navigation;
    navigation.camera.yaw=navigation.camera.pitch=0;
    navigation.camera.projection=Projection::Orthographic;
    ViewportView navigationView;
    auto navigationFrame=[&] {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Viewport navigation");
        navigationView=BeginViewport("view",navigation,{}, {750,500},
                                     imkit::MakePrecisionTheme(imkit::ColorScheme::Dark));
        EndViewport(); ImGui::End(); ImGui::Render();
    };
    navigationFrame(); navigationFrame();
    io.AddMousePosEvent(navigationView.min.x+100,navigationView.min.y+100);
    navigationFrame();
    double originalHeight=navigation.camera.orthographicHeight;
    io.AddMouseWheelEvent(0,1); navigationFrame();
    check(navigation.camera.orthographicHeight<originalHeight,"orthographic wheel through public IO");
    io.AddMousePosEvent(navigationView.min.x+navigationView.size.x-24,navigationView.min.y+56);
    navigationFrame();
    io.AddMouseButtonEvent(0,true); navigationFrame();
    io.AddMouseButtonEvent(0,false); navigationFrame();
    check(std::abs(navigation.camera.yaw-3.141592653589793/2)<1e-12,
          "navigation gizmo axis click through public IO");
    Camera hostCamera; hostCamera.distance=12;hostCamera.yaw=.2;
    navigation.cameraView=&hostCamera;navigation.camera.projection=Projection::Camera;
    navigationFrame();
    check(navigation.camera.distance==12 && navigation.camera.yaw==.2,
          "camera view consumes the non-owning host camera");
    std::array<imkit::editor::Keyframe,2> dopeKeys{{{901,1,imkit::editor::FromSeconds(1),.2},
        {902,1,imkit::editor::FromSeconds(2),.7}}};
    imkit::editor::CurveProvider dopeProvider;
    dopeProvider.user=&dopeKeys;dopeProvider.revision=1;
    dopeProvider.query=[](void *u,imkit::editor::CurveQuery) {
        return std::span<const imkit::editor::Keyframe>(*static_cast<decltype(dopeKeys)*>(u));
    };
    dopeProvider.selected=[](void *u,std::span<const imkit::editor::StableId>) {
        return std::span<const imkit::editor::Keyframe>(*static_cast<decltype(dopeKeys)*>(u));
    };
    imkit::editor::CurveState dopeState;
    std::array<imkit::editor::Transaction,1> dopeCompanions{};dopeState.companionDrags=dopeCompanions;
    std::array<imkit::editor::StableId,2> dopeIds{901,902};
    imkit::editor::Selection dopeSelection{dopeIds,2,901};
    std::array<imkit::editor::Event,8> dopeStorage{};imkit::editor::EventBuffer dopeEvents{dopeStorage};
    auto dopeFrame=[&] {
        dopeEvents.Clear();ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({800,600});ImGui::Begin("Dope test");
        DopeSheet("dope",dopeProvider,dopeState,dopeSelection,dopeEvents,
            imkit::MakePrecisionTheme(imkit::ColorScheme::Dark),{700,400});
        ImGui::End();ImGui::Render();
    };
    dopeFrame();dopeFrame();
    auto gesture=[&](float pixels) {
        const auto x=dopeState.view.min.x+100,y=dopeState.view.min.y+15;
        io.AddMousePosEvent(x,y);dopeFrame();io.AddMouseButtonEvent(0,true);dopeFrame();
        io.AddMousePosEvent(x+pixels,y);dopeFrame();io.AddMouseButtonEvent(0,false);dopeFrame();
    };
    gesture(25);
    check(dopeEvents.count==2 && dopeEvents.Events()[0].proposed.first==imkit::editor::FromSeconds(1.25) &&
        dopeEvents.Events()[1].proposed.first==imkit::editor::FromSeconds(2.25),"Dope Sheet moves selected keys together");
    dopeState.scaleTime=true;gesture(100);
    check(dopeEvents.count==2 && dopeEvents.Events()[0].kind==imkit::editor::EditKind::KeyScale &&
        dopeEvents.Events()[1].proposed.first==imkit::editor::FromSeconds(3),"Dope Sheet scales selected timing");
    io.AddKeyEvent(ImGuiMod_Alt,true);dopeFrame();gesture(25);
    check(dopeEvents.count==2 && dopeEvents.Events()[0].kind==imkit::editor::EditKind::Duplicate &&
        dopeEvents.Events()[1].kind==imkit::editor::EditKind::Duplicate,"Dope Sheet Alt duplicates selection");
    io.AddKeyEvent(ImGuiMod_Alt,false);dopeFrame();
    dopeState.scaleTime=false;dopeState.snapToFrame=true;dopeState.rate={24,1};gesture(11);
    check(dopeEvents.count==2 && dopeEvents.Events()[0].proposed.first==imkit::editor::FrameToTick(27,{24,1}) &&
        dopeEvents.Events()[1].proposed.first==imkit::editor::FrameToTick(51,{24,1}),
        "Dope Sheet snap preserves selected spacing");
    const auto dopeX=dopeState.view.min.x+100,dopeY=dopeState.view.min.y+15;
    io.AddMousePosEvent(dopeX,dopeY);dopeFrame();io.AddMouseButtonEvent(0,true);dopeFrame();
    io.AddMousePosEvent(dopeX+25,dopeY);dopeFrame();
    dopeEvents.storage=std::span(dopeStorage).first(1);
    io.AddMouseButtonEvent(0,false);dopeFrame();
    check(dopeEvents.overflow && dopeEvents.count==0 && dopeState.drag.active && dopeCompanions[0].active,
        "Dope Sheet short terminal buffer retains every transaction");
    dopeEvents.storage=dopeStorage;dopeFrame();
    check(dopeEvents.count==2 && dopeEvents.Events()[0].phase==imkit::editor::Phase::Commit &&
        dopeEvents.Events()[1].phase==imkit::editor::Phase::Commit && !dopeState.drag.active && !dopeCompanions[0].active,
        "Dope Sheet retries complete Commit batch");
    io.AddMousePosEvent(dopeX,dopeY);dopeFrame();io.AddMouseButtonEvent(0,true);dopeFrame();
    ++dopeProvider.revision;dopeFrame();
    check(dopeEvents.count==2 && dopeEvents.Events()[0].phase==imkit::editor::Phase::Cancel &&
        dopeEvents.Events()[1].phase==imkit::editor::Phase::Cancel,"Dope Sheet revision change cancels complete batch");
    io.AddMouseButtonEvent(0,false);dopeFrame();
    io.AddKeyEvent(ImGuiMod_Ctrl,true);dopeFrame();
    io.AddMousePosEvent(dopeX,dopeY);dopeFrame();io.AddMouseButtonEvent(0,true);dopeFrame();
    check(dopeSelection.count==1 && !dopeSelection.Contains(901) && !dopeState.drag.active &&
        dopeEvents.count==0,"Ctrl deselection does not begin a Dope Sheet edit");
    io.AddMouseButtonEvent(0,false);dopeFrame();io.AddKeyEvent(ImGuiMod_Ctrl,false);dopeFrame();
    dopeSelection.Set(901,true);
    std::array dopeBindings{imkit::editor::Binding{imkit::editor::Command::Delete,ImGuiKey_F9}};
    dopeState.bindings=dopeBindings;
    io.AddKeyEvent(ImGuiKey_F9,true);dopeFrame();
    check(dopeEvents.count==2 && dopeEvents.Events()[0].kind==imkit::editor::EditKind::Remove &&
        dopeEvents.Events()[1].kind==imkit::editor::EditKind::Remove,"Dope Sheet remapped Delete removes full selection");
    io.AddKeyEvent(ImGuiKey_F9,false);dopeFrame();dopeKeys[1].locked=true;
    io.AddKeyEvent(ImGuiKey_F9,true);dopeFrame();
    check(dopeEvents.count==0,"Dope Sheet locked member blocks full Delete batch");
    io.AddKeyEvent(ImGuiKey_F9,false);dopeFrame();
    std::array<StripView,1> testStrips{{{7101,1,"Motion",{0,imkit::editor::FromSeconds(4)}}}};
    imkit::editor::CanvasState stripCanvas;stripCanvas.scale={100,1};
    imkit::editor::Transaction stripDrag;
    ImVec2 stripOrigin{};
    int stripSettingsCommits=0,stripSettingsCancels=0;
    auto stripFrame=[&] {
        dopeEvents.Clear();ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({800,600});ImGui::Begin("Strip test");
        stripOrigin=ImGui::GetCursorScreenPos();
        AnimationStrips("strips",testStrips,1,stripCanvas,stripDrag,dopeEvents,
            imkit::MakePrecisionTheme(imkit::ColorScheme::Dark),{700,400});
        ImGui::End();ImGui::Render();
        for (const auto &event:dopeEvents.Events()) if (event.kind==imkit::editor::EditKind::StripSettings) {
            if (event.phase==imkit::editor::Phase::Commit) ++stripSettingsCommits;
            if (event.phase==imkit::editor::Phase::Cancel) ++stripSettingsCancels;
        }
    };
    stripFrame();stripFrame();
    io.AddMousePosEvent(stripOrigin.x+50,stripOrigin.y+12);stripFrame();
    io.AddMouseButtonEvent(0,true);stripFrame();
    io.AddMousePosEvent(stripOrigin.x+75,stripOrigin.y+12);stripFrame();
    check(stripDrag.active && stripDrag.draft.proposed.first==imkit::editor::FromSeconds(.25) &&
        testStrips[0].range.first==0,"strip preview leaves host range unchanged");
    io.AddMouseButtonEvent(0,false);stripFrame();
    check(dopeEvents.count==1 && dopeEvents.Events()[0].phase==imkit::editor::Phase::Commit &&
        dopeEvents.Events()[0].proposed.last==imkit::editor::FromSeconds(4.25),"strip move commits translated range");
    for (bool end:{false,true}) {
        const float edge=ImGui::GetStyle().WindowPadding.x+(end?398.f:2.f);
        io.AddMousePosEvent(stripOrigin.x+edge,stripOrigin.y+12);stripFrame();
        io.AddMouseButtonEvent(0,true);stripFrame();
        io.AddMousePosEvent(stripOrigin.x+edge+25,stripOrigin.y+12);stripFrame();
        io.AddMouseButtonEvent(0,false);stripFrame();
        check(dopeEvents.count==1 && dopeEvents.Events()[0].kind==
            (end?imkit::editor::EditKind::TrimEnd:imkit::editor::EditKind::TrimStart) &&
            dopeEvents.Events()[0].proposed.first==imkit::editor::FromSeconds(end?0:.25) &&
            dopeEvents.Events()[0].proposed.last==imkit::editor::FromSeconds(end?4.25:4),
            "strip edge drag trims only the chosen boundary");
    }
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.AddMousePosEvent(stripOrigin.x+50,stripOrigin.y+12);stripFrame();
    io.AddMouseButtonEvent(1,true);stripFrame();io.AddMouseButtonEvent(1,false);stripFrame();
    check(ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId),"strip settings open from public right-click");
    auto stripKey=[&](ImGuiKey key) {
        io.AddKeyEvent(key,true);stripFrame();io.AddKeyEvent(key,false);stripFrame();
    };
    stripKey(ImGuiKey_DownArrow);stripKey(ImGuiKey_Enter);stripKey(ImGuiKey_RightArrow);
    check(stripDrag.active && stripDrag.draft.kind==imkit::editor::EditKind::StripSettings,
        "strip settings keyboard edit begins a typed transaction");
    stripKey(ImGuiKey_Enter);
    check(stripSettingsCommits==1 && !stripDrag.active,"strip settings keyboard edit commits once");
    stripKey(ImGuiKey_Enter);stripKey(ImGuiKey_RightArrow);stripKey(ImGuiKey_Escape);
    check(stripSettingsCancels==1 && stripSettingsCommits==1 && !stripDrag.active,
        "strip settings Escape cancels without a second commit");
    UVProvider allUV;
    allUV.all=[](void *,UVSelection)->std::span<const imkit::editor::StableId> {
        static constexpr std::array<imkit::editor::StableId,3> ids{8101,8202,8303};return ids;
    };
    UVState allUVState;
    std::array uvBindings{imkit::editor::Binding{imkit::editor::Command::SelectAll,ImGuiKey_F10}};
    allUVState.bindings=uvBindings;
    std::array<imkit::editor::StableId,3> uvIds{};imkit::editor::Selection uvSelected{uvIds};
    auto uvFrame=[&] {
        dopeEvents.Clear();ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({800,600});ImGui::Begin("UV selection");
        UVEditor("uv",allUV,{},allUVState,uvSelected,dopeEvents,
            imkit::MakePrecisionTheme(imkit::ColorScheme::Dark),{700,400});
        ImGui::End();ImGui::Render();
    };
    uvFrame();uvFrame();
    io.AddMousePosEvent(allUVState.view.min.x+25,allUVState.view.min.y+25);uvFrame();
    io.AddMouseButtonEvent(0,true);uvFrame();io.AddMouseButtonEvent(0,false);uvFrame();
    io.AddKeyEvent(ImGuiKey_F10,true);uvFrame();
    check(uvSelected.count==3 && uvSelected.Contains(8303) && dopeEvents.count==3,
        "UV remapped Select All includes provider IDs outside visible query");
    io.AddKeyEvent(ImGuiKey_F10,false);uvFrame();
    uvSelected.Clear();uvSelected.storage=std::span(uvIds).first(1);uvSelected.Set(9999);
    io.AddKeyEvent(ImGuiKey_F10,true);uvFrame();
    check(dopeEvents.overflow && dopeEvents.count==0 && uvSelected.count==1 && uvSelected.Contains(9999),
        "UV Select All preserves old selection on insufficient storage");
    io.AddKeyEvent(ImGuiKey_F10,false);uvFrame();
    allUV.selectionQuery=[](void *,imkit::editor::Rect,UVSelection mode)->std::span<const imkit::editor::SelectablePoint> {
        static constexpr std::array points{imkit::editor::SelectablePoint{8101,{.2,.2},false},
            imkit::editor::SelectablePoint{8202,{.8,.8},false}};
        return mode==UVSelection::Vertex?std::span<const imkit::editor::SelectablePoint>(points):std::span<const imkit::editor::SelectablePoint>{};
    };
    uvSelected.storage=uvIds;
    std::array<imkit::editor::Point,32> uvPath;allUVState.canvas.selectionPath=uvPath;
    auto uvMouse=[&](imkit::editor::Point point) {
        const auto screen=imkit::editor::ToScreen(point,allUVState.canvas,{allUVState.view.min.x,allUVState.view.min.y});
        io.AddMousePosEvent(static_cast<float>(screen.x),static_cast<float>(screen.y));uvFrame();
    };
    for (bool lasso:{false,true}) {
        allUVState.lassoSelect=lasso;
        uvMouse({.1,.1});io.AddMouseButtonEvent(0,true);uvFrame();
        uvMouse({.4,.1});uvMouse({.4,.4});
        if (lasso) uvMouse({.1,.4});
        io.AddMouseButtonEvent(0,false);uvFrame();
        check(uvSelected.count==1 && uvSelected.Contains(8101) && dopeEvents.count==1 &&
            dopeEvents.Events()[0].kind==(lasso?imkit::editor::EditKind::LassoSelect:imkit::editor::EditKind::BoxSelect),
            "UV box and lasso select only enclosed provider candidates");
    }
    allUV.vertices=[](void *,imkit::editor::Rect)->std::span<const UVVertex> {
        static const std::array vertices{UVVertex{8101,1,{.2,.2}}};return vertices;
    };
    allUVState.snap=0;allUVState.pivot={.5,.5};
    for (auto tool:{TransformTool::Rotate,TransformTool::Scale}) {
        allUVState.tool=tool;
        uvMouse({.2,.2});io.AddMouseButtonEvent(0,true);uvFrame();
        uvMouse({tool==TransformTool::Rotate?1.77079632679:.7,.2});
        io.AddMouseButtonEvent(0,false);uvFrame();
        const double expectedX=tool==TransformTool::Rotate?.8:.05;
        check(dopeEvents.count==1 && dopeEvents.Events()[0].kind==
            (tool==TransformTool::Rotate?imkit::editor::EditKind::Rotate:imkit::editor::EditKind::Scale) &&
            std::abs(dopeEvents.Events()[0].proposed.x-expectedX)<.002 &&
            std::abs(dopeEvents.Events()[0].proposed.y-.2)<.002,
            "UV rotation and scale emit pivot-relative coordinates with matching edit kind");
    }
    allUV.selected=[](void *,std::span<const imkit::editor::StableId>,UVSelection)->std::span<const UVVertex> {
        static const std::array vertices{UVVertex{8101,1,{.2,.2}},UVVertex{8202,1,{.8,.8}}};return vertices;
    };
    std::array<imkit::editor::Transaction,1> uvCompanions;allUVState.companionDrags=uvCompanions;
    uvSelected.Set(8101);uvSelected.Set(8202,true);allUVState.tool=TransformTool::Translate;
    uvMouse({.2,.2});io.AddMouseButtonEvent(0,true);uvFrame();uvMouse({.3,.2});
    check(allUVState.drag.active && uvCompanions[0].active &&
        std::abs(uvCompanions[0].draft.proposed.x-.9)<.001,"UV multi-drag transforms every selected vertex");
    dopeEvents.storage=std::span(dopeStorage).first(1);
    io.AddMouseButtonEvent(0,false);uvFrame();
    check(dopeEvents.overflow && dopeEvents.count==0 && allUVState.drag.active && uvCompanions[0].active,
        "UV short terminal buffer retains the entire batch");
    dopeEvents.storage=dopeStorage;uvFrame();
    check(dopeEvents.count==2 && dopeEvents.Events()[0].phase==imkit::editor::Phase::Commit &&
        dopeEvents.Events()[1].phase==imkit::editor::Phase::Commit && !allUVState.drag.active && !uvCompanions[0].active,
        "UV terminal retry commits every selected vertex together");
    ImGui::DestroyContext(context);
    return failures ? 1 : 0;
}
