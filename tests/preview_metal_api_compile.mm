#include <imkit/preview_metal.h>
#import <Metal/Metal.h>

int main() {
    imkit::preview::MetalRenderer renderer;
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();
    if(!device || !renderer.Init((__bridge void*)device,16,16)) return 1;
    if(renderer.Width()!=16 || renderer.Height()!=16 || !renderer.Texture()) return 2;
    renderer.Shutdown(); return renderer.Initialized()?3:0;
}
