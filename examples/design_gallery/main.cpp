#include <windows.h>
#include <objbase.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "design_lab.h"
#include "capture.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace imkit::design;
struct Host {
    GLFWwindow* window=nullptr;
    DesignLabState state;
    bool automated=false;
    ImVec2 mouse{-100,-100};
    std::filesystem::path capture;
    int frames=0;
    void Frame(const std::function<void(ImGuiIO&)>& input={}) {
        glfwPollEvents();ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();
        auto& io=ImGui::GetIO();
        if(automated) {io.DeltaTime=1.f/60;io.AddFocusEvent(true);io.AddMousePosEvent(mouse.x,mouse.y);}
        if(input)input(io);
        ImGui::NewFrame();Show(state);ImGui::Render();
        int w=0,h=0;glfwGetFramebufferSize(window,&w,&h);
        glViewport(0,0,w,h);glClearColor(.1f,.1f,.1f,1);glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if(!capture.empty()) {SaveBackbuffer(capture,w,h);capture.clear();}
        glfwSwapBuffers(window);++frames;
    }
    void Settle(int count=20) {for(int i=0;i<count;++i)Frame();}
    ImVec2 Position(const std::string& name) {
        auto found=state.probes.find(name);
        if(found==state.probes.end())throw std::runtime_error("Missing interaction probe: "+name);
        return found->second.Center();
    }
    void Move(const std::string& name) {mouse=Position(name);Frame();}
    void ClickAt(ImVec2 pos) {mouse=pos;Frame();Frame([](auto& io){io.AddMouseButtonEvent(0,true);});Frame([](auto& io){io.AddMouseButtonEvent(0,false);});Settle(2);}
    void Click(const std::string& name) {ClickAt(Position(name));}
    void Key(ImGuiKey key) {Frame([&](auto&io){io.AddKeyEvent(key,true);});Frame([&](auto&io){io.AddKeyEvent(key,false);});Settle(2);}
    void Replace(const std::string& name,const char* text) {
        Click(name);Frame([](auto&io){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);});
        Frame([](auto&io){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);});
        Frame([&](auto&io){io.AddInputCharactersUTF8(text);});Settle(2);
    }
    void Shot(const std::filesystem::path& path) {capture=path;Frame();}
};
struct Verification {
    std::ofstream log;
    int assertions=0;
    explicit Verification(const std::filesystem::path& path):log(path) {if(!log)throw std::runtime_error("Cannot create verification log");}
    void Check(bool passed,const std::string& name) {
        log<<(passed?"PASS ":"FAIL ")<<name<<'\n';log.flush();
        std::printf("%s %s\n",passed?"PASS":"FAIL",name.c_str());
        ++assertions;if(!passed)throw std::runtime_error("Verification failed: "+name);
    }
};
void Verify(Host& h,Verification& v,const std::filesystem::path& out) {
    h.Click("interactive");h.Settle();
    for(int proposal=0;proposal<static_cast<int>(Proposals(h.state.refined).size());++proposal)for(int dark=0;dark<2;++dark) {
        std::string prefix="proposal "+std::to_string(proposal+1)+(dark?" dark: ":" light: ");
        h.Click("proposal-"+std::to_string(proposal));h.Click(dark?"dark":"light");h.Settle();
        v.Check(h.state.proposal==proposal&&h.state.dark==static_cast<bool>(dark),prefix+"runtime theme switch");
        h.Click("reset");h.Settle();v.Check(h.state.volume==.64f&&h.state.clicks==0&&h.state.enabled,prefix+"Reset");
        h.Click("apply");v.Check(h.state.clicks==1,prefix+"Button click");
        h.Click("disabled");v.Check(h.state.clicks==1,prefix+"disabled Button does not activate");
        h.Click("toggle");v.Check(!h.state.enabled,prefix+"Switch");
        h.Click("checkbox");v.Check(!h.state.checked,prefix+"Checkbox");
        h.Click("mixed");v.Check(h.state.mixed==1,prefix+"Indeterminate to checked");
        h.Click("radio-1");v.Check(h.state.radio==1,prefix+"Radio selection");
        auto slider=h.state.probes.at("slider");
        h.ClickAt({slider.min.x+420,slider.Center().y});v.Check(h.state.volume>.70f,prefix+"Slider value changes");
        float value=h.state.volume;h.Click("disabled-slider");v.Check(h.state.volume==value,prefix+"disabled Slider leaves model unchanged");
        auto range=h.state.probes.at("range");h.mouse={range.min.x+70,range.Center().y};h.Frame();
        h.Frame([](auto&io){io.AddMouseButtonEvent(0,true);});h.mouse.x+=30;h.Frame();
        h.Frame([](auto&io){io.AddMouseButtonEvent(0,false);});h.Settle(2);
        v.Check(h.state.lower>20&&h.state.lower<=h.state.upper,prefix+"Range edit preserves ordering");
        h.Replace("number","72");h.Key(ImGuiKey_Enter);v.Check(h.state.number==72,prefix+"numeric Input edit");
        h.Replace("text","Quality");v.Check(std::string(h.state.text)=="Quality",prefix+"Input edit");
        h.Replace("search","Display");v.Check(std::string(h.state.search)=="Display",prefix+"Search input");
        v.Check(h.state.validation,prefix+"empty field has validation error");
        h.Replace("invalid","48");v.Check(!h.state.validation,prefix+"validation clears after edit");
        h.Click("combo");v.Check(h.state.comboVisible,prefix+"Combo popup opens");
        h.Click("option-disabled");v.Check(h.state.combo==1&&h.state.comboVisible,prefix+"disabled Combo item");
        h.Click("option-2");v.Check(h.state.combo==2,prefix+"Combo item selection");
        h.Click("tab-1");v.Check(h.state.tab==1,prefix+"Tabs switch");
        h.Click("tab-3");v.Check(h.state.tab==1,prefix+"disabled Tab does not switch");
        h.Click("sort-1");v.Check(h.state.rows[0].quality<=h.state.rows[1].quality&&h.state.rows[1].quality<=h.state.rows[2].quality,prefix+"Table ascending sort");
        h.Click("sort-1");v.Check(h.state.rows[0].quality>=h.state.rows[1].quality&&h.state.rows[1].quality>=h.state.rows[2].quality,prefix+"Table descending sort");
        h.Click("row-0");v.Check(h.state.selected==0,prefix+"Table row selection");
        h.Click("edit-1");v.Check(h.state.inlineActions==1,prefix+"Table inline action");
        h.Click("selectable-1");v.Check(h.state.selected==1,prefix+"Selectable");
        h.Click("selectable-2");v.Check(h.state.selected==1,prefix+"disabled Selectable");
        h.Click("popup-action");v.Check(h.state.actionVisible,prefix+"action Popup opens");
        h.Click("reset-action");v.Check(h.state.volume==.64f,prefix+"action Popup executes");
        h.Click("open-modal");h.Settle();v.Check(h.state.modalVisible,prefix+"Modal opens");
        int previous=h.state.clicks;h.ClickAt(h.Position("apply"));v.Check(h.state.clicks==previous&&h.state.modalVisible,prefix+"Modal blocks background input");
        h.Click("modal-combo");v.Check(h.state.comboVisible,prefix+"Combo stacks inside Modal");
        h.Click("modal-option-0");v.Check(h.state.combo==0&&h.state.modalVisible,prefix+"Modal Combo selection");
        h.Click("modal-cancel");h.Settle();v.Check(!h.state.modalVisible,prefix+"Modal closes");
        v.Check(h.state.focusStates["open-modal"],prefix+"focus returns to Modal trigger");
        h.Key(ImGuiKey_Space);h.Settle();v.Check(h.state.modalVisible,prefix+"keyboard activation");
        h.Key(ImGuiKey_Escape);h.Settle();v.Check(!h.state.modalVisible,prefix+"Escape closes Modal");
        h.Key(ImGuiKey_Tab);bool focused=false;for(auto& entry:h.state.focusStates)focused|=entry.second;
        v.Check(focused,prefix+"Tab navigation has visible focus");
        h.Click("reset");h.Settle();
        // Input-generated hover and pressed frames accompany the fixed comparison samples.
        h.Move("apply");h.Frame();
        bool intermediate=false;for(auto& entry:h.state.motions)intermediate|=entry.second.hover>0&&entry.second.hover<1;
        v.Check(intermediate,prefix+"hover animation has an intermediate state");
        if(proposal==3&&dark==1)h.Shot(out/"interaction-hover-transition.png");
        h.Settle();
        bool settled=false;for(auto& entry:h.state.motions)settled|=entry.second.hover==1;
        v.Check(settled,prefix+"hover animation reaches its endpoint");
        h.Frame([](auto&io){io.AddMouseButtonEvent(0,true);});h.Settle();
        if(proposal==3&&dark==1)h.Shot(out/"interaction-pressed.png");
        h.Frame([](auto&io){io.AddMouseButtonEvent(0,false);});h.Settle();
    }
    v.log<<"Assertions: "<<v.assertions<<"\nInput source: public Dear ImGui IO events, after GLFW backend NewFrame.\n";
}
void CaptureAll(Host& h,const std::filesystem::path& out) {
    std::vector<std::filesystem::path> comparisons;
    for(int i=0;i<static_cast<int>(Proposals(h.state.refined).size());++i)for(int dark=0;dark<2;++dark) {
        h.Click("proposal-"+std::to_string(i));h.Click(dark?"dark":"light");h.Click("reset");h.Click("comparison");
        h.mouse={-100,-100};h.Settle(30);
        char name[100];std::snprintf(name,sizeof(name),"proposal-%02d-%s.png",i+1,dark?"dark":"light");
        auto file=out/name;h.Shot(file);comparisons.push_back(file);
        h.Click("interactive");h.Settle();h.Click("open-modal");h.Settle(30);h.Click("modal-combo");h.Settle(30);
        std::snprintf(name,sizeof(name),"overlay-%02d-%s.png",i+1,dark?"dark":"light");h.Shot(out/name);
        h.Key(ImGuiKey_Escape);h.Settle();h.Key(ImGuiKey_Escape);h.Settle();
    }
    MakeContactSheet(comparisons,out/"contact-sheet.png");
    if(h.state.refined) {
        h.Click("proposal-1");h.Click("dark");h.Click("comparison");
        auto base=Tokens(1,true).color;
        const ImVec4 accents[]={{.557f,.722f,.953f,1},{.533f,.792f,.729f,1}};
        const char* names[]={"colors-blue-dark.png","colors-teal-dark.png"};
        std::vector<std::filesystem::path> colors;
        for(int i=0;i<2;++i){auto palette=base;palette.accent=accents[i];palette.focus=accents[i];
            palette.selection={base.surface.x*.82f+accents[i].x*.18f,base.surface.y*.82f+accents[i].y*.18f,base.surface.z*.82f+accents[i].z*.18f,1};
            h.state.palettes[1]=palette;h.state.customColors[1]=true;h.mouse={-100,-100};h.Settle(25);h.Shot(out/names[i]);colors.push_back(out/names[i]);}
        MakeContactSheet(colors,out/"colors-contact-sheet.png");
        h.state.customColors[1]=false;h.state.paletteOpen=true;h.Settle(15);h.Shot(out/"palette-editor.png");h.state.paletteOpen=false;
    }
}
void GlfwError(int code,const char* text) {std::fprintf(stderr,"GLFW %d: %s\n",code,text);}
}
int main(int argc,char**argv) {
    bool capture=false,verify=false,visible=false,original=false;std::filesystem::path out="out/design-refinements";
    for(int i=1;i<argc;++i) {
        std::string arg=argv[i];
        if(arg=="--capture")capture=true;else if(arg=="--verify")verify=true;else if(arg=="--visible")visible=true;
        else if(arg=="--original")original=true;
        else if(arg=="--capture-japanese")capture=true;
        else if(arg=="--output"&&i+1<argc)out=argv[++i];
        else {std::fprintf(stderr,"Usage: imkit_design_gallery [--verify] [--capture | --capture-japanese] [--visible] [--original] [--output directory]\n");return 2;}
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(FAILED(com))return 1;
    glfwSetErrorCallback(GlfwError);if(!glfwInit()){CoUninitialize();return 1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,0);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR,GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE,(!capture&&!verify)||visible?GLFW_TRUE:GLFW_FALSE);
    Host host;host.automated=capture||verify;host.state.refined=!original;
    host.window=glfwCreateWindow(1920,1440,"ImKit Design Gallery",nullptr,nullptr);
    if(!host.window){glfwTerminate();CoUninitialize();return 1;}
    glfwMakeContextCurrent(host.window);glfwSwapInterval(host.automated?0:1);
    IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    bool glfwBackend=ImGui_ImplGlfw_InitForOpenGL(host.window,true);
    bool glBackend=glfwBackend&&ImGui_ImplOpenGL3_Init("#version 130");
    int result=0;
    try {
        if(!glBackend)throw std::runtime_error("Dear ImGui backend initialization failed");
        wchar_t module[MAX_PATH];GetModuleFileNameW(nullptr,module,MAX_PATH);auto assets=std::filesystem::path(module).parent_path()/"design-assets";
        auto regular=(assets/"Inter-Regular.ttf").string(),semibold=(assets/"Inter-SemiBold.ttf").string();
        if(!std::filesystem::exists(regular)||!std::filesystem::exists(semibold))throw std::runtime_error("Missing Inter 4.1 development fonts");
        host.state.regular=io.Fonts->AddFontFromFileTTF(regular.c_str(),16);
        if(!host.state.regular)throw std::runtime_error("Inter Regular loading failed");
        auto japanese=(assets/"NotoSansJP-Regular.otf").string();
        if(!std::filesystem::exists(japanese))throw std::runtime_error("Missing Noto Sans JP development fallback font");
        ImFontConfig fallback;
        fallback.MergeMode=true;
        // Keep Latin in Inter. With the 1.92 dynamic atlas, missing glyphs are
        // resolved from the next source and rasterized on demand.
        static const ImWchar excludeLatin[]={0x0020,0x024f,0};
        fallback.GlyphExcludeRanges=excludeLatin;
        if(!io.Fonts->AddFontFromFileTTF(japanese.c_str(),16,&fallback))throw std::runtime_error("Japanese fallback loading failed");
        host.state.semibold=io.Fonts->AddFontFromFileTTF(semibold.c_str(),16);
        if(!host.state.semibold)throw std::runtime_error("Inter SemiBold loading failed");
        if(!io.Fonts->AddFontFromFileTTF(japanese.c_str(),16,&fallback))throw std::runtime_error("Japanese heading fallback loading failed");
        if(!host.state.regular||!host.state.semibold)throw std::runtime_error("Font loading failed");
        host.Settle();
        if(host.automated) {
            std::filesystem::create_directories(out);
            if(verify) {Verification verification(out/"verification.txt");Verify(host,verification,out);}
            bool japaneseOnly=false;
            for(int i=1;i<argc;++i)japaneseOnly|=std::string(argv[i])=="--capture-japanese";
            if(capture&&japaneseOnly) {
                host.state.japanese=true;host.state.proposal=0;host.state.refined=true;
                for(int dark=0;dark<2;++dark) {
                    host.state.dark=dark!=0;host.Settle();
                    host.Shot(out/(dark?"japanese-dark.png":"japanese-light.png"));
                }
            } else if(capture)CaptureAll(host,out);
            int w=0,h=0;glfwGetFramebufferSize(host.window,&w,&h);float xscale=0,yscale=0;glfwGetWindowContentScale(host.window,&xscale,&yscale);
            std::ofstream manifest(out/"capture-info.txt");
            manifest<<"Renderer: "<<glGetString(GL_RENDERER)<<"\nOpenGL: "<<glGetString(GL_VERSION)<<"\nDear ImGui: "<<ImGui::GetVersion()
                <<"\nFramebuffer: "<<w<<" x "<<h<<"\nUI scale: 1.0\nPNG DPI metadata: 96\nWindow content scale: "<<xscale<<" x "<<yscale
                <<"\nCapture: real OpenGL backbuffer after rendering, before swap; row flip only.\n"
                <<"Fonts: Inter 4.1 primary; Noto Sans JP 2.004 Regular fallback (body and headings).\n"
                <<(japaneseOnly?"Japanese specimen only: Precision Layers light/dark. No overlay or contact sheet captured.\n":"Comparison: labelled forced state samples. Overlay: actual modal and combo popup.\n")
                <<"Native desktop interaction and IME: not performed by this runner.\n";
            std::printf("Completed. Output: %s\n",out.string().c_str());
        } else while(!glfwWindowShouldClose(host.window)) {if(glfwGetWindowAttrib(host.window,GLFW_ICONIFIED)){glfwWaitEventsTimeout(.05);continue;}host.Frame();}
    } catch(const std::exception& error) {std::fprintf(stderr,"Design Gallery: %s\n",error.what());result=1;}
    if(glBackend)ImGui_ImplOpenGL3_Shutdown();if(glfwBackend)ImGui_ImplGlfw_Shutdown();ImGui::DestroyContext();
    glfwDestroyWindow(host.window);glfwTerminate();CoUninitialize();return result;
}
