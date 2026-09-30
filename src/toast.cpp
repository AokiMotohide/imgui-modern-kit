#include <imkit/toast.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit {
namespace {
const char* Safe(const char* s) { return s?s:""; }
const ToastView* Find(std::span<const ToastView> items, StableId id) {
    for(auto i=items.size();i>0;--i) if(items[i-1].id==id) return &items[i-1];
    return nullptr;
}
double Duration(const ToastView& view) {
    if(view.phase==ToastPhase::Loading) return 0;
    if(std::isfinite(view.duration)&&view.duration>=0) return view.duration;
    return view.kind==FeedbackKind::Info||view.kind==FeedbackKind::Success?5:0;
}
ImVec4 Mix(ImVec4 a,ImVec4 b,float t) {
    return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,1};
}
float Positive(float n,float fallback) { return std::isfinite(n)&&n>=0?n:fallback; }
const char* CloseLabel(ComponentOptions o) {
    return o.locale?o.locale->Text("dismiss","Dismiss"):"Dismiss";
}
void StatusIcon(FeedbackKind kind) {
    const float size=ImGui::GetFontSize();const auto p=ImGui::GetCursorScreenPos();
    ImGui::Dummy({size,size});auto* d=ImGui::GetWindowDrawList();
    const ImVec2 c{p.x+size*.5f,p.y+size*.5f};const float r=size*.4f;
    const auto color=ImGui::GetColorU32(ImGuiCol_Text);
    d->AddCircle(c,r,color,20,1.4f);
    if(kind==FeedbackKind::Success) {
        d->AddLine({c.x-r*.55f,c.y},{c.x-r*.1f,c.y+r*.4f},color,1.6f);
        d->AddLine({c.x-r*.1f,c.y+r*.4f},{c.x+r*.6f,c.y-r*.4f},color,1.6f);
    } else if(kind==FeedbackKind::Error) {
        d->AddLine({c.x-r*.4f,c.y-r*.4f},{c.x+r*.4f,c.y+r*.4f},color,1.6f);
        d->AddLine({c.x+r*.4f,c.y-r*.4f},{c.x-r*.4f,c.y+r*.4f},color,1.6f);
    } else {
        const float direction=kind==FeedbackKind::Warning?1.f:-1.f;
        d->AddLine({c.x,c.y-r*.5f*direction},{c.x,c.y+r*.1f*direction},color,1.6f);
        d->AddCircleFilled({c.x,c.y+r*.5f*direction},1.f,color);
    }
}
}
bool ToastEventBuffer::Push(ToastEvent event) {
    if(count>=storage.size()) { overflow=true;return false; }
    storage[count++]=event;return true;
}
void ToastViewportState::Reset() { *this=ToastViewportState{}; }
namespace {
void Synchronize(std::span<const ToastView> items,ToastViewportState& state) {
    state.overflow=false;
    for(auto& e:state.entries) if(e.id&&!Find(items,e.id)) e={};
    for(std::size_t i=0;i<items.size();++i) {
        const auto& v=items[i];if(!v.id||Find(items,v.id)!=&v) continue;
        auto it=std::find_if(state.entries.begin(),state.entries.end(),[&](auto& e){return e.id==v.id;});
        if(it==state.entries.end()) {
            it=std::find_if(state.entries.begin(),state.entries.end(),[](auto& e){return !e.id;});
            if(it==state.entries.end()) {state.overflow=true;continue;}
            *it={};it->id=v.id;it->order=++state.nextOrder;
            it->kind=v.kind;it->phase=v.phase;it->duration=v.duration;it->remaining=Duration(v);
        } else if(it->kind!=v.kind||it->phase!=v.phase||
                  (it->duration!=v.duration&&!(std::isnan(it->duration)&&std::isnan(v.duration)))) {
            it->kind=v.kind;it->phase=v.phase;it->duration=v.duration;it->remaining=Duration(v);
            it->retired=false;it->pendingExpiry=false;it->visible=false;it->shownSeconds=0;
        }
    }
}
}
void UpdateToastViewport(std::span<const ToastView> items,ToastViewportState& state,
                         double now,std::size_t maximum,ToastEventBuffer& events,
                         std::span<const StableId> paused) {
    double dt=0;
    if(std::isfinite(now)) {
        if(state.hasTime) dt=std::max(0.0,now-state.lastTime);
        state.lastTime=state.hasTime?std::max(state.lastTime,now):now;state.hasTime=true;
    }
    Synchronize(items,state);
    std::array<ToastViewportState::Entry*,ToastViewportState::Capacity> ordered{};
    std::size_t count=0;
    for(auto& e:state.entries) {
        if(e.pendingExpiry&&events.Push({e.id,ToastEventKind::Expired})) e.pendingExpiry=false;
        if(e.id&&!e.retired) ordered[count++]=&e;
    }
    std::sort(ordered.begin(),ordered.begin()+count,[](auto* a,auto* b){return a->order>b->order;});
    for(std::size_t i=0;i<count;++i) {
        auto& e=*ordered[i];const bool visible=i<maximum;
        e.paused=std::find(paused.begin(),paused.end(),e.id)!=paused.end();
        if(visible&&e.visible) {
            e.shownSeconds+=dt;
            if(!e.paused&&Duration(*Find(items,e.id))>0) {
                e.remaining=std::max(0.0,e.remaining-dt);
                if(e.remaining<=0) {e.retired=true;e.pendingExpiry=!events.Push({e.id,ToastEventKind::Expired});}
            }
        }
        e.visible=visible&&!e.retired;
    }
    for(auto& e:state.entries) if(e.retired) e.visible=false;
}
void ToastViewport(const char* id,std::span<const ToastView> items,ToastViewportState& state,
                   double now,ToastEventBuffer& events,ToastViewportOptions layout,ComponentOptions o) {
    auto* viewport=ImGui::GetWindowViewport();
    const auto& style=ImGui::GetStyle();
    const float margin=Positive(layout.margin,style.WindowPadding.x);
    const float gap=Positive(layout.gap,style.ItemSpacing.y);
    const float width=std::min(Positive(layout.width,0)>0?layout.width:ImGui::GetFontSize()*25,
                             std::max(1.f,viewport->WorkSize.x-std::min(margin*2,viewport->WorkSize.x-1)));
    const ImVec2 padding{style.WindowPadding.x,style.WindowPadding.y};
    const float textWidth=std::max(1.f,width-padding.x*2);
    auto height=[&](const ToastView& v) {
        float h=padding.y*2+ImGui::CalcTextSize(Safe(v.title),nullptr,false,
                     std::max(1.f,textWidth-ImGui::GetFontSize()-ImGui::GetFrameHeight()-style.ItemSpacing.x*2)).y;
        h=std::max(h,padding.y*2+ImGui::GetFrameHeight());
        if(*Safe(v.description)) h+=style.ItemSpacing.y+ImGui::CalcTextSize(v.description,nullptr,false,textWidth).y;
        if(v.phase==ToastPhase::Loading) h+=style.ItemSpacing.y+ImGui::GetFrameHeight();
        if(*Safe(v.action)) h+=style.ItemSpacing.y+ImGui::GetFrameHeight();
        return h;
    };
    // Synchronize without advancing time, then determine the actual visible
    // cards and current-frame hover/focus before spending their duration.
    Synchronize(items,state);
    std::array<ToastViewportState::Entry*,ToastViewportState::Capacity> ordered{};
    std::size_t count=0;
    for(auto& e:state.entries) if(e.id&&!e.retired) ordered[count++]=&e;
    std::sort(ordered.begin(),ordered.begin()+count,[](auto* a,auto* b){return a->order>b->order;});
    const int position=static_cast<int>(layout.position);
    const bool bottom=position>=3;const int column=position%3;
    const float x=column==0?viewport->WorkPos.x+std::min(margin,viewport->WorkSize.x*.5f):
        column==1?viewport->WorkPos.x+(viewport->WorkSize.x-width)*.5f:
        viewport->WorkPos.x+viewport->WorkSize.x-width-std::min(margin,viewport->WorkSize.x*.5f);
    const float available=std::max(1.f,viewport->WorkSize.y-std::min(margin*2,viewport->WorkSize.y-1));
    float used=0;std::size_t visible=0;
    std::array<float,ToastViewportState::Capacity> heights{};
    std::array<StableId,ToastViewportState::Capacity> paused{};std::size_t pauseCount=0;
    for(;visible<count&&visible<layout.maximum;++visible) {
        heights[visible]=std::min(height(*Find(items,ordered[visible]->id)),available);
        const float extra=heights[visible]+(visible?gap:0);
        if(visible&&used+extra>available) break;
        used+=extra;
    }
    float offset=0;
    ImGui::PushID(Safe(id));const ImGuiID scope=ImGui::GetID("toast-viewport");ImGui::PopID();
    for(std::size_t i=0;i<visible;++i) {
        auto& e=*ordered[i];const auto& v=*Find(items,e.id);
        const float y=bottom?viewport->WorkPos.y+viewport->WorkSize.y-std::min(margin,viewport->WorkSize.y*.5f)-offset-heights[i]:
                              viewport->WorkPos.y+std::min(margin,viewport->WorkSize.y*.5f)+offset;
        char name[80];std::snprintf(name,sizeof(name),"###imkit-toast-card-%08x-%llu",scope,static_cast<unsigned long long>(e.id));
        ImGui::SetNextWindowViewport(viewport->ID);ImGui::SetNextWindowPos({x,y});ImGui::SetNextWindowSize({width,heights[i]});
        ImVec4 accent=o.theme?o.theme->semantic.accent:style.Colors[ImGuiCol_CheckMark];
        if(o.theme&&v.phase!=ToastPhase::Loading) {
            if(v.kind==FeedbackKind::Success) accent=o.theme->semantic.success;
            if(v.kind==FeedbackKind::Warning) accent=o.theme->semantic.warning;
            if(v.kind==FeedbackKind::Error) accent=o.theme->semantic.error;
        }
        const ImVec4 bg=Mix(o.theme?o.theme->semantic.surfaceRaised:style.Colors[ImGuiCol_WindowBg],accent,.32f);
        ImVec4 foreground=o.theme?o.theme->semantic.text:style.Colors[ImGuiCol_Text];
        if(ContrastRatio(foreground,bg)<4.5f) foreground=ContrastRatio({0,0,0,1},bg)>ContrastRatio({1,1,1,1},bg)?ImVec4{0,0,0,1}:ImVec4{1,1,1,1};
        ImGui::PushStyleColor(ImGuiCol_WindowBg,bg);ImGui::PushStyleColor(ImGuiCol_Text,foreground);
        ImGui::PushStyleColor(ImGuiCol_Border,accent);ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,o.theme?o.theme->radius.overlay:style.WindowRounding);
        const bool motion=o.theme&&o.theme->motion.enabled&&!o.theme->motion.reducedMotion;
        const float seconds=o.theme?o.theme->motion.overlaySeconds:0;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha,style.Alpha*(motion&&seconds>0?std::clamp(static_cast<float>((e.shownSeconds+ImGui::GetIO().DeltaTime)/seconds),.15f,1.f):1.f));
        if(ImGui::Begin(name,nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
                        ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing)) {
            const auto nodeId=ImGui::GetID("toast-feedback");
            auto cardOptions=o;cardOptions.parent=nodeId;
            if(o.theme) OverlayDecoration(*o.theme,o.animation);
            if(ImGui::IsWindowHovered()||ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) paused[pauseCount++]=e.id;
            StatusIcon(v.kind);ImGui::SameLine();
            const float startX=ImGui::GetCursorPosX();const float top=ImGui::GetCursorPosY();
            ImGui::PushTextWrapPos(std::max(startX+1,width-padding.x-ImGui::GetFrameHeight()-style.ItemSpacing.x));
            ImGui::TextUnformatted(Safe(v.title));ImGui::PopTextWrapPos();
            const float afterTitle=ImGui::GetCursorPosY();
            bool dismiss=false;
            if(v.dismissible) {
                ImGui::SetCursorPos({std::max(startX,width-padding.x-ImGui::GetFrameHeight()),top});
                dismiss=ImGui::Button("x##dismiss",{ImGui::GetFrameHeight(),ImGui::GetFrameHeight()});
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",CloseLabel(o));
                if(o.accessibility) {
                    accessibility::SemanticNode n;n.id=ImGui::GetID("dismiss");n.parent=nodeId;n.role=accessibility::SemanticRole::Button;
                    n.name=CloseLabel(o);n.actions=accessibility::SemanticAction::Press;
                    n.state.disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
                    if(!n.state.disabled) dismiss|=o.accessibility->Take(n.id,accessibility::SemanticAction::Press);
                    accessibility::AnnotateLastItem(*o.accessibility,n);
                }
            }
            ImGui::SetCursorPos({padding.x,std::max(afterTitle,top+ImGui::GetFrameHeight()+style.ItemSpacing.y)});
            if(*Safe(v.description)) ImGui::TextWrapped("%s",v.description);
            if(v.phase==ToastPhase::Loading) {
                if(std::isfinite(v.progress)&&v.progress>=0) {
                    char percent[20];std::snprintf(percent,sizeof(percent),"%.0f%%",std::clamp(v.progress,0.f,1.f)*100);
                    ImGui::ProgressBar(std::clamp(v.progress,0.f,1.f),{-1,0},percent);
                    if(o.accessibility) {
                        accessibility::SemanticNode n;n.id=ImGui::GetID("progress");n.parent=nodeId;
                        n.name=Safe(v.title);n.role=accessibility::SemanticRole::Progress;
                        n.numericValue=std::clamp(v.progress,0.f,1.f);n.state.busy=true;
                        accessibility::AnnotateLastItem(*o.accessibility,n);
                    }
                } else {
                    Spinner("loading",cardOptions);
                }
            }
            if(*Safe(v.action)) {
                ImGui::BeginDisabled(v.actionDisabled);
                if(ActionButton(v.action,ActionVariant::Secondary,{},cardOptions)) events.Push({e.id,ToastEventKind::Action});
                ImGui::EndDisabled();
            }
            if(o.accessibility) {
                accessibility::SemanticNode n;n.id=nodeId;n.parent=o.parent;n.name=Safe(v.title);n.description=Safe(v.description);
                n.role=v.kind==FeedbackKind::Warning||v.kind==FeedbackKind::Error?accessibility::SemanticRole::Alert:accessibility::SemanticRole::Status;
                n.state.busy=v.phase==ToastPhase::Loading;n.minimum={x,y};n.maximum={x+width,y+heights[i]};
                n.state.disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
                n.actions=v.dismissible?accessibility::SemanticAction::Dismiss:accessibility::SemanticAction::None;
                if(v.dismissible&&!n.state.disabled) dismiss|=o.accessibility->Take(n.id,accessibility::SemanticAction::Dismiss);
                o.accessibility->Add(n);
            }
            if(dismiss&&events.Push({e.id,ToastEventKind::Dismiss})) e.retired=true;
        }
        ImGui::End();ImGui::PopStyleVar(2);ImGui::PopStyleColor(3);
        offset+=heights[i]+gap;
    }
    UpdateToastViewport(items,state,now,visible,events,{paused.data(),pauseCount});
}
} // namespace imkit
