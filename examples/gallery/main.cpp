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
#include <stdexcept>
#include <cstdio>
#include <cstring>
namespace {
struct Host {
    GLFWwindow *window = nullptr;
    imkit::gallery::GalleryState s;
    bool automated = false;
    ImVec2 mouse{-100, -100};
    void Frame(const std::function<void(ImGuiIO &)> &input = {}, const std::filesystem::path &shot = {}) {
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
} // namespace
int main(int argc, char **argv) {
    bool capture = false, verify = false;
    int capturePage = -1;
    std::filesystem::path out = "out/catalog";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--capture")
            capture = true;
        else if (a == "--verify")
            verify = true;
        else if (a == "--output" && i + 1 < argc)
            out = argv[++i];
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, capture || verify ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    Host h;
    h.automated = capture || verify;
    h.window = glfwCreateWindow(1920, 1440, "ImKit Precision Layers", nullptr, nullptr);
    if (!h.window) {
        glfwTerminate();
        CoUninitialize();
        return 1;
    }
    glfwMakeContextCurrent(h.window);
    glfwSwapInterval(h.automated ? 0 : 1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    bool backend = ImGui_ImplGlfw_InitForOpenGL(h.window, true),
         renderer = backend && ImGui_ImplOpenGL3_Init("#version 130");
    int result = 0;
    GLuint texture = 0;
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
        h.Settle();
        if (h.automated) {
            std::filesystem::create_directories(out);
            if (verify)
                Verify(h, out);
            if (capture) {
                for (int dark = 0; dark < 2; ++dark) {
                    h.s.dark = dark != 0;
                    h.s.theme = imkit::MakePrecisionTheme(dark ? imkit::ColorScheme::Dark
                                                               : imkit::ColorScheme::Light);
                    for (int page = 0; page < 6; ++page) {
                        if (capturePage >= 0 && capturePage != page)
                            continue;
                        h.Page(page);
                        h.Frame({},
                                out / ("page-" + std::to_string(page) + (dark ? "-dark.png" : "-light.png")));
                    }
                }
                h.s.scale = 1.5f;
                h.Page(0);
                h.Frame({}, out / "page-0-dark-150.png");
                h.s.scale = 1;
                h.Page(4);
                h.Click("modal");
                h.Settle(10);
                h.Frame({}, out / "modal-dark.png");
            }
            std::ofstream info(out / "capture-info.txt");
            info << "Renderer: " << glGetString(GL_RENDERER) << "\nOpenGL: " << glGetString(GL_VERSION)
                 << "\nDear ImGui: " << ImGui::GetVersion()
                 << "\nActual OpenGL backbuffer before swap, 1920x1440. No image generation.\nInput verifier "
                    "uses public IO; native IME not tested.\n";
        } else
            while (!glfwWindowShouldClose(h.window))
                h.Frame();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Catalog: %s\n", e.what());
        result = 1;
    }
    if (texture)
        glDeleteTextures(1, &texture);
    if (renderer)
        ImGui_ImplOpenGL3_Shutdown();
    if (backend)
        ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(h.window);
    glfwTerminate();
    CoUninitialize();
    return result;
}
