#include "capture.h"
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace imkit::design {
namespace {
using Microsoft::WRL::ComPtr;
void Check(HRESULT result,const char* operation) {
    if(FAILED(result)) {std::ostringstream message;message<<operation<<" failed: 0x"<<std::hex<<result;throw std::runtime_error(message.str());}
}
ComPtr<IWICImagingFactory> Factory() {
    ComPtr<IWICImagingFactory> factory;
    Check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)),"WIC factory");
    return factory;
}
void Save(const std::filesystem::path& path,UINT w,UINT h,const std::vector<unsigned char>& pixels) {
    auto factory=Factory();ComPtr<IWICStream> stream;ComPtr<IWICBitmapEncoder> encoder;ComPtr<IWICBitmapFrameEncode> frame;
    Check(factory->CreateStream(&stream),"Create stream");
    Check(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE),"Open PNG");
    Check(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder),"Create PNG encoder");
    Check(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache),"Initialize PNG");
    Check(encoder->CreateNewFrame(&frame,nullptr),"Create PNG frame");Check(frame->Initialize(nullptr),"Initialize frame");
    Check(frame->SetSize(w,h),"Set dimensions");Check(frame->SetResolution(96,96),"Set DPI");
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;Check(frame->SetPixelFormat(&format),"Set BGRA format");
    if(format!=GUID_WICPixelFormat32bppBGRA)throw std::runtime_error("PNG encoder did not accept lossless BGRA input");
    Check(frame->WritePixels(h,w*4,static_cast<UINT>(pixels.size()),const_cast<BYTE*>(pixels.data())),"Write pixels");
    Check(frame->Commit(),"Commit frame");Check(encoder->Commit(),"Commit PNG");
}
}
void SaveBackbuffer(const std::filesystem::path& path,int width,int height) {
    if(width!=1920||height!=1440)throw std::runtime_error("Capture requires a 1920 x 1440 framebuffer");
    std::vector<unsigned char> pixels(static_cast<size_t>(width)*height*4),flipped(pixels.size());
    while(glGetError()!=GL_NO_ERROR) {}
    constexpr GLenum bgra=0x80E1;
    glReadBuffer(GL_BACK);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,width,height,bgra,GL_UNSIGNED_BYTE,pixels.data());
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL backbuffer read failed");
    for(int y=0;y<height;++y)std::copy_n(pixels.data()+static_cast<size_t>(height-1-y)*width*4,width*4,flipped.data()+static_cast<size_t>(y)*width*4);
    Save(path,width,height,flipped);
}
void MakeContactSheet(const std::vector<std::filesystem::path>& paths,const std::filesystem::path& destination) {
    if(paths.empty()||paths.size()%2)throw std::runtime_error("Contact sheet needs paired comparison images");
    constexpr UINT cellW=960,cellH=720,w=1920;const UINT h=static_cast<UINT>(paths.size()/2)*cellH;
    std::vector<unsigned char> sheet(static_cast<size_t>(w)*h*4),cell(static_cast<size_t>(cellW)*cellH*4);
    auto factory=Factory();
    for(size_t i=0;i<paths.size();++i) {
        ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICBitmapScaler> scaler;ComPtr<IWICFormatConverter> converter;
        Check(factory->CreateDecoderFromFilename(paths[i].c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder),"Read comparison PNG");
        Check(decoder->GetFrame(0,&frame),"Read comparison frame");
        UINT sourceW=0,sourceH=0;Check(frame->GetSize(&sourceW,&sourceH),"Read dimensions");
        if(sourceW!=1920||sourceH!=1440)throw std::runtime_error("Unexpected comparison image dimensions");
        Check(factory->CreateBitmapScaler(&scaler),"Create scaler");Check(scaler->Initialize(frame.Get(),cellW,cellH,WICBitmapInterpolationModeFant),"Scale contact cell");
        Check(factory->CreateFormatConverter(&converter),"Create converter");
        Check(converter->Initialize(scaler.Get(),GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom),"Convert contact cell");
        Check(converter->CopyPixels(nullptr,cellW*4,static_cast<UINT>(cell.size()),cell.data()),"Read contact cell");
        for(UINT y=0;y<cellH;++y)std::copy_n(cell.data()+static_cast<size_t>(y)*cellW*4,cellW*4,sheet.data()+((i/2*cellH+y)*w+(i%2*cellW))*4);
    }
    Save(destination,w,h,sheet);
}
}
