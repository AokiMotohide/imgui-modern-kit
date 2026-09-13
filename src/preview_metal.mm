#include <imkit/preview_metal.h>
#import <Metal/Metal.h>
#include <algorithm>
#include <cmath>

namespace imkit::preview {
namespace {
void Multiply(const float *a, const float *b, float *out) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            out[c * 4 + r] = 0;
            for (int k = 0; k < 4; ++k) out[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
        }
}
void Matrix(const cg::Camera &c, const cg::Transform &t, float aspect, float *result, float *normal) {
    const auto basis = cg::OrientationBasis(cg::Orientation::View, {}, c);
    const auto x = basis.x, y = basis.y, z = basis.z;
    float view[16] = {float(x.x),float(y.x),float(z.x),0,float(x.y),float(y.y),float(z.y),0,
                      float(x.z),float(y.z),float(z.z),0,0,0,0,1};
    view[12] = -float(c.target.x*x.x+c.target.y*x.y+c.target.z*x.z);
    view[13] = -float(c.target.x*y.x+c.target.y*y.y+c.target.z*y.z);
    view[14] = -float(c.distance+c.target.x*z.x+c.target.y*z.y+c.target.z*z.z);
    float projection[16]{};
    constexpr float nearPlane=.01f, farPlane=10000.f;
    if (c.projection == cg::Projection::Orthographic) {
        projection[0]=2/float(c.orthographicHeight)/aspect; projection[5]=2/float(c.orthographicHeight);
        projection[10]=-2/(farPlane-nearPlane); projection[14]=-(farPlane+nearPlane)/(farPlane-nearPlane); projection[15]=1;
    } else {
        projection[5]=1/float(std::tan(c.verticalFov*.5)); projection[0]=projection[5]/aspect;
        projection[10]=-(farPlane+nearPlane)/(farPlane-nearPlane); projection[11]=-1;
        projection[14]=-2*farPlane*nearPlane/(farPlane-nearPlane);
    }
    const auto linear=cg::LinearBasis(t), normals=cg::NormalBasis(t);
    float model[16]{}; std::fill(normal,normal+16,0.f); model[15]=normal[15]=1;
    const cg::Vec3 columns[]{linear.x,linear.y,linear.z}, normalColumns[]{normals.x,normals.y,normals.z};
    for(int column=0;column<3;++column) {
        const auto &v=columns[column], &n=normalColumns[column];
        model[column*4]=float(v.x); model[column*4+1]=float(v.y); model[column*4+2]=float(v.z);
        normal[column*4]=float(n.x); normal[column*4+1]=float(n.y); normal[column*4+2]=float(n.z);
    }
    model[12]=float(t.translation.x); model[13]=float(t.translation.y); model[14]=float(t.translation.z);
    float pv[16]; Multiply(projection,view,pv); Multiply(pv,model,result);
}
template<class T> T Object(void *value) { return (__bridge T)value; }
void Release(void *&value) { if(value) { CFRelease(value); value=nullptr; } }
}

bool MetalRenderer::Init(void *mtlDevice, int width, int height) {
    if (Initialized() || !mtlDevice || width<=0 || height<=0) return false;
    id<MTLDevice> device=Object<id<MTLDevice>>(mtlDevice);
    static const char sourceText[]=R"(
#include <metal_stdlib>
using namespace metal;
struct VertexIn { packed_float3 position; packed_float3 normal; packed_float4 color; };
struct VertexOut { float4 position [[position]]; float3 normal; float4 color; };
struct Uniforms { float4x4 matrix; float4x4 normalMatrix; uint2 objectId; };
vertex VertexOut preview_vertex(uint id [[vertex_id]], const device VertexIn* vertices [[buffer(0)]], constant Uniforms& u [[buffer(1)]]) {
    VertexOut o; VertexIn v=vertices[id]; o.position=u.matrix*float4(v.position,1); o.normal=normalize((u.normalMatrix*float4(v.normal,0)).xyz); o.color=v.color; return o;
}
struct Outputs { float4 color [[color(0)]]; uint2 objectId [[color(1)]]; };
fragment Outputs preview_fragment(VertexOut in [[stage_in]], constant Uniforms& u [[buffer(1)]]) {
    Outputs o; float light=.25+.75*max(dot(normalize(in.normal),normalize(float3(.35,.7,.55))),0.0); o.color=float4(in.color.rgb*light,in.color.a); o.objectId=u.objectId; return o;
}
)";
    NSString *source=[NSString stringWithUTF8String:sourceText];
    NSError *error=nil;
    id<MTLLibrary> library=[device newLibraryWithSource:source options:nil error:&error];
    if(!library) return false;
    MTLRenderPipelineDescriptor *descriptor=[MTLRenderPipelineDescriptor new];
    descriptor.vertexFunction=[library newFunctionWithName:@"preview_vertex"];
    descriptor.fragmentFunction=[library newFunctionWithName:@"preview_fragment"];
    descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    descriptor.colorAttachments[1].pixelFormat=MTLPixelFormatRG32Uint;
    descriptor.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
    id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];
    if(!pipeline) return false;
    MTLDepthStencilDescriptor *depthDescriptor=[MTLDepthStencilDescriptor new];
    depthDescriptor.depthCompareFunction=MTLCompareFunctionLess; depthDescriptor.depthWriteEnabled=YES;
    id<MTLDepthStencilState> depthState=[device newDepthStencilStateWithDescriptor:depthDescriptor];
    device_=CFBridgingRetain(device); pipeline_=CFBridgingRetain(pipeline); depthState_=CFBridgingRetain(depthState);
    return Resize(width,height);
}

bool MetalRenderer::Resize(int width, int height) {
    if(!device_ || width<=0 || height<=0) return false;
    if(width==width_ && height==height_ && color_) return true;
    Release(color_); Release(depth_); Release(picking_);
    id<MTLDevice> device=Object<id<MTLDevice>>(device_);
    auto texture=[&](MTLPixelFormat format,MTLTextureUsage usage,MTLStorageMode storage) {
        MTLTextureDescriptor *d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:width height:height mipmapped:NO];
        d.usage=usage; d.storageMode=storage; return [device newTextureWithDescriptor:d];
    };
    id<MTLTexture> color=texture(MTLPixelFormatBGRA8Unorm,MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead,MTLStorageModePrivate);
    id<MTLTexture> depth=texture(MTLPixelFormatDepth32Float,MTLTextureUsageRenderTarget,MTLStorageModePrivate);
    id<MTLTexture> picking=texture(MTLPixelFormatRG32Uint,MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead,MTLStorageModeShared);
    if(!color || !depth || !picking) return false;
    color_=CFBridgingRetain(color); depth_=CFBridgingRetain(depth); picking_=CFBridgingRetain(picking);
    width_=width; height_=height; return true;
}

bool MetalRenderer::Render(std::span<const Mesh> meshes,const cg::Camera &camera,void *mtlCommandBuffer,ImVec4 background) {
    if(!Initialized() || !mtlCommandBuffer || width_<=0 || height_<=0) return false;
    id<MTLDevice> device=Object<id<MTLDevice>>(device_);
    id<MTLCommandBuffer> commandBuffer=Object<id<MTLCommandBuffer>>(mtlCommandBuffer);
    MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=Object<id<MTLTexture>>(color_); pass.colorAttachments[0].loadAction=MTLLoadActionClear; pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    pass.colorAttachments[0].clearColor=MTLClearColorMake(background.x,background.y,background.z,background.w);
    pass.colorAttachments[1].texture=Object<id<MTLTexture>>(picking_); pass.colorAttachments[1].loadAction=MTLLoadActionClear; pass.colorAttachments[1].storeAction=MTLStoreActionStore;
    pass.depthAttachment.texture=Object<id<MTLTexture>>(depth_); pass.depthAttachment.loadAction=MTLLoadActionClear; pass.depthAttachment.storeAction=MTLStoreActionDontCare; pass.depthAttachment.clearDepth=1;
    id<MTLRenderCommandEncoder> encoder=[commandBuffer renderCommandEncoderWithDescriptor:pass];
    if(!encoder) return false;
    [encoder setRenderPipelineState:Object<id<MTLRenderPipelineState>>(pipeline_)];
    [encoder setDepthStencilState:Object<id<MTLDepthStencilState>>(depthState_)];
    struct Uniforms { float matrix[16]; float normal[16]; std::uint32_t id[2]; } uniforms{};
    for(const auto &mesh:meshes) {
        if(mesh.vertices.empty() || mesh.indices.empty() || std::any_of(mesh.indices.begin(),mesh.indices.end(),[&](auto i){return i>=mesh.vertices.size();})) continue;
        Matrix(camera,mesh.transform,float(width_)/height_,uniforms.matrix,uniforms.normal);
        uniforms.id[0]=std::uint32_t(mesh.id); uniforms.id[1]=std::uint32_t(mesh.id>>32);
        id<MTLBuffer> vertices=[device newBufferWithBytes:mesh.vertices.data() length:mesh.vertices.size_bytes() options:MTLResourceStorageModeShared];
        id<MTLBuffer> indices=[device newBufferWithBytes:mesh.indices.data() length:mesh.indices.size_bytes() options:MTLResourceStorageModeShared];
        [encoder setVertexBuffer:vertices offset:0 atIndex:0]; [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
        [encoder setTriangleFillMode:mesh.wire?MTLTriangleFillModeLines:MTLTriangleFillModeFill];
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:mesh.indices.size() indexType:MTLIndexTypeUInt32 indexBuffer:indices indexBufferOffset:0];
    }
    [encoder endEncoding]; return true;
}

editor::StableId MetalRenderer::Pick(int x,int y) const {
    if(!picking_ || x<0 || y<0 || x>=width_ || y>=height_) return 0;
    std::uint32_t value[2]{};
    [Object<id<MTLTexture>>(picking_) getBytes:value bytesPerRow:sizeof(value) fromRegion:MTLRegionMake2D(x,y,1,1) mipmapLevel:0];
    return editor::StableId(value[0]) | (editor::StableId(value[1])<<32);
}

void MetalRenderer::Shutdown() {
    Release(picking_); Release(depth_); Release(color_); Release(depthState_); Release(pipeline_); Release(device_);
    width_=height_=0;
}
} // namespace imkit::preview
