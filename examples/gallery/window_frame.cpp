#include <imkit/imkit.h>
#include "window_frame.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace imkit::gallery {
FrameLayout LayoutWindowFrame(float width, float dpiScale) {
    FrameLayout l;
    l.scale=std::max(1.f,dpiScale);
    const float height=std::round(32*l.scale), button=std::round(46*l.scale);
    l.title={{0,0},{width,height}};
    l.icon={{0,0},{height,height}};
    for(int i=0;i<3;++i) l.buttons[i]={{width-(3-i)*button,0},{width-(2-i)*button,height}};
    return l;
}
std::string ElideWindowTitle(std::string_view title, float width, ImFont* font, float size) {
    auto measure=[&](std::string_view t) { return font->CalcTextSizeA(size,FLT_MAX,0,t.data(),t.data()+t.size()).x; };
    if(width<=0) return {};
    if(measure(title)<=width) return std::string(title);
    constexpr std::string_view suffix="...";
    if(measure(suffix)>width) return {};
    std::string result(title);
    while(!result.empty()) {
        std::size_t start=result.size()-1;
        while(start && (static_cast<unsigned char>(result[start])&0xc0)==0x80) --start;
        result.resize(start);
        if(measure(result)+measure(suffix)<=width) break;
    }
    return result+std::string(suffix);
}
FrameResult DrawWindowFrame(const Theme& theme, std::string_view title,
                            const FrameLayout& l, const FrameState& state) {
    auto* draw=ImGui::GetForegroundDrawList();
    const auto color=[](ImVec4 c) { return ImGui::ColorConvertFloat4ToU32(c); };
    const auto& c=theme.semantic;
    draw->AddRectFilled(l.title.min,l.title.max,color(c.surface));
    draw->AddLine({0,l.title.max.y-1},{l.title.max.x,l.title.max.y-1},color(c.border));
    const float s=l.scale;
    const auto ink=color(state.active?c.text:c.textSecondary);
    // Original geometric mark; no additional image/font asset.
    draw->AddRect({10*s,10*s},{22*s,22*s},color(c.accent),2*s,0,1.5f*s);
    draw->AddLine({13*s,18*s},{19*s,14*s},color(c.accent),1.5f*s);
    ImFont* font=theme.fonts.regular?theme.fonts.regular:ImGui::GetFont();
    const float size=13*s, left=36*s;
    const auto text=ElideWindowTitle(title,l.buttons[0].min.x-left-12*s,font,size);
    draw->PushClipRect({left,0},{std::max(left,l.buttons[0].min.x-8*s),l.title.max.y},true);
    draw->AddText(font,size,{left,(l.title.max.y-size)*.5f},ink,text.c_str());
    draw->PopClipRect();
    for(int i=0;i<3;++i) {
        const auto& r=l.buttons[i];
        auto glyph=ink;
        if(state.hovered==i) {
            const bool down=state.pressed==i;
            draw->AddRectFilled(r.min,r.max,color(i==2?theme.colors.destructive:
                (down?c.control.pressed:c.control.hover)));
            if(i==2) glyph=color(theme.colors.onDestructive);
        }
        const ImVec2 center{(r.min.x+r.max.x)*.5f,(r.min.y+r.max.y)*.5f};
        const float d=4*s, stroke=std::max(1.f,s);
        if(i==0) draw->AddLine({center.x-d,center.y},{center.x+d,center.y},glyph,stroke);
        if(i==1) {
            if(state.maximized) {
                draw->AddPolyline(std::array<ImVec2,3>{{{center.x-2*s,center.y-5*s},{center.x+5*s,center.y-5*s},{center.x+5*s,center.y+2*s}}}.data(),3,glyph,0,stroke);
            }
            draw->AddRect({center.x-d,center.y-d},{center.x+d,center.y+d},glyph,0,0,stroke);
        }
        if(i==2) {
            draw->AddLine({center.x-d,center.y-d},{center.x+d,center.y+d},glyph,stroke);
            draw->AddLine({center.x-d,center.y+d},{center.x+d,center.y-d},glyph,stroke);
        }
    }
    return {l,state.requested};
}
}
