#include "gallery_host.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <objbase.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "../design_gallery/capture.h"
#include "allocation_probe.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

namespace imkit::gallery::host {
namespace {
int lastGlfwErrorCode = GLFW_NO_ERROR;
} // namespace
void Host::Frame(const std::function<void(ImGuiIO &)> &input, const std::filesystem::path &shot) {
    glfwPollEvents();
    const bool wantsCustomFrame=s.framePreset!=imkit::WindowFramePreset::Native;
    if(wantsCustomFrame && !windowFrame.Attached()) windowFrame.Attach(glfwGetWin32Window(window));
    if(!wantsCustomFrame && windowFrame.Attached()) windowFrame.Detach();
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
    imkit::WindowFrameState frameState{};
    imkit::WindowFrameLayout frameLayout{};
    if(windowFrame.Attached()) {
        RECT client{};GetClientRect(glfwGetWin32Window(window),&client);
        frameState=windowFrame.State();
        frameLayout=imkit::LayoutWindowFrame(static_cast<float>(client.right),s.frameStyle,frameState);
        windowFrame.SetLayout(frameLayout);
        s.windowFrameHeight=frameLayout.titleBar.max.y;
    } else s.windowFrameHeight=0;
    if (iconArtworkShowcase) {
        DrawIconArtworkShowcase(*this);
    } else {
        imkit::gallery::Show(s);
        imkit::gallery::ShowComparison(s);
    }
    if(s.floatingComparison && s.showDearImGuiDemo) {
        ImGui::ShowDemoWindow(&s.showDearImGuiDemo);
        if(!s.demoWindowPositioned) {
            constexpr float margin=24.f;
            const auto display=ImGui::GetIO().DisplaySize;
            ImVec2 position{s.floatingCatalogPosition.x+48.f,
                            s.floatingCatalogPosition.y+48.f};
            if(display.x>=1200.f)
                position={s.floatingCatalogPosition.x+s.floatingCatalogSize.x+margin,
                          s.windowFrameHeight+margin};
            ImGui::SetWindowPos("Dear ImGui Demo",position,ImGuiCond_Always);
            ImGui::SetWindowFocus("Dear ImGui Demo");
            s.demoWindowPositioned=true;
        }
    }
    imkit::WindowFrameEvent frameEvent{};
    if(windowFrame.Attached()) {
        const imkit::WindowFrameContent content{"ImKit",windowTitle,s.frameUnsaved,s.frameWorkspaces,s.frameWorkspace};
        const auto frame=imkit::DrawWindowFrame(s.frameStyle,content,frameLayout,frameState);
        frameEvent=frame.event;
        if(frameEvent.type==imkit::WindowFrameEventType::WorkspaceSelected) s.frameWorkspace=frameEvent.workspace;
    }
    s.editors.RenderPreview();
    if(motion) motion->Cursor();
    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(.1f, .1f, .1f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if(ImGui::GetIO().ConfigFlags&ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(window);
    }
    if (!shot.empty())
        imkit::design::SaveBackbuffer(shot, w, h);
    if(motion) motion->Capture(w,h);
    glfwSwapBuffers(window);
    if(frameEvent.type==imkit::WindowFrameEventType::Operation) windowFrame.Execute(frameEvent.operation);
}
void Host::Settle(int n) {
    while (n--)
        Frame();
}
void Host::Page(int page) {
    s.page = page;
    mouse = {-100, -100};
    Settle();
}
void Host::ClickAt(ImVec2 pos) {
    mouse = pos;
    Frame();
    Frame([](auto &io) { io.AddMouseButtonEvent(0, true); });
    Frame([](auto &io) { io.AddMouseButtonEvent(0, false); });
    Settle(2);
}
// Centre of a probe, scrolling the page with the wheel until it is inside the window.
ImVec2 Host::Reveal(const char *name) {
    const auto size=ImGui::GetIO().DisplaySize;
    for(int attempt=0;;++attempt) {
        const auto it=s.probes.find(name);
        if(it==s.probes.end()) {
            std::string available;
            for(const auto &[key, _] : s.probes) {
                if(!available.empty()) available+=", ";
                available+=key;
            }
            throw std::runtime_error(std::string("missing Gallery probe: ")+name+"; available: "+available);
        }
        const auto centre=it->second.Center();
        if(attempt==40 || (centre.y>=0 && centre.y<size.y-24 && centre.x>=0 && centre.x<size.x)) return centre;
        mouse={size.x*.6f,size.y*.5f};
        Frame([&](auto &io) { io.AddMouseWheelEvent(0, centre.y<0 ? 3.f : -3.f); });
        Settle(2);
    }
}
void Host::Click(const char *name) { ClickAt(Reveal(name)); }
void Host::Key(ImGuiKey key) {
    Frame([&](auto &io) { io.AddKeyEvent(key, true); });
    Frame([&](auto &io) { io.AddKeyEvent(key, false); });
    Settle(2);
}
void Host::Replace(const char *probe, const char *text) {
    Click(probe);
    ReplaceActive(text);
}
void Host::ReplaceActive(const char *text) {
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
GLuint LoadIconArtworkTexture(IWICImagingFactory *factory, const std::filesystem::path &path) {
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                   WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf())))
        throw std::runtime_error("Unable to decode icon artwork PNG");
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())))
        throw std::runtime_error("Unable to read icon artwork PNG frame");
    ComPtr<IWICBitmapScaler> scaler;
    if (FAILED(factory->CreateBitmapScaler(scaler.GetAddressOf())) ||
        FAILED(scaler->Initialize(frame.Get(), 128, 128, WICBitmapInterpolationModeFant)))
        throw std::runtime_error("Unable to resize icon artwork PNG");
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) ||
        FAILED(converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppRGBA,
                                     WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom)))
        throw std::runtime_error("Unable to convert icon artwork PNG");
    constexpr UINT width = 128, height = 128, stride = width * 4;
    std::vector<unsigned char> pixels(stride * height);
    if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data())))
        throw std::runtime_error("Unable to copy icon artwork pixels");
    for (std::size_t offset = 0; offset < pixels.size(); offset += 4)
        pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = 255;
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    constexpr GLint clampToEdge = 0x812F;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}
void LoadIconArtworkShowcase(Host &h) {
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(factory.GetAddressOf()))))
        throw std::runtime_error("Unable to initialize Windows Imaging Component");
    const auto sourceDirectory = std::filesystem::current_path() / "assets" / "icons" / "originals";
    if (!std::filesystem::is_directory(sourceDirectory))
        throw std::runtime_error("Run icon-artwork capture from the repository root");
    const auto catalog = imkit::GetIconCatalog();
    const std::array<std::pair<std::string_view, std::string_view>, 5> groups{{
        {"3D Projection", "Projectors, camera frusta, optics, and projection surfaces."},
        {"Production", "Production controls for layers, monitors, movement, and alignment."},
        {"3D Objects", "Scene primitives, collections, and reusable 3D objects."},
        {"3D Lighting", "Lighting rigs and spatial references for 3D scenes."},
        {"Editor", "Timeline, curve, selection, and geometry tools."},
    }};
    constexpr std::array<std::string_view, 10> editorHighlights{
        "AnimationTimeline", "CurveEditor", "DopeSheet", "Marker", "ProgramMonitor",
        "SafeArea", "SourceMonitor", "Timecode", "Transition", "Waveform"};
    std::set<std::string> sourceCategories;
    std::set<std::string> catalogCategories;
    for (const auto &item : catalog) {
        catalogCategories.emplace(item.category);
        const auto path = sourceDirectory / (std::string(item.name) + ".png");
        if (std::filesystem::exists(path))
            sourceCategories.emplace(item.category);
    }
    h.iconArtworkSourceCount = 0;
    h.iconArtworkSourceCategoryCount = static_cast<int>(sourceCategories.size());
    h.iconArtworkCatalogCategoryCount = static_cast<int>(catalogCategories.size());
    for (const auto &item : catalog)
        if (std::filesystem::exists(sourceDirectory / (std::string(item.name) + ".png")))
            ++h.iconArtworkSourceCount;
    for (const auto &[category, description] : groups) {
        h.iconArtworkSlides.emplace_back();
        auto &slide = h.iconArtworkSlides.back();
        slide.category = category;
        slide.description = description;
        for (const auto &item : catalog) {
            if (std::string_view(item.category) != category)
                continue;
            ++slide.presetCount;
            const auto path = sourceDirectory / (std::string(item.name) + ".png");
            if (!std::filesystem::exists(path))
                continue;
            ++slide.sourceImageCount;
            if (category == "Editor" &&
                std::find(editorHighlights.begin(), editorHighlights.end(), item.name) == editorHighlights.end())
                continue;
            slide.tiles.push_back({std::string(item.name), 0});
            slide.tiles.back().texture = LoadIconArtworkTexture(factory.Get(), path);
        }
        if (slide.tiles.empty())
            throw std::runtime_error("Icon artwork showcase category has no source PNGs");
    }
}
void DestroyIconArtworkShowcase(Host &h) {
    for (const auto &slide : h.iconArtworkSlides)
        for (const auto &tile : slide.tiles)
            if (tile.texture)
                glDeleteTextures(1, &tile.texture);
    h.iconArtworkSlides.clear();
}
void DrawIconArtworkShowcase(Host &h) {
    auto &s = h.s;
    s.theme.fonts = s.fonts;
    imkit::ThemeScope theme(s.theme, s.scale);
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(display, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {28, 22});
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (ImGui::Begin("##icon-artwork-showcase", nullptr, flags)) {
        ImGui::TextDisabled("IMKIT  /  PRESET ICON ARTWORK");
        ImGui::PushFont(s.fonts.emphasis, 27);
        ImGui::TextUnformatted("Ready-made imagery for creative tools");
        ImGui::PopFont();
        ImGui::Text("%d source PNGs  ·  %d image categories  ·  %d presets in %d catalog categories",
                    h.iconArtworkSourceCount, h.iconArtworkSourceCategoryCount,
                    static_cast<int>(imkit::GetIconCatalog().size()), h.iconArtworkCatalogCategoryCount);
        ImGui::Spacing();
        for (int index = 0; index < static_cast<int>(h.iconArtworkSlides.size()); ++index) {
            if (index)
                ImGui::SameLine();
            const auto &slide = h.iconArtworkSlides[static_cast<std::size_t>(index)];
            char label[96];
            std::snprintf(label, sizeof(label), "%s  %d", slide.category.c_str(), slide.presetCount);
            const bool selected = h.iconArtworkSlide == index;
            const auto button = selected ? s.theme.colors.accent : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
            const auto foreground = selected ? s.theme.colors.onAccent : ImGui::GetStyleColorVec4(ImGuiCol_Text);
            ImGui::PushStyleColor(ImGuiCol_Button, button);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, button);
            ImGui::PushStyleColor(ImGuiCol_Text, foreground);
            if (ImGui::Button(label, {0, 28}))
                h.iconArtworkSlide = index;
            ImGui::PopStyleColor(4);
        }
        const auto &slide = h.iconArtworkSlides.at(static_cast<std::size_t>(h.iconArtworkSlide));
        ImGui::Separator();
        ImGui::PushFont(s.fonts.emphasis, 19);
        ImGui::TextUnformatted(slide.category.c_str());
        ImGui::PopFont();
        ImGui::SameLine();
        if (static_cast<int>(slide.tiles.size()) < slide.sourceImageCount)
            ImGui::TextDisabled("%d presets  ·  %d PNG artworks  ·  %d featured",
                                slide.presetCount, slide.sourceImageCount, static_cast<int>(slide.tiles.size()));
        else
            ImGui::TextDisabled("%d presets  ·  %d PNG artworks", slide.presetCount, slide.sourceImageCount);
        ImGui::TextDisabled("%s", slide.description.c_str());
        ImGui::Spacing();
        if (ImGui::BeginTable("##icon-artwork-grid", 5,
                              ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV)) {
            const int activeTile = static_cast<int>((ImGui::GetFrameCount() / 3) % slide.tiles.size());
            for (int index = 0; index < static_cast<int>(slide.tiles.size()); ++index) {
                const auto &tile = slide.tiles[static_cast<std::size_t>(index)];
                ImGui::TableNextColumn();
                ImGui::PushID(tile.name.c_str());
                const bool active = index == activeTile;
                const ImVec4 card = ImGui::GetStyleColorVec4(active ? ImGuiCol_Header : ImGuiCol_FrameBg);
                const ImVec4 border = ImGui::GetStyleColorVec4(active ? ImGuiCol_CheckMark : ImGuiCol_Border);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, card);
                ImGui::PushStyleColor(ImGuiCol_Border, border);
                if (ImGui::BeginChild("##asset-tile", {0, 174}, ImGuiChildFlags_Borders,
                                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                    constexpr float imageSize = 112;
                    const float imageX = std::max(0.0f, (ImGui::GetContentRegionAvail().x - imageSize) * .5f);
                    ImGui::SetCursorPosX(imageX);
                    ImGui::Image(ImTextureRef(static_cast<ImTextureID>(tile.texture)), {imageSize, imageSize});
                    ImGui::Spacing();
                    ImGui::SetCursorPosX(10);
                    ImGui::PushFont(s.fonts.emphasis, 14);
                    ImGui::TextUnformatted(tile.name.c_str());
                    ImGui::PopFont();
                }
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}
} // namespace imkit::gallery::host

using namespace imkit::gallery::host;
using enum imkit::gallery::PageId;

int main(int argc, char **argv) {
    int frameMode=-1;
    bool verifyWindowFrame=false;
    std::string windowTitle="ImKit Precision Layers";
    bool verifyWorkflow=false;
    bool verifyToasts=false;
    bool captureDesign=false;
    bool capture = false, verify = false, verifyIcons = false, verifyEditors = false, verifyColor = false, benchmarkEditors = false, verifyMonitors = false, verifyTrackControls = false, verifyLinkedClips = false, verifyNormals = false;
    bool verifyComparison=false, captureReadme=false;
    int capturePage = -1, animationPage = -1, monitorIndex=-1, captureWidth=1920,captureHeight=1440;
    bool captureJapanese=false;
    bool verifyTimelineUI=false;
    bool listMonitors=false;
    std::string iconSearch, captureDemo;
    bool captureMotion=false;
    std::filesystem::path out = "out/catalog";
    std::filesystem::path themeFile, captureLook; // --theme-file applies a theme text; --capture-look saves one frame of a page.
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if(a=="--window-frame" && i+1<argc) {
            const std::string mode=argv[++i];
            if(mode!="modern" && mode!="native") return 2;
            frameMode=mode=="modern"?1:0; continue;
        }
        if(a=="--window-title" && i+1<argc) {windowTitle=argv[++i];continue;}
        if(a=="--verify-window-frame") {verifyWindowFrame=true;frameMode=1;continue;}
        if(a=="--verify-workflow") {verifyWorkflow=true;capture=true;continue;}
        if(a=="--verify-toasts") {verifyToasts=true;capture=true;capturePage=PageToasts;continue;}
        if(a=="--capture-design-system") {captureDesign=true; capture=true; continue;}
        if(a=="--verify-comparison") {verifyComparison=true;continue;}
        if(a=="--capture-readme") {captureDemo="overview";captureReadme=true;continue;}
        if((a=="--capture-demo"||a=="--capture-motion") && i+1<argc) {
            captureMotion=a=="--capture-motion";
            captureDemo=argv[++i];
            if(captureDemo!="overview" && captureDemo!="comparison" && captureDemo!="themes" &&
               captureDemo!="icons" && captureDemo!="icon-artwork" && captureDemo!="vector" && captureDemo!="workflow" &&
               captureDemo!="progress" && captureDemo!="timeline" && captureDemo!="components" &&
               captureDemo!="preview-contract" && captureDemo!="workspace" && captureDemo!="toasts" && !HasMotionScene(captureDemo)) return 2;
            continue;
        }
        if (a == "--verify-timeline-model") return VerifyTimelineModel();
        if (a == "--verify-inspector-model")
            return VerifyInspectorModel();
        if (a == "--capture")
            capture = true;
        else if (a == "--verify")
            verify = true;
        else if (a == "--capture-editors") { capture = true; capturePage = -2; }
        else if (a == "--verify-normals") verifyNormals=true;
        else if (a == "--verify-linked-clips") verifyLinkedClips=true;
        else if (a == "--verify-track-controls") verifyTrackControls=true;
        else if (a == "--verify-timeline-ui") {verifyTimelineUI=true;verifyTrackControls=true;}
        else if (a == "--verify-monitors") verifyMonitors=true;
        else if (a == "--benchmark-editors") benchmarkEditors=true;
        else if (a == "--verify-editors")
            verifyEditors = true;
        else if (a == "--verify-icons")
            verifyIcons = true;
        else if (a == "--verify-color")
            verifyColor = true;
        else if(a=="--list-monitors") listMonitors=true;
        else if(a=="--monitor" && i+1<argc) monitorIndex=std::stoi(argv[++i]);
        else if(a=="--width" && i+1<argc) captureWidth=std::clamp(std::stoi(argv[++i]),640,7680);
        else if(a=="--height" && i+1<argc) captureHeight=std::clamp(std::stoi(argv[++i]),480,4320);
        else if(a=="--japanese") captureJapanese=true;
        else if (a == "--output" && i + 1 < argc)
            out = argv[++i];
        else if (a == "--theme-file" && i + 1 < argc)
            themeFile = argv[++i];
        else if (a == "--capture-look" && i + 1 < argc) {
            captureLook = argv[++i];
            capture = true;
        }
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
    const auto comResult=CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult)) {
        std::fprintf(stderr,"Catalog: COM initialization failed (0x%08lx)\n",static_cast<unsigned long>(comResult));
        return 1;
    }
    glfwSetErrorCallback([](int code,const char *description) {
        lastGlfwErrorCode=code;
        std::fprintf(stderr,"Catalog: GLFW error %d: %s\n",code,description ? description : "unknown");
    });
    if (!glfwInit()) {
        CoUninitialize();
        return verify && lastGlfwErrorCode==GLFW_API_UNAVAILABLE ? 77 : 1;
    }
    if(listMonitors || monitorIndex>=0) {
        int count=0;auto monitors=glfwGetMonitors(&count);
        for(int i=0;i<count;++i) {
            int x=0,y=0;glfwGetMonitorPos(monitors[i],&x,&y);
            const char *adapter=glfwGetWin32Adapter(monitors[i]);std::string adapterName;
            DISPLAY_DEVICEA device{};device.cb=sizeof(device);
            for(DWORD index=0;EnumDisplayDevicesA(nullptr,index,&device,0);++index)
                if(adapter && std::strcmp(adapter,device.DeviceName)==0) {adapterName=device.DeviceString;break;}
            if(listMonitors) std::printf("%d: %s | %s | position %d,%d\n",i,glfwGetMonitorName(monitors[i]),adapterName.c_str(),x,y);
            if(i==monitorIndex) {glfwWindowHint(GLFW_POSITION_X,x+32);glfwWindowHint(GLFW_POSITION_Y,y+32);}
        }
        if(listMonitors || monitorIndex>=count) {glfwTerminate();CoUninitialize();return listMonitors?0:2;}
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Show only after the custom frame and first rendered frame are ready.
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    // A decorated window is clamped below the monitor height; a motion capture needs the exact framebuffer.
    if(captureMotion) glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    auto hostStorage=std::make_unique<Host>();
    auto &h=*hostStorage;
        h.automated = capture || verify || verifyIcons || verifyEditors || verifyColor || benchmarkEditors || verifyMonitors || verifyTrackControls || verifyLinkedClips || verifyNormals || verifyComparison || !captureDemo.empty();
    h.s.floatingComparison=false;
    h.windowTitle=windowTitle;
    h.window = glfwCreateWindow(captureWidth, captureHeight,windowTitle.c_str(), nullptr, nullptr);
    if (!h.window) {
        const bool openGLUnavailable=lastGlfwErrorCode==GLFW_API_UNAVAILABLE;
        glfwTerminate();
        CoUninitialize();
        if(verify && openGLUnavailable)
            std::fprintf(stderr,"Gallery verification skipped: OpenGL is unavailable on this runner.\n");
        return verify && openGLUnavailable ? 77 : 1;
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
    if(!h.automated) io.ConfigFlags|=ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDpiScaleFonts=!h.automated;
    io.ConfigDpiScaleViewports=!h.automated;
    bool backend = ImGui_ImplGlfw_InitForOpenGL(h.window, true),
         renderer = backend && ImGui_ImplOpenGL3_Init("#version 130");
    if(!h.automated) {
        h.s.theme=imkit::MakeTheme(imkit::ThemePreset::Graphite);
        h.s.presetIndex=2; h.s.dark=true;
    }
    h.s.framePreset=(frameMode==0 || (frameMode<0 && h.automated))?imkit::WindowFramePreset::Native:imkit::WindowFramePreset::Studio;
    h.s.framePresetStyles={imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Native,h.s.theme),
                           imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Studio,h.s.theme),
                           imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Workspace,h.s.theme),
                           imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Tool,h.s.theme)};
    h.s.frameStyle=imkit::MakeWindowFrameStyle(h.s.framePreset,h.s.theme);
    int result = 0;
    GLuint texture = 0;
    std::array<GLuint, 7> iconTextures{};
    std::array<GLuint, 6> imageTextures{};
    GLuint spriteTexture = 0;
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
                !io.Fonts->AddFontFromFileTTF(jp.string().c_str(), 15.4f, &config))
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
        // Six generated pictures (no third-party artwork) for the picture-button pages.
        glGenTextures(6, imageTextures.data());
        for (int kind = 0; kind < 6; ++kind) {
            constexpr int N = 96;
            std::vector<unsigned char> px(static_cast<std::size_t>(N * N * 4));
            for (int y = 0; y < N; ++y)
                for (int x = 0; x < N; ++x) {
                    const float fx = static_cast<float>(x) / (N - 1), fy = static_cast<float>(y) / (N - 1);
                    const float dx = fx - .5f, dy = fy - .5f, d = std::sqrt(dx * dx + dy * dy);
                    float r = 0, g = 0, b = 0;
                    switch (kind) {
                    case 0: { // Sunset
                        r = .98f - .6f * fy; g = .66f - .5f * fy; b = .31f + .25f * fy;
                        const float sun = std::sqrt(dx * dx + (fy - .62f) * (fy - .62f));
                        if (sun < .17f) { r = 1.f; g = .93f; b = .6f; }
                        break;
                    }
                    case 1: { // Waves
                        const float wave = .5f + .5f * std::sin(fx * 18.f + fy * 9.f);
                        r = .05f + .1f * wave; g = .35f + .3f * wave * (1.f - fy * .5f); b = .55f + .3f * wave;
                        break;
                    }
                    case 2: { // Neon grid
                        const bool line = (x % 12 == 0) || (y % 12 == 0);
                        r = line ? .95f : .06f; g = line ? .25f : .05f; b = line ? .85f : .16f;
                        break;
                    }
                    case 3: { // Stripes
                        const bool stripe = ((x + y) / 8) % 2 == 0;
                        r = stripe ? .25f : .75f; g = stripe ? .75f : .95f; b = stripe ? .3f : .45f;
                        break;
                    }
                    case 4: { // Radial
                        const float t = std::min(1.f, d * 2.f);
                        r = 1.f - .15f * t; g = .95f - .85f * t; b = .85f - .8f * t;
                        break;
                    }
                    default: { // Checker with a disc
                        const bool check = ((x / 12) + (y / 12)) % 2 == 0;
                        r = check ? .95f : .45f; g = check ? .55f : .2f; b = check ? .15f : .75f;
                        if (d < .22f) { r = .98f; g = .98f; b = .98f; }
                        break;
                    }
                    }
                    const std::size_t o = static_cast<std::size_t>((y * N + x) * 4);
                    px[o] = static_cast<unsigned char>(std::clamp(r, 0.f, 1.f) * 255.f);
                    px[o + 1] = static_cast<unsigned char>(std::clamp(g, 0.f, 1.f) * 255.f);
                    px[o + 2] = static_cast<unsigned char>(std::clamp(b, 0.f, 1.f) * 255.f);
                    px[o + 3] = 255;
                }
            glBindTexture(GL_TEXTURE_2D, imageTextures[static_cast<std::size_t>(kind)]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            h.s.sampleImages[static_cast<std::size_t>(kind)] = ImTextureRef(static_cast<ImTextureID>(imageTextures[static_cast<std::size_t>(kind)]));
        }
        // A generated sprite sheet (no third-party artwork): 12 frames of a bouncing ball, 4 columns by 3 rows of 64 px.
        {
            constexpr int Cell = 64, Columns = 4, Rows = 3, Frames = 12;
            constexpr int W = Cell * Columns, H = Cell * Rows;
            std::vector<unsigned char> px(static_cast<std::size_t>(W * H * 4), 0);
            for (int frame = 0; frame < Frames; ++frame) {
                const float t = static_cast<float>(frame) / Frames;
                const float lift = std::fabs(std::sin(t * 3.14159265f)) * 22.f; // Jump height.
                const float squash = 1.f + .22f * (1.f - std::min(1.f, lift / 6.f)); // Flattens on the ground.
                const float rx = 14.f * squash, ry = 14.f / squash;
                const float cx = Cell * .5f, cy = Cell - 12.f - lift - ry * .2f;
                for (int y = 0; y < Cell; ++y)
                    for (int x = 0; x < Cell; ++x) {
                        float r = 0, g = 0, b = 0, a = 0;
                        const float sx = (x - cx) / (12.f + lift * .15f), sy = (y - (Cell - 8.f)) / 3.f; // Ground shadow.
                        const float shadow = std::max(0.f, 1.f - (sx * sx + sy * sy));
                        if (shadow > 0.f) { a = shadow * .35f; }
                        const float ex = (x - cx) / rx, ey = (y - cy) / ry, e = ex * ex + ey * ey;
                        if (e < 1.f) {
                            const float shade = .75f + .25f * (-ex * .5f - ey * .8f);
                            r = (.95f * shade); g = (.62f * shade); b = (.18f * shade); a = 1.f;
                            if (e > .8f) { r *= .7f; g *= .6f; b *= .5f; } // Rim.
                        }
                        const int ox = (frame % Columns) * Cell + x, oy = (frame / Columns) * Cell + y;
                        const std::size_t o = static_cast<std::size_t>((oy * W + ox) * 4);
                        px[o] = static_cast<unsigned char>(std::clamp(r, 0.f, 1.f) * 255.f);
                        px[o + 1] = static_cast<unsigned char>(std::clamp(g, 0.f, 1.f) * 255.f);
                        px[o + 2] = static_cast<unsigned char>(std::clamp(b, 0.f, 1.f) * 255.f);
                        px[o + 3] = static_cast<unsigned char>(std::clamp(a, 0.f, 1.f) * 255.f);
                    }
            }
            glGenTextures(1, &spriteTexture);
            glBindTexture(GL_TEXTURE_2D, spriteTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            h.s.spriteSheet = ImTextureRef(static_cast<ImTextureID>(spriteTexture));
        }
        glGenTextures(7, iconTextures.data());
        for (int i = 0; i < 7; ++i) {
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
        if (!themeFile.empty()) {
            h.s.look.watch = !h.automated; // Edit the file in any editor and the open Gallery follows.
            if (!h.s.look.LoadFile(h.s, themeFile))
                throw std::runtime_error("Cannot read theme file " + themeFile.string());
        }
        h.Settle();
        if(!h.automated) glfwShowWindow(h.window);
        if(verifyWindowFrame) {
            std::filesystem::create_directories(out);
            VerifyWindowFrame(h,out);
        } else if (h.automated) {
            std::filesystem::create_directories(out);
            if(verifyToasts) {
                h.s.comparison.open=false;h.Page(PageToasts);
                auto& demo=h.s.toasts;
                auto require=[](bool ok,const char* what){if(!ok) throw std::runtime_error(what);};
                require(demo.items.size()==4,"toast demo seed");
                // Public mouse and keyboard IO through the real Gallery host.
                const auto retry=std::find_if(h.s.probes.begin(),h.s.probes.end(),[](const auto& probe){return probe.first.starts_with("toast-action-")&&probe.first!="toast-action-0";});
                require(retry!=h.s.probes.end(),"toast action probe");
                h.Click(retry->first.c_str());require(demo.actionCount==1,"toast mouse action");
                h.Key(ImGuiKey_Space);require(demo.actionCount==2,"toast keyboard action");
                h.Click("toast-complete");
                require(std::none_of(demo.items.begin(),demo.items.end(),[](auto& v){return v.phase==imkit::ToastPhase::Loading;}),"toast loading completion");
                const auto afterHostClick=std::find_if(h.s.probes.begin(),h.s.probes.end(),[](const auto& probe){return probe.first.starts_with("toast-action-");});
                require(afterHostClick!=h.s.probes.end(),"toast action after host click");
                h.Click(afterHostClick->first.c_str());require(demo.actionCount==3,"toast stays interactive above host after host click");
                for(int dark=0;dark<2;++dark) {
                    h.s.theme=imkit::MakeTheme(dark?imkit::ThemePreset::PrecisionDark:imkit::ThemePreset::PrecisionLight);h.s.theme.fonts=h.s.fonts;
                    h.s.theme.motion.enabled=false;
                    h.s.dark=dark!=0;
                    for(int position=0;position<6;++position) {
                        demo.position=position;demo.state.Reset();h.Settle();
                        h.Frame({},out/("toasts-"+std::to_string(position)+(dark?"-dark.png":"-light.png")));
                    }
                }
                demo.japanese=true;demo.items.clear();demo.initialized=false;demo.state.Reset();
                demo.position=2;demo.indeterminate=true;h.Settle();
                h.Frame({},out/"toasts-japanese-loading.png");
                const auto close=std::find_if(h.s.probes.begin(),h.s.probes.end(),[](const auto& probe){return probe.first.starts_with("toast-close-");});
                require(close!=h.s.probes.end(),"toast close probe");
                const auto before=demo.items.size();h.Click(close->first.c_str());require(demo.items.size()==before-1,"toast mouse dismissal");
                std::ofstream report(out/"toasts-verification.txt");
                report<<"PASS: native OpenGL Gallery; mouse action/dismiss; keyboard Space action; same-ID loading completion; six positions in light/dark; Japanese indeterminate loading.\nPublic ImGui IO is not physical OS input, IME, screen-reader or multi-monitor DPI acceptance.\n";
            }
            if (verify)
                Verify(h, out);
            if (verifyComparison)
                VerifyComparison(h,out);
            if (benchmarkEditors) BenchmarkEditors(h,out);
            if(verifyTimelineUI) VerifyTimelineUI(h,out);
            else if (verifyTrackControls) VerifyTrackControls(h,out);
            if (verifyLinkedClips) VerifyLinkedClips(h,out);
            if (verifyNormals) {
                h.Page(PageCG);h.s.editors.viewport.normals=true;h.s.editors.viewport.faceNormals=true;
                h.s.editors.viewport.cameraFrame=true;h.s.editors.viewport.safeFrame=true;
                h.s.editors.viewport.renderRegion=true;h.s.editors.viewport.passepartout=true;
                h.s.editors.viewport.measurement=true;
                const auto meshObject=std::find_if(h.s.editors.objects.begin(),h.s.editors.objects.end(),[](const auto &o){return o.geometry!=0;});
                if (meshObject==h.s.editors.objects.end()) throw std::runtime_error("normal capture requires mesh object");
                meshObject->transform.shear={.5,.2,0};h.s.editors.objectSelection.Set(meshObject->id);h.Settle();
                h.Frame({},out/"cg-normals-light.png");
                h.s.editors.japanese=true;h.s.dark=true;h.s.theme=imkit::MakePrecisionTheme(imkit::ColorScheme::Dark);h.s.scale=1.5f;h.Settle();
                h.Frame({},out/"cg-normals-dark-150.png");
            }
            if (verifyMonitors) VerifyMonitors(h,out);
            if (verifyEditors)
                VerifyEditors(h, out, previewFunctions);
            if (verifyColor)
                VerifyColor(h, out);
            if (verifyIcons)
                VerifyIcons(h, out);
            if(verifyWorkflow) {
                std::ofstream log(out/"workflow-interaction.txt");
                auto check=[&](bool ok,const char* text){log<<(ok?"PASS ":"FAIL ")<<text<<'\n';if(!ok)throw std::runtime_error(text);};
                h.Page(PageGenericWorkspace);h.s.workflow.notice=false;h.Settle();
                bool before=h.s.workflow.chip;h.Click("workflow-chip");check(h.s.workflow.chip!=before,"filter chip click");
                // The workspace page is taller than the window; scroll until the canvas centre is on screen.
                for(int i=0;i<12&&h.s.probes.at("workflow-canvas").Center().y>ImGui::GetIO().DisplaySize.y-160;++i) {
                    h.mouse={620,500};h.Frame([](auto& io){io.AddMouseWheelEvent(0,-3);});h.Settle();
                }
                auto bounds=h.s.probes.at("workflow-canvas");h.mouse=bounds.Center();h.Frame();
                auto scale=h.s.workflow.image.canvas.scale.x;
                h.Frame([](auto& io){io.AddMouseWheelEvent(0,1);});h.Settle();
                check(h.s.workflow.image.canvas.scale.x>scale,"image cursor zoom");
                h.Key(ImGuiKey_Tab);h.Key(ImGuiKey_Space);
                for(int dark=0;dark<2;++dark)for(int contrast=0;contrast<2;++contrast)for(int density=0;density<3;++density){
                    h.s.dark=dark!=0;h.s.theme=imkit::MakeTheme(static_cast<imkit::ColorScheme>(dark),static_cast<imkit::ContrastMode>(contrast),static_cast<imkit::Density>(density));
                    h.s.theme.motion.reducedMotion=true;
                    for(int page:{PageGenericWorkspace,PageFeedback,PagePreviewTiles}){h.Page(page);h.Frame({},out/("workflow-"+std::to_string(page)+"-"+std::to_string(dark)+"-"+std::to_string(contrast)+"-"+std::to_string(density)+".png"));}
                }
                h.s.workflow.japanese=true;h.s.workflow.vertical=true;h.s.workflow.disabled=true;
                glfwSetWindowSize(h.window,640,800);h.Page(PagePreviewTiles);h.Frame({},out/"workflow-narrow-disabled.png");
                h.s.workflow.disabled=false;h.Page(PageGenericWorkspace);h.Frame({},out/"workflow-japanese.png");
                h.mouse={620,700};h.Frame([](auto& io){io.AddMouseWheelEvent(0,-20);});h.Settle(2);
                h.Frame({},out/"workflow-image-aspects.png");
                check(true,"Light/Dark, contrast, density, reduced motion, Japanese, narrow, disabled GPU captures");
                log<<"Public ImGui IO and native GPU backbuffer; native OS/IME and external applications are not tested.\n";
            } else if(!captureDemo.empty()) {
                if(captureMotion) CaptureMotion(h,out,captureDemo);
                else CaptureDemo(h,out,captureDemo,captureReadme);
            } else if(captureDesign) {
                h.s.comparison.open=false;
                for(int dark=0;dark<2;++dark) for(int contrast=0;contrast<2;++contrast) for(int density=0;density<3;++density) {
                    h.s.dark=dark!=0;
                    h.s.theme=imkit::MakeTheme(static_cast<imkit::ColorScheme>(dark),static_cast<imkit::ContrastMode>(contrast),static_cast<imkit::Density>(density));
                    h.s.design.density=density; h.s.design.contrast=contrast;
                    for(int page:{PageFoundations,PageReferenceComponents,PagePatterns,PageAccessibility,PageResponsive}) {
                        h.Page(page);
                        h.Frame({},out/("design-"+std::to_string(page)+"-"+std::to_string(dark)+"-"+std::to_string(contrast)+"-"+std::to_string(density)+".png"));
                    }
                }
                h.s.design.language=1; h.s.scale=1.5f; h.Page(PagePatterns); h.Frame({},out/"japanese-150.png");
                h.s.design.language=2; h.s.scale=2; h.Page(PageResponsive); h.Frame({},out/"rtl-200.png");
            } else if (!captureLook.empty()) {
                // One frame of one page with the theme from --theme-file, for documentation and review.
                if (captureLook.has_parent_path()) std::filesystem::create_directories(captureLook.parent_path());
                h.s.comparison.open=false;
                h.Page(capturePage >= 0 ? capturePage : PageLookStudio);
                h.Frame({}, captureLook);
            } else if (capture) {
                h.s.editors.japanese=captureJapanese;
                h.s.workflow.japanese=captureJapanese;
                if (capturePage == -2) h.s.editors.Dataset(false);
                for (int dark = 0; dark < 2; ++dark) {
                    h.s.dark = dark != 0;
                    h.s.theme = imkit::MakePrecisionTheme(dark ? imkit::ColorScheme::Dark
                                                               : imkit::ColorScheme::Light);
                    const int pageLimit=capturePage>=0?capturePage+1:10;
                    for (int page = 0; page < pageLimit; ++page) {
                        if (capturePage == -2 && page < 7) continue;
                        if (capturePage >= 0 && capturePage != page)
                            continue;
                        h.Page(page);
                        h.Frame({},
                                out / ("page-" + std::to_string(page) + (dark ? "-dark.png" : "-light.png")));
                        if(page==PagePtz) {
                            h.mouse={ImGui::GetIO().DisplaySize.x*.7f,ImGui::GetIO().DisplaySize.y*.7f};
                            h.Frame([](auto& io){io.AddMouseWheelEvent(0,-100);});h.mouse={-100,-100};h.Settle();
                            h.Frame({},out/(dark?"ptz-scrolled-dark.png":"ptz-scrolled-light.png"));
                        }
                        if(page==PageWorkbench) {
                            h.mouse={ImGui::GetIO().DisplaySize.x*.7f,ImGui::GetIO().DisplaySize.y*.7f};
                            h.Frame([](auto& io){io.AddMouseWheelEvent(0,-100);});h.mouse={-100,-100};h.Settle();
                            h.Frame({},out/(dark?"workbench-scrolled-dark.png":"workbench-scrolled-light.png"));
                        }
                        if (page == PageIcons || (page >= 8 && page<=15)) {
                            h.s.scale = 1.5f; h.Settle();
                            h.Frame({}, out / ("page-"+std::to_string(page)+(dark ? "-dark-150.png" : "-light-150.png")));
                            if(page==PageIcons || page==PageVideo) {
                                const auto oldPanel=h.s.editors.activeVideoPanel;
                                if(page==PageVideo) {h.s.editors.videoPanel=1;h.s.editors.colorCurve.fitRequested=true;h.Settle();}
                                h.mouse={page==PageVideo?h.s.editors.colorCurve.view.max.x+16:1200,1320};h.Frame([](ImGuiIO &io){io.AddMouseWheelEvent(0,-100);});h.Settle();
                                const auto scrollMouse=h.mouse;h.mouse={-100,-100};h.Settle();
                                h.Frame({},out/((page==PageIcons?"icons-bottom":"color-curves")+std::string(dark?"-dark-150.png":"-light-150.png")));
                                h.mouse=scrollMouse;h.Frame([](ImGuiIO &io){io.AddMouseWheelEvent(0,100);});h.Settle();
                                if(page==PageVideo) h.s.editors.videoPanel=oldPanel;
                            }
                            h.s.scale = 1; h.Settle();
                        }
                        if (page == PageIcons) {
                            h.s.iconSizeIndex = 0;
                            h.Settle();
                            h.Frame({}, out / (dark ? "icons-16-dark.png" : "icons-16-light.png"));
                            h.s.iconSizeIndex=2;h.Settle();
                            h.Frame({},out/(dark ? "icons-24-dark.png" : "icons-24-light.png"));
                            h.s.iconSizeIndex = 1;
                        }
                    }
                }
                if (capturePage == -1 || capturePage == PageBasic) {
                    h.s.scale = 1.5f;
                    h.Page(PageBasic);
                    h.Frame({}, out / "page-0-dark-150.png");
                    h.s.scale = 1;
                }
                if (capturePage == -1 || capturePage == PageOverlay) {
                    h.Page(PageOverlay);
                    h.Click("modal");
                    h.Settle(10);
                    h.Frame({}, out / "modal-dark.png");
                }
            }
            std::ofstream info(out / "capture-info.txt");
            info << "Renderer: " << glGetString(GL_RENDERER) << "\nOpenGL: " << glGetString(GL_VERSION)
                 << "\nDear ImGui: " << ImGui::GetVersion()
                 << "\nActual OpenGL backbuffer before swap, " << captureWidth << "x" << captureHeight << ". Icons use recorded raster/code sources.\nInput verifier "
                    "uses public IO; native IME not tested.\n";
        } else
            while (!glfwWindowShouldClose(h.window))
                h.Frame();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Catalog: %s\n", e.what());
        result = 1;
    }
    h.s.editors.previewRenderer.Shutdown();
    DestroyIconArtworkShowcase(h);
    if (texture)
        glDeleteTextures(1, &texture);
    h.s.icons.Clear();
    glDeleteTextures(7, iconTextures.data());
    glDeleteTextures(6, imageTextures.data());
    if (spriteTexture)
        glDeleteTextures(1, &spriteTexture);
    if (renderer)
        ImGui_ImplOpenGL3_Shutdown();
    h.windowFrame.Detach();
    if (backend)
        ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImGui::SetAllocatorFunctions(h.originalAlloc,h.originalFree,h.originalAllocatorUser);
    glfwDestroyWindow(h.window);
    glfwTerminate();
    CoUninitialize();
    return result;
}
