#include <imkit/imkit.h>
#include <imkit/window_frame_macos.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <array>

namespace {
void EditStyle(imkit::WindowFrameStyle& style) {
    struct Color { const char* name; ImVec4* value; };
    Color colors[]={{"Active background",&style.activeBackground},{"Inactive background",&style.inactiveBackground},
        {"Border",&style.border},{"Title text",&style.titleText},{"Auxiliary text",&style.auxiliaryText},
        {"Icon",&style.icon},{"Button text",&style.buttonText},{"Button hover",&style.buttonHover},
        {"Button pressed",&style.buttonPressed},{"Close hover",&style.closeButtonHover},
        {"Close pressed",&style.closeButtonPressed}};
    for(auto& color:colors) ImGui::ColorEdit4(color.name,&color.value->x);
    auto& m=style.metrics;
    ImGui::DragFloat("Height",&m.height,.25f,0,96);ImGui::DragFloat("Icon area",&m.iconAreaWidth,.25f,0,160);
    ImGui::DragFloat("Title left",&m.titlePaddingLeft,.25f,0,80);ImGui::DragFloat("Title right",&m.titlePaddingRight,.25f,0,80);
    ImGui::DragFloat("Button width",&m.buttonWidth,.25f,0,160);ImGui::DragFloat("Border width",&m.borderWidth,.1f,0,8);
    auto& f=style.features;
    ImGui::Checkbox("Icon",&f.icon);ImGui::Checkbox("Application name",&f.applicationName);
    ImGui::Checkbox("Project name",&f.projectName);ImGui::Checkbox("Unsaved",&f.unsavedIndicator);
    ImGui::Checkbox("Workspace",&f.workspaceSwitcher);ImGui::Checkbox("Minimize",&f.minimize);
    ImGui::Checkbox("Maximize",&f.maximizeRestore);ImGui::Checkbox("Close",&f.close);
}
}

int main() {
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    GLFWwindow* window=glfwCreateWindow(1100,760,"ImKit macOS Window Frame Demo",nullptr,nullptr);
    if(!window){glfwTerminate();return 1;}
    glfwMakeContextCurrent(window);glfwSwapInterval(1);
    IMGUI_CHECKVERSION();ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window,true);ImGui_ImplOpenGL3_Init("#version 150");
    auto theme=imkit::MakeTheme(imkit::ThemePreset::Graphite);
    auto preset=imkit::WindowFramePreset::Workspace;
    auto style=imkit::MakeWindowFrameStyle(preset,theme);
    imkit::WindowFrameMacOSAdapter adapter;
    adapter.Attach((__bridge void*)glfwGetCocoaWindow(window),preset);
    const std::array<std::string_view,4> workspaces{"Edit","Color","Audio","Deliver"};
    std::size_t selected=0;
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        auto state=adapter.State();int width=0,height=0;glfwGetWindowSize(window,&width,&height);
        const auto layout=imkit::LayoutWindowFrame(static_cast<float>(width),style,state);
        ImGui::SetNextWindowPos({0,layout.titleBar.max.y});
        ImGui::SetNextWindowSize({static_cast<float>(width),std::max(1.f,static_cast<float>(height)-layout.titleBar.max.y)});
        ImGui::Begin("Window Frame Demo",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
        const char* names[]={"Native","Studio","Workspace","Tool"};int index=static_cast<int>(preset);
        if(ImGui::Combo("Preset",&index,names,4)) {preset=static_cast<imkit::WindowFramePreset>(index);style=imkit::MakeWindowFrameStyle(preset,theme);adapter.Configure(preset);}
        if(ImGui::Button("Regenerate from Theme")) style=imkit::MakeWindowFrameStyle(preset,theme);
        EditStyle(style);
        const auto contrast=imkit::ValidateWindowFrameContrast(style);
        ImGui::Text("Contrast: %s",contrast.valid?"valid":"warning");
        ImGui::End();
        const auto result=imkit::DrawWindowFrame(style,{"ImKit","macOS demo",true,workspaces,selected},layout,state);
        if(result.event.type==imkit::WindowFrameEventType::WorkspaceSelected) selected=result.event.workspace;
        ImGui::Render();int fbw=0,fbh=0;glfwGetFramebufferSize(window,&fbw,&fbh);glViewport(0,0,fbw,fbh);
        glClearColor(.08f,.08f,.09f,1);glClear(GL_COLOR_BUFFER_BIT);ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());glfwSwapBuffers(window);
    }
    adapter.Detach();ImGui_ImplOpenGL3_Shutdown();ImGui_ImplGlfw_Shutdown();ImGui::DestroyContext();
    glfwDestroyWindow(window);glfwTerminate();return 0;
}
