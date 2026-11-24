#include "gallery_host.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "../design_gallery/capture.h"
#include "allocation_probe.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace imkit::gallery::host {
using enum imkit::gallery::PageId;

void Verify(Host &h, const std::filesystem::path &out) {
    std::ofstream log(out / "interaction.txt");
    auto check = [&](bool ok, const char *name) {
        log << (ok ? "PASS " : "FAIL ") << name << '\n';
        log.flush();
        if (!ok)
            throw std::runtime_error(name);
    };
    h.Page(PageStart);
    h.Click("start-components");
    check(h.s.page == PageBasic, ("Start route: components (page " + std::to_string(h.s.page) + ")").c_str());
    h.Page(PageStart);
    h.Click("start-icons");
    check(h.s.page == PageIcons, "Start route: visual system");
    h.Page(PageStart);
    h.Click("start-feedback");
    check(h.s.page == PageFeedback, "Start route: feedback and progress");
    h.Page(PageStart);
    h.Click("start-workspaces");
    check(h.s.page == PageWorkbench, "Start route: workspaces");
    h.Page(PageStart);
    h.Click("start-world-3");
    check(h.s.presetIndex == 3, "Start route: wear a world");
    imkit::gallery::ApplyThemePreset(h.s, 0);
    h.Page(PageBasic);
    h.Click("apply");
    check(h.s.clicks == 1, "Action activation");
    h.Click("disabled");
    check(h.s.clicks == 1, "Disabled action");
    h.Click("checkbox");
    check(!h.s.checked, "Checkbox");
    h.Click("toggle");
    check(h.s.toggle, "Switch");
    h.Click("mixed");
    check(h.s.mixed == imkit::CheckState::Checked, "Mixed to checked");
    h.Click("radio");
    check(h.s.radio == 1, "Radio");
    h.Click("same-first");
    h.Click("same-second");
    check(h.s.clicks == 12, "Independent same-label IDs");
    h.s.focusApply = true;
    h.Settle();
    check(h.s.applyFocused, "Programmatic focus");
    ImGui::SetNavCursorVisible(true);
    h.Key(ImGuiKey_Space);
    check(h.s.clicks == 13, "Keyboard activation");
    h.Key(ImGuiKey_Tab);
    check(!h.s.applyFocused, "Tab moves focus");
    h.Frame([](auto &io) { io.AddKeyEvent(ImGuiMod_Shift, true); });
    h.Key(ImGuiKey_Tab);
    h.Frame([](auto &io) { io.AddKeyEvent(ImGuiMod_Shift, false); });
    check(h.s.applyFocused, "Shift-Tab restores focus");
    h.Page(PageNumeric);
    auto slider = h.s.probes.at("slider");
    h.ClickAt({slider.min.x + 350, slider.Center().y});
    check(h.s.scalar > .7f, "Slider edit");
    h.Frame([](auto &io) { io.AddKeyEvent(ImGuiMod_Ctrl, true); });
    h.Click("drag");
    h.Frame([](auto &io) { io.AddKeyEvent(ImGuiMod_Ctrl, false); });
    h.ReplaceActive("0.375");
    h.Key(ImGuiKey_Enter);
    check(h.s.scalar == .375f, "Ctrl-click drag direct input");
    h.Replace("int64", "9007199254740995");
    h.Key(ImGuiKey_Enter);
    check(h.s.integer64 == 9007199254740995LL, "64-bit precision");
    auto range = h.s.probes.at("range");
    h.mouse = {range.min.x + 60, range.Center().y};
    h.Frame();
    h.Frame([](auto &io) { io.AddMouseButtonEvent(0, true); });
    h.mouse.x += 170;
    h.Frame();
    h.Frame([](auto &io) { io.AddMouseButtonEvent(0, false); });
    h.Settle();
    check(h.s.lower > 20 && h.s.lower <= h.s.upper, "Range edit and ordering");
    h.Page(PageInput);
    h.Replace("text", "Quality / 品質");
    check(std::strcmp(h.s.name, "Quality / 品質") == 0 && h.s.callbackCount > 0, "UTF-8 edit and callback");
    h.Replace("resize-text", "A growing UTF-8 buffer / 日本語の設定");
    check(h.s.growing.size() > 8 && std::strstr(h.s.growing.data(), "日本語") != nullptr,
          "Resize callback preserves UTF-8");
    h.Replace("validation", "Display");
    check(h.s.validation[0] != 0, "Validation field edit");
    h.Replace("multiline", "Two lines\n日本語");
    check(std::strstr(h.s.notes, "日本語") != nullptr, "Multiline UTF-8");
    h.Click("image-button");
    check(h.s.imageActivated, "Image button activation");
    const ImVec4 beforeColor = h.s.color;
    auto picker = h.s.probes.at("picker");
    h.ClickAt({picker.min.x + 25, picker.min.y + 35});
    check(h.s.color.x != beforeColor.x || h.s.color.y != beforeColor.y || h.s.color.z != beforeColor.z,
          "Color picker input");
    h.Page(PageHierarchy);
    h.Click("tree");
    check(h.s.treeOpen, "Tree expansion");
    h.Click("row-2");
    h.Click("edit-0");
    check(h.s.selectedRow == 2 && h.s.inlineActions == 1, "Table selection and independent inline action");
    h.Click("sort-quality");
    h.Click("sort-quality");
    check(h.s.rows[0] > h.s.rows[2], "Table descending sort");
    h.Click("tab-quality");
    check(h.s.tab == 1, "Tab switch");
    auto quality = h.s.probes.at("tab-quality");
    auto display = h.s.probes.at("tab-display");
    h.mouse = quality.Center();
    h.Frame();
    h.Frame([](auto &io) { io.AddMouseButtonEvent(0, true); });
    h.mouse.x = display.min.x + 3;
    h.Settle(3);
    h.Frame([](auto &io) { io.AddMouseButtonEvent(0, false); });
    h.Settle();
    check(h.s.probes.at("tab-quality").min.x < h.s.probes.at("tab-display").min.x, "Tab reorder");
    auto close = h.s.probes.at("tab-close");
    h.ClickAt({close.max.x - 17, close.Center().y});
    check(!h.s.tabOpen, "Tab close");
    h.mouse = h.s.probes.at("row-2").Center();
    h.Frame();
    h.Frame([](auto &io) { io.AddMouseWheelEvent(0, -4); });
    h.Settle();
    check(h.s.tableScroll > 0, "Table scrolling with frozen header and clipped rows");
    h.Page(PageOverlay);
    h.Click("popup");
    check(h.s.popupVisible, "Popup opens");
    h.Key(ImGuiKey_Escape);
    check(!h.s.popupVisible, "Popup Escape");
    h.Click("popup");
    const int clicksBeforeMenu = h.s.clicks;
    h.Click("popup-apply");
    check(h.s.clicks == clicksBeforeMenu + 1 && !h.s.popupVisible, "Menu selection closes popup");
    h.Click("modal");
    check(h.s.modalVisible, "Modal opens");
    h.Click("popup");
    check(h.s.modalVisible && !h.s.popupVisible, "Modal blocks background input");
    h.Click("modal-combo");
    h.Key(ImGuiKey_DownArrow);
    h.Key(ImGuiKey_Enter);
    check(h.s.modalVisible, "Nested combo retains modal");
    h.Key(ImGuiKey_Escape);
    h.Settle();
    check(!h.s.modalVisible, "Modal Escape");
    h.Click("modal");
    h.Click("cancel");
    check(!h.s.modalVisible, "Modal cancellation");
    check(h.s.modalLauncherFocused, "Modal restores launcher focus");
    h.Page(PageComposites);
    auto r = h.s.probes.at("segments");
    h.ClickAt({r.max.x - 20, r.Center().y});
    check(h.s.segment == 2, "Segmented selection");
    h.s.combo = 0;
    std::strcpy(h.s.search, "Medium");
    h.Click("search-combo");
    h.Key(ImGuiKey_Tab);
    h.Key(ImGuiKey_Enter);
    check(h.s.combo == 0, "Disabled search result does not select");
    h.Key(ImGuiKey_Escape);
    std::strcpy(h.s.search, "High");
    h.Click("search-combo");
    h.Key(ImGuiKey_Tab);
    h.Key(ImGuiKey_Enter);
    check(h.s.combo == 2, "Filtered search result selection");
    h.Key(ImGuiKey_Escape);
    log << "Input: public Dear ImGui IO events; not native OS/IME acceptance.\n";
}
void VerifyIcons(Host &h, const std::filesystem::path &out) {
    std::ofstream log(out / "icons-interaction.txt");
    auto check = [&](bool ok, const char *name) {
        log << (ok ? "PASS " : "FAIL ") << name << '\n';
        log.flush();
        if (!ok)
            throw std::runtime_error(name);
    };
    h.Page(PageIcons);
    h.Click("icon-button");
    check(h.s.iconClicks == 1, "Icon button mouse activation");
    h.Click("icon-label-button");
    check(h.s.iconClicks == 2, "Label button mouse activation");
    h.Click("icon-disabled");
    check(h.s.iconClicks == 2, "Disabled icon button rejects mouse");
    h.s.iconFocus = true;
    h.Settle();
    check(h.s.iconFocused, "Icon button receives keyboard focus");
    ImGui::SetNavCursorVisible(true);
    h.Key(ImGuiKey_Space);
    check(h.s.iconClicks == 3, "Icon button Space activation");
    h.Key(ImGuiKey_Tab);
    h.Key(ImGuiKey_Enter);
    check(h.s.iconClicks == 4, "Label button Tab and Enter activation");
    h.Replace("icon-search", "camera");
    check(std::strcmp(h.s.iconSearch, "camera") == 0, "Catalogue search input");
    h.Replace("icon-search", "");
    h.Key(ImGuiKey_Backspace); // Empty text input alone does not remove a selected range.
    check(h.s.iconSearch[0] == '\0', "Catalogue search clears");
    h.s.iconCustomColor = true;
    h.s.iconColor = {.8f, .2f, .3f, .7f};
    h.s.iconSizeIndex = 3;
    h.Settle();
    const auto rect = h.s.probes.at("icon-button");
    check(rect.max.x - rect.min.x >= 32, "32px icon button layout");
    h.Frame({}, out / "icons-custom-color-32.png");
    h.s.iconCustomColor = false;
    h.s.iconSizeIndex = 1;
    log << "Input: public Dear ImGui IO events; GPU backbuffer capture; not native OS automation.\n";
}

void VerifyEditors(Host &h, const std::filesystem::path &out, const imkit::preview::GLFunctions &functions) {
    std::ofstream log(out / "editors-interaction.txt");
    auto check = [&](bool ok, const char *message) {
        log << (ok ? "PASS " : "FAIL ") << message << '\n'; log.flush();
        if (!ok) throw std::runtime_error(message);
    };
    auto &s = h.s.editors;
    auto &renderer = s.previewRenderer;
    imkit::cg::Camera camera; camera.yaw = camera.pitch = 0;
    imkit::preview::Mesh meshes[2] = {
        {0x123456789abcdef0ULL, s.cubeVertices, s.cubeIndices},
        {23, s.cubeVertices, s.cubeIndices}
    };
    meshes[0].transform.translation.z = 1;
    meshes[1].transform.translation.z = -1;
    check(renderer.Render(meshes, camera), "GL indexed cube rendering");
    check(renderer.Pick(renderer.Width()/2, renderer.Height()/2) == meshes[0].id,
          "GL depth test and full 64-bit picking, near object submitted first");
    check(renderer.Pick(0, 0) == 0, "GL background picking");
    const auto resizeTexture=renderer.Texture();
    check(renderer.Resize(800, 450) && renderer.Render(meshes, camera), "GL FBO resize");
    check(renderer.Texture()==resizeTexture,"GL resize preserves texture borrowed by current draw commands");
    check(renderer.Pick(400, 225) == meshes[0].id, "GL picking after resize");
    check(!renderer.Resize(0, 1) && renderer.Initialized(), "GL invalid resize preserves renderer");
    auto oldTexture=renderer.Texture(); renderer.Shutdown();
    auto isTexture=reinterpret_cast<GLboolean (*)(GLuint)>(glfwGetProcAddress("glIsTexture"));
    check(!renderer.Initialized() && renderer.Texture()==0 && isTexture && !isTexture(oldTexture), "GL shutdown deletes texture resources");
    check(renderer.Init(functions,640,480),"GL reinitialization after shutdown");
    std::array<imkit::preview::Vertex,325> sphereVertices{};
    std::array<std::uint32_t,1728> sphereIndices{};
    check(imkit::preview::Sphere(sphereVertices,sphereIndices),"Sphere primitive generation");
    imkit::preview::Mesh sphere{99,sphereVertices,sphereIndices};
    check(renderer.Render({&sphere,1},camera) && renderer.Pick(320,240)==99,"GL sphere rendering and picking");
    imkit::preview::Vertex normalVertices[3] = {
        {{-1,-1,0},{1,1,1}}, {{1,-1,0},{1,1,1}}, {{0,1,0},{1,1,1}}};
    const std::uint32_t normalIndices[] = {0,1,2};
    imkit::preview::Mesh normalMesh{100, normalVertices, normalIndices};
    normalMesh.transform.scale = {2,1,.5};
    normalMesh.transform.rotation.z = 3.141592653589793 / 2;
    check(renderer.Render({&normalMesh,1},camera) && renderer.Pick(320,240)==100,
          "GL rotated nonuniform mesh picking");
    auto getTexImage = reinterpret_cast<void (*)(unsigned,int,unsigned,unsigned,void*)>(
        glfwGetProcAddress("glGetTexImage"));
    check(getTexImage != nullptr, "GL texture readback available");
    std::vector<unsigned char> normalPixels(640*480*4);
    functions.BindTexture(0x0DE1, renderer.Texture());
    getTexImage(0x0DE1,0,0x1908,0x1401,normalPixels.data());
    functions.BindTexture(0x0DE1,0);
    int expectedNormal = static_cast<int>(std::lround(255 * (.25 + .75 * 1.1 / std::sqrt(5.25 * .98))));
    check(std::abs(static_cast<int>(normalPixels[(240*640+320)*4]) - expectedNormal) <= 2,
          "GL inverse transpose normal under rotation and nonuniform scale");
    normalMesh.transform.shear={.5,0,0};
    check(renderer.Render({&normalMesh,1},camera) && renderer.Pick(320,240)==100,"GL sheared mesh preserves depth and picking");
    functions.BindTexture(0x0DE1,renderer.Texture());
    getTexImage(0x0DE1,0,0x1908,0x1401,normalPixels.data());
    const int expectedShear=static_cast<int>(std::lround(255*(.25+.75*1.175/std::sqrt(4.8125*.98))));
    check(std::abs(static_cast<int>(normalPixels[(240*640+320)*4])-expectedShear)<=2,
          "GL inverse transpose preserves sheared surface lighting");
    h.Page(PageVideo);
    int dragCapture=0;
    auto drag = [&](ImVec2 from, ImVec2 to) {
        h.mouse = from; h.Frame();
        h.Frame([](auto &io) { io.AddMouseButtonEvent(0, true); });
        h.mouse = to; h.Frame();
        h.Frame({},out/("drag-preview-"+std::to_string(dragCapture++)+".png"));
        if (h.s.page==PageCG && s.viewport.drag.active) {
            const auto pose=imkit::cg::PreviewTransform(s.objects[1],s.viewport);
            const auto center=imkit::cg::Project(pose.translation,s.viewport.camera,{0,0},s.viewportSize);
            check(center.visible && renderer.Pick(static_cast<int>(center.screen.x),static_cast<int>(center.screen.y))==s.objects[1].id,
                  "CG preview center shares visible GL mesh, outline and gizmo during drag");
        }
        h.Frame([](auto &io) { io.AddMouseButtonEvent(0, false); });
        h.Settle(2);
    };
    auto clipPosition = [&](imkit::editor::StableId id, float edge) {
        const auto clip = std::find_if(s.clips.begin(), s.clips.end(), [&](const auto &c) { return c.id == id; });
        const auto &view = s.timeline.view;
        return ImVec2{view.min.x + s.timeline.headerWidth + static_cast<float>(
            (imkit::editor::Seconds(clip->start) - s.timeline.canvas.origin.x) * s.timeline.canvas.scale.x) + edge,
            view.min.y + 20};
    };
    auto findClip = [&]() -> const imkit::video::ClipView & {
        return *std::find_if(s.clips.begin(), s.clips.end(), [](auto &c) { return c.id == 1000; });
    };
    auto before = findClip().start;
    h.Frame({},out/"timeline-before.png");
    auto pos = clipPosition(1000, 90); drag(pos, {pos.x + 40, pos.y});
    h.Frame({},out/"timeline-after.png");
    check(findClip().start > before, "Timeline move via public IO");
    before = findClip().duration;
    pos = clipPosition(1000, static_cast<float>(imkit::editor::Seconds(before)*s.timeline.canvas.scale.x)-3);
    drag(pos, {pos.x-25,pos.y});
    check(findClip().duration < before, "Timeline end trim via public IO");
    auto count = s.clips.size(); s.timeline.tool = imkit::video::Tool::Razor;
    h.ClickAt(clipPosition(1000, 100));
    check(s.clips.size() == count+1, "Timeline razor split via public IO");
    s.timeline.tool = imkit::video::Tool::Select;
    h.Page(PageCG);
    auto projected = imkit::cg::Project(s.objects[1].transform.translation, s.viewport.camera, s.viewportOrigin, s.viewportSize);
    auto end = imkit::cg::Project({s.objects[1].transform.translation.x+1,s.objects[1].transform.translation.y,s.objects[1].transform.translation.z},s.viewport.camera,s.viewportOrigin,s.viewportSize);
    float dx=end.screen.x-projected.screen.x,dy=end.screen.y-projected.screen.y,len=std::sqrt(dx*dx+dy*dy);
    pos={projected.screen.x+dx/len*70,projected.screen.y+dy/len*70};
    auto oldX=s.objects[1].transform.translation.x;
    drag(pos,{pos.x+40,pos.y});
    check(s.objects[1].transform.translation.x>oldX,"CG X gizmo preview and commit via public IO");
    check(s.objectSelection.active==s.objects[1].id,"CG shared selection stable ID");
    h.Frame({},out/"cg-edited.png");
    // Exercise the shared preview contract with actual GPU output and public input.
    const auto savedCamera=s.viewport.camera;
    const auto savedRotation=s.objects[1].transform.rotation;
    const auto nearVec=[](auto a,auto b) {return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)<1e-9;};
    for (auto projection:{imkit::cg::Projection::Perspective,imkit::cg::Projection::Orthographic})
        for (auto orientation:{imkit::cg::Orientation::World,imkit::cg::Orientation::Local})
            for (bool multiple:{false,true}) {
                s.viewport.camera.projection=projection;s.viewport.orientation=orientation;
                s.objects[1].transform.rotation={.2,.3,.4};
                s.objectSelection.Set(s.objects[1].id);
                if (multiple) {s.objectSelection.Set(s.objects[2].id,true);s.objectSelection.active=s.objects[1].id;}
                h.Settle(2);
                const auto original=s.objects[1].transform.translation,companion=s.objects[2].transform.translation;
                const auto historyCount=s.history.size();
                auto startGesture=[&] {
                    const auto pivot=s.SelectionPivot(s.viewport.pivot);
                    const auto basis=imkit::cg::OrientationBasis(orientation,s.objects[1].transform,s.viewport.camera);
                    const auto a=imkit::cg::Project(pivot,s.viewport.camera,s.viewportOrigin,s.viewportSize).screen;
                    const auto b=imkit::cg::Project({pivot.x+basis.x.x,pivot.y+basis.x.y,pivot.z+basis.x.z},s.viewport.camera,s.viewportOrigin,s.viewportSize).screen;
                    const float length=std::hypot(b.x-a.x,b.y-a.y),ux=(b.x-a.x)/length,uy=(b.y-a.y)/length;
                    h.mouse={a.x+ux*70,a.y+uy*70};h.Frame();
                    h.Frame([](auto &io){io.AddMouseButtonEvent(0,true);});
                    h.mouse={a.x+ux*100,a.y+uy*100};h.Frame();
                    check(s.viewport.drag.active,"CG matrix gesture begins on arrow");
                    const auto proposed=imkit::cg::PreviewTransform(s.objects[1],s.viewport).translation;
                    h.Frame();
                    check(nearVec(proposed,imkit::cg::PreviewTransform(s.objects[1],s.viewport).translation),
                          "CG stationary pointer does not accumulate preview movement");
                    const auto center=imkit::cg::Project(proposed,s.viewport.camera,{0,0},s.viewportSize);
                    check(renderer.Pick(static_cast<int>(center.screen.x),static_cast<int>(center.screen.y))==s.objects[1].id,
                          "CG matrix GPU mesh follows projected preview");
                    return proposed;
                };
                startGesture();h.Key(ImGuiKey_Escape);
                h.Frame([](auto &io){io.AddMouseButtonEvent(0,false);});
                check(nearVec(s.objects[1].transform.translation,original) && nearVec(s.objects[2].transform.translation,companion) && s.history.size()==historyCount,
                      "CG matrix Escape restores all targets without history");
                const auto proposed=startGesture();
                h.Frame([](auto &io){io.AddMouseButtonEvent(0,false);});
                check(nearVec(s.objects[1].transform.translation,proposed),"CG matrix release commits exact preview without jump");
                if (multiple) {
                    const auto moved=s.objects[2].transform.translation;
                    check(nearVec(imkit::cg::Vec3{moved.x-companion.x,moved.y-companion.y,moved.z-companion.z},
                                  imkit::cg::Vec3{proposed.x-original.x,proposed.y-original.y,proposed.z-original.z}),
                          "CG matrix companion shares translation delta");
                }
                check(s.Undo() && nearVec(s.objects[1].transform.translation,original) && nearVec(s.objects[2].transform.translation,companion),
                      "CG matrix Undo restores complete gesture");
            }
    s.viewport.camera=savedCamera;s.viewport.orientation=imkit::cg::Orientation::World;
    s.objects[1].transform.rotation=savedRotation;s.objectSelection.Set(s.objects[1].id);
    h.Page(PageEditorCore);
    auto &key=s.keys[4]; const auto keyId=key.id; auto keyTick=key.tick;
    auto keyScreen=imkit::editor::ToScreen({imkit::editor::Seconds(key.tick),-key.value},s.curve.canvas,{s.curve.view.min.x,s.curve.view.min.y});
    drag({static_cast<float>(keyScreen.x),static_cast<float>(keyScreen.y)}, {static_cast<float>(keyScreen.x+20),static_cast<float>(keyScreen.y+10)});
    auto editedKey=std::find_if(s.keys.begin(),s.keys.end(),[&](const auto &k){return k.id==keyId;});
    check(editedKey->tick>keyTick,"Curve key move via public IO");
    s.animationPage=2; h.Page(PageCG); s.animationPage=-1; h.Settle();
    auto vertex=s.uv[0].uv;auto uvScreen=imkit::editor::ToScreen(vertex,s.uvState.canvas,{s.uvState.view.min.x,s.uvState.view.min.y});
    drag({static_cast<float>(uvScreen.x),static_cast<float>(uvScreen.y)},{static_cast<float>(uvScreen.x+30),static_cast<float>(uvScreen.y+15)});
    check(s.uv[0].uv.x>vertex.x&&s.uv[0].uv.y>vertex.y,"UV vertex transform via public IO");
    h.Frame({},out/"uv-edited.png");
    s.Dataset(true); h.Page(PageVideo); h.Settle(20);
    std::array<double,180> timings{};
    std::size_t maxQueries=0,maxClips=0;
    h.imguiAllocations=0; h.countImGuiAllocations=true;
    imkit::gallery::CountAllocations(true);
    for (auto &elapsed:timings) {
        s.timeline.canvas.origin.x+=.01;
        const auto start=std::chrono::steady_clock::now(); h.Frame();
        elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        maxQueries=(std::max)(maxQueries,s.queryCount); maxClips=(std::max)(maxClips,s.queriedClips);
    }
    imkit::gallery::CountAllocations(false);h.countImGuiAllocations=false;
    const auto cppAllocations=imkit::gallery::AllocationCount();
    std::sort(timings.begin(),timings.end());
    std::ofstream perf(out/"performance.txt");
    perf << "tracks="<<s.tracks.size()<<" clips="<<s.clips.size()<<" keys="<<s.keys.size()
         <<"\nframe_cpu_wall_p95_ms="<<timings[170]<<"\nvisible_queries_max="<<maxQueries
         <<"\nvisible_clips_max="<<maxClips<<"\ncpp_new_allocations="<<cppAllocations<<"\nimgui_allocations="<<h.imguiAllocations<<"\n180 frames, pan interaction, 1920x1440, includes GL submission and swap.\n";
    check(maxClips<1000 && maxQueries<30,"Large dataset only queries visible clips/tracks");
    h.Frame({},out/"video-large.png");
    log<<"Public IO and actual GPU; native OS/IME and media decode not tested.\n";
}

void BenchmarkEditors(Host &h,const std::filesystem::path &out) {
    using namespace imkit;
    auto &s=h.s.editors;
    std::ofstream report(out/"editor-performance.csv");
    report<<"operation,frames,p95_ms,max_ms,queries_max,clips_max,keys_max,tracks_max,cpp_new,imgui_allocations,interaction_verified\n";
    std::ofstream context(out/"editor-performance-context.txt");
#ifdef NDEBUG
    context<<"configuration=Release\n";
#else
    context<<"configuration=Debug (not Release acceptance)\n";
#endif
    int width=0,height=0;glfwGetFramebufferSize(h.window,&width,&height);
    context<<"framebuffer="<<width<<'x'<<height<<"\n20 warm-up frames, 180 measured frames per operation.\n"
        <<"Boundary: Host::Frame wall time including host event apply, preview render, ImGui, GL submission and swap; vsync off.\n"
        <<"Returned keys include clip-local editing spans and Curve query neighbors, counted per return; full borrowed evaluation channels are excluded. Tracks count returned Timeline rows.\n"
        <<"Allocations: C++ new and ImGui allocator calls; excludes driver/internal OS allocation.\n"
        <<"Public ImGui IO, hidden native GL window; not native OS/IME input.\n";
    const char *names[]={"pan","zoom","selection","clip_drag","clip_trim","keyframe_drag"};
    bool allPassed=width==1920 && height==1440;
    for (int operation=0;operation<6;++operation) {
        s.Dataset(true);s.selection.Clear();s.timeline={};s.timeline.snapping=false;
        h.Page(PageVideo);h.Settle(20);
        const auto &clip=s.clips.front();const auto clipId=clip.id;
        const auto startTick=clip.start,originalDuration=clip.duration;
        const float left=s.timeline.view.min.x+s.timeline.headerWidth;
        ImVec2 start{left+100,s.timeline.view.min.y+25};
        if (operation==4) start.x=left+float(editor::Seconds(clip.duration)*s.timeline.canvas.scale.x)-3;
        if (operation==5) {
            if (clip.keys.empty()) throw std::runtime_error("benchmark requires inline clip keys");
            start.x=left+float(editor::Seconds(clip.keys[clip.keys.size()/2].tick)*s.timeline.canvas.scale.x);
            start.y=s.timeline.view.min.y+video::TrackExtent(s.tracks.front())-16;
        }
        s.timeline.tool=operation==0 ? video::Tool::Hand : video::Tool::Select;
        h.mouse=start;h.Frame();
        if (operation!=1 && operation!=2) h.Frame([](auto &io){io.AddMouseButtonEvent(0,true);});
        const auto editedKey=s.timeline.keyDrag.draft.target;
        const auto oldKeyTick=s.timeline.keyDrag.draft.original.first;
        bool gestureValid=operation<3 || (operation==5 ? s.timeline.keyDrag.active : s.timeline.drag.active);
        if (operation==4) gestureValid &= s.timeline.drag.draft.kind==editor::EditKind::TrimEnd;
        const auto origin=s.timeline.canvas.origin.x,scale=s.timeline.canvas.scale.x;
        auto frame=[&](int index) {
            if (operation==1) {
                h.Frame([&](auto &io){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddMouseWheelEvent(0,index%2 ? -.04f : .05f);});
            } else if (operation==2) {
                h.mouse={left+(index/2%2 ? 500.f : 100.f),start.y};
                h.Frame([&](auto &io){io.AddMouseButtonEvent(0,index%2==0);});
            } else {
                const float distance=operation==4 ? -20.f : 20.f;
                h.mouse={start.x+distance+float(index%20)*.3f,start.y};h.Frame();
            }
        };
        for (int i=0;i<20;++i) frame(i);
        std::array<double,180> timings{};std::size_t queries=0,clips=0,keys=0,tracks=0;
        h.imguiAllocations=0;h.countImGuiAllocations=true;gallery::CountAllocations(true);
        for (int i=0;i<180;++i) {
            const auto begin=std::chrono::steady_clock::now();frame(i);
            timings[i]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
            queries=std::max(queries,s.queryCount);clips=std::max(clips,s.queriedClips);
            keys=std::max(keys,s.queriedKeys);tracks=std::max(tracks,s.queriedTracks);
        }
        gallery::CountAllocations(false);h.countImGuiAllocations=false;
        const auto allocations=gallery::AllocationCount(),imguiAllocations=h.imguiAllocations;
        // Commit is outside steady drag sampling; report its cost separately.
        const auto terminalBegin=std::chrono::steady_clock::now();
        h.Frame([](auto &io){io.AddMouseButtonEvent(0,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);});
        const auto terminalMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-terminalBegin).count();
        const auto found=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==clipId;});
        if (operation==0) gestureValid &= s.timeline.canvas.origin.x!=origin;
        if (operation==1) gestureValid &= s.timeline.canvas.scale.x!=scale;
        if (operation==2) gestureValid &= s.selection.count==1 && s.selection.active==s.clips[1].id;
        if (operation==3) gestureValid &= found!=s.clips.end() && found->start!=startTick;
        if (operation==4) gestureValid &= found!=s.clips.end() && found->duration!=originalDuration;
        if (operation==5) gestureValid &= std::any_of(s.keys.begin(),s.keys.end(),[&](const auto &key){return key.id==editedKey && key.tick!=oldKeyTick;});
        std::sort(timings.begin(),timings.end());
        report<<names[operation]<<",180,"<<timings[170]<<','<<timings.back()<<','<<queries<<','<<clips<<','
            <<keys<<','<<tracks<<','<<allocations<<','<<imguiAllocations<<','<<gestureValid<<'\n';report.flush();
        context<<names[operation]<<" terminal_frame_ms="<<terminalMs<<" tracks="<<s.tracks.size()<<" clips="<<s.clips.size()<<" keys="<<s.keys.size()<<'\n';context.flush();
        allPassed &= gestureValid && timings[170]<=16.7 && queries<30 && clips<1000 && keys<1000 && tracks<30 && allocations==0 && imguiAllocations==0;
    }
    if (!allPassed) throw std::runtime_error("Editor benchmark did not meet interaction, size, query or P95 gates; see report");
}

void VerifyLinkedClips(Host &h,const std::filesystem::path &out) {
    using namespace imkit;
    auto &s=h.s.editors;s.Dataset(false);h.Page(PageVideo);h.Settle();
    s.timeline.canvas.origin.x=7;s.timeline.canvas.scale.x=100;s.timeline.snapping=false;
    h.Settle();
    const auto target=s.clips[2].id;
    const auto members=s.QuerySelectedClips(std::span<const editor::StableId>(&target,1));
    if (members.size()!=4) throw std::runtime_error("linked sample requires four related clips");
    std::array<video::ClipView,4> before;std::copy(members.begin(),members.end(),before.begin());
    h.mouse={s.timeline.view.min.x+s.timeline.headerWidth+150,s.timeline.view.min.y+25};h.Frame();
    h.Frame([](auto &io){io.AddMouseButtonEvent(0,true);});
    h.mouse.x+=50;h.Frame();h.Frame([](auto &io){io.AddMouseButtonEvent(0,false);});h.Settle(2);
    std::ofstream log(out/"linked-clips.txt");
    for (const auto &original:before) {
        const auto clip=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==original.id;});
        const bool ok=clip!=s.clips.end() && clip->start==original.start+editor::FromSeconds(.5);
        log<<(ok ? "PASS " : "FAIL ")<<"related clip moved "<<original.id<<'\n';log.flush();
        if (!ok) throw std::runtime_error("linked native move did not apply to all members");
    }
    h.mouse={-100,-100};h.Frame({},out/"linked-clips-light.png");
    h.mouse={s.timeline.view.min.x+s.timeline.headerWidth+200,s.timeline.view.min.y+25};h.Frame();
    h.Frame([](auto &io){io.AddMouseButtonEvent(1,true);});h.Frame([](auto &io){io.AddMouseButtonEvent(1,false);});
    h.Key(ImGuiKey_Home);h.Key(ImGuiKey_Enter);h.Settle(2);
    const auto unlinked=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==target;});
    const bool detached=unlinked!=s.clips.end() && !unlinked->linked;
    log<<(detached ? "PASS " : "FAIL ")<<"native context menu unlinks clicked clip\n";log.flush();
    if (!detached) throw std::runtime_error("native unlink did not update host relationship");
    s.japanese=true;h.Frame();
    h.mouse.y+=video::TrackExtent(s.tracks.front());h.Frame();
    h.Frame([](auto &io){io.AddMouseButtonEvent(1,true);});h.Frame([](auto &io){io.AddMouseButtonEvent(1,false);});
    h.Key(ImGuiKey_Home);h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_Enter);h.Settle(2);
    const auto audio=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==before[1].id;});
    const bool ungrouped=audio!=s.clips.end() && !audio->group && audio->linked;
    log<<(ungrouped ? "PASS " : "FAIL ")<<"Japanese native context detaches group while preserving link\n";log.flush();
    if (!ungrouped) throw std::runtime_error("native ungroup did not preserve independent link relation");
    s.selection.Set(target);s.selection.Set(before[1].id,true);
    for (int relation=0;relation<2;++relation) {
        h.Frame([](auto &io){io.AddMouseButtonEvent(1,true);});h.Frame([](auto &io){io.AddMouseButtonEvent(1,false);});
        h.Key(ImGuiKey_Home);for (int i=0;i<3;++i) h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_Enter);h.Settle(2);
        const auto first=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==target;});
        const auto second=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==before[1].id;});
        const auto set=relation ? first->linked : first->group;
        const bool joined=set && set==(relation ? second->linked : second->group);
        log<<(joined ? "PASS " : "FAIL ")<<(relation ? "native selected link creation" : "native selected group creation")<<'\n';log.flush();
        if (!joined) throw std::runtime_error("native relationship creation failed");
    }
    h.Frame([](auto &io){io.AddMouseButtonEvent(1,true);});h.Frame([](auto &io){io.AddMouseButtonEvent(1,false);});
    h.Frame({},out/"clip-relations-menu-japanese.png");h.Key(ImGuiKey_Escape);
}
void VerifyTimelineUI(Host &h,const std::filesystem::path &out) {
    using namespace imkit;
    auto &s=h.s.editors;s.japanese=false;s.tracks={{710,"Video A"},{711,"Video B"},{712,"Video C"}};
    s.clips.clear();s.selection.Clear();s.trackSelection.Clear();s.timeline.canvas.origin={0,0};s.timeline.canvas.scale={120,1};
    for(int i=0;i<2;++i) {video::ClipView c;c.id=810+i;c.track=710;c.label="Editable clip";c.start=editor::FromSeconds(2*i);c.duration=editor::FromSeconds(2);c.sourceIn=editor::FromSeconds(10);s.clips.push_back(c);}
    s.selection.Set(810);s.RebuildTrackLayout();s.RebuildClipIndex();s.RebuildKeyIndex();h.Page(PageVideo);
    std::ofstream log(out/"timeline-ui.txt");
    const auto require=[&](bool ok,const char *message){log<<(ok ? "PASS " : "FAIL ")<<message<<'\n';log.flush();if(!ok) throw std::runtime_error(message);};
    const auto origin=s.timeline.view.min;
    h.mouse={origin.x+s.timeline.headerWidth+5,origin.y+11};h.Settle();
    h.Frame([](ImGuiIO &io){io.AddMouseButtonEvent(0,true);});
    require(s.timeline.fadeDrag.active,"native Gallery captures zero-length fade handle");
    h.mouse.x+=60;h.Frame();
    require(s.FindClip(810)->fades.inDuration==0 && s.timeline.fadeDrag.draft.proposed.first>0,"native fade drag previews without host mutation");
    h.Frame([](ImGuiIO &io){io.AddMouseButtonEvent(0,false);});h.Settle();
    require(s.FindClip(810)->fades.inDuration>0 && s.FindClip(810)->duration==editor::FromSeconds(2),"native fade release updates host without trimming clip");
    h.mouse={-100,-100};h.Frame({},out/"timeline-fade.png");
    require(s.Undo() && s.FindClip(810)->fades.inDuration==0,"native gesture has one undo");h.Settle();
    h.ClickAt({origin.x+55,origin.y+9});
    h.Frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiMod_Ctrl,true);});
    h.ClickAt({origin.x+55,origin.y+75});h.Frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiMod_Ctrl,false);});
    require(s.trackSelection.count==2,"native track multi-selection is independent");
    h.mouse={origin.x+55,origin.y+9};h.Settle();h.Frame([](ImGuiIO &io){io.AddMouseButtonEvent(0,true);});
    h.mouse={origin.x+55,origin.y+static_cast<float>(s.trackOffsets[2])+15};h.Frame();h.Frame();h.Frame([](ImGuiIO &io){io.AddMouseButtonEvent(0,false);});h.Settle();
    require(s.tracks[0].id==712 && s.tracks[1].id==710 && s.tracks[2].id==711,"native multi-track drag appends and preserves order");
    h.mouse={-100,-100};h.Frame({},out/"timeline-track-reorder.png");
    s.Dataset(true);h.Settle();
    require(s.clips.size()>=100000 && s.queriedClips<1000 && s.queriedTracks<64,"100k fixture queries only visible timeline rows and clips");
    log<<"Public ImGui IO with native OpenGL backbuffer; native OS/IME input is not tested.\n";
}
void VerifyComparison(Host &h,const std::filesystem::path &out) {
    auto require=[](bool ok,const char *message) { if(!ok) throw std::runtime_error(message); };
    h.s.page=PageCompare;h.s.comparison.open=true;h.Settle();
    for(int mode=0;mode<2;++mode) {
        h.s.comparison.mode=mode;h.Settle();
        const ImGuiStyle before=ImGui::GetStyle();h.Frame();
        require(std::memcmp(&before,&ImGui::GetStyle(),sizeof(ImGuiStyle))==0,"comparison style restoration failed");
        const int applied=h.s.comparison.applyCount;
        h.Click("comparison-default-apply");h.Click("comparison-imkit-apply");
        require(h.s.comparison.applyCount==applied+2,"both actions must update shared state");
        const bool enabled=h.s.comparison.enabled;
        h.Click("comparison-default-enabled");h.Click("comparison-imkit-enabled");
        require(h.s.comparison.enabled==enabled,"both columns must edit the same value");
        h.Replace("comparison-default-name","Shared from left");
        require(std::string(h.s.comparison.name)=="Shared from left","left edit failed");
        h.Replace("comparison-imkit-name","Shared from right");
        require(std::string(h.s.comparison.name)=="Shared from right","right edit failed");
        h.Click("comparison-default-apply");h.s.comparison.focusDefault=true;h.Settle();ImGui::SetNavCursorVisible(true);h.Key(ImGuiKey_Space);
        require(h.s.comparison.applyCount==applied+4,"keyboard activation failed");
        h.s.comparison.disabled=true;h.Settle();
        const int disabledCount=h.s.comparison.applyCount;
        h.Click("comparison-default-apply");h.Click("comparison-imkit-apply");
        require(h.s.comparison.applyCount==disabledCount,"disabled controls changed state");
        h.s.comparison.disabled=false;
    }
    h.s.comparison.open=false;h.Page(PageStart);
    require(!h.s.probes.contains("comparison-default-apply"),"comparison leaked into Start");
    h.s.comparison.open=true;h.Page(PageCompare);
    h.Frame({},out/"comparison-native.png");
    h.s.comparison.open=false;h.Page(PageWorkbench);
    const bool setting=h.s.workflow.settingEnabled;h.Click("workbench-enabled");
    require(h.s.workflow.settingEnabled!=setting,"workbench setting request was not applied");
    h.Click("workbench-row-1");
    require(h.s.workflow.selected==2,"workbench selection request was not applied");
    h.Frame({},out/"workbench-native.png");
    std::ofstream log(out/"comparison-verification.txt");
    log<<"PASS: Theme only and Components; shared left/right edits, keyboard activation, disabled controls, complete style restoration, isolated pages, host-applied workbench requests.\nPublic ImGui IO and native GL backbuffer; native OS/IME input not tested.\n";
}
void VerifyTrackControls(Host &h,const std::filesystem::path &out) {
    using namespace imkit;
    // 420 px: seven icon buttons stay inline only while the header is wider than their padded width at 125% scale.
    auto &s=h.s.editors;s.japanese=false;s.timeline.headerWidth=420;
    s.tracks.front().label="Picture / a long production track name / 映像トラックの長い名前";h.Page(PageVideo);
    std::ofstream log(out/"track-controls.txt");
    auto require=[&](bool ok,const char *label) {
        log<<(ok ? "PASS " : "FAIL ")<<label<<'\n';log.flush();
        if (!ok) throw std::runtime_error(label);
    };
    h.ClickAt({s.timeline.view.min.x+12,s.timeline.view.min.y+40});
    require(!s.tracks.front().visible,"native track visibility icon changes host state");
    h.mouse={-100,-100};h.Frame({},out/"track-icons-light.png");
    h.ClickAt({s.timeline.view.min.x+12,s.timeline.view.min.y+40});
    require(s.tracks.front().visible,"native visibility icon restores host state");
    s.japanese=true;s.timeline.headerWidth=150;h.s.dark=true;
    h.s.theme=MakePrecisionTheme(ColorScheme::Dark);h.s.scale=1.5f;h.Settle();
    h.ClickAt({s.timeline.view.min.x+15,s.timeline.view.min.y+38});
    h.mouse={-100,-100};h.Frame({},out/"track-menu-japanese-dark-150.png");
    h.Key(ImGuiKey_Home);h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_Enter);
    require(s.tracks.front().mute,"native Japanese icon menu applies mute to host track");
    h.ClickAt({s.timeline.view.min.x+15,s.timeline.view.min.y+38});h.mouse={-100,-100};h.Frame();
    h.Key(ImGuiKey_Home);h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_Enter);
    require(s.tracks.front().solo,"native Solo glyph menu applies solo to host track");
    for (int field:{5,6}) {
        h.ClickAt({s.timeline.view.min.x+15,s.timeline.view.min.y+38});h.mouse={-100,-100};h.Frame();
        h.Key(ImGuiKey_Home);for (int i=0;i<field;++i) h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_Enter);
        require(field==5 ? !s.tracks.front().target : s.tracks.front().source,
                field==5 ? "native Target menu applies track target" : "native SourcePatch menu applies source patch");
    }
    log<<"Public ImGui IO and native GL backbuffer; native OS/IME input not tested.\n";
}

void VerifyMonitors(Host &h,const std::filesystem::path &out) {
    using namespace imkit;
    auto &s=h.s.editors;h.Page(PageVideo);
    s.timeline.time.playhead=0;s.markers[0]={s.nextId++,0,"Opening cue / 開始の合図"};s.markerCount=1;
    std::ofstream log(out/"monitor-interaction.txt");
    const auto timelineScale=s.timeline.canvas.scale.x,timelineOrigin=s.timeline.canvas.origin.x;
    for (int preset:{0,2,1}) {
        h.mouse={(s.programMonitorMin.x+s.programMonitorMax.x)*.5f,
                 (s.programMonitorMin.y+s.programMonitorMax.y)*.5f};h.Frame();
        h.Frame([](auto &io){io.AddMouseButtonEvent(1,true);});
        h.Frame([](auto &io){io.AddMouseButtonEvent(1,false);});
        h.mouse={-100,-100};h.Frame();
        h.ClickAt({(s.programMonitorMetadataMin.x+s.programMonitorMetadataMax.x)*.5f,
                   (s.programMonitorMetadataMin.y+s.programMonitorMetadataMax.y)*.5f});
        h.Key(ImGuiKey_Home);
        for (int i=0;i<preset;++i) h.Key(ImGuiKey_DownArrow);
        h.Key(ImGuiKey_Enter);
        const bool ok=static_cast<int>(s.monitorMetadata)==preset;
        log<<(ok ? "PASS " : "FAIL ")<<"Program Monitor context preset "<<preset<<'\n';log.flush();
        if (!ok) throw std::runtime_error("Monitor context preset public IO failed");
        const bool unchanged=s.timeline.canvas.scale.x==timelineScale && s.timeline.canvas.origin.x==timelineOrigin;
        log<<(unchanged ? "PASS " : "FAIL ")<<"popup navigation does not invoke Timeline shortcuts\n";log.flush();
        if (!unchanged) throw std::runtime_error("Popup input leaked into editor commands");
    }
    s.monitorMetadata=video::MonitorMetadataPreset::Details;
    h.Frame({},out/"monitor-details-light.png");
    h.s.dark=true;h.s.theme=MakePrecisionTheme(ColorScheme::Dark);h.s.scale=1.5f;h.Settle();
    h.Frame({},out/"monitor-details-dark-150.png");
    s.events.Push({s.clipPropertyIds[1],s.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,1.2}});
    s.events.Push({s.clipPropertyIds[2],s.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,.1}});
    s.ApplyEvents();h.Frame({},out/"monitor-transform-host.png");
    const bool transformed=s.clipPropertyValues[1]==1.2 && s.clipPropertyValues[2]==.1;
    log<<(transformed ? "PASS " : "FAIL ")<<"host Inspector events change Monitor bounds parameters\n";
    if (!transformed) throw std::runtime_error("Monitor bounds host apply failed");
    s.monitorMode=3;
    const char *aspectNames[]={"4x3","16x9","1x1","9x16"};
    const char *stateNames[]={"ready","loading","empty","offline","error"};
    for(int aspect=0;aspect<4;++aspect) {
        s.monitorContractAspect=aspect;
        for(int state=0;state<5;++state) {
            s.monitorContractStatus=state;h.Settle(2);
            h.Frame({},out/(std::string("monitor-contract-")+aspectNames[aspect]+"-"+stateNames[state]+".png"));
            log<<"PASS Preview Monitor "<<aspectNames[aspect]<<' '<<stateNames[state]<<" Fit/Fill/Stretch capture\n";
        }
    }
    h.Page(PageCG);
    const auto savedCamera=s.viewport.camera;
    s.viewport.camera.yaw=s.viewport.camera.pitch=s.viewport.camera.roll=0;s.viewport.camera.target={};
    for(int aspect=0;aspect<4;++aspect) {
        s.cgPreviewAspect=aspect;h.Settle(2);
        h.Frame({},out/(std::string("cg-aspect-")+aspectNames[aspect]+".png"));
        const auto center=cg::Project({},s.viewport.camera,s.viewportOrigin,s.viewportSize);
        const auto x=cg::Project({1,0,0},s.viewport.camera,s.viewportOrigin,s.viewportSize);
        const auto y=cg::Project({0,1,0},s.viewport.camera,s.viewportOrigin,s.viewportSize);
        const bool uniform=center.visible&&x.visible&&y.visible&&
            std::abs(std::hypot(x.screen.x-center.screen.x,x.screen.y-center.screen.y)-
                     std::hypot(y.screen.x-center.screen.x,y.screen.y-center.screen.y))<1e-3;
        log<<(uniform?"PASS ":"FAIL ")<<"CG viewport "<<aspectNames[aspect]<<" uniform projected units\n";log.flush();
        if(!uniform) throw std::runtime_error("CG viewport aspect verification failed");
    }
    s.viewport.camera=savedCamera;
    log<<"Public ImGui IO and native GL capture; native OS/IME input not tested.\n";
}

void VerifyColor(Host &h, const std::filesystem::path &out) {
    auto &s=h.s.editors;
    s.videoPanel=1; h.Page(PageVideo); h.Settle(4);
    std::ofstream log(out/"color-interaction.txt");
    for (int wheel=0; wheel<3; ++wheel) {
        auto center=s.colorState.wheelCenters[wheel];
        auto before=s.commits;
        h.mouse=center; h.Frame();
        h.Frame([](auto &io){io.AddMouseButtonEvent(0,true);});
        h.mouse={center.x+s.colorState.wheelRadius*.5f,center.y}; h.Frame();
        h.Frame([](auto &io){io.AddMouseButtonEvent(0,false);});
        h.Settle(2);
        const float *rgb=wheel==0 ? s.colors.lift : wheel==1 ? s.colors.gamma : s.colors.gain;
        if (s.commits<=before || rgb[0]<=rgb[1])
            throw std::runtime_error("Color wheel host application failed");
        log<<"PASS wheel "<<wheel<<" public IO RGB host commit\n";
    }
    for (int dark=0;dark<2;++dark) {
        h.s.dark=dark!=0;
        h.s.theme=imkit::MakePrecisionTheme(dark ? imkit::ColorScheme::Dark : imkit::ColorScheme::Light);
        h.Settle(2);
        h.Frame({},out/(dark ? "color-dark.png" : "color-light.png"));
    }
}
void VerifyWindowFrame(Host& h,const std::filesystem::path& out) {
    auto require=[](bool ok,const char* message) { if(!ok) throw std::runtime_error(message); };
    require(h.windowFrame.Attached(),"Modern frame did not attach");
    h.Settle(2);
    auto* font=h.s.fonts.regular;
    require(font!=nullptr,"Frame font missing");
    for(float scale:{1.f,1.5f,2.f}) {
        imkit::WindowFrameState state;state.dpiScale=scale;
        const auto layout=imkit::LayoutWindowFrame(640*scale,h.s.frameStyle,state);
        require(layout.titleBar.max.y==h.s.frameStyle.metrics.height*scale,"Frame height must follow DPI");
        require(layout.minimize.min.x>layout.icon.max.x,"Frame regions overlap");
        require(layout.minimize.max.x==layout.maximizeRestore.min.x && layout.maximizeRestore.max.x==layout.close.min.x,"Button gap");
        const auto title=imkit::ElideWindowFrameTitle("ModernKIT / 長い日本語タイトル / a very long project title",100*scale,font,13*scale);
        require(title.ends_with("..."),"Long title must be elided");
        require(font->CalcTextSizeA(13*scale,FLT_MAX,0,title.c_str()).x<=100*scale,"Title escaped available width");
        require(imkit::ElideWindowFrameTitle("Title",0,font,13*scale).empty(),"No space must produce no title");
    }
    HWND window=glfwGetWin32Window(h.window);
    auto hit=[&](int x,int y) {
        POINT p{x,y}; ClientToScreen(window,&p);
        return SendMessageW(window,WM_NCHITTEST,0,MAKELPARAM(p.x,p.y));
    };
    RECT layoutClient{};GetClientRect(window,&layoutClient);
    auto frameState=h.windowFrame.State();
    auto layout=imkit::LayoutWindowFrame(static_cast<float>(layoutClient.right),h.s.frameStyle,frameState);
    h.windowFrame.SetLayout(layout);
    const int middleX=layoutClient.right/2, middleY=layoutClient.bottom/2;
    require(hit(middleX,0)==HTTOP,"Top edge must resize");
    require(hit(0,0)==HTTOPLEFT && hit(layoutClient.right-1,0)==HTTOPRIGHT,
            "Top corners must resize");
    require(hit(0,middleY)==HTLEFT && hit(layoutClient.right-1,middleY)==HTRIGHT,
            "Side edges must resize");
    require(hit(middleX,layoutClient.bottom-1)==HTBOTTOM,"Bottom edge must resize");
    require(hit(0,layoutClient.bottom-1)==HTBOTTOMLEFT &&
            hit(layoutClient.right-1,layoutClient.bottom-1)==HTBOTTOMRIGHT,
            "Bottom corners must resize");
    require(hit(100,int(layout.titleBar.max.y/2))==HTCAPTION,"Drag region hit test");
    const std::array<imkit::WindowFrameRect,3> buttons{layout.minimize,layout.maximizeRestore,layout.close};
    for(int i=0;i<3;++i) {
        const auto& r=buttons[i];
        require(hit(int((r.min.x+r.max.x)/2),int((r.min.y+r.max.y)/2))==
                (i==0?HTMINBUTTON:i==1?HTMAXBUTTON:HTCLOSE),"Caption button hit test");
    }
    require(hit(100,int(layout.titleBar.max.y)+30)==HTCLIENT,"Content must receive client input");
    RECT windowRect{};GetWindowRect(window,&windowRect);
    SIZE dpiScaledSize{windowRect.right-windowRect.left,windowRect.bottom-windowRect.top};
    const SIZE originalSize=dpiScaledSize;
    SendMessageW(window,WM_GETDPISCALEDSIZE,GetDpiForWindow(window)+48,
                 reinterpret_cast<LPARAM>(&dpiScaledSize));
    require(dpiScaledSize.cx==originalSize.cx && dpiScaledSize.cy==originalSize.cy,
            "Custom frame must not add native border on DPI change");
    h.windowFrame.Execute(imkit::WindowFrameOperation::MaximizeRestore); h.Settle(2);
    require(IsZoomed(window),"Maximize failed");
    require(hit(100,int(layout.titleBar.max.y/2))==HTCAPTION && hit(middleX,0)!=HTTOP,
            "Maximized frame must not resize");
    RECT client{};GetClientRect(window,&client);
    POINT origin{};ClientToScreen(window,&origin);
    MONITORINFO monitor{sizeof(MONITORINFO)};
    GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);
    require(origin.x>=monitor.rcWork.left && origin.y>=monitor.rcWork.top &&
            origin.x+client.right<=monitor.rcWork.right && origin.y+client.bottom<=monitor.rcWork.bottom,
            "Maximized content exceeds monitor work area");
    h.windowFrame.Execute(imkit::WindowFrameOperation::MaximizeRestore);h.Settle(2);
    require(!IsZoomed(window),"Restore failed");
    h.windowFrame.Execute(imkit::WindowFrameOperation::Minimize);
    require(IsIconic(window),"Minimize failed");
    SendMessageW(window,WM_SYSCOMMAND,SC_RESTORE,0);h.Settle(2);
    for(const auto& preset:imkit::ThemePresets()) {
        h.s.theme=imkit::MakeTheme(preset.preset);
        h.s.frameStyle=imkit::MakeWindowFrameStyle(h.s.framePreset,h.s.theme);
        h.s.dark=preset.scheme==imkit::ColorScheme::Dark;
        h.s.presetIndex=static_cast<int>(preset.preset);
        h.Settle(2);
        h.Frame({},out/("frame-"+std::string(preset.id)+".png"));
    }
    h.windowTitle="ModernKIT / 長い日本語タイトル / Very long project title for testing safe truncation without hiding window controls";
    glfwSetWindowSize(h.window,640,480);h.Settle(2);
    h.Frame({},out/"frame-long-title.png");
    const std::array framePresets{imkit::WindowFramePreset::Native,imkit::WindowFramePreset::Studio,
                                  imkit::WindowFramePreset::Workspace,imkit::WindowFramePreset::Tool};
    for(auto preset:framePresets) {
        imkit::gallery::SelectFramePreset(h.s,preset);
        const auto style=h.s.frameStyle;
        const auto contrast=imkit::ValidateWindowFrameContrast(style);
        require(preset==imkit::WindowFramePreset::Native || style.metrics.height>0,"Preset metrics missing");
        require(contrast.activeTitle>0,"Preset contrast missing");
    }
    imkit::gallery::SelectFramePreset(h.s,imkit::WindowFramePreset::Workspace);
    h.s.frameStyle.metrics={57,27,9,11,39,2};
    h.s.frameStyle.features={true,true,true,true,true,true,true,true};
    std::array<ImVec4*,11> editedColors{&h.s.frameStyle.activeBackground,&h.s.frameStyle.inactiveBackground,
        &h.s.frameStyle.border,&h.s.frameStyle.titleText,&h.s.frameStyle.auxiliaryText,&h.s.frameStyle.icon,
        &h.s.frameStyle.buttonText,&h.s.frameStyle.buttonHover,&h.s.frameStyle.buttonPressed,
        &h.s.frameStyle.closeButtonHover,&h.s.frameStyle.closeButtonPressed};
    for(std::size_t i=0;i<editedColors.size();++i) *editedColors[i]={.01f*float(i+1),.02f*float(i+1),.03f*float(i+1),1};
    const auto customLayout=imkit::LayoutWindowFrame(1200,h.s.frameStyle,{2});
    require(customLayout.titleBar.Height()==114 && customLayout.icon.Width()==54,"Height/icon metrics did not affect layout");
    require(customLayout.applicationName.min.x==customLayout.icon.max.x+18,"Left padding did not affect layout");
    require(customLayout.close.Width()==78 && customLayout.maximizeRestore.Width()==78 && customLayout.minimize.Width()==78,
            "Button width did not affect layout");
    require(customLayout.applicationName.Width()>0 && customLayout.projectName.Width()>0 &&
            customLayout.unsavedIndicator.Width()>0 && customLayout.workspaceSwitcher.Width()>0,
            "Enabled content features did not affect layout");
    require(h.s.frameStyle.metrics.titlePaddingRight==11 && h.s.frameStyle.metrics.borderWidth==2,
            "Right padding/border metrics were not retained");
    const auto generated=imkit::MakeWindowFrameStyle(h.s.framePreset,h.s.theme);
    imkit::gallery::RegenerateFrameColors(h.s);
    require(h.s.frameStyle.metrics.height==57,"Theme regeneration must preserve edited metrics");
    const std::array<const ImVec4*,11> regeneratedColors{&h.s.frameStyle.activeBackground,&h.s.frameStyle.inactiveBackground,
        &h.s.frameStyle.border,&h.s.frameStyle.titleText,&h.s.frameStyle.auxiliaryText,&h.s.frameStyle.icon,
        &h.s.frameStyle.buttonText,&h.s.frameStyle.buttonHover,&h.s.frameStyle.buttonPressed,
        &h.s.frameStyle.closeButtonHover,&h.s.frameStyle.closeButtonPressed};
    const std::array<const ImVec4*,11> expectedColors{&generated.activeBackground,&generated.inactiveBackground,
        &generated.border,&generated.titleText,&generated.auxiliaryText,&generated.icon,&generated.buttonText,
        &generated.buttonHover,&generated.buttonPressed,&generated.closeButtonHover,&generated.closeButtonPressed};
    for(std::size_t i=0;i<regeneratedColors.size();++i)
        require(regeneratedColors[i]->x==expectedColors[i]->x && regeneratedColors[i]->y==expectedColors[i]->y &&
                regeneratedColors[i]->z==expectedColors[i]->z && regeneratedColors[i]->w==expectedColors[i]->w,
                "Theme regeneration did not refresh every color");
    h.s.frameStyle.features={false,false,false,false,false,false,false,false};
    const auto disabledLayout=imkit::LayoutWindowFrame(900,h.s.frameStyle);
    require(disabledLayout.icon.Width()==0 && disabledLayout.close.Width()==0 && disabledLayout.workspaceSwitcher.Width()==0,
            "Feature flags did not affect layout");
    imkit::gallery::ResetFramePreset(h.s);
    require(h.s.frameStyle.metrics.height==generated.metrics.height && h.s.frameStyle.features.workspaceSwitcher,
            "Preset reset did not restore complete values");
    imkit::gallery::SelectFramePreset(h.s,imkit::WindowFramePreset::Studio);
    h.windowFrame.Execute(imkit::WindowFrameOperation::Close);
    require(glfwWindowShouldClose(h.window),"Close must reach GLFW should-close path");
    std::ofstream report(out/"window-frame-verification.txt");
    report<<"PASS: four presets; all colors/metrics/features; Theme color regeneration and complete preset reset; DPI layout and UTF-8 elision at 100/150/200 percent; native hit tests; maximize/work area/restore/minimize; GLFW close; 12 theme backbuffers.\n"
          <<"Actual window DPI: "<<GetDpiForWindow(window)<<"\n"
          <<"Synthetic Win32 messages are not physical pointer/keyboard or cross-monitor DPI acceptance.\n";
    std::puts("Window frame verification passed");
}
} // namespace imkit::gallery::host
