#include <windows.h>
#include <objbase.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "gallery.h"
#include "../design_gallery/capture.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <chrono>
#include "allocation_probe.h"
#include <algorithm>
namespace {
const std::filesystem::path &NoCapturePath() {
    static const std::filesystem::path empty;
    return empty;
}
struct Host {
    GLFWwindow *window = nullptr;
    imkit::gallery::GalleryState s;
    bool automated = false;
    ImGuiMemAllocFunc originalAlloc=nullptr;
    ImGuiMemFreeFunc originalFree=nullptr;
    void *originalAllocatorUser=nullptr;
    bool countImGuiAllocations=false;
    std::size_t imguiAllocations=0;
    ImVec2 mouse{-100, -100};
    void Frame(const std::function<void(ImGuiIO &)> &input = {}, const std::filesystem::path &shot = NoCapturePath()) {
        s.editors.RenderPreview();
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        auto &io = ImGui::GetIO();
        if (automated) {
            io.DeltaTime = 1.f / 60;
            io.AddFocusEvent(true);
            io.AddMousePosEvent(mouse.x, mouse.y);
        }
        if (input)
            input(io);
        ImGui::NewFrame();
        imkit::gallery::Show(s);
        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(.1f, .1f, .1f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!shot.empty())
            imkit::design::SaveBackbuffer(shot, w, h);
        glfwSwapBuffers(window);
    }
    void Settle(int n = 5) {
        while (n--)
            Frame();
    }
    void Page(int page) {
        s.page = page;
        mouse = {-100, -100};
        Settle();
    }
    void ClickAt(ImVec2 pos) {
        mouse = pos;
        Frame();
        Frame([](auto &io) { io.AddMouseButtonEvent(0, true); });
        Frame([](auto &io) { io.AddMouseButtonEvent(0, false); });
        Settle(2);
    }
    void Click(const char *name) {
        ClickAt(s.probes.at(name).Center());
    }
    void Key(ImGuiKey key) {
        Frame([&](auto &io) { io.AddKeyEvent(key, true); });
        Frame([&](auto &io) { io.AddKeyEvent(key, false); });
        Settle(2);
    }
    void Replace(const char *probe, const char *text) {
        Click(probe);
        ReplaceActive(text);
    }
    void ReplaceActive(const char *text) {
        Frame([](auto &io) {
            io.AddKeyEvent(ImGuiMod_Ctrl, true);
            io.AddKeyEvent(ImGuiKey_A, true);
        });
        Frame([](auto &io) {
            io.AddKeyEvent(ImGuiKey_A, false);
            io.AddKeyEvent(ImGuiMod_Ctrl, false);
        });
        Frame([&](auto &io) { io.AddInputCharactersUTF8(text); });
        Settle();
    }
};
void Verify(Host &h, const std::filesystem::path &out) {
    std::ofstream log(out / "interaction.txt");
    auto check = [&](bool ok, const char *name) {
        log << (ok ? "PASS " : "FAIL ") << name << '\n';
        log.flush();
        if (!ok)
            throw std::runtime_error(name);
    };
    h.Page(0);
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
    h.Page(1);
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
    h.mouse.x += 25;
    h.Frame();
    h.Frame([](auto &io) { io.AddMouseButtonEvent(0, false); });
    h.Settle();
    check(h.s.lower > 20 && h.s.lower <= h.s.upper, "Range edit and ordering");
    h.Page(2);
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
    h.Page(3);
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
    h.Page(4);
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
    h.Page(5);
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
    h.Page(6);
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
    check(renderer.Resize(800, 450) && renderer.Render(meshes, camera), "GL FBO resize");
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
    h.Page(8);
    auto drag = [&](ImVec2 from, ImVec2 to) {
        h.mouse = from; h.Frame();
        h.Frame([](auto &io) { io.AddMouseButtonEvent(0, true); });
        h.mouse = to; h.Frame();
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
    auto pos = clipPosition(1000, 90); drag(pos, {pos.x + 40, pos.y});
    check(findClip().start > before, "Timeline move via public IO");
    before = findClip().duration;
    pos = clipPosition(1000, static_cast<float>(imkit::editor::Seconds(before)*s.timeline.canvas.scale.x)-3);
    drag(pos, {pos.x-25,pos.y});
    check(findClip().duration < before, "Timeline end trim via public IO");
    auto count = s.clips.size(); s.timeline.tool = imkit::video::Tool::Razor;
    h.ClickAt(clipPosition(1000, 100));
    check(s.clips.size() == count+1, "Timeline razor split via public IO");
    s.timeline.tool = imkit::video::Tool::Select;
    h.Page(9);
    auto projected = imkit::cg::Project(s.objects[1].transform.translation, s.viewport.camera, s.viewportOrigin, s.viewportSize);
    auto end = imkit::cg::Project({s.objects[1].transform.translation.x+1,s.objects[1].transform.translation.y,s.objects[1].transform.translation.z},s.viewport.camera,s.viewportOrigin,s.viewportSize);
    float dx=end.screen.x-projected.screen.x,dy=end.screen.y-projected.screen.y,len=std::sqrt(dx*dx+dy*dy);
    pos={projected.screen.x+dx/len*70,projected.screen.y+dy/len*70};
    auto oldX=s.objects[1].transform.translation.x;
    drag(pos,{pos.x+40,pos.y});
    check(s.objects[1].transform.translation.x>oldX,"CG X gizmo preview and commit via public IO");
    check(s.objectSelection.active==s.objects[1].id,"CG shared selection stable ID");
    h.Frame({},out/"cg-edited.png");
    h.Page(7);
    auto &key=s.keys[4]; const auto keyId=key.id; auto keyTick=key.tick;
    auto keyScreen=imkit::editor::ToScreen({imkit::editor::Seconds(key.tick),-key.value},s.curve.canvas,{s.curve.view.min.x,s.curve.view.min.y});
    drag({static_cast<float>(keyScreen.x),static_cast<float>(keyScreen.y)}, {static_cast<float>(keyScreen.x+20),static_cast<float>(keyScreen.y+10)});
    auto editedKey=std::find_if(s.keys.begin(),s.keys.end(),[&](const auto &k){return k.id==keyId;});
    check(editedKey->tick>keyTick,"Curve key move via public IO");
    s.animationPage=2; h.Page(9); s.animationPage=-1; h.Settle();
    auto vertex=s.uv[0].uv;auto uvScreen=imkit::editor::ToScreen(vertex,s.uvState.canvas,{s.uvState.view.min.x,s.uvState.view.min.y});
    drag({static_cast<float>(uvScreen.x),static_cast<float>(uvScreen.y)},{static_cast<float>(uvScreen.x+30),static_cast<float>(uvScreen.y+15)});
    check(s.uv[0].uv.x>vertex.x&&s.uv[0].uv.y>vertex.y,"UV vertex transform via public IO");
    h.Frame({},out/"uv-edited.png");
    s.Dataset(true); h.Page(8); h.Settle(20);
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
    report<<"operation,frames,p95_ms,max_ms,queries_max,clips_max,cpp_new,imgui_allocations,interaction_verified\n";
    std::ofstream context(out/"editor-performance-context.txt");
#ifdef NDEBUG
    context<<"configuration=Release\n";
#else
    context<<"configuration=Debug (not Release acceptance)\n";
#endif
    int width=0,height=0;glfwGetFramebufferSize(h.window,&width,&height);
    context<<"framebuffer="<<width<<'x'<<height<<"\n20 warm-up frames, 180 measured frames per operation.\n"
        <<"Boundary: Host::Frame wall time including host event apply, preview render, ImGui, GL submission and swap; vsync off.\n"
        <<"Allocations: C++ new and ImGui allocator calls; excludes driver/internal OS allocation.\n"
        <<"Public ImGui IO, hidden native GL window; not native OS/IME input.\n";
    const char *names[]={"pan","zoom","selection","clip_drag","clip_trim","keyframe_drag"};
    bool allPassed=width==1920 && height==1440;
    for (int operation=0;operation<6;++operation) {
        s.Dataset(true);s.selection.Clear();s.timeline={};s.timeline.snapping=false;
        h.Page(8);h.Settle(20);
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
        std::array<double,180> timings{};std::size_t queries=0,clips=0;
        h.imguiAllocations=0;h.countImGuiAllocations=true;gallery::CountAllocations(true);
        for (int i=0;i<180;++i) {
            const auto begin=std::chrono::steady_clock::now();frame(i);
            timings[i]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
            queries=std::max(queries,s.queryCount);clips=std::max(clips,s.queriedClips);
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
            <<allocations<<','<<imguiAllocations<<','<<gestureValid<<'\n';report.flush();
        context<<names[operation]<<" terminal_frame_ms="<<terminalMs<<" tracks="<<s.tracks.size()<<" clips="<<s.clips.size()<<" keys="<<s.keys.size()<<'\n';context.flush();
        allPassed &= gestureValid && timings[170]<=16.7 && queries<30 && clips<1000;
    }
    if (!allPassed) throw std::runtime_error("Editor benchmark did not meet interaction, size, query or P95 gates; see report");
}

void VerifyTrackControls(Host &h,const std::filesystem::path &out) {
    using namespace imkit;
    auto &s=h.s.editors;s.japanese=false;s.timeline.headerWidth=340;h.Page(8);
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
    log<<"Public ImGui IO and native GL backbuffer; native OS/IME input not tested.\n";
}

void VerifyMonitors(Host &h,const std::filesystem::path &out) {
    using namespace imkit;
    auto &s=h.s.editors;h.Page(8);
    s.timeline.time.playhead=0;s.markers[0]={s.nextId++,0,"Opening cue / 開始の合図"};s.markerCount=1;
    std::ofstream log(out/"monitor-interaction.txt");
    const auto timelineScale=s.timeline.canvas.scale.x,timelineOrigin=s.timeline.canvas.origin.x;
    for (int preset:{0,2,1}) {
        h.mouse={(s.programMonitorMin.x+s.programMonitorMax.x)*.5f,
                 (s.programMonitorMin.y+s.programMonitorMax.y)*.5f};h.Frame();
        h.Frame([](auto &io){io.AddMouseButtonEvent(1,true);});
        h.Frame([](auto &io){io.AddMouseButtonEvent(1,false);});
        h.mouse={-100,-100};h.Frame();h.Key(ImGuiKey_Home);
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
    log<<"Public ImGui IO and native GL capture; native OS/IME input not tested.\n";
}

void VerifyColor(Host &h, const std::filesystem::path &out) {
    auto &s=h.s.editors;
    s.videoPanel=1; h.Page(8); h.Settle(4);
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
} // namespace
int VerifyInspectorModel() {
    using namespace imkit;
    auto storage=std::make_unique<gallery::EditorWorkspaces>();
    auto &state=*storage;
    state.Initialize();
    int failures=0;
    auto check=[&](bool ok,const char *name) {
        std::printf("%s %s\n",ok?"PASS":"FAIL",name);
        if (!ok) ++failures;
    };
    {
        auto copyStorage=std::make_unique<gallery::EditorWorkspaces>();
        auto &copies=*copyStorage;copies.Initialize();
        const auto original=copies.clips.front().id,property=copies.clipPropertyIds[1];
        copies.clipProperties.at(original).values[1]=1.75;
        copies.propertyFlags[property]=8;
        copies.propertyKeys[property]={{copies.nextId++,property,0,1.75}};
        copies.clipEnvelopes[original]={{copies.nextId++,0,.75},{copies.nextId++,1000,1}};
        copies.clips.front().envelope=copies.clipEnvelopes.at(original);
        auto duplicate=[&] {
            copies.events.Push({original,copies.revision,editor::Phase::Commit,editor::EditKind::Duplicate,{},
                editor::Value{editor::FromSeconds(100)}});copies.ApplyEvents();
        };
        const auto count=copies.clips.size();copies.tracks.front().locked=true;duplicate();
        check(copies.clips.size()==count,"locked track rejects host clip duplication");copies.tracks.front().locked=false;
        duplicate();
        const auto copy=std::find_if(copies.clips.begin(),copies.clips.end(),[&](const auto &c) {
            return c.track==copies.tracks.front().id && c.start==editor::FromSeconds(100);
        });
        check(copy!=copies.clips.end() && copies.clips.size()==count+1,"host duplicates clip");
        if (copy!=copies.clips.end()) {
            const auto copyProperty=copies.clipProperties.at(copy->id).ids[1];
            check(copyProperty!=property && copies.clipProperties.at(copy->id).values[1]==1.75 &&
                  copies.propertyFlags.at(copyProperty)==8 && copies.propertyKeys.at(copyProperty).front().channel==copyProperty &&
                  copies.propertyKeys.at(copyProperty).front().id!=copies.propertyKeys.at(property).front().id,
                  "duplicate copies Inspector values flags and property keys with independent IDs");
            check(copy->keyChannel!=copies.clips.front().keyChannel && !copy->keys.empty() &&
                  copy->keys.front().id!=copies.clips.front().keys.front().id &&
                  copy->keys.front().value==copies.clips.front().keys.front().value,
                  "duplicate receives independent inline key channel and keys");
            copies.clipEnvelopes.at(copy->id).front().gain=.25;
            check(copy->envelope.front().id!=copies.clips.front().envelope.front().id &&
                  copies.clips.front().envelope.front().gain==.75,
                  "duplicate envelope edits preserve original clip");
        }
    }
    {
        auto splitStorage=std::make_unique<gallery::EditorWorkspaces>();auto &split=*splitStorage;split.Initialize();
        const auto id=split.clips.front().id;const auto start=split.clips.front().start;
        split.clipEnvelopes[id]={{split.nextId++,0,.5},{split.nextId++,1000,1.5}};
        split.clips.front().envelope=split.clipEnvelopes.at(id);
        auto applySplit=[&] {
            split.events.Push({id,split.revision,editor::Phase::Commit,editor::EditKind::Split,{},editor::Value{start+500}});
            split.ApplyEvents();
        };
        const auto count=split.clips.size();split.tracks.front().locked=true;applySplit();
        check(split.clips.size()==count,"locked track rejects host clip split");split.tracks.front().locked=false;applySplit();
        const auto right=std::find_if(split.clips.begin(),split.clips.end(),[&](const auto &c){return c.track==split.tracks.front().id && c.start==start+500;});
        check(right!=split.clips.end() && split.clips.size()==count+1,"host splits clip at requested tick");
        if (right!=split.clips.end()) {
            const auto &left=split.clipEnvelopes.at(id);
            check(left.size()==2 && left.back().tick==500 && left.back().gain==1 &&
                  right->envelope.size()==2 && right->envelope.front().tick==0 && right->envelope.front().gain==1 &&
                  right->envelope.back().tick==500 && right->envelope.back().gain==1.5 &&
                  right->envelope.front().id!=left.back().id,
                  "split interpolates envelope boundary and shifts independent right points to local time");
            check(video::EvaluateEnvelope(left,250)==.75 && video::EvaluateEnvelope(right->envelope,250)==1.25,
                  "split envelope preserves original gain on both sides");
        }
    }
    {
        auto rippleStorage=std::make_unique<gallery::EditorWorkspaces>();auto &ripple=*rippleStorage;ripple.Initialize();
        const auto source=ripple.clips.front(),following=ripple.clips[1];const auto beforeRevision=ripple.revision;
        auto apply=[&] {
            ripple.events.Push({source.id,ripple.revision,editor::Phase::Commit,editor::EditKind::Ripple,{},
                {source.start,source.start+source.duration+100,source.sourceIn,source.track,source.speed}});ripple.ApplyEvents();
        };
        ripple.clips[1].locked=true;apply();
        check(ripple.revision==beforeRevision && ripple.clips.front().duration==source.duration &&
              ripple.clips[1].start==following.start,"locked following clip rejects whole host ripple before source trim");
        ripple.clips[1].locked=false;apply();
        check(ripple.clips.front().duration==source.duration+100 && ripple.clips[1].start==following.start+100,
              "unlocked ripple applies source trim and following shift together");
    }
    const auto firstClipId=state.clips.front().id,secondClipId=state.clips[1].id;
    const auto firstScaleId=state.clipPropertyIds[1];
    state.events.Push({firstScaleId,state.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,1.5}});state.ApplyEvents();
    state.selection.Set(secondClipId);state.SyncClipProperties();
    check(state.clipPropertyIds[1]!=firstScaleId && state.clipPropertyValues[1]==1,"clip selection uses independent property IDs and defaults");
    state.events.Push({firstScaleId,state.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,2}});state.ApplyEvents();
    state.selection.Set(firstClipId);state.SyncClipProperties();
    check(state.clipPropertyValues[1]==1.5,"clip property value survives selection and rejects stale owner edit");
    state.tracks.front().locked=true;
    state.events.Push({firstScaleId,state.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,2}});state.ApplyEvents();
    check(state.clipProperties.at(firstClipId).values[1]==1.5,"locked track rejects clip Inspector edit");state.tracks.front().locked=false;
    const auto lockedClip=state.clips.front();const auto lockedRevision=state.revision;
    state.tracks.front().locked=true;
    for (auto kind:{editor::EditKind::Move,editor::EditKind::TrimStart,editor::EditKind::TrimEnd,
                   editor::EditKind::Ripple,editor::EditKind::Roll,editor::EditKind::Slip,editor::EditKind::Slide}) {
        state.events.Push({lockedClip.id,state.revision,editor::Phase::Commit,kind,{},
            {lockedClip.start+1,lockedClip.start+lockedClip.duration+1,lockedClip.sourceIn+1,lockedClip.track,lockedClip.speed}});
        state.ApplyEvents();
        check(state.revision==lockedRevision && state.clips.front().start==lockedClip.start &&
              state.clips.front().duration==lockedClip.duration && state.clips.front().sourceIn==lockedClip.sourceIn,
              "locked track rejects host clip edit without changing revision");
    }
    state.tracks.front().locked=false;
    const auto unchangedRevision=state.revision;
    const auto &unchangedClip=state.clips.front();
    editor::Value sameClip{unchangedClip.start,unchangedClip.start+unchangedClip.duration,unchangedClip.sourceIn,unchangedClip.track,unchangedClip.speed};
    for (auto kind:{editor::EditKind::Move,editor::EditKind::TrimStart,editor::EditKind::TrimEnd,
                   editor::EditKind::Ripple,editor::EditKind::Roll,editor::EditKind::Slip,editor::EditKind::Slide}) {
        state.events.Push({unchangedClip.id,state.revision,editor::Phase::Commit,kind,sameClip,sameClip});state.ApplyEvents();
        check(state.revision==unchangedRevision,"unchanged clip edit preserves host revision and skips index rebuild");
    }
    const auto clipChannel=state.clips.front().keyChannel;
    const auto countBeforeInsert=state.keys.size();
    editor::Event clipInsert{clipChannel,state.revision,editor::Phase::Commit,editor::EditKind::KeyInsert};
    clipInsert.proposed.parent=state.clips.front().id;clipInsert.proposed.first=12345;clipInsert.proposed.x=.4;
    state.events.Push(clipInsert);state.ApplyEvents();
    check(state.keys.size()==countBeforeInsert+1 && std::any_of(state.clips.front().keys.begin(),state.clips.front().keys.end(),
        [](const auto &key){return key.tick==12345 && key.value==.4;}),"clip insertion applies to explicit host channel and refreshes span");
    const auto savedKeys=state.keys;
    std::erase_if(state.keys,[&](const auto &key){return key.channel==clipChannel;});state.RebuildKeyIndex();
    check(state.clips.front().keyChannel==clipChannel && state.clips.front().keys.empty(),
          "empty clip channel keeps its explicit ID instead of adopting another channel");
    clipInsert.revision=state.revision;state.events.Push(clipInsert);state.ApplyEvents();
    check(state.clips.front().keys.size()==1 && state.clips.front().keys.front().channel==clipChannel,
          "first key can be inserted again into an emptied clip channel");
    state.clips[1].keyChannel=clipChannel;
    state.keys=savedKeys;state.RebuildKeyIndex();
    check(state.clips[1].keys.data()==state.clips.front().keys.data() && !state.clips[1].keys.empty(),
          "all clips referencing an explicit channel refresh their borrowed key spans");
    state.clips[1].keyChannel=0;state.RebuildKeyIndex();
    const auto transitionClip=state.clips.front().id;
    state.events.Push({transitionClip,state.revision,editor::Phase::Commit,editor::EditKind::TransitionDuration,{},
        {editor::FromSeconds(.2),editor::FromSeconds(.3)}});state.ApplyEvents();
    check(state.clips.front().transitionIn==editor::FromSeconds(.2) && state.UndoTransition() &&
          state.clips.front().transitionIn==0,"transition duration host apply and Undo");
    check(state.UndoTransition(true) && state.clips.front().transitionOut==editor::FromSeconds(.3),"transition duration Redo");
    state.events.Push({transitionClip,state.revision,editor::Phase::Commit,editor::EditKind::TransitionType,{}, {2,3}});state.ApplyEvents();
    check(state.clips.front().transitionInKind==video::TransitionKind::Fade && state.UndoTransition() &&
          state.clips.front().transitionInKind==video::TransitionKind::Dissolve,"transition kind host apply and Undo");
    state.UndoTransition();
    auto audioClip=std::find_if(state.clips.begin(),state.clips.end(),[](const auto &c){return !c.envelope.empty();});
    const auto envelopeCount=audioClip->envelope.size();
    auto envelopeEdit=[&](editor::StableId target,editor::Tick action,editor::Tick tick,double gain) {
        editor::Event event{target,state.revision,editor::Phase::Commit,editor::EditKind::AudioEnvelope};
        event.proposed.parent=audioClip->id;event.proposed.offset=action;event.proposed.first=tick;event.proposed.x=gain;
        state.events.Push(event);state.ApplyEvents();
    };
    envelopeEdit(audioClip->id,1,editor::FromSeconds(2),.8);
    auto addedPoint=std::find_if(audioClip->envelope.begin(),audioClip->envelope.end(),[](const auto &p){return p.tick==editor::FromSeconds(2);});
    check(audioClip->envelope.size()==envelopeCount+1 && addedPoint!=audioClip->envelope.end(),"envelope insert refreshes host span");
    if (addedPoint!=audioClip->envelope.end()) {
        const auto pointId=addedPoint->id;
        envelopeEdit(pointId,0,editor::FromSeconds(2.2),1.4);
        check(std::any_of(audioClip->envelope.begin(),audioClip->envelope.end(),[&](const auto &p){return p.id==pointId && p.gain==1.4;}),"envelope edit preserves point identity");
        envelopeEdit(pointId,2,0,1);
        check(audioClip->envelope.size()==envelopeCount,"envelope remove refreshes host span");
    }
    const auto rendererComponent=state.components[0].view.id,wireComponent=state.components[1].view.id;
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,1}});state.ApplyEvents();
    check(state.BuildSceneMeshes().front().wire,"enabled wire component changes preview rendering");
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Reorder,{},
        {0,0,-1,state.objects[1].id}});state.ApplyEvents();
    check(!state.BuildSceneMeshes().front().wire,"component order changes renderer override result");
    state.events.Push({rendererComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,0}});state.ApplyEvents();
    check(state.BuildSceneMeshes().empty(),"disabled renderer component removes preview mesh");
    state.events.Push({rendererComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,1}});state.ApplyEvents();
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Reorder,{},
        {0,0,1,state.objects[1].id}});state.ApplyEvents();
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,0}});state.ApplyEvents();
    state.RebuildOutlinerRows();check(state.outlinerRows.size()==4,"Outliner expanded hierarchy includes all rows");
    state.objects[0].expanded=false;state.RebuildOutlinerRows();
    check(state.outlinerRows.size()==1,"Outliner collapse hides descendants");
    std::snprintf(state.outliner.search,sizeof(state.outliner.search),"Camera");state.RebuildOutlinerRows();
    check(state.outlinerRows.size()==2 && state.outlinerRows[1].id==state.objects[3].id &&
          state.outlinerRows[1].depth==1,"Outliner search includes match and ancestor through collapsed hierarchy");
    state.outliner.search[0]=0;state.objects[0].expanded=true;
    const auto oldParent=state.objects[3].parent;state.objects[3].parent=state.objects[1].id;
    state.RebuildOutlinerRows();
    check(state.outlinerRows.size()==4 && state.outlinerRows[2].id==state.objects[3].id &&
          state.outlinerRows[2].depth==2 && state.outlinerRows[1].hasChildren,"Outliner rebuild reflects reparented hierarchy");
    state.objects[3].parent=oldParent;
    const auto initialOrder=state.objectOrder;
    editor::Event reorderObject{state.objects[2].id,state.revision,editor::Phase::Commit,editor::EditKind::Reorder};
    reorderObject.proposed.parent=state.objects[2].parent;reorderObject.proposed.offset=-1;
    state.events.Push(reorderObject);state.ApplyEvents();state.RebuildOutlinerRows();
    check(state.outlinerRows[1].id==state.objects[2].id && state.objects[1].id==1000001,
          "Outliner sibling reorder preserves object storage identity");
    state.objects[1].locked=true;reorderObject.revision=state.revision;reorderObject.proposed.offset=1;
    state.events.Push(reorderObject);state.ApplyEvents();
    check(state.objectOrder[1]==state.objects[2].id,"Outliner reorder cannot cross locked sibling");
    state.objects[1].locked=false;state.objectOrder=initialOrder;
    state.objectSelection.Set(state.objects[1].id);
    state.objectSelection.Set(state.objects[2].id,true);
    auto send=[&](editor::Phase phase,int component,double value,editor::EditKind kind=editor::EditKind::Property) {
        state.events.Clear();
        state.events.Push({state.objectPropertyIds[1][component],state.revision,phase,kind,{},editor::Value{0,0,0,0,value}});
        state.ApplyEvents();
    };
    send(editor::Phase::Begin,4,.6);send(editor::Phase::Commit,4,.6);
    check(state.objects[1].transform.rotation.y==.6 && state.objects[2].transform.rotation.y==.6,
          "explicit rotation property applies to selected objects");
    send(editor::Phase::Begin,8,1.8);send(editor::Phase::Commit,8,1.8);
    check(state.objects[1].transform.scale.z==1.8 && state.objects[2].transform.scale.z==1.8,
          "scale property applies to selected objects");
    send(editor::Phase::Commit,8,1,editor::EditKind::Reset);
    check(state.objects[1].transform.scale.z==1 && state.objects[2].transform.scale.z==1,"scale reset uses unit value");
    state.objects[2].locked=true;
    send(editor::Phase::Begin,4,2);send(editor::Phase::Commit,4,2);
    check(state.objects[1].transform.rotation.y==.6 && state.objects[2].transform.rotation.y==.6,
          "locked selected object rejects whole edit");
    state.objects[2].locked=false;
    state.propertyFlags[state.objectPropertyIds[2][4]]=16;
    send(editor::Phase::Begin,4,2);send(editor::Phase::Commit,4,2);
    check(state.objects[1].transform.rotation.y==.6,"locked selected property rejects whole edit");
    state.propertyFlags[state.objectPropertyIds[2][4]]=0;
    send(editor::Phase::Begin,4,2);
    state.objectSelection.Set(state.objects[1].id);
    send(editor::Phase::Commit,4,2);
    check(state.objects[1].transform.rotation.y==.6,"selection change rejects gesture commit");
    auto key=[&](editor::PropertyKeyAction action,editor::Tick tick) {
        state.events.Clear();
        state.events.Push({state.objectPropertyIds[1][4],state.revision,editor::Phase::Commit,
            editor::EditKind::PropertyKey,{},editor::Value{tick,0,static_cast<editor::Tick>(action),0,.6}});
        state.ApplyEvents();
    };
    key(editor::PropertyKeyAction::Add,100);key(editor::PropertyKeyAction::Add,200);
    check(state.propertyKeys[state.objectPropertyIds[1][4]].size()==2,"property keys stored in host channel");
    key(editor::PropertyKeyAction::Previous,200);
    check(state.timeline.time.playhead==100,"previous property key navigation");
    key(editor::PropertyKeyAction::Next,100);
    check(state.timeline.time.playhead==200,"next property key navigation");
    key(editor::PropertyKeyAction::Remove,100);
    check(state.propertyKeys[state.objectPropertyIds[1][4]].size()==1,"property key removal");
    const auto originalKey=state.keys[1];
    const auto keyCount=state.keys.size();
    state.events.Clear();
    state.events.Push({originalKey.id,state.revision,editor::Phase::Commit,editor::EditKind::Duplicate,{},
        editor::Value{originalKey.tick+editor::TicksPerSecond,0,0,0,originalKey.value+.1}});
    state.ApplyEvents();
    auto source=std::find_if(state.keys.begin(),state.keys.end(),[&](const auto &key){return key.id==originalKey.id;});
    auto duplicate=std::find_if(state.keys.begin(),state.keys.end(),[&](const auto &key){return key.id!=originalKey.id &&
        key.channel==originalKey.channel && key.tick==originalKey.tick+editor::TicksPerSecond && key.value==originalKey.value+.1;});
    check(state.keys.size()==keyCount+1 && source!=state.keys.end() && source->tick==originalKey.tick && duplicate!=state.keys.end(),
        "curve duplication assigns a new ID and preserves source key");
    state.Dataset(true);
    auto visible=state.QueryKeys({{editor::FromSeconds(100),editor::FromSeconds(101)},-10,10});
    check(state.keyChannels.size()==3 && visible.size()<32,"100k curve query returns bounded keys from three channels");
    bool ordered=true;
    for (std::size_t i=1;i<visible.size();++i)
        ordered &= visible[i-1].channel<visible[i].channel ||
            (visible[i-1].channel==visible[i].channel && visible[i-1].tick<=visible[i].tick);
    check(ordered,"curve query keeps each channel contiguous and time sorted");
    bool matches=true;
    for (auto mode:{editor::HandleMode::Auto,editor::HandleMode::AutoClamped,editor::HandleMode::Vector,
                   editor::HandleMode::Aligned,editor::HandleMode::Free}) {
        for (auto &key:state.keys) key.handles=mode;
        visible=state.QueryKeys({{editor::FromSeconds(100.1),editor::FromSeconds(100.9)},-10,10});
        for (auto [first,last]:state.keyChannels) {
            auto channel=std::span<const editor::Keyframe>(state.keys).subspan(first,last-first);
            auto a=std::find_if(visible.begin(),visible.end(),[&](const auto &key){return key.channel==channel.front().channel;});
            auto b=std::find_if(a,visible.end(),[&](const auto &key){return key.channel!=channel.front().channel;});
            for (double seconds:{100.1,100.25,100.75,100.9}) {
                const auto sample=editor::FromSeconds(seconds);
                matches &= std::abs(editor::Evaluate(channel,sample)-editor::Evaluate({a,b},sample))<1e-9;
            }
        }
    }
    check(matches,"five handle modes preserve full-channel evaluation at visible boundaries");
    const auto stripId=state.animationStrips[0].id,neighborId=state.animationStrips[1].id;
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Update,editor::EditKind::StripSettings,{},
        {0,0,1,0,2,3,.4}});state.ApplyEvents();
    check(state.animationStrips[0].scale==1,"strip Update preserves host settings");
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::StripSettings,{},
        {0,0,1,0,2,3,.4}});state.ApplyEvents();
    check(state.animationStrips[0].scale==2 && state.animationStrips[0].repeat==3 &&
        state.animationStrips[0].blend==.4 && state.animationStrips[0].muted,"strip Commit applies settings");
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::Reorder,{},
        {0,0,1,neighborId}});state.ApplyEvents();
    check(state.animationStrips[1].id==stripId && state.animationStrips[0].id==neighborId,
        "strip Reorder swaps host order by StableId");
    state.animationStrips[1].locked=true;
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::StripSettings,{},
        {0,0,0,0,8,9,.8}});state.ApplyEvents();
    check(state.animationStrips[1].locked && state.animationStrips[1].scale==2,
        "locked strip rejects simultaneous unlock and settings mutation");
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::StripSettings,{},
        {0,0,1,0,2,3,.4}});state.ApplyEvents();
    check(!state.animationStrips[1].locked,"locked strip accepts explicit unchanged-settings unlock");
    const auto sourceId=state.objects[1].id;const auto sourceX=state.objects[1].transform.translation.x;
    state.events.Push({sourceId,state.revision,editor::Phase::Commit,editor::EditKind::Duplicate});state.ApplyEvents();
    check(state.objects.size()==5 && state.objects.back().id!=sourceId && state.objectSelection.active==state.objects.back().id,
          "Outliner duplicate creates and selects a new object ID");
    bool distinctProperties=true;
    for (auto id:state.objectPropertyIds.back()) for (std::size_t i=0;i+1<state.objectPropertyIds.size();++i)
        for (auto previous:state.objectPropertyIds[i]) distinctProperties &= id!=previous;
    check(distinctProperties && state.BuildSceneMeshes().size()==2,"duplicate has independent property IDs and a preview mesh");
    state.events.Push({state.objects.back().id,state.revision,editor::Phase::Commit,editor::EditKind::Translate,{},
        {0,0,0,0,12,0,0}});state.ApplyEvents();
    check(state.objects.back().transform.translation.x==12 && state.objects[1].transform.translation.x==sourceX,
          "duplicate transform edit preserves source object");
    const auto independentGeometry=state.objects.back().geometry, sourceGeometry=state.objects[1].geometry;
    const auto independentX=state.geometries.at(independentGeometry).vertices.front().position[0];
    editor::Event linked{sourceId,state.revision,editor::Phase::Commit,editor::EditKind::Duplicate};linked.proposed.offset=1;
    state.events.Push(linked);state.ApplyEvents();
    const auto linkedId=state.objects.back().id;
    check(state.objects.back().geometry==sourceGeometry && independentGeometry!=sourceGeometry,
          "linked duplicate shares geometry while ordinary duplicate owns copied data");
    state.geometries.at(sourceGeometry).vertices.front().position[0]+=.25f;
    auto meshes=state.BuildSceneMeshes();
    auto linkedMesh=std::find_if(meshes.begin(),meshes.end(),[&](const auto &v){return v.id==linkedId;});
    check(linkedMesh!=meshes.end() && linkedMesh->vertices.front().position[0]==independentX+.25f &&
          state.geometries.at(independentGeometry).vertices.front().position[0]==independentX,
          "geometry edits reach linked preview and preserve independent duplicate");
    state.events.Push({linkedId,state.revision,editor::Phase::Commit,editor::EditKind::LinkGeometry});state.ApplyEvents();
    check(state.objects.back().geometry!=sourceGeometry &&
          state.geometries.at(state.objects.back().geometry).vertices.front().position[0]==independentX+.25f,
          "make single user copies current shared geometry");
    editor::Event relink{linkedId,state.revision,editor::Phase::Commit,editor::EditKind::LinkGeometry};relink.proposed.parent=sourceId;
    state.events.Push(relink);state.ApplyEvents();
    check(state.objects.back().geometry==sourceGeometry,"link geometry reconnects to active object data");
    const auto componentTotal=state.components.size();
    state.events.Push({sourceId,state.revision,editor::Phase::Commit,editor::EditKind::ComponentAdd,{}, {0,0,0,7802}});state.ApplyEvents();
    check(state.components.size()==componentTotal+1 && state.components.back().view.owner==sourceId &&
          state.components.back().wireOverride,"component add creates selected host type for owner");
    state.objects[1].locked=true;
    state.events.Push({sourceId,state.revision,editor::Phase::Commit,editor::EditKind::ComponentAdd,{}, {0,0,0,7801}});state.ApplyEvents();
    check(state.components.size()==componentTotal+1,"locked object rejects component addition");
    state.objects[1].locked=false;
    std::puts("Evidence: host model/event application; no native OS or GUI input.");
    return failures?1:0;
}
int main(int argc, char **argv) {
    bool capture = false, verify = false, verifyIcons = false, verifyEditors = false, verifyColor = false, benchmarkEditors = false, verifyMonitors = false, verifyTrackControls = false;
    int capturePage = -1, animationPage = -1;
    std::string iconSearch;
    std::filesystem::path out = "out/catalog";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--verify-inspector-model")
            return VerifyInspectorModel();
        if (a == "--capture")
            capture = true;
        else if (a == "--verify")
            verify = true;
        else if (a == "--capture-editors") { capture = true; capturePage = -2; }
        else if (a == "--verify-track-controls") verifyTrackControls=true;
        else if (a == "--verify-monitors") verifyMonitors=true;
        else if (a == "--benchmark-editors") benchmarkEditors=true;
        else if (a == "--verify-editors")
            verifyEditors = true;
        else if (a == "--verify-icons")
            verifyIcons = true;
        else if (a == "--verify-color")
            verifyColor = true;
        else if (a == "--output" && i + 1 < argc)
            out = argv[++i];
        else if (a == "--animation-page" && i + 1 < argc) {
            animationPage=std::stoi(argv[++i]);
            if (animationPage<0 || animationPage>3) return 2;
        }
        else if (a == "--icon-search" && i + 1 < argc) iconSearch=argv[++i];
        else if (a == "--page" && i + 1 < argc)
            capturePage = std::stoi(argv[++i]);
        else
            return 2;
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
        return 1;
    if (!glfwInit()) {
        CoUninitialize();
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, capture || verify || verifyIcons || verifyEditors || verifyColor || benchmarkEditors || verifyMonitors || verifyTrackControls ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    Host h;
    h.automated = capture || verify || verifyIcons || verifyEditors || verifyColor || benchmarkEditors || verifyMonitors || verifyTrackControls;
    h.window = glfwCreateWindow(1920, 1440, "ImKit Precision Layers", nullptr, nullptr);
    if (!h.window) {
        glfwTerminate();
        CoUninitialize();
        return 1;
    }
    glfwMakeContextCurrent(h.window);
    glfwSwapInterval(h.automated ? 0 : 1);
    IMGUI_CHECKVERSION();
    ImGui::GetAllocatorFunctions(&h.originalAlloc,&h.originalFree,&h.originalAllocatorUser);
    ImGui::SetAllocatorFunctions([](std::size_t size,void *user)->void *{auto &h=*static_cast<Host *>(user);if(h.countImGuiAllocations)++h.imguiAllocations;return h.originalAlloc(size,h.originalAllocatorUser);},
        [](void *memory,void *user){auto &h=*static_cast<Host *>(user);h.originalFree(memory,h.originalAllocatorUser);},&h);
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    bool backend = ImGui_ImplGlfw_InitForOpenGL(h.window, true),
         renderer = backend && ImGui_ImplOpenGL3_Init("#version 130");
    int result = 0;
    GLuint texture = 0;
    std::array<GLuint, 6> iconTextures{};
    try {
        if (!renderer)
            throw std::runtime_error("Backend initialization failed");
        wchar_t module[MAX_PATH];
        GetModuleFileNameW(nullptr, module, MAX_PATH);
        auto assets = std::filesystem::path(module).parent_path() / "design-assets";
        auto load = [&](const char *file) {
            auto path = assets / file;
            if (!std::filesystem::exists(path))
                throw std::runtime_error("Missing catalog font assets");
            auto *font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), 14);
            if (!font)
                throw std::runtime_error("Font loading failed");
            ImFontConfig config;
            config.MergeMode = true;
            static const ImWchar exclude[] = {0x20, 0x24f, 0};
            config.GlyphExcludeRanges = exclude;
            auto jp = assets / "NotoSansJP-Regular.otf";
            if (!std::filesystem::exists(jp) ||
                !io.Fonts->AddFontFromFileTTF(jp.string().c_str(), 14, &config))
                throw std::runtime_error("Japanese fallback missing");
            return font;
        };
        h.s.fonts = {load("Inter-Regular.ttf"), load("Inter-SemiBold.ttf")};
        const unsigned char pixels[] = {220, 220, 230, 255, 100, 100, 120, 255,
                                        100, 100, 120, 255, 220, 220, 230, 255};
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        h.s.texture = ImTextureRef(static_cast<ImTextureID>(texture));
        glGenTextures(6, iconTextures.data());
        for (int i = 0; i < 6; ++i) {
            const auto atlas = imkit::GetIconAtlasPixels(imkit::IconPixelSizes[i]);
            glBindTexture(GL_TEXTURE_2D, iconTextures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            constexpr GLint clampToEdge = 0x812F; // OpenGL 1.2; Windows GL.h exposes only 1.1.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas.width, atlas.height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, atlas.rgba.data());
            h.s.icons.SetTexture(atlas.iconPixels, ImTextureRef(static_cast<ImTextureID>(iconTextures[i])));
        }
        imkit::preview::GLFunctions previewFunctions;
        previewFunctions.GenFramebuffers = reinterpret_cast<decltype(previewFunctions.GenFramebuffers)>(glfwGetProcAddress("glGenFramebuffers"));
        previewFunctions.DeleteFramebuffers = reinterpret_cast<decltype(previewFunctions.DeleteFramebuffers)>(glfwGetProcAddress("glDeleteFramebuffers"));
        previewFunctions.BindFramebuffer = reinterpret_cast<decltype(previewFunctions.BindFramebuffer)>(glfwGetProcAddress("glBindFramebuffer"));
        previewFunctions.CheckFramebufferStatus = reinterpret_cast<decltype(previewFunctions.CheckFramebufferStatus)>(glfwGetProcAddress("glCheckFramebufferStatus"));
        previewFunctions.FramebufferTexture2D = reinterpret_cast<decltype(previewFunctions.FramebufferTexture2D)>(glfwGetProcAddress("glFramebufferTexture2D"));
        previewFunctions.DrawBuffers = reinterpret_cast<decltype(previewFunctions.DrawBuffers)>(glfwGetProcAddress("glDrawBuffers"));
        previewFunctions.ReadBuffer = reinterpret_cast<decltype(previewFunctions.ReadBuffer)>(glfwGetProcAddress("glReadBuffer"));
        previewFunctions.ReadPixels = reinterpret_cast<decltype(previewFunctions.ReadPixels)>(glfwGetProcAddress("glReadPixels"));
        previewFunctions.GenTextures = reinterpret_cast<decltype(previewFunctions.GenTextures)>(glfwGetProcAddress("glGenTextures"));
        previewFunctions.DeleteTextures = reinterpret_cast<decltype(previewFunctions.DeleteTextures)>(glfwGetProcAddress("glDeleteTextures"));
        previewFunctions.BindTexture = reinterpret_cast<decltype(previewFunctions.BindTexture)>(glfwGetProcAddress("glBindTexture"));
        previewFunctions.TexImage2D = reinterpret_cast<decltype(previewFunctions.TexImage2D)>(glfwGetProcAddress("glTexImage2D"));
        previewFunctions.TexParameteri = reinterpret_cast<decltype(previewFunctions.TexParameteri)>(glfwGetProcAddress("glTexParameteri"));
        previewFunctions.GenVertexArrays = reinterpret_cast<decltype(previewFunctions.GenVertexArrays)>(glfwGetProcAddress("glGenVertexArrays"));
        previewFunctions.DeleteVertexArrays = reinterpret_cast<decltype(previewFunctions.DeleteVertexArrays)>(glfwGetProcAddress("glDeleteVertexArrays"));
        previewFunctions.BindVertexArray = reinterpret_cast<decltype(previewFunctions.BindVertexArray)>(glfwGetProcAddress("glBindVertexArray"));
        previewFunctions.GenBuffers = reinterpret_cast<decltype(previewFunctions.GenBuffers)>(glfwGetProcAddress("glGenBuffers"));
        previewFunctions.DeleteBuffers = reinterpret_cast<decltype(previewFunctions.DeleteBuffers)>(glfwGetProcAddress("glDeleteBuffers"));
        previewFunctions.BindBuffer = reinterpret_cast<decltype(previewFunctions.BindBuffer)>(glfwGetProcAddress("glBindBuffer"));
        previewFunctions.BufferData = reinterpret_cast<decltype(previewFunctions.BufferData)>(glfwGetProcAddress("glBufferData"));
        previewFunctions.CreateShader = reinterpret_cast<decltype(previewFunctions.CreateShader)>(glfwGetProcAddress("glCreateShader"));
        previewFunctions.ShaderSource = reinterpret_cast<decltype(previewFunctions.ShaderSource)>(glfwGetProcAddress("glShaderSource"));
        previewFunctions.CompileShader = reinterpret_cast<decltype(previewFunctions.CompileShader)>(glfwGetProcAddress("glCompileShader"));
        previewFunctions.GetShaderiv = reinterpret_cast<decltype(previewFunctions.GetShaderiv)>(glfwGetProcAddress("glGetShaderiv"));
        previewFunctions.DeleteShader = reinterpret_cast<decltype(previewFunctions.DeleteShader)>(glfwGetProcAddress("glDeleteShader"));
        previewFunctions.CreateProgram = reinterpret_cast<decltype(previewFunctions.CreateProgram)>(glfwGetProcAddress("glCreateProgram"));
        previewFunctions.AttachShader = reinterpret_cast<decltype(previewFunctions.AttachShader)>(glfwGetProcAddress("glAttachShader"));
        previewFunctions.LinkProgram = reinterpret_cast<decltype(previewFunctions.LinkProgram)>(glfwGetProcAddress("glLinkProgram"));
        previewFunctions.GetProgramiv = reinterpret_cast<decltype(previewFunctions.GetProgramiv)>(glfwGetProcAddress("glGetProgramiv"));
        previewFunctions.DeleteProgram = reinterpret_cast<decltype(previewFunctions.DeleteProgram)>(glfwGetProcAddress("glDeleteProgram"));
        previewFunctions.UseProgram = reinterpret_cast<decltype(previewFunctions.UseProgram)>(glfwGetProcAddress("glUseProgram"));
        previewFunctions.GetUniformLocation = reinterpret_cast<decltype(previewFunctions.GetUniformLocation)>(glfwGetProcAddress("glGetUniformLocation"));
        previewFunctions.UniformMatrix4fv = reinterpret_cast<decltype(previewFunctions.UniformMatrix4fv)>(glfwGetProcAddress("glUniformMatrix4fv"));
        previewFunctions.Uniform2ui = reinterpret_cast<decltype(previewFunctions.Uniform2ui)>(glfwGetProcAddress("glUniform2ui"));
        previewFunctions.EnableVertexAttribArray = reinterpret_cast<decltype(previewFunctions.EnableVertexAttribArray)>(glfwGetProcAddress("glEnableVertexAttribArray"));
        previewFunctions.VertexAttribPointer = reinterpret_cast<decltype(previewFunctions.VertexAttribPointer)>(glfwGetProcAddress("glVertexAttribPointer"));
        previewFunctions.DrawElements = reinterpret_cast<decltype(previewFunctions.DrawElements)>(glfwGetProcAddress("glDrawElements"));
        previewFunctions.Viewport = reinterpret_cast<decltype(previewFunctions.Viewport)>(glfwGetProcAddress("glViewport"));
        previewFunctions.ClearBufferfv = reinterpret_cast<decltype(previewFunctions.ClearBufferfv)>(glfwGetProcAddress("glClearBufferfv"));
        previewFunctions.ClearBufferuiv = reinterpret_cast<decltype(previewFunctions.ClearBufferuiv)>(glfwGetProcAddress("glClearBufferuiv"));
        previewFunctions.Enable = reinterpret_cast<decltype(previewFunctions.Enable)>(glfwGetProcAddress("glEnable"));
        previewFunctions.Disable = reinterpret_cast<decltype(previewFunctions.Disable)>(glfwGetProcAddress("glDisable"));
        previewFunctions.DepthFunc = reinterpret_cast<decltype(previewFunctions.DepthFunc)>(glfwGetProcAddress("glDepthFunc"));
        previewFunctions.DepthMask = reinterpret_cast<decltype(previewFunctions.DepthMask)>(glfwGetProcAddress("glDepthMask"));
        previewFunctions.PolygonMode = reinterpret_cast<decltype(previewFunctions.PolygonMode)>(glfwGetProcAddress("glPolygonMode"));
        h.s.editors.Initialize();
        h.s.editors.animationPage=animationPage;
        std::snprintf(h.s.iconSearch,sizeof(h.s.iconSearch),"%s",iconSearch.c_str());
        if (!h.s.editors.previewRenderer.Init(previewFunctions, 640, 480))
            throw std::runtime_error("Preview initialization failed");
        if (capturePage >= 0)
            h.s.page = capturePage;
        h.Settle();
        if (h.automated) {
            std::filesystem::create_directories(out);
            if (verify)
                Verify(h, out);
            if (benchmarkEditors) BenchmarkEditors(h,out);
            if (verifyTrackControls) VerifyTrackControls(h,out);
            if (verifyMonitors) VerifyMonitors(h,out);
            if (verifyEditors)
                VerifyEditors(h, out, previewFunctions);
            if (verifyColor)
                VerifyColor(h, out);
            if (verifyIcons)
                VerifyIcons(h, out);
            if (capture) {
                if (capturePage == -2) h.s.editors.Dataset(false);
                for (int dark = 0; dark < 2; ++dark) {
                    h.s.dark = dark != 0;
                    h.s.theme = imkit::MakePrecisionTheme(dark ? imkit::ColorScheme::Dark
                                                               : imkit::ColorScheme::Light);
                    for (int page = 0; page < 10; ++page) {
                        if (capturePage == -2 && page < 7) continue;
                        if (capturePage >= 0 && capturePage != page)
                            continue;
                        h.Page(page);
                        h.Frame({},
                                out / ("page-" + std::to_string(page) + (dark ? "-dark.png" : "-light.png")));
                        if (page == 6 || page >= 8) {
                            h.s.scale = 1.5f; h.Settle();
                            h.Frame({}, out / ("page-"+std::to_string(page)+(dark ? "-dark-150.png" : "-light-150.png")));
                            h.s.scale = 1; h.Settle();
                        }
                        if (page == 6) {
                            h.s.iconSizeIndex = 0;
                            h.Settle();
                            h.Frame({}, out / (dark ? "icons-16-dark.png" : "icons-16-light.png"));
                            h.s.iconSizeIndex = 1;
                        }
                    }
                }
                if (capturePage == -1 || capturePage == 0) {
                    h.s.scale = 1.5f;
                    h.Page(0);
                    h.Frame({}, out / "page-0-dark-150.png");
                    h.s.scale = 1;
                }
                if (capturePage == -1 || capturePage == 4) {
                    h.Page(4);
                    h.Click("modal");
                    h.Settle(10);
                    h.Frame({}, out / "modal-dark.png");
                }
            }
            std::ofstream info(out / "capture-info.txt");
            info << "Renderer: " << glGetString(GL_RENDERER) << "\nOpenGL: " << glGetString(GL_VERSION)
                 << "\nDear ImGui: " << ImGui::GetVersion()
                 << "\nActual OpenGL backbuffer before swap, 1920x1440. Icons use ImageGen source assets.\nInput verifier "
                    "uses public IO; native IME not tested.\n";
        } else
            while (!glfwWindowShouldClose(h.window))
                h.Frame();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Catalog: %s\n", e.what());
        result = 1;
    }
    h.s.editors.previewRenderer.Shutdown();
    if (texture)
        glDeleteTextures(1, &texture);
    h.s.icons.Clear();
    glDeleteTextures(6, iconTextures.data());
    if (renderer)
        ImGui_ImplOpenGL3_Shutdown();
    if (backend)
        ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImGui::SetAllocatorFunctions(h.originalAlloc,h.originalFree,h.originalAllocatorUser);
    glfwDestroyWindow(h.window);
    glfwTerminate();
    CoUninitialize();
    return result;
}
