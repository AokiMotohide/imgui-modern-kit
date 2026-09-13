#include "gallery.h"
#include <imkit/window_frame_macos.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_metal.h>
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <array>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {
struct Host {
    GLFWwindow *window=nullptr;
    id<MTLDevice> device=nil;
    id<MTLCommandQueue> queue=nil;
    CAMetalLayer *layer=nil;
    imkit::gallery::GalleryState state;
    imkit::WindowFrameMacOSAdapter frame;
    std::string title="ImKit Precision Layers";
    std::array<id<MTLTexture>,7> iconTextures{};
    id<MTLTexture> sampleTexture=nil;

    bool Draw() {
        @autoreleasepool {
            glfwPollEvents();
            const bool custom=state.framePreset!=imkit::WindowFramePreset::Native;
            if(custom && !frame.Attached()) frame.Attach((__bridge void*)glfwGetCocoaWindow(window),state.framePreset);
            if(!custom && frame.Attached()) frame.Detach();

            int width=0,height=0; glfwGetFramebufferSize(window,&width,&height);
            if(width<=0 || height<=0) return true;
            layer.drawableSize=CGSizeMake(width,height);
            id<CAMetalDrawable> drawable=[layer nextDrawable]; if(!drawable) return true;
            id<MTLCommandBuffer> commandBuffer=[queue commandBuffer];
            MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor renderPassDescriptor];
            pass.colorAttachments[0].texture=drawable.texture;
            pass.colorAttachments[0].loadAction=MTLLoadActionClear;
            pass.colorAttachments[0].storeAction=MTLStoreActionStore;
            pass.colorAttachments[0].clearColor=MTLClearColorMake(.06,.07,.09,1);

            ImGui_ImplMetal_NewFrame(pass); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
            imkit::WindowFrameState frameState{}; imkit::WindowFrameLayout layout{};
            if(frame.Attached()) {
                frameState=frame.State();
                const float backingScale=frameState.dpiScale; frameState.dpiScale=1.f;
                layout=imkit::LayoutWindowFrame(ImGui::GetIO().DisplaySize.x,state.frameStyle,frameState);
                frameState.dpiScale=backingScale; state.windowFrameHeight=layout.titleBar.max.y;
            } else state.windowFrameHeight=0;
            imkit::gallery::Show(state); imkit::gallery::ShowComparison(state);
            imkit::WindowFrameEvent event{};
            if(frame.Attached()) {
                const imkit::WindowFrameContent content{"ImKit",title,state.frameUnsaved,state.frameWorkspaces,state.frameWorkspace};
                event=imkit::DrawWindowFrame(state.frameStyle,content,layout,frameState).event;
                if(event.type==imkit::WindowFrameEventType::WorkspaceSelected) state.frameWorkspace=event.workspace;
            }
            state.editors.RenderPreview();
            ImGui::Render();
            id<MTLRenderCommandEncoder> encoder=[commandBuffer renderCommandEncoderWithDescriptor:pass];
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(),commandBuffer,encoder);
            [encoder endEncoding];
            if(ImGui::GetIO().ConfigFlags&ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows(); ImGui::RenderPlatformWindowsDefault();
            }
            [commandBuffer presentDrawable:drawable]; [commandBuffer commit];
            if(event.type==imkit::WindowFrameEventType::Operation) frame.Execute(event.operation);
            return true;
        }
    }
};

id<MTLTexture> Texture(id<MTLDevice> device,int width,int height,const void *rgba) {
    auto *descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
    descriptor.usage=MTLTextureUsageShaderRead; descriptor.storageMode=MTLStorageModeShared;
    id<MTLTexture> texture=[device newTextureWithDescriptor:descriptor];
    [texture replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:rgba bytesPerRow:width*4]; return texture;
}
}

int main(int argc,char **argv) {
    bool smoke=false,japanese=false; int page=-1; imkit::WindowFramePreset preset=imkit::WindowFramePreset::Studio;
    for(int i=1;i<argc;++i) {
        const std::string argument=argv[i];
        if(argument=="--smoke") smoke=true;
        else if(argument=="--japanese") japanese=true;
        else if(argument=="--page" && i+1<argc) page=std::stoi(argv[++i]);
        else if(argument=="--window-frame" && i+1<argc) preset=std::string(argv[++i])=="native"?imkit::WindowFramePreset::Native:imkit::WindowFramePreset::Studio;
        else return 2;
    }
    glfwSetErrorCallback([](int code,const char *text){std::fprintf(stderr,"Gallery GLFW %d: %s\n",code,text?text:"");});
    if(!glfwInit()) return 1;
    const float scale=ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API); glfwWindowHint(GLFW_VISIBLE,smoke?GLFW_FALSE:GLFW_TRUE);
    Host host; host.window=glfwCreateWindow(int(1440*scale),int(1000*scale),host.title.c_str(),nullptr,nullptr);
    if(!host.window) { glfwTerminate(); return 1; }
    host.device=MTLCreateSystemDefaultDevice(); host.queue=[host.device newCommandQueue];
    NSWindow *window=glfwGetCocoaWindow(host.window); host.layer=[CAMetalLayer layer]; host.layer.device=host.device;
    host.layer.pixelFormat=MTLPixelFormatBGRA8Unorm; window.contentView.layer=host.layer; window.contentView.wantsLayer=YES;

    IMGUI_CHECKVERSION(); ImGui::CreateContext(); auto &io=ImGui::GetIO(); io.IniFilename=nullptr;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_DockingEnable|ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDpiScaleFonts=true; io.ConfigDpiScaleViewports=true;
    ImGui::GetStyle().ScaleAllSizes(scale); ImGui::GetStyle().FontScaleDpi=scale;
    if(!ImGui_ImplGlfw_InitForOther(host.window,true) || !ImGui_ImplMetal_Init(host.device)) return 1;

    host.state.theme=imkit::MakeTheme(imkit::ThemePreset::Graphite); host.state.presetIndex=2; host.state.dark=true;
    host.state.framePreset=preset;
    host.state.framePresetStyles={imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Native,host.state.theme),
        imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Studio,host.state.theme),
        imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Workspace,host.state.theme),
        imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::Tool,host.state.theme)};
    host.state.frameStyle=imkit::MakeWindowFrameStyle(preset,host.state.theme);
    const auto resources=std::filesystem::path([[[NSBundle mainBundle] resourcePath] UTF8String]);
    const auto assets=resources/"design-assets";
    auto loadFont=[&](const char *file) {
        const auto path=assets/file; if(!std::filesystem::exists(path)) return (ImFont*)nullptr;
        ImFont *font=io.Fonts->AddFontFromFileTTF(path.string().c_str(),14);
        ImFontConfig config; config.MergeMode=true; static const ImWchar exclude[]={0x20,0x24f,0}; config.GlyphExcludeRanges=exclude;
        const auto jp=assets/"NotoSansJP-Regular.otf"; if(std::filesystem::exists(jp)) io.Fonts->AddFontFromFileTTF(jp.string().c_str(),14,&config); return font;
    };
    host.state.fonts={loadFont("Inter-Regular.ttf"),loadFont("Inter-SemiBold.ttf")};
    static const unsigned char sample[]={220,220,230,255,100,100,120,255,100,100,120,255,220,220,230,255};
    host.sampleTexture=Texture(host.device,2,2,sample); host.state.texture=ImTextureRef((__bridge void*)host.sampleTexture);
    for(std::size_t i=0;i<host.iconTextures.size();++i) {
        const auto atlas=imkit::GetIconAtlasPixels(imkit::IconPixelSizes[i]);
        host.iconTextures[i]=Texture(host.device,atlas.width,atlas.height,atlas.rgba.data());
        host.state.icons.SetTexture(atlas.iconPixels,ImTextureRef((__bridge void*)host.iconTextures[i]));
    }
    host.state.editors.Initialize(); host.state.editors.useGL=false; host.state.editors.japanese=japanese;
    if(page>=0) host.state.page=page;
    int frames=0; while(!glfwWindowShouldClose(host.window) && (!smoke || frames<5)) { host.Draw(); ++frames; }
    host.frame.Detach(); host.state.icons.Clear(); ImGui_ImplMetal_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwDestroyWindow(host.window); glfwTerminate();
    if(smoke) std::puts("macOS Gallery: Metal, docking, multi-viewport and five native frames passed");
    return 0;
}
