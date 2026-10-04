#pragma once
// Development hosts only: deterministic public-IO capture, never installed.
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <cmath>

namespace imkit::design {
class MotionCapture {
    FILE* pipe=nullptr;
    std::ofstream events;
    std::vector<unsigned char> pixels;
    int width=0,height=0,index=0;
    float ringAge=100;
    ImVec2 ring{};
public:
    void Start(const std::filesystem::path& output,int w,int h) {
        const auto filename=std::filesystem::absolute(output).string();
        if(filename.find_first_of("\"%\r\n")!=std::string::npos) throw std::runtime_error("Invalid capture path");
        std::filesystem::create_directories(output.parent_path());
        width=w;height=h;pixels.resize(static_cast<size_t>(w)*h*4);
        events.open(output.string()+".jsonl");
        const std::string command="ffmpeg -y -hide_banner -loglevel error -f rawvideo -pixel_format bgra -video_size "+
            std::to_string(w)+"x"+std::to_string(h)+" -framerate 60 -i pipe:0 -vf vflip -c:v ffv1 -level 3 -threads 2 \""+filename+"\"";
        pipe=_popen(command.c_str(),"wb");
        if(!pipe||!events) throw std::runtime_error("Unable to open native video sink");
    }
    bool Active()const{return pipe!=nullptr;}
    int Frames()const{return index;}
    void Mark(const char* name) {events<<"{\"action\":\""<<name<<"\",\"frame\":"<<index<<"}\n";}
    void Check(const char* name,bool passed,double value=0) {
        events<<"{\"check\":\""<<name<<"\",\"passed\":"<<(passed?"true":"false")<<",\"value\":"<<value<<",\"frame\":"<<index<<"}\n";events.flush();
        if(!passed)throw std::runtime_error(std::string("Native capture check failed: ")+name);
    }
    void Cursor() {
        auto& io=ImGui::GetIO();auto* d=ImGui::GetForegroundDrawList();const auto p=io.MousePos;
        if(ImGui::IsMouseClicked(0)||ImGui::IsMouseClicked(2)){ring=p;ringAge=0;}
        ringAge+=1.f/60;
        if(ringAge<.4f) {const float u=ringAge/.4f;d->AddCircle(ring,18+40*u,IM_COL32(169,231,203,static_cast<int>(220*(1-u))),48,4);}
        if(p.x<0||p.y<0)return;
        const ImVec2 arrow[]={{p.x,p.y},{p.x+4,p.y+58},{p.x+18,p.y+44},{p.x+29,p.y+66},{p.x+40,p.y+61},{p.x+29,p.y+39},{p.x+49,p.y+37}};
        d->AddConcavePolyFilled(arrow,7,io.MouseDown[0]||io.MouseDown[2]?IM_COL32(169,231,203,255):IM_COL32(248,255,251,255));
        d->AddPolyline(arrow,7,IM_COL32(12,24,29,255),ImDrawFlags_Closed,3.5f);
    }
    void Capture(int w,int h) {
        if(w!=width||h!=height)throw std::runtime_error("Capture framebuffer dimensions changed");
        glReadBuffer(GL_BACK);glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(0,0,w,h,0x80E1,GL_UNSIGNED_BYTE,pixels.data());
        if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("Capture GL read failed");
        if(std::fwrite(pixels.data(),1,pixels.size(),pipe)!=pixels.size())throw std::runtime_error("Video sink closed");
        const auto& io=ImGui::GetIO();
        events<<"{\"frame\":"<<index<<",\"time\":"<<index/60.0<<",\"x\":"<<io.MousePos.x<<",\"y\":"<<io.MousePos.y
            <<",\"down\":"<<(io.MouseDown[0]?"true":"false")<<",\"middle\":"<<(io.MouseDown[2]?"true":"false")<<",\"wheel\":"<<io.MouseWheel<<"}\n";
        ++index;
    }
    void Finish() {if(pipe){auto* p=pipe;pipe=nullptr;events.close();if(_pclose(p)!=0)throw std::runtime_error("Native FFmpeg failed");}}
    ~MotionCapture(){if(pipe)_pclose(pipe);}
};
}
