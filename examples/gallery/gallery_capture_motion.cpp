#include "gallery_host.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace imkit::gallery::host {
using enum imkit::gallery::PageId;

void CaptureFrames(Host &h,const std::filesystem::path &dir,int &index,int count) {
    for(int i=0;i<count;++i) {
        char name[32];std::snprintf(name,sizeof(name),"frame-%03d.png",index++);
        h.Frame({},dir/name);
    }
}
// Scripted public-IO input for the motion captures. Every frame is recorded, and the pointer moves like a hand.
struct Motion {
    Host& h;
    imkit::design::MotionCapture& rec;
    void Hold(int n) { h.Settle(n); }
    void Move(ImVec2 target,int n=28) {
        auto start=h.mouse;
        for(int i=1;i<=n;++i) {float u=float(i)/n;u=u*u*(3-2*u);h.mouse={start.x+(target.x-start.x)*u,start.y+(target.y-start.y)*u};h.Frame();}
    }
    void ClickAt(ImVec2 p,const char* action) {
        rec.Mark(action);Move(p);h.Frame([](auto&io){io.AddMouseButtonEvent(0,true);});Hold(7);
        h.Frame([](auto&io){io.AddMouseButtonEvent(0,false);});Hold(20);
    }
    const imkit::gallery::Probe& At(const char* name) const { return h.s.probes.at(name); }
    void Click(const char* name) { ClickAt(At(name).Center(),name); }
    void Drag(ImVec2 from,ImVec2 to,const char* action,int frames=66) {
        rec.Mark(action);Move(from);h.Frame([](auto&io){io.AddMouseButtonEvent(0,true);});Hold(8);
        Move(to,frames);h.Frame([](auto&io){io.AddMouseButtonEvent(0,false);});Hold(22);
    }
    // Drags a slider probe (whose rectangle is the slider frame) from one fraction of its width to another.
    void Slide(const char* probe,float from,float to,const char* action,int frames=54) {
        const auto b=At(probe);const float y=b.Center().y,w=b.max.x-b.min.x;
        Drag({b.min.x+w*from,y},{b.min.x+w*to,y},action,frames);
    }
    void Type(const char* probe,const char* text) {
        Click(probe);
        h.Frame([](auto&io){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);});
        h.Frame([](auto&io){io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddKeyEvent(ImGuiKey_A,false);});
        for(auto p=text;*p;++p) {char c[]={*p,0};h.Frame([&](auto&io){io.AddInputCharactersUTF8(c);});Hold(5);}
        h.Key(ImGuiKey_Enter);Hold(18);
    }
    // Wheel ticks with the pointer resting at `at`, one short pause per tick.
    void Scroll(ImVec2 at,int ticks,int pause=7) {
        Move(at,24);
        for(int i=0;i<std::abs(ticks);++i) {h.Frame([&](auto&io){io.AddMouseWheelEvent(0,ticks<0?-1.f:1.f);});Hold(pause);}
    }
    // Scrolls the page until a probe is comfortably inside the window.
    void ScrollTo(const char* name,float targetY) {
        for(int i=0;i<40 && At(name).Center().y>targetY;++i) Scroll({h.mouse.x<0?960.f:h.mouse.x,540.f},-1,5);
    }
};
// One scripted take for the film or a GIF: a page, a look and the hand that drives it.
struct MotionScene {
    const char* name;
    imkit::gallery::PageId page;
    imkit::ThemePreset preset;
    float scale;
    void (*setup)(imkit::gallery::GalleryState&);
    void (*play)(Motion&);
    int minimumFrames;
};
void PlayHook(Motion& m) {
    auto& p=m.h.s.start;
    m.Hold(54); // The plain Dear ImGui version, before anything happens.
    m.Click("start-default-apply");
    m.Slide("start-default-level",.62f,.28f,"Plain slider edits the shared value");
    m.rec.Check("plain slider changed",p.level<.4f,p.level);
    const auto flip=[&](int style,const char* action) {
        m.ClickAt(m.At(style==1?"start-style-imkit":"start-style-plain").Center(),action);
        m.rec.Check("style flipped",p.style==style,p.style);
    };
    flip(1,"Flip to the ImKit theme");
    m.Hold(40);
    m.Slide("start-imkit-level",.28f,.9f,"ImKit slider edits the same value");
    m.rec.Check("modern slider changed",p.level>.8f,p.level);
    m.Click("start-imkit-apply");m.Click("start-imkit-apply");
    m.rec.Check("applied twice",p.applied>=3,p.applied);
    flip(0,"Back to plain");m.Hold(36);
    flip(1,"And to ImKit again");m.Hold(60);
}
void PlayWorlds(Motion& m) {
    auto& s=m.h.s;
    m.Hold(40);
    m.ScrollTo("start-world-2",760.f);m.Hold(30);
    // Graphite is the starting theme; wear each of the others, ending where we began.
    for(int world:{3,4,8,6,7,5,2}) {
        const std::string probe="start-world-"+std::to_string(world);
        m.Click(probe.c_str());m.Hold(42);
        m.rec.Check("world applied",s.presetIndex==world,s.presetIndex);
    }
}
void PlayStudio(Motion& m) {
    auto& s=m.h.s;
    m.Hold(40);
    for(const char* world:{"blueprint","pastel-pop","sticker"}) {
        const std::string probe=std::string("look-world-")+world;
        const auto before=s.theme.semantic.canvas;
        m.Click(probe.c_str());m.Hold(60);
        m.rec.Check("world file applied",s.theme.semantic.canvas.x!=before.x||s.theme.semantic.canvas.z!=before.z||s.theme.semantic.canvas.y!=before.y);
    }
    const bool hard=s.look.hard;
    m.Click("look-hard-shadow");m.Hold(48);
    m.rec.Check("hard shadow toggled",s.look.hard!=hard);
    m.Click("look-hard-shadow");m.Hold(36);
    // Sharp corners, or pill corners when the world already has them: a change the eye cannot miss.
    const int target=s.look.corner==0?3:0;
    {const auto b=m.At("look-corners");m.ClickAt({b.min.x+60.f,b.Center().y},"look-corners");}
    m.h.Key(ImGuiKey_Home);for(int i=0;i<target;++i) m.h.Key(ImGuiKey_DownArrow);m.h.Key(ImGuiKey_Enter);m.Hold(54);
    m.rec.Check("corner style changed",s.look.corner==target,s.look.corner);
    {const auto b=m.At("look-preset");m.ClickAt({b.min.x+60.f,b.Center().y},"look-preset");}m.h.Key(ImGuiKey_Home);m.h.Key(ImGuiKey_DownArrow);m.h.Key(ImGuiKey_DownArrow);m.h.Key(ImGuiKey_Enter);m.Hold(60);
    m.rec.Check("preset restored",s.look.preset==2,s.look.preset);
}
void PlayShowLive(Motion& m) {
    auto& show=m.h.s.show;
    m.Hold(40);
    // ARM, then GO three times. The keys sit just above the Next card, whose rectangle is the GoBar probe.
    const auto bar=m.At("show-gobar");const float k=m.h.s.scale;
    m.ClickAt({bar.min.x+39.f*k,bar.min.y-49.f*k},"ARM the GO key");m.Hold(24);
    for(int i=0;i<3;++i) {
        const int before=show.cue.goCount;
        m.ClickAt({bar.min.x+172.f*k,bar.min.y-49.f*k},"GO");m.Hold(60);
        m.rec.Check("GO fired a cue",show.cue.goCount==before+1,show.cue.goCount);
    }
    // Launch a few clips.
    const auto launcher=m.At("show-launcher");
    for(float u:{.34f,.52f,.70f}) m.ClickAt({launcher.min.x+(launcher.max.x-launcher.min.x)*u,launcher.min.y+(launcher.max.y-launcher.min.y)*.55f},"Launch a clip");
    m.Hold(60);
}
void PlayShowSurface(Motion& m) {
    m.Hold(40);
    const auto surface=m.At("surface");const float w=surface.max.x-surface.min.x,h=surface.max.y-surface.min.y;
    const auto at=[&](float u,float v){return ImVec2{surface.min.x+w*u,surface.min.y+h*v};};
    // Operate: pull the first fader down and back, then the third.
    m.Drag(at(.083f,.29f),at(.083f,.60f),"Pull a fader down",48);
    m.Drag(at(.083f,.60f),at(.083f,.36f),"and back up",48);
    m.Drag(at(.275f,.40f),at(.275f,.24f),"Raise another",48);
    m.Click("surface-edit");m.Hold(60);
    m.rec.Check("edit layout on",m.h.s.show.surface.state.editing);
    m.Hold(90);
}
void PlayCharts(Motion& m) {
    m.Hold(40);
    const auto donut=m.At("charts-donut");
    // Sweep the donut ring, then click a segment and a bar.
    m.Move({donut.min.x+(donut.max.x-donut.min.x)*.25f,donut.min.y+(donut.max.y-donut.min.y)*.28f},40);m.Hold(30);
    m.Move({donut.min.x+(donut.max.x-donut.min.x)*.45f,donut.min.y+(donut.max.y-donut.min.y)*.20f},40);m.Hold(30);
    m.ClickAt({donut.min.x+(donut.max.x-donut.min.x)*.45f,donut.min.y+(donut.max.y-donut.min.y)*.20f},"Pick a donut segment");
    const auto bar=m.At("charts-bar");
    for(float u:{.2f,.45f,.7f}) {
        m.Move({bar.min.x+(bar.max.x-bar.min.x)*u,bar.min.y+(bar.max.y-bar.min.y)*.55f},34);m.Hold(26);
    }
    m.ClickAt({bar.min.x+(bar.max.x-bar.min.x)*.7f,bar.min.y+(bar.max.y-bar.min.y)*.55f},"Pick a bar");
    m.Hold(60);
}
void PlayPtz(Motion& m) {
    m.Hold(40);
    const auto ptz=m.At("ptz-main");const auto c=ptz.Center();
    // Drag across the pad; the simulated camera follows.
    const float r=std::min(ptz.max.x-ptz.min.x,ptz.max.y-ptz.min.y)*.18f;
    m.Drag(c,{c.x+r,c.y-r},"Pan right and tilt up",50);
    m.Drag(c,{c.x-r*1.4f,c.y+r*.5f},"Pan left and tilt down",50);
    m.Hold(90);
}
void PlayTileMap(Motion& m) {
    auto& led=m.h.s.show.led;
    m.Hold(40);
    // A section below the fold submits no items, so it has no probe until it scrolls into view.
    m.Move({960.f,540.f},24);
    for(int i=0;i<150 && (!m.h.s.probes.count("led-tiles")||m.At("led-tiles").max.y>1000.f);++i) {
        m.h.Frame([](auto&io){io.AddMouseWheelEvent(0,-1.f);});m.Hold(3);
    }
    m.Hold(30);
    // Light up one port at a time: the other rows dim and its cable order stays in view.
    for(int port:{1,3,2}) {
        // PortStrip wraps cells of 6 x 2.6 frame heights with a 0.35 gap, and the probe is the whole block, so the
        // cells per row and the frame height follow from the block's proportions.
        const auto ports=m.At("led-ports");const float bw=ports.max.x-ports.min.x,bh=ports.max.y-ports.min.y;
        int perRow=1;float best=1e9f;
        for(int n=1;n<=8;++n) {
            const float err=std::fabs((6.35f*n-.35f)/(2.95f*float((8+n-1)/n)-.35f)-bw/bh);
            if(err<best) {best=err;perRow=n;}
        }
        const float u=bw/(6.35f*perRow-.35f),cell=u*6.f,gap=u*.35f,row=u*2.6f;
        m.ClickAt({ports.min.x+float(port%perRow)*(cell+gap)+cell*.5f,ports.min.y+float(port/perRow)*(row+gap)+row*.5f},"Pick a port");m.Hold(34);
        m.rec.Check("port highlighted",led.selectedPort==port,led.selectedPort);
    }
    // Hover a few tiles for the tooltip, then click one.
    const auto tiles=m.At("led-tiles");const float w=tiles.max.x-tiles.min.x,h=tiles.max.y-tiles.min.y;
    for(const auto cell:{std::pair{1,0},std::pair{5,1},std::pair{2,2}}) {
        m.Move({tiles.min.x+w*(float(cell.first)+.5f)/8.f,tiles.min.y+h*(float(cell.second)+.5f)/4.f},38);m.Hold(36);
    }
    m.ClickAt({tiles.min.x+w*(5.5f/8.f),tiles.min.y+h*(3.5f/4.f)},"Select a tile");m.Hold(50);
    m.rec.Check("tile selected",led.selectedTile>=0,led.selectedTile);
}
void PlaySprite(Motion& m) {
    auto& look=m.h.s.look;
    m.Hold(70); // The sprites loop and the bar advances by itself.
    const float fps=look.spriteFps;
    m.Slide("look-sprite-fps",.33f,.82f,"Play faster",48);m.Hold(60);
    m.rec.Check("playback sped up",look.spriteFps>fps,look.spriteFps);
    m.Slide("look-sprite-fps",.82f,.33f,"and slower",48);m.Hold(50);
    m.Click("look-sprite-restart");m.Hold(130);
}
// Glow frame looks, then a scripted remote session framing the whole window.
void PlayGlow(Motion& m) {
    auto& glow=m.h.s.glow;
    m.Hold(110);
    m.Click("glow-preset-1");m.Hold(170);
    m.rec.Check("orbit look applied",glow.style.motion==imkit::GlowMotion::Orbit,glow.preset);
    m.Click("glow-preset-2");m.Hold(150);
    m.Click("glow-pulse");m.Hold(40);m.Click("glow-pulse");m.Hold(60);
    m.Click("glow-preset-4");m.Hold(130);
    m.Click("glow-preset-5");m.Hold(150);
    m.Click("glow-preset-0");m.Hold(40);
    const float spread=glow.style.spread;
    m.Slide("glow-spread",.24f,.62f,"Spread the light",54);m.Hold(70);
    m.rec.Check("spread widened",glow.style.spread>spread,glow.style.spread);
    m.Slide("glow-intensity",.8f,1.f,"Brighter",40);m.Hold(110);
    m.Click("glow-simulate");
    m.rec.Check("remote session started",glow.simulate,glow.phase);
    m.Hold(760);
}
const MotionScene kScenes[]={
    {"hook",PageStart,imkit::ThemePreset::Graphite,1.5f,[](imkit::gallery::GalleryState& s){s.start.style=0;s.start.level=.62f;},PlayHook,840},
    {"worlds",PageStart,imkit::ThemePreset::Graphite,1.5f,nullptr,PlayWorlds,720},
    {"studio",PageLookStudio,imkit::ThemePreset::Graphite,1.5f,nullptr,PlayStudio,840},
    {"show-live",PageShowLive,imkit::ThemePreset::Graphite,1.5f,nullptr,PlayShowLive,720},
    {"show-surface",PageShowSurface,imkit::ThemePreset::Graphite,1.5f,nullptr,PlayShowSurface,720},
    {"charts",PageCharts,imkit::ThemePreset::Graphite,1.5f,nullptr,PlayCharts,600},
    {"ptz",PagePtz,imkit::ThemePreset::Graphite,1.5f,nullptr,PlayPtz,600},
    {"tilemap",PageShowLed,imkit::ThemePreset::Graphite,1.5f,nullptr,PlayTileMap,720},
    {"sprite",PageLookSkins,imkit::ThemePreset::Graphite,1.5f,nullptr,PlaySprite,600},
    {"glow",PageGlowFrame,imkit::ThemePreset::Graphite,1.25f,nullptr,PlayGlow,1800},
};
const MotionScene* FindScene(const std::string& name) {
    for(const auto& scene:kScenes) if(name==scene.name) return &scene;
    return nullptr;
}
void CaptureMotion(Host& h,const std::filesystem::path& out,const std::string& demo) {
    using namespace imkit;
    if(const auto* scene=FindScene(demo)) {
        h.s.theme=MakeTheme(scene->preset);h.s.theme.fonts=h.s.fonts;
        h.s.scale=scene->scale;h.s.presetIndex=static_cast<int>(scene->preset);h.s.dark=h.s.theme.scheme==ColorScheme::Dark;
        h.s.comparison.open=false;h.s.presentation=true;
        if(scene->setup) scene->setup(h.s);
        h.Page(scene->page);h.mouse={1500,900};h.Settle();
        std::filesystem::create_directories(out);
        {
            // Probe rectangles at the first frame, for tuning a script without re-recording.
            std::ofstream layout(out/(demo+".layout.json"));layout<<"{";bool first=true;
            for(const auto& [name,b]:h.s.probes){if(!first)layout<<",";first=false;layout<<"\""<<name<<"\":["<<b.min.x<<","<<b.min.y<<","<<b.max.x<<","<<b.max.y<<"]";}
            layout<<"}";
        }
        design::MotionCapture recording;recording.Start(out/(demo+".mkv"),1920,1080);h.motion=&recording;
        Motion m{h,recording};
        m.Hold(24);scene->play(m);
        // Closing hold, so a following cut never lands on motion.
        while(recording.Frames()<scene->minimumFrames) m.Hold(6);
        h.motion=nullptr;recording.Finish();
        return;
    }
    h.s.theme=MakeTheme(ThemePreset::Graphite);h.s.theme.fonts=h.s.fonts;
    h.s.scale=1.3f;h.s.presetIndex=2;h.s.dark=true;h.s.comparison.open=false;
    int page=demo=="comparison"?PageCompare:demo=="workspace"?PageWorkbench:demo=="icons"||demo=="themes"||demo=="icon-artwork"?PageIcons:
        demo=="toasts"?PageToasts:demo=="timeline"||demo=="preview-contract"?PageVideo:demo=="workflow"?PageGenericWorkspace:demo=="overview"?PageStart:PageBasic;
    if(page==PageCompare)h.s.comparison.open=true;
    if(page==PageVideo)h.s.editors.Dataset(false);
    if(demo=="preview-contract")h.s.editors.monitorMode=3;
    if(page==PageToasts){h.s.toasts.initialized=true;h.s.toasts.items.clear();h.s.toasts.state.Reset();}
    // The film shows the page itself; the navigation and top bar stay out of frame.
    h.s.presentation=true;
    if(demo=="comparison"||demo=="toasts"||demo=="icons"||demo=="workspace") h.s.scale=2.0f;
    h.Page(page);h.mouse={350,150};h.Settle();
    std::filesystem::create_directories(out);
    std::ofstream layout(out/(demo+".layout.json"));layout<<"{";bool first=true;
    for(const auto& [name,b]:h.s.probes){if(!first)layout<<",";first=false;layout<<"\""<<name<<"\":["<<b.min.x<<","<<b.min.y<<","<<b.max.x<<","<<b.max.y<<"]";}layout<<"}";layout.close();
    imkit::design::MotionCapture recording;recording.Start(out/(demo+".mkv"),1920,1080);h.motion=&recording;
    auto hold=[&](int n){h.Settle(n);};
    auto move=[&](ImVec2 target,int n=28){auto start=h.mouse;for(int i=1;i<=n;++i){float u=float(i)/n;u=u*u*(3-2*u);h.mouse={start.x+(target.x-start.x)*u,start.y+(target.y-start.y)*u};h.Frame();}};
    auto clickAt=[&](ImVec2 p,const char* action){recording.Mark(action);move(p);h.Frame([](auto&io){io.AddMouseButtonEvent(0,true);});hold(7);h.Frame([](auto&io){io.AddMouseButtonEvent(0,false);});hold(20);};
    auto click=[&](const char* name){clickAt(h.s.probes.at(name).Center(),name);};
    auto drag=[&](ImVec2 from,ImVec2 to,const char* action){recording.Mark(action);move(from);h.Frame([](auto&io){io.AddMouseButtonEvent(0,true);});hold(8);move(to,66);h.Frame([](auto&io){io.AddMouseButtonEvent(0,false);});hold(22);};
    auto type=[&](const char* probe,const char* text){click(probe);h.Frame([](auto&io){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);});h.Frame([](auto&io){io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddKeyEvent(ImGuiKey_A,false);});for(auto p=text;*p;++p){char c[]={*p,0};h.Frame([&](auto&io){io.AddInputCharactersUTF8(c);});hold(5);}h.Key(ImGuiKey_Enter);hold(18);};
    hold(24);
    if(demo=="comparison") {
        auto b=h.s.probes.at("comparison-default-level");auto p=b.Center();p.x=b.min.x+(b.max.x-b.min.x)*.64f;
        drag(p,{b.min.x+(b.max.x-b.min.x)*.30f,p.y},"Default slider edits shared value");recording.Check("shared slider changed",h.s.comparison.level<.45,h.s.comparison.level);
        click("comparison-default-apply");type("comparison-imkit-name","Modern Studio");
        b=h.s.probes.at("comparison-imkit-level");p=b.Center();p.x=b.min.x+(b.max.x-b.min.x)*.30f;
        drag(p,{b.min.x+(b.max.x-b.min.x)*.88f,p.y},"ImKit slider edits same value");
        recording.Check("modern shared value changed",h.s.comparison.level>.65,h.s.comparison.level);
        click("comparison-imkit-apply");click("comparison-default-enabled");click("comparison-imkit-enabled");
        recording.Check("both Apply buttons",h.s.comparison.applyCount==2,h.s.comparison.applyCount);
    } else if(demo=="workspace") {
        click("workbench-tab-1");recording.Check("Inspect selected",h.s.workflow.workspace==102,h.s.workflow.workspace);
        click("workbench-row-1");recording.Check("hierarchy selection",h.s.workflow.selected==2,h.s.workflow.selected);
        click("workbench-enabled");click("workbench-enabled");
        auto b=h.s.probes.at("workbench-position");ImVec2 p{b.min.x+60,b.max.y-ImGui::GetFrameHeight()*.5f};
        drag(p,{p.x+100,p.y},"Drag position X / live preview");recording.Check("axis input edited",h.s.workflow.position[0]!=0,h.s.workflow.position[0]);
        clickAt({b.min.x+(b.max.x-b.min.x)*.35f,b.max.y-ImGui::GetFrameHeight()*.5f},"Select next axis");
        click("workbench-mobile");recording.Check("Mobile selected",h.s.workflow.choiceSource==703,h.s.workflow.choiceSource);
        click("workbench-apply");recording.Check("Apply request handled",h.s.workflow.actions>0,h.s.workflow.actions);
        click("workbench-tab-2");recording.Check("Deliver selected",h.s.workflow.workspace==103,h.s.workflow.workspace);
        click("workbench-tab-0");
        click("workbench-hierarchy");recording.Check("hierarchy collapsed",!h.s.workflow.hierarchyOpen);
        click("workbench-hierarchy");recording.Check("hierarchy expanded",h.s.workflow.hierarchyOpen);
    } else if(demo=="icons"||demo=="icon-artwork") {
        type("icon-search","camera");click("icon-pick-Camera");recording.Check("Camera selected",h.s.selectedIcon==static_cast<int>(IconId::Camera),h.s.selectedIcon);
        click("icon-label-button");type("icon-search","projector");click("icon-pick-Projector3D");
        recording.Check("Projector selected",h.s.selectedIcon==static_cast<int>(IconId::Projector3D),h.s.selectedIcon);click("icon-button");
    } else if(demo=="themes") {
        for(int theme:{4,8,2,0}) {
            click("appearance");click("appearance-theme-picker");const auto key="appearance-theme-"+std::to_string(theme);
            // Use the same public keyboard navigation as a consumer. Home/Down
            // scrolls the popup to off-screen choices without stale hit boxes.
            recording.Mark(key.c_str());h.Key(ImGuiKey_Home);
            for(int i=0;i<theme;++i){h.Key(ImGuiKey_DownArrow);hold(3);}
            h.Key(ImGuiKey_Enter);hold(20);
            recording.Check("theme selection",h.s.presetIndex==theme,h.s.presetIndex);h.Key(ImGuiKey_Escape);hold(24);
        }
    } else if(demo=="toasts") {
        click("toast-loading");recording.Check("loading toast created",!h.s.toasts.items.empty());hold(45);
        click("toast-complete");recording.Check("success completion",std::none_of(h.s.toasts.items.begin(),h.s.toasts.items.end(),[](auto&v){return v.phase==ToastPhase::Loading;}));
        click("toast-success");hold(36);
    } else if(demo=="preview-contract") {
        auto& s=h.s.editors;
        for(int state:{1,4,0,3}) {
            clickAt({(s.motionStateMin.x+s.motionStateMax.x)/2,(s.motionStateMin.y+s.motionStateMax.y)/2},"Preview state selector");
            h.Key(ImGuiKey_Home);for(int i=0;i<state;++i)h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_Enter);hold(24);
            recording.Check("preview state applied",s.monitorContractStatus==state,s.monitorContractStatus);
        }
        clickAt({(s.motionAspectMin.x+s.motionAspectMax.x)/2,(s.motionAspectMin.y+s.motionAspectMax.y)/2},"Preview aspect selector");
        h.Key(ImGuiKey_Home);h.Key(ImGuiKey_DownArrow);h.Key(ImGuiKey_Enter);recording.Check("preview aspect applied",s.monitorContractAspect==1,s.monitorContractAspect);
    } else if(demo=="timeline") {
        auto& s=h.s.editors;const auto origin=s.timeline.view.min;
        auto p=ImVec2{origin.x+s.timeline.headerWidth+15,origin.y-30};
        const auto previous=s.timeline.time.playhead;
        drag(p,{p.x+240,p.y},"Scrub playhead");recording.Check("playhead changed",s.timeline.time.playhead!=previous,double(s.timeline.time.playhead));
        const auto clipStart=s.clips.front().start;
        drag({p.x+80,origin.y+32},{p.x+110,origin.y+32},"Move clip within its available gap");recording.Check("clip moved",s.clips.front().start!=clipStart,double(s.clips.front().start));
        auto play=ImVec2{s.motionPlay.x+ImGui::GetFrameHeight()*.5f,s.motionPlay.y+ImGui::GetFrameHeight()*.5f};
        clickAt(play,"Play timeline");recording.Check("playback started",s.timeline.time.playing);hold(75);clickAt(play,"Pause timeline");recording.Check("playback paused",!s.timeline.time.playing);
        drag({p.x+240,p.y},{p.x+75,p.y},"Scrub back");
    } else if(demo=="overview") {
        click("start-workspaces");click("workbench-row-1");click("workbench-tab-1");click("workbench-enabled");click("workbench-enabled");
        recording.Check("Start route applied",h.s.page==PageWorkbench);
    } else if(demo=="workflow") {
        click("workflow-chip");recording.Check("filter enabled",h.s.workflow.chip);
        click("workflow-review");
        recording.Check("Review selected",h.s.workflow.selected==3,h.s.workflow.selected);
        recording.Mark("Scroll to notification action");move({1450,800});
        for(int i=0;i<30 && h.s.probes.at("workflow-notify").max.y>1065;++i){h.Frame([](auto& io){io.AddMouseWheelEvent(0,-1);});hold(4);}
        recording.Check("notification control visible",h.s.probes.at("workflow-notify").max.y<1078);
        click("workflow-notify");recording.Check("notification displayed",h.s.workflow.notice);hold(70);
    } else {click("apply");click("toggle");click("mixed");click("checkbox");click("toggle");recording.Check("component action",h.s.clicks>0,h.s.clicks);}
    move({1750,990},24);hold(24);
    // Enough genuine per-frame action for the edit, rather than duplicating stills.
    const int minimum=demo=="workspace"?960:demo=="comparison"?840:480;
    while(recording.Frames()<minimum){
        if(demo=="toasts"){click("toast-loading");hold(75);click("toast-complete");}
        else if(demo=="comparison") {click("comparison-imkit-apply");click("comparison-default-apply");}
        else if(demo=="workspace"||demo=="overview") {click("workbench-tab-1");click("workbench-tab-0");}
        else if(demo=="timeline") {const auto& s=h.s.editors;auto p=ImVec2{s.timeline.view.min.x+s.timeline.headerWidth+80,s.timeline.view.min.y-30};drag(p,{p.x+140,p.y},"Scrub edit continuation");}
        else if(demo=="preview-contract") {auto& s=h.s.editors;clickAt({(s.motionStateMin.x+s.motionStateMax.x)/2,(s.motionStateMin.y+s.motionStateMax.y)/2},"Preview state navigation");h.Key(ImGuiKey_Home);h.Key(ImGuiKey_Enter);hold(30);}
        else if(demo=="icons"||demo=="icon-artwork") {click("icon-label-button");click("icon-button");}
        else if(demo=="workflow") {click("workflow-chip");click("workflow-steps");}
        else {click("toggle");click("checkbox");}
    }
    h.motion=nullptr;recording.Finish();
}
void CaptureDemo(Host &h,const std::filesystem::path &out,const std::string &demo,bool legacyReadme) {
    const auto dir=out/(legacyReadme ? "readme-frames" : demo);
    std::filesystem::create_directories(dir);
    int frame=0;
    if(demo=="overview") {
        h.s.page=PageStart;h.s.comparison.open=false;h.Settle();
        CaptureFrames(h,dir,frame,16);
        h.s.page=PageCompare;h.s.comparison.open=true;h.Settle();
        CaptureFrames(h,dir,frame,24);
        h.s.page=PageWorkbench;h.s.comparison.open=false;h.Settle();
        CaptureFrames(h,dir,frame,20);
    } else if(demo=="comparison") {
        h.s.page=PageCompare;h.s.comparison.open=true;h.Settle();
        CaptureFrames(h,dir,frame,14);
        h.Click("comparison-default-apply");
        CaptureFrames(h,dir,frame,14);
        h.Replace("comparison-default-name","Studio Console");
        CaptureFrames(h,dir,frame,14);
        h.Click("comparison-imkit-enabled");
        CaptureFrames(h,dir,frame,14);
    } else if(demo=="workspace") {
        h.s.comparison.open=false;h.Page(PageWorkbench);
        CaptureFrames(h,dir,frame,16);
        h.Click("workbench-enabled");h.mouse={-100,-100};h.Settle();CaptureFrames(h,dir,frame,12);
        h.Click("workbench-row-1");h.mouse={-100,-100};h.Settle();CaptureFrames(h,dir,frame,12);
        h.Click("workbench-tabs");h.mouse={-100,-100};h.Settle();CaptureFrames(h,dir,frame,12);
        h.Click("workbench-enabled");h.mouse={-100,-100};h.Settle();CaptureFrames(h,dir,frame,12);
    } else if(demo=="toasts") {
        h.s.comparison.open=false;h.s.toasts.initialized=true;h.s.toasts.items.clear();h.s.toasts.state.Reset();h.Page(PageToasts);
        h.Click("toast-loading");CaptureFrames(h,dir,frame,24);
        h.Click("toast-complete");CaptureFrames(h,dir,frame,24);
        h.Click("toast-success");CaptureFrames(h,dir,frame,16);
    } else if(demo=="components") {
        h.s.page=PageBasic;h.s.comparison.open=false;h.Settle();
        CaptureFrames(h,dir,frame,16);
        h.Click("apply");CaptureFrames(h,dir,frame,12);
        h.Click("toggle");CaptureFrames(h,dir,frame,12);
        h.Click("mixed");CaptureFrames(h,dir,frame,12);
    } else if(demo=="icons") {
        const auto catalog=imkit::GetIconCatalog();
        auto iconIndex=[&](std::string_view name) {
            const auto item=std::find_if(catalog.begin(),catalog.end(),[&](const auto &entry){return entry.name==name;});
            return item==catalog.end()?static_cast<int>(imkit::IconId::Search):static_cast<int>(item->id);
        };
        auto categoryIndex=[&](std::string_view name) {
            std::vector<std::string_view> categories{"All"};
            for(const auto &item:catalog)
                if(std::find(categories.begin(),categories.end(),item.category)==categories.end())
                    categories.push_back(item.category);
            const auto category=std::find(categories.begin(),categories.end(),name);
            return category==categories.end()?0:static_cast<int>(category-categories.begin());
        };
        h.s.page=PageIcons;h.s.comparison.open=false;h.s.iconSearch[0]='\0';h.s.iconCategory=0;
        h.s.selectedIcon=iconIndex("Camera");h.Settle();
        CaptureFrames(h,dir,frame,20);
        h.s.iconCategory=categoryIndex("Editor");h.s.selectedIcon=iconIndex("Timecode");h.Settle();
        CaptureFrames(h,dir,frame,12);
        h.s.iconCategory=categoryIndex("Media");h.s.selectedIcon=iconIndex("Camera");h.Settle();
        CaptureFrames(h,dir,frame,12);
        h.s.iconCategory=0;h.s.selectedIcon=iconIndex("Projector3D");
        std::strcpy(h.s.iconSearch,"projector");h.Settle();
        CaptureFrames(h,dir,frame,12);
    } else if(demo=="icon-artwork") {
        h.s.theme=imkit::MakeTheme(imkit::ThemePreset::Graphite);
        h.s.theme.fonts=h.s.fonts;
        h.s.scale=1;
        LoadIconArtworkShowcase(h);
        h.iconArtworkShowcase=true;
        h.mouse={-100,-100};
        for (h.iconArtworkSlide=0;h.iconArtworkSlide<static_cast<int>(h.iconArtworkSlides.size());++h.iconArtworkSlide) {
            h.Settle();
            CaptureFrames(h,dir,frame,12);
        }
        h.iconArtworkShowcase=false;
    } else if(demo=="themes") {
        auto applyPreset=[&](int index) {
            h.s.presetIndex=index;
            h.s.theme=imkit::MakeTheme(imkit::ThemePresets()[index].preset);
            h.s.theme.fonts=h.s.fonts;
            h.s.dark=h.s.theme.scheme==imkit::ColorScheme::Dark;
            h.s.design.contrast=static_cast<int>(h.s.theme.contrast);
            h.s.design.density=static_cast<int>(h.s.theme.density);
            h.Settle();
        };
        h.s.page=PageIcons;h.s.comparison.open=false;h.s.palette=true;h.Settle();
        CaptureFrames(h,dir,frame,8);
        applyPreset(4);
        CaptureFrames(h,dir,frame,8);
        applyPreset(8);
        CaptureFrames(h,dir,frame,8);
        applyPreset(static_cast<int>(imkit::ThemePreset::Slate));
        CaptureFrames(h,dir,frame,8);
        h.s.palette=false;h.Settle();
        CaptureFrames(h,dir,frame,8);
        h.s.page=PageCompare;h.s.comparison.open=true;h.Settle();
        CaptureFrames(h,dir,frame,16);
    } else if(demo=="vector") {
        h.s.page=PageNumeric;h.s.comparison.open=false;h.s.floatingComparison=false;
        h.s.theme=imkit::MakeTheme(imkit::ThemePreset::Slate);h.s.theme.fonts=h.s.fonts;h.s.dark=true;h.Settle();
        CaptureFrames(h,dir,frame,20);
    } else if(demo=="workflow") {
        h.s.page=PageGenericWorkspace;h.s.comparison.open=false;h.Settle();
        CaptureFrames(h,dir,frame,10);
        h.Click("workflow-chip");
        CaptureFrames(h,dir,frame,10);
        h.Click("workflow-notify");
        CaptureFrames(h,dir,frame,10);
        h.Click("workflow-steps");
        CaptureFrames(h,dir,frame,10);
        h.s.page=PageFeedback;h.Settle();
        for(int i=0;i<24;++i) {
            h.s.workflow.fraction=static_cast<float>(i)/23.f;
            char name[32];std::snprintf(name,sizeof(name),"frame-%03d.png",frame++);
            h.Frame({},dir/name);
        }
    } else if(demo=="preview-contract") {
        auto &s=h.s.editors;s.Dataset(false);h.s.page=PageVideo;h.s.comparison.open=false;h.Settle();
        s.videoTopRatio=.95f;
        s.monitorMode=3;s.monitorContractSource=0;s.monitorContractAspect=1;
        for(int state=0;state<5;++state) {
            s.monitorContractStatus=state;h.Settle(2);
            CaptureFrames(h,dir,frame,10);
        }
    } else if(demo=="progress") {
        h.s.page=PageFeedback;h.s.comparison.open=false;h.Settle();
        h.s.workflow.taskFraction=0.f;
        h.mouse={1700,800};
        h.Frame([](ImGuiIO& io){io.AddMouseWheelEvent(0,-5);});h.Settle();
        CaptureFrames(h,dir,frame,10);
        for(int step=0;step<6;++step) {
            h.Click("progress-advance");
            CaptureFrames(h,dir,frame,10);
        }
        if(h.s.workflow.taskFraction<1.f) throw std::runtime_error("progress operation did not complete the host task");
        CaptureFrames(h,dir,frame,10);
    } else if(demo=="timeline") {
        auto &s=h.s.editors;s.Dataset(false);h.s.page=PageVideo;h.s.comparison.open=false;h.Settle();
        CaptureFrames(h,dir,frame,16);
        const auto origin=s.timeline.view.min;
        h.mouse={origin.x+s.timeline.headerWidth+5,origin.y+11};h.Settle();
        h.Frame([](ImGuiIO &io){io.AddMouseButtonEvent(0,true);});
        h.mouse.x+=60;h.Frame();
        CaptureFrames(h,dir,frame,16);
        h.Frame([](ImGuiIO &io){io.AddMouseButtonEvent(0,false);});h.Settle();
        h.mouse={-100,-100};h.Settle();
        CaptureFrames(h,dir,frame,16);
        s.Undo();h.Settle();
        CaptureFrames(h,dir,frame,16);
    } else {
        throw std::runtime_error("unknown capture demo");
    }
    std::ofstream metadata(dir/"capture.txt");
    metadata<<"scenario="<<demo<<"\nframes="<<frame<<"\nsize="<<ImGui::GetIO().DisplaySize.x<<"x"<<ImGui::GetIO().DisplaySize.y
            <<"\nsource=native OpenGL backbuffer; public Dear ImGui IO; not native OS/IME automation\n";
}
bool HasMotionScene(const std::string &name) {
    return FindScene(name) != nullptr;
}
} // namespace imkit::gallery::host
