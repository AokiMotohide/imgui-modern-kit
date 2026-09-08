#include "design_lab.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace imkit::design {
namespace {
ImVec2 P(float x,float y) { return {x,y}; }
ImVec2 Plus(ImVec2 a,ImVec2 b) { return {a.x+b.x,a.y+b.y}; }
ImVec4 Hex(unsigned n) { return {((n>>16)&255)/255.f,((n>>8)&255)/255.f,(n&255)/255.f,1}; }
ImVec4 Mix(ImVec4 a,ImVec4 b,float f) { return {a.x+(b.x-a.x)*f,a.y+(b.y-a.y)*f,a.z+(b.z-a.z)*f,a.w+(b.w-a.w)*f}; }
ImU32 C(ImVec4 c,float alpha=1) { c.w*=alpha; return ImGui::ColorConvertFloat4ToU32(c); }
struct Paint {
    DesignTokens t;
    DesignLabState& s;
    ImDrawList* d;
    void Text(ImVec2 p,const char* str,float size=0,bool bold=false, bool muted=false) const {
        d->AddText(bold?s.semibold:s.regular,size?size:t.body,p,C(muted?t.color.muted:t.color.text),str);
    }
    void Label(ImVec2 p,const char* str) const { Text(p,str,12,false,true); }
    void Shadow(ImVec2 p,ImVec2 size,float strength=1) const {
        if(t.shadow<=0) return;
        for(int i=8;i>0;--i) {
            float r=i*t.shadow/8;
            d->AddRectFilled(P(p.x-r,p.y-r+3),P(p.x+size.x+r,p.y+size.y+r+3),
                IM_COL32(0,0,0,static_cast<int>((t.dark?5:3)*strength)),t.radius+r);
        }
    }
    void WindowElevation() const {
        if(t.shadow<=0)return;
        auto pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();auto* draw=ImGui::GetWindowDrawList();
        draw->PushClipRectFullScreen();
        for(int i=static_cast<int>(t.shadow);i>0;--i) {
            draw->AddRect(P(pos.x-i,pos.y-i),P(pos.x+size.x+i,pos.y+size.y+i),IM_COL32(0,0,0,t.dark?16:10),t.radius+i,1.f,ImDrawFlags_None);
        }
        draw->PopClipRect();
    }
    void Box(ImVec2 p,ImVec2 size,ImVec4 fill,float border=-1,float radius=-1) const {
        float r=radius<0?t.radius:radius;
        d->AddRectFilled(p,Plus(p,size),C(fill),r);
        float b=border<0?t.border:border;
        if(b>0) d->AddRect(p,Plus(p,size),C(t.color.border),r,0,b);
    }
    void Focus(ImVec2 p,ImVec2 size) const {
        d->AddRect(P(p.x-3,p.y-3),P(p.x+size.x+3,p.y+size.y+3),C(t.color.focus),t.radius+3,0,t.focus);
        if(t.proposal==2) d->AddRect(P(p.x-6,p.y-6),P(p.x+size.x+6,p.y+size.y+6),C(t.color.text),t.radius+5);
        if(t.proposal==4) d->AddLine(P(p.x,p.y+size.y-1),P(p.x+size.x,p.y+size.y-1),C(t.color.focus),3);
    }
    void Icon(ImVec2 center,int icon,ImVec4 color,float scale=1) const {
        auto col=C(color); float r=5*scale;
        if(icon==0) { d->AddLine(P(center.x-r,center.y),P(center.x+r,center.y),col,1.7f); d->AddLine(P(center.x,center.y-r),P(center.x,center.y+r),col,1.7f); }
        if(icon==1) { d->AddCircle(P(center.x-2,center.y-2),r,col,16,1.7f); d->AddLine(P(center.x+2,center.y+2),P(center.x+7,center.y+7),col,1.7f); }
        if(icon==2) { d->AddLine(P(center.x-4,center.y-2),P(center.x,center.y+2),col,1.7f); d->AddLine(P(center.x,center.y+2),P(center.x+4,center.y-2),col,1.7f); }
        if(icon==3) { d->AddRect(P(center.x-4,center.y-3),P(center.x+4,center.y+6),col,1); d->AddLine(P(center.x-6,center.y-5),P(center.x+6,center.y-5),col,1.7f); d->AddLine(P(center.x-2,center.y-7),P(center.x+2,center.y-7),col,1.7f); }
        if(icon==4) { d->AddLine(P(center.x-4,center.y),P(center.x-1,center.y+3),col,2); d->AddLine(P(center.x-1,center.y+3),P(center.x+5,center.y-4),col,2); }
    }
    void Button(ImVec2 p,ImVec2 size,const char* label,Action action,SpecimenState st,float hover=-1,float pressed=-1) const {
        bool disabled=st==SpecimenState::Disabled;
        ImVec4 fill=t.color.raised, fg=t.color.text;
        float border=t.border;
        if(action==Action::Primary) {fill=t.color.accent;fg=t.color.onAccent;border=0;}
        if(action==Action::Destructive) {fill=t.color.danger;fg=t.color.onDanger;border=0;}
        if(action==Action::Ghost || action==Action::Icon) {fill=t.color.surface;border=0;}
        float h=hover<0?(st==SpecimenState::Hover?1.f:0.f):hover;
        float a=pressed<0?(st==SpecimenState::Pressed?1.f:0.f):pressed;
        fill=Mix(fill,action==Action::Primary||action==Action::Destructive?fg:t.color.selection,h*.14f);
        fill=Mix(fill,t.color.text,a*(t.dark?.2f:.12f));
        if(disabled) { fill=Mix(t.color.surface,fill,.35f);fg=Mix(t.color.surface,fg,.40f); }
        if(t.proposal==3) p.y+=a*2;
        if((t.proposal==1||t.proposal==3) && (t.refinement<0 || (t.refinement>0 && action!=Action::Ghost && action!=Action::Icon)))
            Shadow(p,size,disabled?.15f:(1-a)*.45f);
        Box(p,size,fill,border);
        if(action==Action::Icon) Icon(P(p.x+size.x*.5f,p.y+size.y*.5f),0,fg);
        else {
            auto f=s.semibold; float fs=t.body;
            ImVec2 textSize=f->CalcTextSizeA(fs,1000,0,label);
            d->AddText(f,fs,P(p.x+(size.x-textSize.x)*.5f,p.y+(size.y-textSize.y)*.5f),C(fg),label);
        }
        if(st==SpecimenState::Focused) Focus(p,size);
    }
    void Selection(ImVec2 p,int kind,int value,bool disabled=false,float fraction=-1) const {
        ImVec4 ink=disabled?Mix(t.color.surface,t.color.accent,.35f):t.color.accent;
        ImVec4 fg=disabled?Mix(t.color.surface,t.color.onAccent,.45f):t.color.onAccent;
        if(kind==0) {
            float f=fraction<0?(value?1.f:0.f):fraction;
            ImVec4 track=Mix(t.color.border,ink,f);
            d->AddRectFilled(p,P(p.x+44,p.y+24),C(track),12);
            if(t.proposal==2) d->AddRect(p,P(p.x+44,p.y+24),C(t.color.text),12,0,1.5f);
            d->AddCircleFilled(P(p.x+12+20*f,p.y+12),8,C(value?fg:t.color.surface));
        } else if(kind==1) {
            Box(p,P(22,22),value?ink:t.color.inset,disabled?1:t.border,std::min(t.radius,5.f));
            if(value==2) d->AddLine(P(p.x+6,p.y+11),P(p.x+16,p.y+11),C(fg),2);
            else if(value) Icon(P(p.x+11,p.y+11),4,fg);
        } else {
            d->AddCircleFilled(P(p.x+11,p.y+11),11,C(t.color.inset));
            d->AddCircle(P(p.x+11,p.y+11),10,C(value?ink:t.color.border),0,std::max(1.f,t.border));
            if(value) d->AddCircleFilled(P(p.x+11,p.y+11),5,C(ink));
        }
    }
    void Slider(ImVec2 p,float width,float lo,float hi,bool disabled=false) const {
        auto ink=disabled?Mix(t.color.surface,t.color.accent,.35f):t.color.accent;
        float thick=t.proposal==2?7.f:5.f;
        d->AddRectFilled(P(p.x,p.y+11),P(p.x+width,p.y+11+thick),C(t.color.border),thick/2);
        d->AddRectFilled(P(p.x+width*lo,p.y+11),P(p.x+width*hi,p.y+11+thick),C(ink),thick/2);
        for(int i=0;i<(lo>0?2:1);++i) {
            float x=p.x+width*(i==0?hi:lo);
            d->AddCircleFilled(P(x,p.y+13.5f),t.proposal==2?9.f:8.f,C(t.color.surface));
            d->AddCircle(P(x,p.y+13.5f),t.proposal==2?9.f:8.f,C(ink),0,2);
        }
    }
    void Field(ImVec2 p,ImVec2 size,const char* text,SpecimenState st,bool search=false,bool error=false,bool placeholder=false) const {
        ImVec4 bg=st==SpecimenState::Disabled?Mix(t.color.inset,t.color.surface,.7f):t.color.inset;
        Box(p,size,bg);
        if(t.proposal==4) d->AddLine(P(p.x,p.y+size.y),Plus(p,size),C(t.color.border),1);
        if(error) d->AddRect(p,Plus(p,size),C(t.color.danger),t.radius,0,1.5f);
        if(st==SpecimenState::Focused) Focus(p,size);
        if(search) Icon(P(p.x+18,p.y+size.y/2),1,t.color.muted);
        Text(P(p.x+(search?38:12),p.y+(size.y-t.body)/2),text,0,false,placeholder||st==SpecimenState::Disabled);
    }
    void ValueField(ImVec2 pos,float width,const char* value,const char* unit,bool disabled=false) const {
        Field(pos,P(width,t.height),"",disabled?SpecimenState::Disabled:SpecimenState::Normal);
        float unitWidth=unit[0]?32.f:0.f;
        auto textSize=s.regular->CalcTextSizeA(t.body,1000,0,value);
        Text(P(pos.x+width-12-unitWidth-textSize.x,pos.y+(t.height-t.body)/2),value,0,false,disabled);
        if(unit[0])Label(P(pos.x+width-28,pos.y+(t.height-12)/2),unit);
    }
};

void Probe(DesignLabState& s,const std::string& name) {
    s.probes[name]={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
    s.focusStates[name]=ImGui::IsItemFocused();
}
float Approach(float from,float to,float duration) {
    float step=ImGui::GetIO().DeltaTime/std::max(.001f,duration);
    return from<to?std::min(to,from+step):std::max(to,from-step);
}
bool ActionButton(Paint& p,const char* id,const char* label,Action action,ImVec2 size={},bool disabled=false) {
    if(size.x==0) size={132,p.t.height};
    ImGui::BeginDisabled(disabled);
    ImVec2 pos=ImGui::GetCursorScreenPos(); ImGuiID key=ImGui::GetID(id);
    bool hit=ImGui::InvisibleButton(id,size,ImGuiButtonFlags_EnableNav);
    Probe(p.s,id);
    bool hov=ImGui::IsItemHovered(),active=ImGui::IsItemActive(),focus=ImGui::IsItemFocused();
    auto& motion=p.s.motions[key];
    motion.hover=Approach(motion.hover,hov?1.f:0.f,p.t.animation);
    motion.pressed=Approach(motion.pressed,active?1.f:0.f,p.t.animation);
    auto st=disabled?SpecimenState::Disabled:focus?SpecimenState::Focused:SpecimenState::Normal;
    p.Button(pos,size,label,action,st,motion.hover,motion.pressed);
    ImGui::EndDisabled(); return hit;
}
bool SelectionControl(Paint& p,const char* id,const char* label,int kind,int& value,bool disabled=false) {
    ImGui::BeginDisabled(disabled);
    auto pos=ImGui::GetCursorScreenPos(); ImGuiID key=ImGui::GetID(id);
    bool hit=ImGui::InvisibleButton(id,P(240,p.t.height),ImGuiButtonFlags_EnableNav);
    Probe(p.s,id);
    if(hit) value=value==2?1:!value;
    auto& m=p.s.motions[key]; m.selected=Approach(m.selected,value?1.f:0.f,p.t.animation);
    p.Selection(P(pos.x,pos.y+(p.t.height-24)/2),kind,value,disabled,m.selected);
    p.Text(P(pos.x+60,pos.y+(p.t.height-p.t.body)/2),label,0,false,disabled);
    if(ImGui::IsItemFocused()) p.Focus(pos,P(240,p.t.height));
    ImGui::EndDisabled(); return hit;
}
void NativeField(Paint& p,const char* id,char* buffer,size_t size,const char* hint,bool error=false,bool disabled=false) {
    ImGui::BeginDisabled(disabled);
    auto pos=ImGui::GetCursorScreenPos();
    bool search=std::strcmp(id,"search")==0;
    p.Field(pos,P(340,p.t.height),"",SpecimenState::Normal,search,error);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,P(search?38.f:12.f,(p.t.height-p.t.body)/2));
    ImGui::SetNextItemWidth(340);
    std::string hidden="##";hidden+=id;
    ImGui::InputTextWithHint(hidden.c_str(),hint,buffer,size);
    Probe(p.s,id);
    if(ImGui::IsItemActive()||ImGui::IsItemFocused()) p.Focus(pos,P(340,p.t.height));
    ImGui::PopStyleVar(2); ImGui::PopStyleColor(); ImGui::EndDisabled();
}
void LiveSlider(Paint& p,const char* id,const char* label,float& value,bool disabled=false) {
    ImGui::TextUnformatted(label);ImGui::SameLine(460);ImGui::Text("%.0f%%",value*100);
    auto pos=ImGui::GetCursorScreenPos();ImGui::BeginDisabled(disabled);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0);ImGui::SetNextItemWidth(520);
    std::string hidden="##";hidden+=id;ImGui::SliderFloat(hidden.c_str(),&value,0,1,"");Probe(p.s,id);
    bool focus=ImGui::IsItemFocused();ImGui::PopStyleVar();ImGui::EndDisabled();
    p.Slider(P(pos.x+8,pos.y+(p.t.height-27)/2),504,0,value,disabled);
    if(focus)p.Focus(pos,P(520,p.t.height));
}
bool LiveCombo(Paint& p,const char* id,const char* text,float width) {
    auto pos=ImGui::GetCursorScreenPos();p.Field(pos,P(width,p.t.height),text,SpecimenState::Normal);
    p.Icon(P(pos.x+width-22,pos.y+p.t.height/2),2,p.t.color.muted);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(0,0,0,0));ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::SetNextItemWidth(width);std::string hidden="##";hidden+=id;
    bool open=ImGui::BeginCombo(hidden.c_str(),"",ImGuiComboFlags_NoArrowButton);Probe(p.s,id);
    ImGui::PopStyleVar();ImGui::PopStyleColor(3);
    if(open) {p.Focus(pos,P(width,p.t.height));p.WindowElevation();}
    return open;
}
bool ComboItem(Paint& p,const char* id,const char* label,bool selected,bool disabled=false) {
    ImGui::BeginDisabled(disabled);
    auto pos=ImGui::GetCursorScreenPos();ImVec2 size=P(ImGui::GetContentRegionAvail().x,p.t.height);
    ImGui::PushStyleColor(ImGuiCol_Header,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImVec4(0,0,0,0));
    std::string hidden="##";hidden+=id;
    bool hit=ImGui::Selectable(hidden.c_str(),selected,0,size);Probe(p.s,id);
    auto* previous=p.d;p.d=ImGui::GetWindowDrawList();
    bool hover=ImGui::IsItemHovered();
    if(selected||hover)p.Box(pos,size,selected?p.t.color.selection:Mix(p.t.color.raised,p.t.color.text,.07f),0);
    p.Text(P(pos.x+12,pos.y+(size.y-p.t.body)/2),label,0,selected,disabled);
    if(selected)p.Icon(P(pos.x+size.x-20,pos.y+size.y/2),4,p.t.color.accent);
    if(ImGui::IsItemFocused())p.Focus(pos,size);
    p.d=previous;ImGui::PopStyleColor(3);ImGui::EndDisabled();return hit;
}
void Apply(const DesignTokens& t) {
    ImGuiStyle style; ImGui::GetStyle()=style;
    auto& st=ImGui::GetStyle();
    st.WindowPadding=P(0,0);st.ChildBorderSize=0;st.WindowBorderSize=t.border;
    st.FramePadding=P(12,(t.height-t.body)/2);st.ItemSpacing=P(t.spacing,t.spacing);
    st.FrameRounding=t.radius;st.GrabRounding=t.radius;st.PopupRounding=t.radius;
    st.WindowRounding=t.radius;st.FrameBorderSize=t.border;st.PopupBorderSize=std::max(1.f,t.border);
    st.ScrollbarSize=10;st.TabRounding=t.radius;st.GrabMinSize=12;st.DisabledAlpha=.4f;
    auto& c=st.Colors;
    c[ImGuiCol_Text]=t.color.text;c[ImGuiCol_TextDisabled]=t.color.muted;
    c[ImGuiCol_WindowBg]=t.color.canvas;c[ImGuiCol_ChildBg]=ImVec4(0,0,0,0);
    c[ImGuiCol_PopupBg]=t.color.raised;c[ImGuiCol_Border]=t.color.border;
    c[ImGuiCol_FrameBg]=t.color.inset;c[ImGuiCol_FrameBgHovered]=t.color.selection;
    c[ImGuiCol_FrameBgActive]=Mix(t.color.inset,t.color.accent,.2f);
    c[ImGuiCol_Button]=t.color.raised;c[ImGuiCol_ButtonHovered]=t.color.selection;c[ImGuiCol_ButtonActive]=Mix(t.color.raised,t.color.accent,.3f);
    c[ImGuiCol_CheckMark]=t.color.accent;c[ImGuiCol_SliderGrab]=t.color.accent;c[ImGuiCol_SliderGrabActive]=t.color.focus;
    c[ImGuiCol_Header]=t.color.selection;c[ImGuiCol_HeaderHovered]=Mix(t.color.selection,t.color.accent,.12f);c[ImGuiCol_HeaderActive]=t.color.selection;
    c[ImGuiCol_Tab]=t.color.inset;c[ImGuiCol_TabHovered]=t.color.selection;c[ImGuiCol_TabSelected]=t.color.selection;c[ImGuiCol_TabSelectedOverline]=t.color.accent;
    c[ImGuiCol_TableHeaderBg]=t.color.inset;c[ImGuiCol_TableBorderStrong]=t.color.border;c[ImGuiCol_TableBorderLight]=t.color.border;
    c[ImGuiCol_TableRowBg]=t.color.surface;c[ImGuiCol_TableRowBgAlt]=t.color.inset;
    c[ImGuiCol_NavCursor]=t.color.focus;c[ImGuiCol_TextSelectedBg]=t.color.selection;
    c[ImGuiCol_ModalWindowDimBg]=ImVec4(0.02f,.03f,.05f,t.dark?.62f:.30f);
    c[ImGuiCol_TitleBgActive]=t.color.raised;c[ImGuiCol_TitleBg]=t.color.raised;
    c[ImGuiCol_PlotHistogram]=t.color.accent;
}

void SampleButtons(Paint& p,ImVec2 o) {
    const char* states[]={"Normal","Hover","Pressed","Focused","Disabled"};
    const char* actions[]={"Primary","Secondary","Ghost","Destructive","Icon"};
    for(int j=0;j<5;++j) p.Label(P(o.x+112+j*91,o.y),states[j]);
    for(int i=0;i<5;++i) {
        float y=o.y+31+i*53;
        p.Label(P(o.x,y+(p.t.height-12)/2),actions[i]);
        for(int j=0;j<5;++j) p.Button(P(o.x+106+j*91,y),P(81,p.t.height),i==3?"Delete":"Apply",static_cast<Action>(i),static_cast<SpecimenState>(j));
    }
}
void SampleSelection(Paint& p,ImVec2 o) {
    const char* cols[]={"Unselected","Selected","Disabled"};
    const char* rows[]={"Switch","Checkbox","Mixed","Radio"};
    for(int j=0;j<3;++j) p.Label(P(o.x+124+j*146,o.y),cols[j]);
    for(int i=0;i<4;++i) {
        float y=o.y+36+i*57; p.Text(P(o.x,y+4),rows[i]);
        for(int j=0;j<3;++j) p.Selection(P(o.x+142+j*146,y),i==0?0:i==3?2:1,i==2?2:j==0?0:1,j==2);
    }
    p.Label(P(o.x,o.y+280),"A distinct mark carries selection beyond color.");
}
void SampleNumeric(Paint& p,ImVec2 o) {
    p.Text(o,"Volume",0,true);p.Text(P(o.x+465,o.y),"64%",0,true);
    p.Slider(P(o.x+8,o.y+27),520,0,.64f);
    p.Label(P(o.x,o.y+66),"0");p.Label(P(o.x+517,o.y+66),"100");
    p.Text(P(o.x,o.y+102),"Range",0,true);p.Text(P(o.x+454,o.y+102),"20 - 80");
    p.Slider(P(o.x+8,o.y+126),520,.2f,.8f);
    p.Label(P(o.x,o.y+177),p.t.refinement>=0?"Numeric input / units":"Numeric input");
    p.Label(P(o.x+288,o.y+177),"Disabled");
    if(p.t.refinement>=0) {
        p.ValueField(P(o.x,o.y+199),250,"48.000","px");p.ValueField(P(o.x+288,o.y+199),250,"48.000","px",true);
        const char* axes[]={"X","Y","Z"};const char* values[]={"0.000","12.500","-2.000"};
        for(int i=0;i<3;++i) {p.Label(P(o.x+i*184,o.y+275),axes[i]);p.ValueField(P(o.x+20+i*184,o.y+260),148,values[i],"");}
    } else {
        p.Field(P(o.x,o.y+199),P(180,p.t.height),"48",SpecimenState::Normal);
        p.Field(P(o.x+288,o.y+199),P(250,p.t.height),"48",SpecimenState::Disabled);
        p.Slider(P(o.x+8,o.y+270),520,0,.64f,true);
    }
}
void SampleFields(Paint& p,ImVec2 o) {
    const char* labels[]={"Default","Search","Placeholder","Focused","Validation","Disabled"};
    const char* values[]={"Display","Search settings","Enter a value","Display","Value required","Display"};
    for(int i=0;i<6;++i) {
        float y=o.y+i*47;
        p.Label(P(o.x,y+(p.t.height-12)/2),labels[i]);
        p.Field(P(o.x+110,y),P(420,p.t.height),values[i],i==3?SpecimenState::Focused:i==5?SpecimenState::Disabled:SpecimenState::Normal,i==1,i==4,i==1||i==2);
    }
}
void SampleCombo(Paint& p,ImVec2 o) {
    p.Label(o,"Closed");p.Field(P(o.x,o.y+24),P(248,p.t.height),"Medium",SpecimenState::Normal);p.Icon(P(o.x+226,o.y+24+p.t.height/2),2,p.t.color.muted);
    p.Label(P(o.x+286,o.y),"Open popup");
    p.Field(P(o.x+286,o.y+24),P(248,p.t.height),"Medium",SpecimenState::Focused);p.Icon(P(o.x+512,o.y+24+p.t.height/2),2,p.t.color.muted);
    ImVec2 q=P(o.x+286,o.y+36+p.t.height);p.Shadow(q,P(248,203));p.Box(q,P(248,203),p.t.color.raised,1);
    const char* items[]={"Low","Medium","High","Unavailable"};
    for(int i=0;i<4;++i) {
        ImVec2 r=P(q.x+8,q.y+9+i*46);
        if(i==1||i==2) p.Box(r,P(232,40),i==1?p.t.color.selection:Mix(p.t.color.raised,p.t.color.text,.07f),0);
        p.Text(P(r.x+12,r.y+12),items[i],0,i==1,i==3);
        if(i==1) p.Icon(P(r.x+211,r.y+20),4,p.t.color.accent);
    }
    p.Label(P(o.x,o.y+111),"Selected: Medium");p.Label(P(o.x,o.y+137),"Hovered: High");p.Label(P(o.x,o.y+163),"Disabled: Unavailable");
    p.Label(P(o.x,o.y+287),"Stable value, clear position, keyboard navigation.");
}
void SampleTabs(Paint& p,ImVec2 o) {
    const char* labels[]={"General","Display","Quality","Advanced"};
    for(int i=0;i<4;++i) {
        ImVec2 pos=P(o.x+i*137,o.y+23);bool active=i==1;
        if((active&&p.t.refinement!=0)||i==2) p.Box(pos,P(126,p.t.height),active?p.t.color.selection:p.t.color.inset,0);
        if(active&&p.t.refinement==2)p.Focus(pos,P(126,p.t.height));
        p.Text(P(pos.x+12,pos.y+(p.t.height-p.t.body)/2),labels[i],0,active,i==3);
        if(active) p.d->AddLine(P(pos.x,pos.y+p.t.height+4),P(pos.x+126,pos.y+p.t.height+4),C(p.t.color.accent),p.t.proposal==2?4:2);
    }
    const char* status[]={"Inactive","Active","Hover","Disabled"};
    for(int i=0;i<4;++i) p.Label(P(o.x+12+i*137,o.y+84),status[i]);
    p.Box(P(o.x,o.y+128),P(548,157),p.t.color.inset,0);
    p.Text(P(o.x+22,o.y+150),"Display settings",p.t.heading,true);
    p.Text(P(o.x+22,o.y+191),"A quiet surface keeps the active section clear.",0,false,true);
    p.Label(P(o.x+22,o.y+250),"One active section / consistent content position");
}
void SampleTable(Paint& p,ImVec2 o) {
    p.Box(o,P(548,40),p.t.color.inset,0);
    p.Label(P(o.x+14,o.y+14),"NAME");p.Label(P(o.x+247,o.y+14),"QUALITY");p.Label(P(o.x+439,o.y+14),"ACTION");
    p.Icon(P(o.x+315,o.y+19),2,p.t.color.text);
    const char* names[]={"Low","Medium","High","Custom"};const char* val[]={"24","64","96","48"};
    for(int i=0;i<4;++i) {
        float y=o.y+44+i*54;
        p.Box(P(o.x,y),P(548,50),i==1?p.t.color.selection:i%2?p.t.color.inset:p.t.color.surface,0);
        if(i==1) {p.d->AddRectFilled(P(o.x,y+6),P(o.x+3,y+44),C(p.t.color.accent),1);p.Icon(P(o.x+17,y+25),4,p.t.color.accent);}
        p.Text(P(o.x+35,y+18),names[i],0,i==1);p.Text(P(o.x+261,y+18),val[i]);
        p.Button(P(o.x+423,y+7),P(108,36),"Edit",Action::Ghost,SpecimenState::Normal);
        p.d->AddLine(P(o.x,y+50),P(o.x+548,y+50),C(p.t.color.border),p.t.proposal==4?1:.5f);
    }
    p.Label(P(o.x,o.y+286),"4 items / 1 selected / Sort: Quality");
}
void SampleOverlays(Paint& p,ImVec2 o) {
    p.Label(o,"Overlay Preview / Modal");
    ImVec2 q=P(o.x,o.y+28);p.Shadow(q,P(315,220));p.Box(q,P(315,220),p.t.color.raised,1);
    p.Text(P(q.x+20,q.y+20),"Apply settings?",p.t.heading,true);
    p.Text(P(q.x+20,q.y+67),"Review the changes before applying.",13,false,true);
    p.Button(P(q.x+18,q.y+113),P(129,p.t.height),"Apply",Action::Primary,SpecimenState::Normal);
    p.Button(P(q.x+163,q.y+113),P(129,p.t.height),"Cancel",Action::Secondary,SpecimenState::Normal);
    p.Button(P(q.x+18,q.y+171),P(129,32),"Delete",Action::Destructive,SpecimenState::Normal);
    p.Label(P(o.x+344,o.y),"Popup action");
    q=P(o.x+344,o.y+28);p.Shadow(q,P(204,154));p.Box(q,P(204,154),p.t.color.raised,1);
    p.Text(P(q.x+16,q.y+19),"Edit settings");p.Text(P(q.x+16,q.y+63),"Reset values");
    p.d->AddLine(P(q.x+12,q.y+98),P(q.x+192,q.y+98),C(p.t.color.border));
    p.d->AddText(p.s.regular,p.t.body,P(q.x+16,q.y+117),C(p.t.color.danger),"Delete");
    p.Label(P(o.x+344,o.y+201),"Tooltip");
    p.Box(P(o.x+344,o.y+227),P(204,40),p.t.color.raised,1);
    p.Text(P(o.x+358,o.y+239),"Change display quality",12);
    p.Label(P(o.x,o.y+286),"Appearance samples / open real overlays in Interactive");
}
void SampleProgress(Paint& p,ImVec2 o) {
    p.Text(o,"Progress",0,true);p.Text(P(o.x+496,o.y),"64%");
    p.Box(P(o.x,o.y+35),P(548,12),p.t.color.inset,0,6);
    p.Box(P(o.x,o.y+35),P(548*.64f,12),p.t.color.accent,0,6);
    p.Label(P(o.x,o.y+79),"Selectable");
    const char* rows[]={"General","Display","Advanced"};
    for(int i=0;i<3;++i) {
        ImVec2 q=P(o.x,o.y+105+i*58);p.Box(q,P(548,48),i==1?p.t.color.selection:p.t.color.surface,i==1?0:p.t.border);
        p.Text(P(q.x+16,q.y+16),rows[i],0,i==1,i==2);
        if(i==1) p.Icon(P(q.x+523,q.y+24),4,p.t.color.accent);
        p.Label(P(q.x+408,q.y+17),i==1?"Selected":i==2?"Disabled":"Available");
    }
    p.Label(P(o.x,o.y+291),"Familiar behavior / experimental appearance");
}

void Interactive(Paint& p,int panel) {
    auto& s=p.s; ImGui::PushID(panel);
    if(panel==0) {
        if(ActionButton(p,"apply","Apply",Action::Primary)) ++s.clicks;
        ImGui::SameLine();ActionButton(p,"secondary","Secondary",Action::Secondary);
        if(ActionButton(p,"ghost","Ghost",Action::Ghost)) ++s.clicks;
        ImGui::SameLine();if(ActionButton(p,"delete","Delete",Action::Destructive)) ++s.clicks;
        ActionButton(p,"icon","",Action::Icon,P(p.t.height,p.t.height));
        ImGui::SameLine();ActionButton(p,"disabled","Disabled",Action::Primary,P(160,p.t.height),true);
        if(s.restoreFocus) {ImGui::SetKeyboardFocusHere();ImGui::SetNavCursorVisible(true);s.restoreFocus=false;}
        if(ActionButton(p,"open-modal","Open modal",Action::Secondary,P(180,p.t.height))) s.openModal=true;
        ImGui::Text("Actions: %d",s.clicks);
        ImGui::TextDisabled("Tab to focus / Space or Enter to activate");
    } else if(panel==1) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,P(p.t.spacing,6));
        int v=s.enabled;SelectionControl(p,"toggle","Enabled",0,v);s.enabled=v!=0;
        v=s.checked;SelectionControl(p,"checkbox","Checkbox",1,v);s.checked=v!=0;
        SelectionControl(p,"mixed","Mixed checkbox",1,s.mixed);
        for(int i=0;i<2;++i) {int selected=s.radio==i;if(SelectionControl(p,i==0?"radio-0":"radio-1",i==0?"General":"Display",2,selected))s.radio=i;}
        v=1;SelectionControl(p,"disabled-check","Disabled",1,v,true);
        ImGui::PopStyleVar();
    } else if(panel==2) {
        LiveSlider(p,"slider","Volume",s.volume);
        ImGui::SetNextItemWidth(410);ImGui::DragFloatRange2("Range",&s.lower,&s.upper,1,0,100,"%.0f","%.0f");Probe(s,"range");
        ImGui::SetNextItemWidth(280);ImGui::InputFloat("Value",&s.number,1,5,"%.0f");Probe(s,"number");
        float disabled=.64f;LiveSlider(p,"disabled-slider","Disabled",disabled,true);
        ImGui::TextDisabled("Range: %.0f - %.0f",s.lower,s.upper);
    } else if(panel==3) {
        NativeField(p,"text",s.text,sizeof(s.text),"Enter a value");
        NativeField(p,"search",s.search,sizeof(s.search),"Search settings");
        NativeField(p,"invalid",s.invalid,sizeof(s.invalid),"Required value",s.invalid[0]=='\0');
        s.validation=s.invalid[0]=='\0';
        ImGui::TextColored(s.validation?p.t.color.danger:p.t.color.muted,s.validation?"Value required":"Value accepted");
        char disabled[32]="Display";NativeField(p,"disabled-input",disabled,sizeof(disabled),"",false,true);
    } else if(panel==4) {
        ImGui::TextUnformatted("Quality");ImGui::SetNextItemWidth(340);
        const char* options[]={"Low","Medium","High"};
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,P(10,10));
        bool open=LiveCombo(p,"combo",options[s.combo],340);
        s.comboVisible=open;
        if(open) {
            for(int i=0;i<3;++i) {if(ComboItem(p,("option-"+std::to_string(i)).c_str(),options[i],s.combo==i))s.combo=i;if(s.combo==i)ImGui::SetItemDefaultFocus();}
            ComboItem(p,"option-disabled","Unavailable",false,true);ImGui::EndCombo();
        }
        ImGui::PopStyleVar();
        ImGui::TextDisabled("Arrow keys to navigate / Enter to select");
        if(ActionButton(p,"popup-action","Open actions",Action::Secondary,P(190,p.t.height))) {ImGui::OpenPopup("Actions");s.popupTime=0;}
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,P(16,16));
        s.actionVisible=ImGui::BeginPopup("Actions");
        if(s.actionVisible) {p.WindowElevation();if(ImGui::MenuItem("Reset values"))s.volume=.64f;Probe(s,"reset-action");if(ImGui::MenuItem("Delete"))++s.clicks;ImGui::EndPopup();}
        ImGui::PopStyleVar();
    } else if(panel==5) {
        if(ImGui::BeginTabBar("tabs")) {
            const char* tabs[]={"General","Display","Quality","Advanced"};
            for(int i=0;i<4;++i) {
                ImGui::BeginDisabled(i==3);
                bool active=ImGui::BeginTabItem(tabs[i],nullptr,s.resetTabs&&i==0?ImGuiTabItemFlags_SetSelected:0);Probe(s,"tab-"+std::to_string(i));
                if(active) {s.tab=i;ImGui::Text("%s settings",tabs[i]);ImGui::TextDisabled("Only the active section is shown.");ImGui::EndTabItem();}
                ImGui::EndDisabled();
            } ImGui::EndTabBar();s.resetTabs=false;
        }
    } else if(panel==6) {
        if(ImGui::BeginTable("data",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_Sortable|ImGuiTableFlags_BordersInnerH)) {
            ImGui::TableSetupColumn("Name",ImGuiTableColumnFlags_DefaultSort,0,0);ImGui::TableSetupColumn("Quality",0,0,1);ImGui::TableSetupColumn("Action",ImGuiTableColumnFlags_NoSort);
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            for(int i=0;i<3;++i) {ImGui::TableSetColumnIndex(i);ImGui::TableHeader(i==0?"Name":i==1?"Quality":"Action");Probe(s,"sort-"+std::to_string(i));}
            if(auto* sort=ImGui::TableGetSortSpecs();sort&&sort->SpecsCount&&sort->SpecsDirty) {
                auto spec=sort->Specs[0];s.sortDirection=static_cast<int>(spec.SortDirection);
                std::stable_sort(s.rows.begin(),s.rows.end(),[spec](const Row&a,const Row&b){int cmp=spec.ColumnUserID==1?a.quality-b.quality:std::strcmp(a.name,b.name);return spec.SortDirection==ImGuiSortDirection_Ascending?cmp<0:cmp>0;});sort->SpecsDirty=false;
            }
            for(auto& row:s.rows) {
                ImGui::PushID(row.id);ImGui::TableNextRow(0,p.t.height+6);ImGui::TableSetColumnIndex(0);
                if(s.selected==row.id)ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,C(p.t.color.selection));
                if(ImGui::Selectable(row.name,s.selected==row.id))s.selected=row.id;Probe(s,"row-"+std::to_string(row.id));
                ImGui::TableSetColumnIndex(1);ImGui::Text("%d",row.quality);ImGui::TableSetColumnIndex(2);
                if(ActionButton(p,("edit-"+std::to_string(row.id)).c_str(),"Edit",Action::Ghost,P(100,p.t.height)))++s.inlineActions;ImGui::PopID();
            }ImGui::EndTable();
        }
        ImGui::TextDisabled("Inline actions: %d",s.inlineActions);
    } else if(panel==7) {
        if(ActionButton(p,"overlay-modal","Open modal",Action::Primary,P(180,p.t.height)))s.openModal=true;
        ImGui::TextWrapped("A real modal blocks the underlying controls. Open its select to inspect stacking.");
        ActionButton(p,"tooltip","Hover for help",Action::Ghost,P(200,p.t.height));
        if(ImGui::IsItemHovered()) {ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,P(12,10));ImGui::SetTooltip("Change display quality");ImGui::PopStyleVar();}
    } else {
        ImGui::ProgressBar(s.volume,P(500,14));
        const char* names[]={"General","Display","Advanced"};
        for(int i=0;i<3;++i) {
            ImGui::BeginDisabled(i==2);if(ImGui::Selectable(names[i],s.selected==i,0,P(500,p.t.height)))s.selected=i;Probe(s,"selectable-"+std::to_string(i));ImGui::EndDisabled();
        }
    }
    ImGui::PopID();
}
void ActualModal(Paint& p) {
    auto& s=p.s;
    if(s.openModal) {ImGui::OpenPopup("Apply settings?###design-modal");s.modal=true;s.modalTime=0;s.openModal=false;}
    ImGui::SetNextWindowPos(P(690,480),ImGuiCond_Always);
    ImGui::SetNextWindowSize(P(540,420),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,P(28,28));
    s.modalTime=std::min(s.modalTime+ImGui::GetIO().DeltaTime,p.t.overlayAnimation);
    float alpha=std::clamp(s.modalTime/p.t.overlayAnimation,0.f,1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,alpha);
    s.modalVisible=ImGui::BeginPopupModal("Apply settings?###design-modal",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoTitleBar);
    if(s.modalVisible) {
        p.d=ImGui::GetWindowDrawList();
        p.WindowElevation();ImGui::PushFont(s.semibold,p.t.heading);ImGui::TextUnformatted("Apply settings?");ImGui::PopFont();
        ImGui::Spacing();
        ImGui::TextWrapped("Review the changes before applying.");
        ImGui::Spacing();ImGui::SetNextItemWidth(450);
        const char* opts[]={"Low","Medium","High"};
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,P(10,10));
        bool open=LiveCombo(p,"modal-combo",opts[s.combo],450);s.comboVisible=open;
        if(open) {
            for(int i=0;i<3;++i) {if(ComboItem(p,("modal-option-"+std::to_string(i)).c_str(),opts[i],i==s.combo))s.combo=i;if(i==s.combo)ImGui::SetItemDefaultFocus();}
            ComboItem(p,"modal-option-disabled","Unavailable",false,true);ImGui::EndCombo();
        }
        ImGui::PopStyleVar();
        ImGui::Dummy(P(1,20));
        bool close=ActionButton(p,"modal-apply","Apply",Action::Primary);
        ImGui::SameLine();close|=ActionButton(p,"modal-cancel","Cancel",Action::Secondary);
        ImGui::SameLine();close|=ActionButton(p,"modal-delete","Delete",Action::Destructive);
        if(close||(!open&&ImGui::IsKeyPressed(ImGuiKey_Escape))) {ImGui::CloseCurrentPopup();s.modal=false;s.restoreFocus=true;}
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
}
}

std::span<const Proposal> Proposals(bool refined) {
    static constexpr std::array<Proposal,3> refinements{{
        {"Precision Layers","Compact editing, inset values, restrained depth for dense panels.","Precise numeric editing","Smaller interaction targets"},
        {"Balanced Layers","A neutral canvas with clear inputs and selective elevation.","Editing and reading in balance","Moderate control density"},
        {"Focus Layers","Larger targets and stronger selection keep actions easy to locate.","Clear focus and selection","Fewer controls in narrow panels"}
    }};
    static constexpr std::array<Proposal,5> items{{
        {"Compact Outline","Precise boundaries, compact rhythm, immediate feedback.","More controls in view","Smaller interaction targets"},
        {"Soft Surface","Gentle surfaces and rounded controls make groups feel calm.","Approachable and balanced","Subtle surfaces need care"},
        {"Clear Contrast","Strong edges and unmistakable focus make state easy to read.","Highly legible states","A stronger visual presence"},
        {"Layered Depth","Raised surfaces reveal the hierarchy of actions and overlays.","Clear overlay hierarchy","More shadow and movement"},
        {"Spacious Type","Generous rhythm and a clear type scale prioritize reading.","Comfortable reading","Fewer controls in view"}
    }};if(refined)return refinements;return items;
}
DesignTokens Tokens(int proposal,bool dark,bool refined) {
    if(refined) {
        auto t=Tokens(3,dark,false);
        static constexpr float dimensions[3][10]={{28,6,4,1,5,14,18,1.5f,.06f,.10f},{34,9,6,.8f,8,15,21,2,.10f,.15f},{40,12,8,1,10,16,23,2.5f,.10f,.18f}};
        auto v=dimensions[proposal];
        t.height=v[0];t.spacing=v[1];t.radius=v[2];t.border=v[3];t.shadow=v[4];t.body=v[5];t.heading=v[6];t.focus=v[7];t.animation=v[8];t.overlayAnimation=v[9];t.refinement=proposal;
        t.color.canvas=Hex(dark?0x14161b:0xedeef1);t.color.surface=Hex(dark?0x20232a:0xf8f9fb);
        t.color.inset=Hex(dark?0x171a20:0xeff1f5);t.color.raised=Hex(dark?0x30343e:0xffffff);
        t.color.text=Hex(dark?0xeff0f4:0x242833);t.color.muted=Hex(dark?0xacb2bf:0x636b7b);
        t.color.border=Hex(dark?(proposal==2?0x626b7d:0x454d5c):(proposal==2?0xa4acbc:0xcbd0da));
        t.color.selection=Mix(t.color.surface,t.color.accent,dark?(proposal==2?.26f:.16f):(proposal==2?.17f:.09f));
        return t;
    }
    static constexpr float values[5][11]={{28,6,3,1,0,14,18,2,.08f,.10f,0},{36,10,10,.7f,5,15,21,2,.14f,.18f,0},{34,8,2,2,0,15,20,3,.04f,.08f,0},{38,12,8,.7f,12,15,22,2,.12f,.20f,0},{42,14,6,0,4,17,26,2,.16f,.22f,0}};
    static constexpr unsigned neutral[5][7]={{0xf0f2f5,0xffffff,0xf6f7f9,0xffffff,0x202936,0x627080,0xc2cbd6},{0xf2f0ed,0xfaf9f7,0xf0eeea,0xffffff,0x293632,0x63736c,0xd4ddd6},{0xf4f4f4,0xffffff,0xf7f7f7,0xffffff,0x101820,0x48525e,0x657283},{0xeceff5,0xf8faff,0xeaf0f8,0xffffff,0x222a43,0x616c87,0xc9d2e5},{0xf4f1eb,0xfffdf8,0xf3efe7,0xffffff,0x233a39,0x657673,0xbecac3}};
    static constexpr unsigned night[5][7]={{0x11161e,0x1b232e,0x121a25,0x273241,0xe8edf5,0xa1aec1,0x475569},{0x171d1b,0x222d28,0x1a2520,0x2e3d35,0xe7eee8,0xa4b8ac,0x41574a},{0x090e15,0x121b28,0x0e1520,0x182539,0xf6f9ff,0xb8c7da,0x91a4be},{0x101421,0x1c2438,0x141c2e,0x2a3550,0xedf0ff,0xadbad7,0x425475},{0x141e1d,0x1d2c29,0x172421,0x293b36,0xf0f1e7,0xa9beb3,0x536b61}};
    static constexpr unsigned accents[5][2]={{0x285ab4,0x8eb4ff},{0x28675c,0x80cdb8},{0x164ec5,0x94baff},{0x6950b4,0xbba8ff},{0x276457,0x91caba}};
    auto a=dark?night[proposal]:neutral[proposal];auto v=values[proposal];
    Palette p{Hex(a[0]),Hex(a[1]),Hex(a[2]),Hex(a[3]),Hex(a[4]),Hex(a[5]),Hex(a[6]),Hex(accents[proposal][dark]),Hex(dark?0x142031:0xffffff),{},Hex(dark?0xf2a2a5:0xad3441),Hex(dark?0x35191e:0xffffff),Hex(accents[proposal][dark])};
    p.selection=Mix(p.surface,p.accent,dark?.19f:.11f);
    return {p,v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],proposal,dark};
}
void DesignLabState::Reset() {
    int saved=proposal;bool mode=comparison,night=dark;auto* r=regular;auto* b=semibold;
    bool series=refined,editor=paletteOpen;auto colors=palettes;auto custom=customColors;
    *this=DesignLabState{};proposal=saved;comparison=mode;dark=night;regular=r;semibold=b;
    refined=series;paletteOpen=editor;palettes=colors;customColors=custom;
}
void PaletteEditor(DesignLabState& s,const DesignTokens& tokens) {
    if(!s.paletteOpen)return;
    ImGui::SetNextWindowPos(P(1310,202),ImGuiCond_FirstUseEver);ImGui::SetNextWindowSize(P(555,1040),ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,P(22,22));
    if(ImGui::Begin("Palette",&s.paletteOpen,ImGuiWindowFlags_NoSavedSettings)) {
        const int mode=s.dark?1:0;Palette edit=s.customColors[mode]?s.palettes[mode]:tokens.color;
        ImGui::TextUnformatted(s.dark?"Dark palette":"Light palette");
        ImGui::TextWrapped("Colors are independent of density. Changes apply across proposals in this session.");
        auto preset=[&](const char* label,unsigned light,unsigned dark){
            if(ImGui::Button(label)) {edit.accent=Hex(s.dark?dark:light);edit.focus=edit.accent;
                edit.onAccent=Hex(s.dark?0x142031:0xffffff);edit.selection=Mix(edit.surface,edit.accent,s.dark?.18f:.11f);
                s.palettes[mode]=edit;s.customColors[mode]=true;}
        };
        preset("Violet",0x6950b4,0xbba8ff);ImGui::SameLine();preset("Blue",0x235bab,0x8eb8f3);ImGui::SameLine();preset("Teal",0x286c64,0x88caba);
        bool changed=false;
        struct Entry {const char* label;ImVec4* color;};
        Entry entries[]={{"Accent",&edit.accent},{"On accent",&edit.onAccent},{"Canvas",&edit.canvas},{"Surface",&edit.surface},{"Input surface",&edit.inset},{"Raised surface",&edit.raised},{"Text",&edit.text},{"Muted text",&edit.muted},{"Border",&edit.border},{"Selection",&edit.selection},{"Focus",&edit.focus},{"Destructive",&edit.danger},{"On destructive",&edit.onDanger}};
        for(auto& e:entries) {ImGui::SetNextItemWidth(320);changed|=ImGui::ColorEdit3(e.label,&e.color->x,ImGuiColorEditFlags_DisplayHex|ImGuiColorEditFlags_NoInputs);}
        if(changed){s.palettes[mode]=edit;s.customColors[mode]=true;}
        if(ImGui::Button("Reset palette"))s.customColors[mode]=false;
        ImGui::TextWrapped("Light and dark are separate. Reset only clears edited values; Reset palette restores colors. Session only.");
    }
    ImGui::End();ImGui::PopStyleVar();
}
void Show(DesignLabState& s) {
    auto t=Tokens(s.proposal,s.dark,s.refined);if(s.customColors[s.dark?1:0])t.color=s.palettes[s.dark?1:0];
    Apply(t);s.probes.clear();s.focusStates.clear();s.comboVisible=false;
    ImGui::PushFont(s.regular,t.body);
    ImGui::SetNextWindowPos(P(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowContentSize(P(1900,1436));
    ImGui::Begin("Design Lab",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_HorizontalScrollbar);
    Paint p{t,s,ImGui::GetWindowDrawList()};
    ImVec2 origin=P(-ImGui::GetScrollX(),-ImGui::GetScrollY());
    ImGui::SetCursorPos(P(24,20));
    for(int i=0;i<static_cast<int>(Proposals(s.refined).size());++i) {
        ImGui::PushID(i);char label[16];std::snprintf(label,sizeof(label),"%02d",i+1);
        if(ActionButton(p,("proposal-"+std::to_string(i)).c_str(),label,i==s.proposal?Action::Primary:Action::Ghost,P(48,32))) {s.proposal=i;s.motions.clear();}
        ImGui::PopID();ImGui::SameLine();
    }
    if(ActionButton(p,"light","Light",s.dark?Action::Ghost:Action::Primary,P(84,32)))s.dark=false;ImGui::SameLine();
    if(ActionButton(p,"dark","Dark",s.dark?Action::Primary:Action::Ghost,P(84,32)))s.dark=true;ImGui::SameLine();
    if(ActionButton(p,"reset","Reset",Action::Secondary,P(84,32))) {s.Reset();ImGui::SetWindowFocus(nullptr);}ImGui::SameLine();
    if(ActionButton(p,"comparison","Comparison",s.comparison?Action::Primary:Action::Ghost,P(136,32)))s.comparison=true;ImGui::SameLine();
    if(ActionButton(p,"interactive","Interactive",!s.comparison?Action::Primary:Action::Ghost,P(136,32)))s.comparison=false;
    ImGui::SameLine();if(ActionButton(p,"palette","Colors",Action::Secondary,P(96,32)))s.paletteOpen=!s.paletteOpen;
    ImGui::SameLine();if(ActionButton(p,"series",s.refined?"Original 5":"Refined 3",Action::Ghost,P(112,32))) {s.refined=!s.refined;s.proposal=0;s.customColors={};s.motions.clear();}
    ImGui::SameLine();if(ActionButton(p,"japanese","日本語 / Japanese",s.japanese?Action::Primary:Action::Ghost,P(180,32)))s.japanese=!s.japanese;
    if(s.japanese) {
        p.Text(Plus(origin,P(26,82)),"日本語 UI / Japanese specimen",32,true);
        p.Text(Plus(origin,P(26,135)),"表示設定 — ひらがな・カタカナ・漢字・半角ｶﾅ、句読点。",18);
        p.Label(Plus(origin,P(26,171)),"Inter 4.1 + Noto Sans JP 2.004 / experimental gallery only");
        auto panel=Plus(origin,P(24,208));
        p.Box(panel,P(900,600),t.color.surface);
        p.Text(Plus(panel,P(24,24)),"ディスプレイ設定 / Display settings",t.heading,true);
        p.Text(Plus(panel,P(24,68)),"名前を編集し、表示を有効にしてください。",t.body);
        ImGui::SetCursorScreenPos(Plus(panel,P(24,110)));
        ImGui::SetNextItemWidth(420);
        ImGui::InputText("名前 / Name",s.japaneseName,sizeof(s.japaneseName));
        ImGui::SetCursorScreenPos(Plus(panel,P(24,160)));
        ImGui::Checkbox("表示を有効にする / Enabled",&s.japaneseEnabled);
        p.Text(Plus(panel,P(24,218)),"解像度 1920 × 1080 px   /   更新頻度 60 Hz",t.body);
        p.Text(Plus(panel,P(24,258)),"位置 X: 12.50 mm   Y: -8.00 mm   Z: 0.00 mm",t.body);
        p.Text(Plus(panel,P(24,310)),s.japaneseEnabled?"状態：表示中 / Active":"状態：停止中 / Inactive",t.heading,true);
        ImGui::SetCursorScreenPos(Plus(panel,P(24,365)));
        if(ActionButton(p,"japanese-apply","適用 / Apply",Action::Primary,P(170,t.height)))++s.clicks;
        ImGui::SameLine();
        if(ActionButton(p,"japanese-reset","初期値に戻す",Action::Secondary,P(170,t.height))) {
            std::strcpy(s.japaneseName,"ディスプレイ A");s.japaneseEnabled=true;s.clicks=0;
        }
        ImGui::SameLine();ActionButton(p,"japanese-disabled","削除できません",Action::Destructive,P(180,t.height),true);
        char applied[96];std::snprintf(applied,sizeof(applied),"適用回数：%d / Applied",s.clicks);
        p.Text(Plus(panel,P(24,425)),applied,t.body);
        p.Text(Plus(panel,P(24,476)),"入力例：明るさ、品質、回転、拡大率。「保存」で確定。",t.body);
        p.Label(Plus(panel,P(24,530)),"Live controls above / glyph rendering specimen; IME acceptance is separate.");
        p.Label(Plus(origin,P(26,850)),"LATIN PRIMARY: Inter Regular / SemiBold    JAPANESE FALLBACK: Noto Sans JP Regular");
        ImGui::End();PaletteEditor(s,t);ImGui::PopFont();return;
    }
    char title[160];std::snprintf(title,sizeof(title),"%02d  %s",s.proposal+1,Proposals(s.refined)[s.proposal].name);
    p.Text(Plus(origin,P(26,78)),title,38,true);
    p.Text(Plus(origin,P(26,129)),Proposals(s.refined)[s.proposal].intent,17,false,true);
    p.Text(Plus(origin,P(1350,82)),s.dark?"DARK / DESIGN PROPOSAL":"LIGHT / DESIGN PROPOSAL",15,true);
    char meta[180];std::snprintf(meta,sizeof(meta),"Height %.0f   Gap %.0f   Radius %.0f   Motion %.0f / %.0f ms",t.height,t.spacing,t.radius,t.animation*1000,t.overlayAnimation*1000);
    p.Label(Plus(origin,P(1350,116)),meta);
    std::snprintf(meta,sizeof(meta),"Strength: %s  /  Consider: %s",Proposals(s.refined)[s.proposal].strength,Proposals(s.refined)[s.proposal].caution);
    p.Label(Plus(origin,P(26,168)),meta);
    p.Label(Plus(origin,P(1350,153)),s.comparison?"STATE SAMPLES / shared rendering / fixed data":"INTERACTIVE / live input / experimental controls");
    const char* titles[]={"Buttons","Selection controls","Numeric controls","Text fields","Combo / Select","Tabs","Table","Overlays","Progress / Selectable"};
    for(int i=0;i<9;++i) {
        auto pos=Plus(origin,P(24+(i%3)*632.f,208+(i/3)*404.f));
        if((t.proposal==1||t.proposal==3)&&t.refinement!=0)p.Shadow(pos,P(608,380),.4f);
        p.Box(pos,P(608,380),t.color.surface,t.proposal==4?0:t.border,t.radius+3);
        char index[8];std::snprintf(index,sizeof(index),"%02d",i+1);p.Label(P(pos.x+22,pos.y+25),index);
        p.Text(P(pos.x+54,pos.y+19),titles[i],t.heading,true);
        p.d->AddLine(P(pos.x+22,pos.y+55),P(pos.x+586,pos.y+55),C(t.color.border,.65f));
        auto content=P(pos.x+24,pos.y+72);
        if(s.comparison) {
            switch(i) {case 0:SampleButtons(p,content);break;case 1:SampleSelection(p,content);break;case 2:SampleNumeric(p,content);break;case 3:SampleFields(p,content);break;case 4:SampleCombo(p,content);break;case 5:SampleTabs(p,content);break;case 6:SampleTable(p,content);break;case 7:SampleOverlays(p,content);break;case 8:SampleProgress(p,content);break;}
        } else {
            ImGui::SetCursorScreenPos(content);ImGui::PushID(i);
            ImGui::BeginChild("content",P(560,298),0,ImGuiWindowFlags_NoSavedSettings);
            p.d=ImGui::GetWindowDrawList();Interactive(p,i);ImGui::EndChild();ImGui::PopID();p.d=ImGui::GetWindowDrawList();
        }
    }
    p.Label(Plus(origin,P(26,1415)),"DESIGN LAB    /    Experimental proposals    /    Inter 4.1    /    Light and dark");
    ActualModal(p);
    ImGui::End();PaletteEditor(s,t);ImGui::PopFont();
}
}
