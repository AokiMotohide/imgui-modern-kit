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
    Transform orbitObject;orbitObject.translation={3,1,0};orbitObject.rotation={.3,.4,.5};
    auto pivoted=TransformAroundPivot(orbitObject,TransformTool::Rotate,Axis::Z,{0,0,3.141592653589793/2},{},{1,1,0});
    check(std::abs(pivoted.translation.x-1)<1e-12 && std::abs(pivoted.translation.y-3)<1e-12,
          "rotation moves offset around external pivot");
    auto beforeBasis=OrientationBasis(Orientation::Local,orbitObject,{});
    auto afterBasis=OrientationBasis(Orientation::Local,pivoted,{});
    check(std::abs(afterBasis.x.x+beforeBasis.x.y)<1e-12 && std::abs(afterBasis.x.y-beforeBasis.x.x)<1e-12 &&
          std::abs(afterBasis.x.z-beforeBasis.x.z)<1e-12,"world rotation composes existing orientation");
    Basis turned{{0,1,0},{-1,0,0},{0,0,1}};
    pivoted=TransformAroundPivot(orbitObject,TransformTool::Scale,Axis::Y,{0,1,0},turned,{1,1,0});
    check(std::abs(pivoted.translation.x-5)<1e-12 && std::abs(pivoted.translation.y-1)<1e-12,
          "pivot scale follows chosen orientation basis");
    Transform affine;affine.rotation={.3,.4,.7};affine.scale={2,3,4};affine.shear={.2,.1,-.3};
    const auto oldLinear=LinearBasis(affine);
    const auto stretched=TransformDelta(affine,TransformTool::Scale,Axis::X,{1,0,0},{});
    const auto newLinear=LinearBasis(stretched);
    const auto correct=[](Vec3 a,Vec3 b){return std::abs(b.x-2*a.x)<1e-10 && std::abs(b.y-a.y)<1e-10 && std::abs(b.z-a.z)<1e-10;};
    check(correct(oldLinear.x,newLinear.x) && correct(oldLinear.y,newLinear.y) && correct(oldLinear.z,newLinear.z),
          "world nonuniform scale preserves full affine transform including shear");
    const auto normal=NormalBasis(stretched);
    const auto dot=[](Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    check(std::abs(dot(normal.x,newLinear.x)-1)<1e-10 && std::abs(dot(normal.x,newLinear.y))<1e-10 &&
          std::abs(dot(normal.y,newLinear.z))<1e-10,"sheared normal matrix is inverse transpose");
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
    mesh.transform.shear={.5,.2,.1};
    check(imkit::preview::DrawMeshNormals(*ImGui::GetWindowDrawList(),{&mesh,1},c,{0,0},{800,600},{true,true})==4,
          "sheared mesh draws three vertex normals and one face normal");
    check(imkit::preview::DrawMeshNormals(*ImGui::GetWindowDrawList(),{&mesh,1},c,{0,0},{800,600},{false,true})==1,
          "face normal overlay independently selectable");
    check(imkit::preview::DrawMeshNormals(*ImGui::GetWindowDrawList(),{&mesh,1},c,{0,0},{800,600},{true,true,0})==0,
          "zero-length normal overlay emits no segments");
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
    allUV.edges=[](void *,imkit::editor::Rect)->std::span<const UVEdge> {
        static const std::array edges{UVEdge{9901,{.2,.2},{.8,.8},false,false,8101,8202}};return edges;
    };
    allUVState.selection=UVSelection::Edge;uvSelected.Clear();
    uvMouse({.5,.5});io.AddMouseButtonEvent(0,true);uvFrame();
    check(uvSelected.count==1 && uvSelected.Contains(9901) && allUVState.drag.draft.target==8101 &&
        uvCompanions[0].draft.target==8202,"UV edge selection expands to endpoint transactions");
    uvMouse({.6,.5});io.AddMouseButtonEvent(0,false);uvFrame();
    check(dopeEvents.count==2 && std::abs(dopeEvents.Events()[0].proposed.x-.3)<.001 &&
        std::abs(dopeEvents.Events()[1].proposed.x-.9)<.001 && uvSelected.Contains(9901),
        "UV edge drag moves both endpoints while keeping edge selection IDs");
    std::array faceVertices{UVVertex{8101,1,{.2,.2}},UVVertex{8202,1,{.8,.2}},UVVertex{8303,1,{.2,.8}}};
    std::array faces{UVFace{9911,9922,faceVertices,false}};allUV.user=&faces;
    allUV.faces=[](void *u,imkit::editor::Rect) {return std::span<const UVFace>(*static_cast<decltype(faces)*>(u));};
    allUV.selected=[](void *u,std::span<const imkit::editor::StableId>,UVSelection) {
        return static_cast<decltype(faces)*>(u)->front().vertices;
    };
    std::array<imkit::editor::Transaction,2> faceCompanions;allUVState.companionDrags=faceCompanions;
    for (auto mode:{UVSelection::Face,UVSelection::Island}) {
        allUVState.selection=mode;uvSelected.Clear();
        uvMouse({.35,.35});io.AddMouseButtonEvent(0,true);uvFrame();uvMouse({.45,.35});
        io.AddMouseButtonEvent(0,false);uvFrame();
        check(uvSelected.count==1 && uvSelected.Contains(mode==UVSelection::Face?9911:9922) &&
            dopeEvents.count==3 && std::abs(dopeEvents.Events()[2].proposed.x-.3)<.001,
            "UV face and island drag expands to every constituent vertex");
    }
    ViewportState handleState;
    handleState.camera.projection=Projection::Orthographic;
    handleState.camera.orthographicHeight=5;
    ObjectView handleObject; handleObject.id=0x100000055ull;
    imkit::editor::Event handleStorage[8];
    imkit::editor::EventBuffer handleEvents{handleStorage};
    const ViewportView handleView{{20,50},{700,500},true};
    auto handleFrame=[&] {
        handleEvents.Clear(); ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Transform handles");
        TransformGizmo(handleView,handleObject,handleState,7,handleEvents,
                       imkit::MakePrecisionTheme(imkit::ColorScheme::Dark));
        ImGui::End(); ImGui::Render();
    };
    for (TransformTool tool:{TransformTool::Translate,TransformTool::Scale})
    for (Axis plane:{Axis::XY,Axis::YZ,Axis::ZX,Axis::Screen}) {
        handleState.tool=tool;
        handleState.drag={};
        AlignCamera(handleState.camera,plane==Axis::YZ ? Axis::X : plane==Axis::ZX ? Axis::Y : Axis::Z);
        const auto origin=Project({},handleState.camera,handleView.min,handleView.size).screen;
        const Vec3 units[3]={{1,0,0},{0,1,0},{0,0,1}};
        const int a=plane==Axis::Screen ? 0 : static_cast<int>(plane)-static_cast<int>(Axis::XY);
        const int b=(a+1)%3;
        auto first=Project(units[a],handleState.camera,handleView.min,handleView.size).screen;
        auto second=Project(units[b],handleState.camera,handleView.min,handleView.size).screen;
        ImVec2 start=origin;
        if (plane!=Axis::Screen) {
            auto la=std::hypot(first.x-origin.x,first.y-origin.y);
            auto lb=std::hypot(second.x-origin.x,second.y-origin.y);
            start={float(origin.x+26*(first.x-origin.x)/la+26*(second.x-origin.x)/lb),
                   float(origin.y+26*(first.y-origin.y)/la+26*(second.y-origin.y)/lb)};
        }
        io.AddMousePosEvent(start.x,start.y); handleFrame();handleFrame();
        io.AddMouseButtonEvent(0,true);handleFrame();
        check(handleState.drag.active && handleState.activeAxis==plane,"plane/screen handle hit begins transaction");
        const double dx=plane==Axis::Screen ? 20 : .2*(first.x-origin.x)+.35*(second.x-origin.x);
        const double dy=plane==Axis::Screen ? -35 : .2*(first.y-origin.y)+.35*(second.y-origin.y);
        io.AddMousePosEvent(float(start.x+dx),float(start.y+dy));handleFrame();
        double values[3]{};
        if (plane==Axis::Screen) {values[0]=.2;values[1]=.35;}
        else {values[a]=.2;values[b]=.35;}
        if (tool==TransformTool::Scale) {
            if (plane==Axis::Screen) values[0]=values[1]=values[2]=.55;
            for (double &value:values) value+=1;
        }
        const auto proposed=handleState.drag.draft.proposed;
        if (tool==TransformTool::Scale) check(proposed.hasAffine,"scale IO event carries affine rotation and shear payload");
        check(std::abs(proposed.x-values[0])<1e-5 && std::abs(proposed.y-values[1])<1e-5 &&
              std::abs(proposed.z-values[2])<1e-5,"plane/screen independent two dimensional translation");
        io.AddMouseButtonEvent(0,false);handleFrame();
        check(!handleState.drag.active && handleEvents.Events().size()==1 &&
              handleEvents.Events()[0].phase==imkit::editor::Phase::Commit,
              "plane/screen commits once without mutating host transform");
        check(handleObject.transform.translation.x==0 && handleObject.transform.translation.y==0 &&
              handleObject.transform.translation.z==0,"gizmo leaves host data unchanged");
    }
    handleState.tool=TransformTool::Rotate;
    for (Axis axis:{Axis::X,Axis::Y,Axis::Z}) {
        handleState.drag={}; AlignCamera(handleState.camera,axis);
        const auto origin=Project({},handleState.camera,handleView.min,handleView.size).screen;
        const Vec3 units[3]={{1,0,0},{0,1,0},{0,0,1}};
        const int n=static_cast<int>(axis)-1;
        auto u=Project(units[(n+1)%3],handleState.camera,handleView.min,handleView.size).screen;
        auto w=Project(units[(n+2)%3],handleState.camera,handleView.min,handleView.size).screen;
        const double extent=std::max(std::hypot(u.x-origin.x,u.y-origin.y),std::hypot(w.x-origin.x,w.y-origin.y));
        auto ringPoint=[&](double angle) {return ImVec2{
            float(origin.x+70/extent*((u.x-origin.x)*std::cos(angle)+(w.x-origin.x)*std::sin(angle))),
            float(origin.y+70/extent*((u.y-origin.y)*std::cos(angle)+(w.y-origin.y)*std::sin(angle)))};};
        auto start=ringPoint(.3),end=ringPoint(.9);
        io.AddMousePosEvent(start.x,start.y);handleFrame();handleFrame();
        io.AddMouseButtonEvent(0,true);handleFrame();
        check(handleState.drag.active && handleState.activeAxis==axis,"rotation ring hit selects its normal axis");
        io.AddMousePosEvent(end.x,end.y);handleFrame();
        auto result=handleState.drag.draft.proposed;
        // Public IO rounds mouse coordinates to pixels; allow the corresponding angular error.
        check(std::abs(result.x-(n==0 ? .6 : 0))<.02 && std::abs(result.y-(n==1 ? .6 : 0))<.02 &&
              std::abs(result.z-(n==2 ? .6 : 0))<.02,"rotation ring uses projected angular drag");
        io.AddMouseButtonEvent(0,false);handleFrame();
        check(!handleState.drag.active && handleEvents.count==1 &&
              handleEvents.Events()[0].kind==imkit::editor::EditKind::Rotate &&
              handleEvents.Events()[0].phase==imkit::editor::Phase::Commit,"rotation ring commits typed rotation");
    }
    handleState.drag={}; AlignCamera(handleState.camera,Axis::Z);
    auto ringCenter=Project({},handleState.camera,handleView.min,handleView.size).screen;
    io.AddMousePosEvent(ringCenter.x+88,ringCenter.y);handleFrame();handleFrame();
    io.AddMouseButtonEvent(0,true);handleFrame();
    check(handleState.drag.active && handleState.activeAxis==Axis::Screen,"screen rotation ring begins");
    io.AddMousePosEvent(ringCenter.x,ringCenter.y-88);handleFrame();
    check(std::abs(handleState.drag.draft.proposed.z-3.141592653589793/2)<1e-6,
          "screen ring rotates around view normal");
    for (int step=2;step<=9;++step) {
        const double angle=step*3.141592653589793/2;
        io.AddMousePosEvent(float(ringCenter.x+88*std::cos(angle)),float(ringCenter.y-88*std::sin(angle)));
        handleFrame();
        check(std::abs(handleState.rotationAngle-angle)<.02,"screen rotation accumulates across multiple turns");
    }
    io.AddMouseButtonEvent(0,false);handleFrame();
    check(!handleState.drag.active && handleEvents.count==1,"screen rotation commits once");
    handleState.tool=TransformTool::Unified;
    for (auto kind:{imkit::editor::EditKind::Translate,imkit::editor::EditKind::Rotate,imkit::editor::EditKind::Scale}) {
        handleState.drag={};
        const float radius=kind==imkit::editor::EditKind::Translate ? 70.f : kind==imkit::editor::EditKind::Scale ? 110.f : 88.f;
        const float angle=kind==imkit::editor::EditKind::Rotate ? .4f : 0.f;
        const float x=ringCenter.x+radius*std::cos(angle), y=ringCenter.y-radius*std::sin(angle);
        io.AddMousePosEvent(x,y);handleFrame();handleFrame();
        io.AddMouseButtonEvent(0,true);handleFrame();
        check(handleState.drag.active && handleState.drag.draft.kind==kind,"Unified handles select distinct operation kinds");
        io.AddMousePosEvent(x+12,y-9);handleFrame();
        check(!(handleState.drag.draft.proposed==handleState.drag.draft.original),"Unified handle changes selected transform component");
        io.AddMouseButtonEvent(0,false);handleFrame();
        check(!handleState.drag.active && handleEvents.count==1 && handleEvents.Events()[0].kind==kind,
              "Unified preserves operation through commit");
    }
    handleState.tool=TransformTool::Rotate;handleState.pivot=Pivot::Cursor;
    handleState.pivotPosition={};handleObject.transform.translation={2,0,0};
    io.AddMousePosEvent(ringCenter.x+88,ringCenter.y);handleFrame();handleFrame();
    io.AddMouseButtonEvent(0,true);handleFrame();
    check(handleState.drag.active && handleState.pivotDrag.active && handleEvents.count==2,
          "external pivot begins rotation and position atomically");
    io.AddMousePosEvent(ringCenter.x,ringCenter.y-88);handleFrame();
    check(std::abs(handleState.pivotDrag.draft.proposed.x)<1e-6 &&
          std::abs(handleState.pivotDrag.draft.proposed.y-2)<1e-6,"external pivot previews rotated position");
    handleEvents.storage={handleStorage,1};io.AddMouseButtonEvent(0,false);handleFrame();
    check(handleEvents.overflow && handleEvents.count==0 && handleState.drag.active && handleState.pivotDrag.active,
          "short terminal buffer retains complete pivot transaction");
    handleEvents.storage=handleStorage;handleFrame();
    check(!handleState.drag.active && !handleState.pivotDrag.active && handleEvents.count==2 &&
          handleEvents.Events()[0].phase==imkit::editor::Phase::Commit &&
          handleEvents.Events()[1].phase==imkit::editor::Phase::Commit,"pivot terminal batch retries intact");
    ObjectView selectedObjects[]={handleObject,handleObject};selectedObjects[1].id++;
    selectedObjects[1].transform.translation={4,0,0};
    TransformCompanion companionStorage[1];handleState.selectedObjects=selectedObjects;handleState.companions=companionStorage;
    io.AddMousePosEvent(ringCenter.x+88,ringCenter.y);handleFrame();handleFrame();
    io.AddMouseButtonEvent(0,true);handleFrame();
    check(handleEvents.count==4 && handleState.companionCount==1,"multi-object pivot begins complete transform batch");
    io.AddMousePosEvent(ringCenter.x,ringCenter.y-88);handleFrame();
    check(std::abs(companionStorage[0].position.draft.proposed.x)<1e-6 &&
          std::abs(companionStorage[0].position.draft.proposed.y-4)<1e-6,"multi-object rotation uses shared pivot");
    handleEvents.storage={handleStorage,3};io.AddMouseButtonEvent(0,false);handleFrame();
    check(handleEvents.count==0 && handleEvents.overflow && companionStorage[0].transform.active,
          "multi-object terminal buffer rejects partial commit");
    handleEvents.storage=handleStorage;handleFrame();
    check(handleEvents.count==4 && !handleState.drag.active && !companionStorage[0].transform.active &&
          handleState.companionCount==0,"multi-object terminal retry commits entire batch");
    for (int cancellation=0;cancellation<6;++cancellation) {
        handleState.selectedObjects=selectedObjects;selectedObjects[1].locked=false;selectedObjects[1].visible=true;handleObject.visible=true;
        handleState.pivotPosition={};
        io.AddMousePosEvent(ringCenter.x+88,ringCenter.y);handleFrame();handleFrame();
        io.AddMouseButtonEvent(0,true);handleFrame();
        check(handleState.companionCount==1,"multi-object cancellation setup");
        if (cancellation==0) handleState.selectedObjects={selectedObjects,1};
        else if (cancellation==1) selectedObjects[1].locked=true;
        else if (cancellation==2) ++handleObject.id;
        else if (cancellation==3) handleObject.visible=false;
        else if (cancellation==4) selectedObjects[1].visible=false;
        else handleState.pivotPosition={0,0,1000};
        handleEvents.storage={handleStorage,3};handleFrame();
        check(handleEvents.count==0 && handleEvents.overflow && handleState.drag.active &&
              companionStorage[0].position.active,"cancellation overflow retains every target");
        handleEvents.storage=handleStorage;handleFrame();
        bool allCancelled=handleEvents.count==4;
        for (const auto &event:handleEvents.Events()) allCancelled &= event.phase==imkit::editor::Phase::Cancel;
        check(allCancelled && !handleState.drag.active && !companionStorage[0].position.active,
              "disappearance or lock cancels complete batch after capacity recovers");
        io.AddMouseButtonEvent(0,false);handleFrame();
        if (cancellation==2) --handleObject.id;
    }
    OutlinerState nameState;ObjectView nameObject;nameObject.id=0x100000099ull;nameObject.label="Original";
    SceneProvider nameProvider{&nameObject,1,1,[](void *u,int,int,std::string_view){return std::span<const ObjectView>(static_cast<ObjectView*>(u),1);}};
    imkit::editor::StableId nameIds[2];imkit::editor::Selection nameSelection{nameIds};
    ImVec2 nameOrigin{};
    imkit::editor::Event contextResult{};int contextActions=0;
    auto nameFrame=[&] {
        handleEvents.Clear();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Outliner rename");nameOrigin=ImGui::GetCursorScreenPos();
        Outliner("names",nameProvider,nameState,nameSelection,handleEvents);
        for (const auto &event:handleEvents.Events())
            if (event.kind==imkit::editor::EditKind::Reorder || event.kind==imkit::editor::EditKind::Reparent ||
                event.kind==imkit::editor::EditKind::Duplicate || event.kind==imkit::editor::EditKind::LinkGeometry) {
                contextResult=event;++contextActions;
            }
        ImGui::End();ImGui::Render();
    };
    nameFrame();nameFrame();
    auto openRename=[&](int item=1) {
        io.AddMousePosEvent(nameOrigin.x+35,nameOrigin.y+ImGui::GetFrameHeightWithSpacing()+10);nameFrame();
        io.AddMouseButtonEvent(1,true);nameFrame();io.AddMouseButtonEvent(1,false);nameFrame();nameFrame();
        io.AddMousePosEvent(790,590);nameFrame();
        io.AddKeyEvent(ImGuiKey_Home,true);nameFrame();io.AddKeyEvent(ImGuiKey_Home,false);nameFrame();
        for (int step=1;step<item;++step) {
            io.AddKeyEvent(ImGuiKey_DownArrow,true);nameFrame();io.AddKeyEvent(ImGuiKey_DownArrow,false);nameFrame();
        }
        io.AddKeyEvent(ImGuiKey_Enter,true);nameFrame();io.AddKeyEvent(ImGuiKey_Enter,false);nameFrame();nameFrame();
    };
    openRename();
    check(nameState.renameTransaction.active,"Outliner context action begins rename");
    io.AddInputCharactersUTF8("新しい名前");nameFrame();
    io.AddKeyEvent(ImGuiKey_Enter,true);nameFrame();
    check(!nameState.renameTransaction.active && handleEvents.count==1 &&
          handleEvents.Events()[0].phase==imkit::editor::Phase::Commit &&
          std::string_view(handleEvents.Events()[0].proposedText.data())=="新しい名前" &&
          std::string_view(handleEvents.Events()[0].originalText.data())=="Original","Outliner UTF-8 rename commits original and proposed text");
    io.AddKeyEvent(ImGuiKey_Enter,false);nameFrame();
    openRename();io.AddInputCharactersUTF8("Cancelled");nameFrame();
    io.AddKeyEvent(ImGuiKey_Escape,true);nameFrame();
    check(!nameState.renameTransaction.active && handleEvents.count>=1 &&
          handleEvents.Events().back().phase==imkit::editor::Phase::Cancel,"Outliner Escape cancels rename");
    io.AddKeyEvent(ImGuiKey_Escape,false);nameFrame();
    nameObject.parent=0x100000050ull;
    openRename(3);
    check(contextActions==1 && contextResult.target==nameObject.id && contextResult.kind==imkit::editor::EditKind::Reorder &&
          contextResult.proposed.offset==-1 && contextResult.proposed.parent==nameObject.parent,"Outliner Move up emits parent-scoped preceding sibling intent");
    openRename(4);
    check(contextActions==2 && contextResult.proposed.offset==1,"Outliner Move down emits following sibling intent");
    openRename(5);
    check(contextActions==3 && contextResult.kind==imkit::editor::EditKind::Reparent &&
          contextResult.proposed.parent==0 && contextResult.original.parent==nameObject.parent,"Outliner Move to root retains original parent");
    openRename(2);
    check(contextActions==4 && contextResult.target==nameObject.id && contextResult.kind==imkit::editor::EditKind::Duplicate &&
          contextResult.phase==imkit::editor::Phase::Commit,"Outliner Duplicate emits one action for the source StableId");
    nameObject.locked=true;openRename(2);
    check(contextActions==4 && !nameState.renameTransaction.active,"locked Outliner row rejects duplicate context action");
    io.AddKeyEvent(ImGuiKey_Escape,true);nameFrame();io.AddKeyEvent(ImGuiKey_Escape,false);nameFrame();
    nameObject.locked=false;nameObject.geometry=700;nameSelection.active=800;
    openRename(3);
    check(contextActions==5 && contextResult.kind==imkit::editor::EditKind::Duplicate && contextResult.proposed.offset==1,
          "Linked duplicate context action requests shared geometry");
    openRename(4);
    check(contextActions==6 && contextResult.kind==imkit::editor::EditKind::LinkGeometry &&
          contextResult.proposed.parent==800 && contextResult.original.parent==700,"Link geometry context action preserves source object and prior data ID");
    openRename(5);
    check(contextActions==7 && contextResult.kind==imkit::editor::EditKind::LinkGeometry && contextResult.proposed.parent==0,
          "Single user context action requests private geometry copy");
    ComponentView stackComponent{0x100000123ull,0x100000234ull,"Renderer","Host component"};
    ImVec2 stackOrigin{};ComponentStackOptions stackOptions{};
    auto stackFrame=[&] {
        handleEvents.Clear();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Component stack");stackOrigin=ImGui::GetCursorScreenPos();
        ComponentStack("stack",{&stackComponent,1},9,handleEvents,stackOptions);
        ImGui::End();ImGui::Render();
    };
    stackFrame();stackFrame();
    io.AddMousePosEvent(stackOrigin.x+10,stackOrigin.y+10);stackFrame();
    io.AddMouseButtonEvent(0,true);stackFrame();io.AddMouseButtonEvent(0,false);stackFrame();
    check(handleEvents.count==1 && handleEvents.Events()[0].target==stackComponent.id &&
          handleEvents.Events()[0].kind==imkit::editor::EditKind::Toggle && handleEvents.Events()[0].proposed.x==0 &&
          handleEvents.Events()[0].proposed.y==0 && stackComponent.enabled,"ComponentStack enabled click emits proposal without changing host");
    stackComponent.locked=true;stackFrame();
    io.AddMouseButtonEvent(0,true);stackFrame();io.AddMouseButtonEvent(0,false);stackFrame();
    check(handleEvents.count==0 && stackComponent.enabled,"locked ComponentStack rejects enabled edit");
    const ComponentTypeView stackTypes[]={{901,"Renderer"},{902,"Wire override"}};
    stackOptions={stackComponent.owner,stackTypes,false};stackComponent.locked=false;stackFrame();
    io.AddMousePosEvent(stackOrigin.x+12,stackOrigin.y+10);stackFrame();
    io.AddMouseButtonEvent(0,true);stackFrame();io.AddMouseButtonEvent(0,false);stackFrame();stackFrame();
    io.AddMousePosEvent(790,590);stackFrame();
    io.AddKeyEvent(ImGuiKey_Home,true);stackFrame();io.AddKeyEvent(ImGuiKey_Home,false);stackFrame();
    io.AddKeyEvent(ImGuiKey_DownArrow,true);stackFrame();io.AddKeyEvent(ImGuiKey_DownArrow,false);stackFrame();
    io.AddKeyEvent(ImGuiKey_Enter,true);stackFrame();
    check(handleEvents.count==1 && handleEvents.Events()[0].kind==imkit::editor::EditKind::ComponentAdd &&
          handleEvents.Events()[0].target==stackComponent.owner && handleEvents.Events()[0].proposed.parent==902,
          "ComponentStack add menu preserves owner and chosen type ID");
    io.AddKeyEvent(ImGuiKey_Enter,false);stackFrame();
    ViewportState selectionViewport;selectionViewport.tool=TransformTool::Select;
    selectionViewport.camera.projection=Projection::Orthographic;selectionViewport.camera.yaw=selectionViewport.camera.pitch=0;
    imkit::editor::SelectablePoint projectedPoints[2];imkit::editor::Point viewportPath[16];
    selectionViewport.selectionPoints=projectedPoints;selectionViewport.selectionCanvas.selectionPath=viewportPath;
    ObjectView selectObjects[2];selectObjects[0].id=9101;selectObjects[1].id=9102;selectObjects[1].transform.translation={2,0,0};
    auto viewportSelectionFrame=[&] {
        handleEvents.Clear();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Viewport selection");
        ViewportObjects(handleView,selectObjects,selectionViewport,nameSelection,1,handleEvents,
                        imkit::MakePrecisionTheme(imkit::ColorScheme::Dark));
        ImGui::End();ImGui::Render();
    };
    viewportSelectionFrame();viewportSelectionFrame();
    auto selectionCenter=Project({},selectionViewport.camera,handleView.min,handleView.size).screen;
    for (bool lasso:{false,true}) {
        nameSelection.Clear();selectionViewport.lassoSelection=lasso;
        io.AddMousePosEvent(selectionCenter.x-30,selectionCenter.y-30);viewportSelectionFrame();
        io.AddMouseButtonEvent(0,true);viewportSelectionFrame();
        io.AddMousePosEvent(selectionCenter.x+30,selectionCenter.y-30);viewportSelectionFrame();
        io.AddMousePosEvent(selectionCenter.x+30,selectionCenter.y+30);viewportSelectionFrame();
        if (lasso) {io.AddMousePosEvent(selectionCenter.x-30,selectionCenter.y+30);viewportSelectionFrame();}
        io.AddMouseButtonEvent(0,false);viewportSelectionFrame();
        check(nameSelection.count==1 && nameSelection.Contains(9101) && !nameSelection.Contains(9102) &&
              handleEvents.count==1 && handleEvents.Events()[0].kind==(lasso ? imkit::editor::EditKind::LassoSelect : imkit::editor::EditKind::BoxSelect),
              "Viewport box/lasso selects only enclosed projected origins");
    }
    ImGui::DestroyContext(context);
    return failures ? 1 : 0;
}
