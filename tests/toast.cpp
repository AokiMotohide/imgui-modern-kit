#include <imkit/toast.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace {
int failures=0;
void Check(bool ok,const char* label) { if(!ok) {std::fprintf(stderr,"FAIL: %s\n",label);++failures;} }
imkit::ToastViewportState::Entry& Entry(imkit::ToastViewportState& s,imkit::StableId id) {
    for(auto& e:s.entries) if(e.id==id) return e;
    std::abort();
}
}
int main() {
    using namespace imkit;
    ToastViewportState s;
    std::array<ToastEvent,16> storage{};ToastEventBuffer events{storage};
    ToastView v{1,"Saved","",FeedbackKind::Success};
    UpdateToastViewport({&v,1},s,0,4,events);
    UpdateToastViewport({&v,1},s,2,4,events);
    Check(Entry(s,1).remaining==3,"success default duration");
    StableId pause=1;UpdateToastViewport({&v,1},s,20,4,events,{&pause,1});
    Check(Entry(s,1).remaining==3,"pause freezes remaining time");
    UpdateToastViewport({&v,1},s,22,4,events);
    Check(Entry(s,1).remaining==1,"resume spends only unpaused time");
    UpdateToastViewport({&v,1},s,23,4,events);
    Check(events.count==1&&storage[0].kind==ToastEventKind::Expired,"expiry emitted once");
    UpdateToastViewport({&v,1},s,24,4,events);Check(events.count==1,"expired ID does not reappear");
    s.Reset();events.count=0;
    std::array<ToastView,2> pair{{{1,"Old","",FeedbackKind::Success},{2,"New","",FeedbackKind::Success}}};
    UpdateToastViewport(pair,s,0,1,events);UpdateToastViewport(pair,s,5,1,events);
    Check(Entry(s,1).remaining==5&&!Entry(s,1).visible&&Entry(s,2).retired,"newest visible; older waits without aging");
    UpdateToastViewport(pair,s,10,1,events);Check(Entry(s,1).remaining==5&&Entry(s,1).visible,"waiting entry starts when shown");
    UpdateToastViewport(pair,s,11,1,events);Check(Entry(s,1).remaining==4,"displayed queued entry ages");
    pair[0].phase=ToastPhase::Loading;
    UpdateToastViewport(pair,s,12,1,events);UpdateToastViewport(pair,s,100,1,events);
    Check(!Entry(s,1).retired,"loading remains indefinitely");
    pair[0].phase=ToastPhase::Message;pair[0].kind=FeedbackKind::Success;
    UpdateToastViewport(pair,s,200,1,events);
    Check(Entry(s,1).remaining==5,"loading completion starts fresh duration");
    s.Reset();ToastEventBuffer full{};
    v.duration=1;UpdateToastViewport({&v,1},s,0,1,full);UpdateToastViewport({&v,1},s,1,1,full);
    Check(full.overflow&&Entry(s,1).pendingExpiry,"overflow expiry retained");
    events.count=0;UpdateToastViewport({&v,1},s,2,1,events);
    Check(events.count==1&&!Entry(s,1).pendingExpiry,"overflow expiry retried");
    s.Reset();events.count=0;pair[0]={1,"Warning","",FeedbackKind::Warning};pair[1]=pair[0];pair[1].title="Replacement";pair[1].duration=0;
    UpdateToastViewport(pair,s,0,4,events);UpdateToastViewport(pair,s,999,4,events);
    Check(s.nextOrder==1&&!Entry(s,1).retired&&Entry(s,1).duration==0,"duplicate last wins and zero persists");
    UpdateToastViewport({},s,1000,4,events);Check(!s.entries[0].id,"removed IDs release storage");
    std::array<ToastView,65> many{};for(std::size_t i=0;i<many.size();++i) many[i].id=i+1;
    UpdateToastViewport(many,s,1001,4,events);Check(s.overflow,"bounded state overflow reported");
    s.Reset();v.duration=5;UpdateToastViewport({&v,1},s,10,1,events);UpdateToastViewport({&v,1},s,9,1,events);
    Check(Entry(s,1).remaining==5,"backwards time never subtracts negative duration");
    UpdateToastViewport({&v,1},s,std::numeric_limits<double>::quiet_NaN(),1,events);
    Check(std::isfinite(Entry(s,1).remaining),"nonfinite time ignored");
    UpdateToastViewport({&v,1},s,10,1,events);Check(Entry(s,1).remaining==5,"clock rollback does not spend repeated interval");

    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={640,480};io.DeltaTime=1.f/60;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->AddFontDefault();unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    auto theme=MakeTheme(ThemePreset::PrecisionLight);ApplyTheme(theme);
    std::array<accessibility::SemanticNode,64> nodes{};accessibility::AccessibilityFrame semantics{nodes};
    ToastViewportOptions options;options.width=300;options.margin=10;
    v={7,"Saved","A description",FeedbackKind::Success,0};v.action="Undo";
    auto frame=[&](double now) {
        events.count=0;semantics.Begin(ImGui::GetFrameCount()+1);
        ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host",nullptr,ImGuiWindowFlags_NoBringToFrontOnFocus);ToastViewport("test",{&v,1},s,now,events,options,{&theme,nullptr,&semantics});ImGui::End();ImGui::Render();
    };
    auto noticeNode=[&]() -> const accessibility::SemanticNode* {
        for(const auto& n:semantics.Tree().nodes) if(n.name=="Saved"&&n.parent==0) return &n;
        return nullptr;
    };
    for(int p=0;p<6;++p) {
        s.Reset();options.position=static_cast<ToastPosition>(p);frame(0);frame(.1);
        auto* node=noticeNode();Check(node!=nullptr,"notification semantic node");
        Check(semantics.Tree().Validate(),"notification semantic tree valid");
        if(node) {
            Check(node->minimum.x>=0&&node->minimum.y>=0&&node->maximum.x<=640&&node->maximum.y<=480,"six positions clamped inside work area");
            Check((p<3?node->minimum.y==10:node->maximum.y==470),"vertical anchor");
            const int col=p%3;Check(col==0?node->minimum.x==10:col==1?std::abs((node->minimum.x+node->maximum.x)*.5f-320)<1:node->maximum.x==630,"horizontal anchor");
        }
    }
    options.position=ToastPosition::TopRight;
    auto click=[&](accessibility::SemanticNode node,double now) {
        io.AddMousePosEvent((node.minimum.x+node.maximum.x)*.5f,(node.minimum.y+node.maximum.y)*.5f);
        frame(now);io.AddMouseButtonEvent(0,true);frame(now+.1);io.AddMouseButtonEvent(0,false);frame(now+.2);
    };
    s.Reset();frame(1);frame(1.1);
    accessibility::SemanticNode action{};
    for(auto& n:semantics.Tree().nodes) if(n.name=="Undo") action=n;
    Check(action.id!=0,"action semantic button published");click(action,1.2);
    Check(events.count==1&&storage[0].kind==ToastEventKind::Action&&storage[0].id==7,"public mouse action request");
    io.AddKeyEvent(ImGuiKey_Space,true);frame(1.6);
    bool keyboardRequested=events.count==1&&storage[0].kind==ToastEventKind::Action;
    io.AddKeyEvent(ImGuiKey_Space,false);frame(1.7);
    keyboardRequested|=events.count==1&&storage[0].kind==ToastEventKind::Action;
    Check(keyboardRequested,"public keyboard action request");
    v.duration=1;io.AddMousePosEvent(-100,-100);frame(1.8);frame(10);
    Check(!Entry(s,7).retired&&Entry(s,7).paused,"keyboard focus pauses expiry without hover");
    v.duration=0;
    v.actionDisabled=true;frame(2);frame(2.1);
    for(auto& n:semantics.Tree().nodes) if(n.name=="Undo") action=n;
    click(action,2.2);Check(events.count==0,"disabled action blocked");
    accessibility::SemanticNode close{};for(auto& n:semantics.Tree().nodes) if(n.name=="Dismiss") close=n;
    click(close,3);Check(events.count==1&&storage[0].kind==ToastEventKind::Dismiss,"public mouse dismiss request");
    s.Reset();v.actionDisabled=false;v.duration=1;io.AddMousePosEvent(0,0);frame(4);frame(4.1);
    auto* node=noticeNode();
    io.AddMousePosEvent(node->minimum.x+10,node->minimum.y+10);frame(5);frame(20);
    Check(!Entry(s,7).retired,"draw path hover pauses expiry");
    io.DisplaySize={160,100};options.width=1000;options.margin=50;s.Reset();frame(30);frame(30.1);
    node=noticeNode();Check(node&&node->maximum.x<=160&&node->maximum.y<=100,"narrow oversized card remains bounded");
    io.DisplaySize={640,480};options.width=300;options.margin=10;s.Reset();
    v={7,"Saved","A description",FeedbackKind::Success,0};v.phase=ToastPhase::Loading;v.duration=0;v.progress=.5f;v.stage="Inspecting";v.progressText="";v.status=ProgressStatus::Running;v.action="Details";
    frame(31);
    const accessibility::SemanticNode* progressNode=nullptr;
    for(const auto& semantic:semantics.Tree().nodes)if(semantic.role==accessibility::SemanticRole::Progress)progressNode=&semantic;
    Check(progressNode&&progressNode->name=="Saved"&&progressNode->value=="Running"&&progressNode->description=="Inspecting"&&std::abs(progressNode->numericValue-.5)<.001&&progressNode->state.busy,"loading toast exposes shared progress semantics");
    v.progress=-1;v.status=ProgressStatus::Paused;frame(32);
    progressNode=nullptr;for(const auto& semantic:semantics.Tree().nodes)if(semantic.role==accessibility::SemanticRole::Progress)progressNode=&semantic;
    Check(progressNode&&!progressNode->state.busy&&!progressNode->state.invalid,"paused unknown toast progress stays static");
    ImGui::DestroyContext();
    std::printf("Toast failures: %d\n",failures);return failures?1:0;
}
