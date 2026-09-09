#include <imkit/editor_core.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <cstring>

namespace imkit::editor {
namespace {
Tick Rounded(long double value) {
    return static_cast<Tick>(std::clamp(value, static_cast<long double>((std::numeric_limits<Tick>::min)()),
        static_cast<long double>((std::numeric_limits<Tick>::max)())));
}
bool DropRate(FrameRate r) { return r.denominator == 1001 && (r.numerator == 30000 || r.numerator == 60000); }
ImVec2 Screen(Point p, const CanvasState &s, ImVec2 origin) {
    auto v = ToScreen(p, s, {origin.x, origin.y}); return {static_cast<float>(v.x), static_cast<float>(v.y)};
}
void Diamond(ImDrawList *d, ImVec2 p, ImU32 color, float r = 5) {
    d->AddQuadFilled({p.x, p.y-r}, {p.x+r,p.y}, {p.x,p.y+r}, {p.x-r,p.y}, color);
}
bool Flag(PropertyFlags f, PropertyFlags bit) { return (static_cast<unsigned>(f) & static_cast<unsigned>(bit)) != 0; }
void Action(EventBuffer &out, StableId id, std::uint64_t revision, EditKind kind, Value from = {}, Value to = {}) {
    out.Push({id, revision, Phase::Commit, kind, from, to, CurrentModifiers()});
}
}
bool Valid(FrameRate r) { return r.numerator > 0 && r.denominator > 0; }
Tick FrameToTick(std::int64_t f, FrameRate r) {
    return Valid(r) ? Rounded(std::round(static_cast<long double>(f) * TicksPerSecond * r.denominator / r.numerator)) : 0;
}
std::int64_t TickToFrame(Tick t, FrameRate r) {
    return Valid(r) ? Rounded(std::round(static_cast<long double>(t) * r.numerator / (static_cast<long double>(TicksPerSecond) * r.denominator))) : 0;
}
double Seconds(Tick t) { return static_cast<double>(t) / TicksPerSecond; }
Tick FromSeconds(double s) { return std::isfinite(s) ? Rounded(std::round(static_cast<long double>(s) * TicksPerSecond)) : 0; }
bool FormatTimecode(Tick tick, FrameRate rate, bool drop, std::span<char> out) {
    if (!Valid(rate) || out.empty() || (drop && !DropRate(rate))) return false;
    auto frame = TickToFrame(tick, rate);
    const bool negative = frame < 0;
    // Tick range is narrower than frame range for supported timecode rates.
    if (frame == (std::numeric_limits<Tick>::min)()) return false;
    if (negative) frame = -frame;
    const int fps = static_cast<int>(std::lround(static_cast<double>(rate.numerator) / rate.denominator));
    if (fps < 1 || fps > 120) return false;
    if (drop) {
        const int d = fps / 15, ten = fps * 600 - d * 9, minute = fps * 60 - d;
        const auto rest = frame % ten;
        frame += d * 9 * (frame / ten) + (rest >= d ? d * ((rest - d) / minute) : 0);
    }
    const auto hours = frame / (fps * 3600);
    int n = std::snprintf(out.data(), out.size(), "%s%02lld:%02lld:%02lld%c%02lld", negative ? "-" : "",
        static_cast<long long>(hours), static_cast<long long>(frame / (fps*60) % 60),
        static_cast<long long>(frame / fps % 60), drop ? ';' : ':', static_cast<long long>(frame % fps));
    return n >= 0 && static_cast<std::size_t>(n) < out.size();
}
bool ParseTimecode(std::string_view text, FrameRate r, bool drop, Tick &result) {
    if (!Valid(r) || (drop && !DropRate(r))) return false;
    bool negative = !text.empty() && text.front() == '-';
    if (negative) text.remove_prefix(1);
    if (text.size() != 11 || text[2] != ':' || text[5] != ':' || text[8] != (drop ? ';' : ':')) return false;
    int v[4]{};
    for (int i=0; i<4; ++i) {
        auto a=text[i*3], b=text[i*3+1];
        if (a<'0'||a>'9'||b<'0'||b>'9') return false;
        v[i]=(a-'0')*10+b-'0';
    }
    int fps=static_cast<int>(std::lround(static_cast<double>(r.numerator)/r.denominator));
    if (fps<1 || fps>120 || v[1]>=60 || v[2]>=60 || v[3]>=fps) return false;
    int d=drop ? fps/15 : 0;
    if (drop && v[1]%10 && v[2]==0 && v[3]<d) return false;
    Tick minutes=v[0]*60+v[1];
    Tick f=((minutes*60)+v[2])*fps+v[3]-d*(minutes-minutes/10);
    result=FrameToTick(negative ? -f : f,r); return true;
}
Modifiers CurrentModifiers() { const auto &io=ImGui::GetIO(); return {io.KeyShift,io.KeyCtrl,io.KeyAlt}; }
bool EventBuffer::Push(const Event &e) {
    if (count >= storage.size()) { overflow=true; return false; }
    storage[count++]=e; return true;
}
void EventBuffer::Clear() { count=0; overflow=false; }
std::span<const Event> EventBuffer::Events() const { return storage.first(count); }
bool Transaction::Begin(StableId id, std::uint64_t rev, EditKind kind, Value value, Modifiers mod, EventBuffer &out) {
    if (active) return false;
    Event event{id,rev,Phase::Begin,kind,value,value,mod};
    if (!out.Push(event)) return false;
    draft=event; active=true; return true;
}
bool Transaction::Update(std::uint64_t rev, Value value, EventBuffer &out) {
    if (!active) return false;
    if (rev!=draft.revision) { Cancel(out); return false; }
    Event event=draft; event.phase=Phase::Update; event.proposed=value;
    if (!out.Push(event)) return false;
    draft=event; return true;
}
bool Transaction::Commit(std::uint64_t rev, EventBuffer &out) {
    if (!active) return false;
    if (rev!=draft.revision) { Cancel(out); return false; }
    Event event=draft; event.phase=Phase::Commit;
    if (!out.Push(event)) return false;
    active=false; return true;
}
void Transaction::Cancel(EventBuffer &out) {
    if (!active) return;
    Event event=draft; event.phase=Phase::Cancel; event.proposed=event.original;
    if (out.Push(event)) active=false;
}
bool Selection::Contains(StableId id) const { return std::find(storage.begin(),storage.begin()+count,id)!=storage.begin()+count; }
void Selection::Clear() { count=0; active=0; }
bool Selection::Set(StableId id,bool additive,bool toggle) {
    if (!additive && !toggle) Clear();
    auto end=storage.begin()+count, it=std::find(storage.begin(),end,id);
    if (it!=end) { if (toggle) { std::move(it+1,end,it); --count; } active=count ? storage[count-1] : 0; return true; }
    if (count==storage.size()) return false;
    storage[count++]=id; active=id; return true;
}
SnapResult ResolveSnap(Tick t,std::span<const SnapCandidate> candidates,double scale,double threshold,StableId exclude) {
    SnapResult result{t}; result.distancePixels=threshold;
    if (!std::isfinite(scale) || scale<=0 || threshold<0) return result;
    for (const auto &c:candidates) {
        if (exclude && c.id==exclude) continue;
        double distance=std::abs(static_cast<double>(static_cast<long double>(c.tick)-t)*scale);
        if (distance>threshold) continue;
        if (!result.snapped || c.priority>result.candidate.priority ||
            (c.priority==result.candidate.priority && distance<result.distancePixels))
            result={c.tick,true,c,distance};
    }
    return result;
}
Point ToScreen(Point v,const CanvasState &s,Point o) { return {o.x+(v.x-s.origin.x)*s.scale.x,o.y+(v.y-s.origin.y)*s.scale.y}; }
Point FromScreen(Point v,const CanvasState &s,Point o) { return {s.origin.x+(v.x-o.x)/s.scale.x,s.origin.y+(v.y-o.y)/s.scale.y}; }
Rect VisibleRange(const CanvasState &s,Point size) { return {s.origin,{s.origin.x+size.x/s.scale.x,s.origin.y+size.y/s.scale.y}}; }
void ZoomAt(CanvasState &s,Point anchor,Point factor) {
    Point at=FromScreen(anchor,s,{});
    s.scale={std::clamp(s.scale.x*factor.x,1e-9,1e9),std::clamp(s.scale.y*factor.y,1e-9,1e9)};
    s.origin={at.x-anchor.x/s.scale.x,at.y-anchor.y/s.scale.y};
}
void Fit(CanvasState &s,Rect r,Point size,double pad) {
    s.scale={(std::max)(1.,size.x-2*pad)/(std::max)(1e-9,r.max.x-r.min.x),
             (std::max)(1.,size.y-2*pad)/(std::max)(1e-9,r.max.y-r.min.y)};
    s.origin={r.min.x-pad/s.scale.x,r.min.y-pad/s.scale.y};
}
bool InPolygon(Point p,std::span<const Point> polygon) {
    if (polygon.size()<3) return false;
    bool inside=false;
    for (std::size_t i=0,j=polygon.size()-1;i<polygon.size();j=i++) {
        auto a=polygon[i],b=polygon[j];
        if (((a.y>p.y)!=(b.y>p.y)) && p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x) inside=!inside;
    }
    return inside;
}
CanvasView BeginCanvas(const char *id,CanvasState &s,ImVec2 size,const Theme &t) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg,t.colors.canvas);
    ImGui::BeginChild(id,size,ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    CanvasView v; v.min=ImGui::GetCursorScreenPos(); auto available=ImGui::GetContentRegionAvail();
    v.max={v.min.x+available.x,v.min.y+available.y};
    v.hovered=ImGui::IsWindowHovered();
    if (v.hovered) {
        const auto &io=ImGui::GetIO();
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) { s.origin.x-=io.MouseDelta.x/s.scale.x; s.origin.y-=io.MouseDelta.y/s.scale.y; }
        if (io.MouseWheel && !ImGui::IsAnyItemActive()) {
            const double factor=std::pow(1.15,io.MouseWheel);
            ZoomAt(s,{io.MousePos.x-v.min.x,io.MousePos.y-v.min.y},{factor,io.KeyShift ? 1 : factor});
        }
        s.origin.x-=io.MouseWheelH*32/s.scale.x;
    }
    v.visible=VisibleRange(s,{available.x,available.y});
    ImGui::GetWindowDrawList()->PushClipRect(v.min,v.max,true); return v;
}
void EndCanvas() { ImGui::GetWindowDrawList()->PopClipRect(); ImGui::EndChild(); ImGui::PopStyleColor(); }
void DrawGrid(const CanvasView &v,const CanvasState &s,Point spacing,const Theme &t) {
    auto *d=ImGui::GetWindowDrawList(); auto color=ImGui::GetColorU32(t.colors.border);
    for (int axis=0;axis<2;++axis) {
        double step=axis ? spacing.y:spacing.x, scale=axis ? s.scale.y:s.scale.x;
        if (step<=0 || !std::isfinite(step)) continue;
        while (step*scale<20) step*=10;
        double lo=axis ? v.visible.min.y:v.visible.min.x, hi=axis ? v.visible.max.y:v.visible.max.x;
        for (double p=std::ceil(lo/step)*step;p<=hi;p+=step) {
            auto q=Screen(axis ? Point{v.visible.min.x,p}:Point{p,v.visible.min.y},s,v.min);
            d->AddLine(q,axis ? ImVec2{v.max.x,q.y}:ImVec2{q.x,v.max.y},color);
        }
    }
}
std::size_t MakeBindings(ShortcutPreset preset,std::span<Binding> dst) {
    const Binding defaults[]={{Command::PlayPause,ImGuiKey_Space},{Command::Stop,ImGuiKey_K},
        {Command::PreviousFrame,ImGuiKey_LeftArrow},{Command::NextFrame,ImGuiKey_RightArrow},
        {Command::SetIn,ImGuiKey_I},{Command::SetOut,ImGuiKey_O},{Command::Loop,ImGuiMod_Ctrl|ImGuiKey_L},
        {Command::Split,ImGuiMod_Ctrl|ImGuiKey_B},{Command::Delete,ImGuiKey_Delete},
        {Command::Duplicate,ImGuiMod_Ctrl|ImGuiKey_D},{Command::SelectAll,ImGuiMod_Ctrl|ImGuiKey_A},
        {Command::Fit,ImGuiKey_Home},{Command::AddKey,ImGuiKey_I},
        {Command::PreviousKey,ImGuiKey_UpArrow},{Command::NextKey,ImGuiKey_DownArrow},
        {Command::Undo,ImGuiMod_Ctrl|ImGuiKey_Z},{Command::Redo,ImGuiMod_Ctrl|ImGuiMod_Shift|ImGuiKey_Z}};
    auto n=(std::min)(dst.size(),std::size(defaults)); std::copy_n(defaults,n,dst.begin());
    for (auto &b:dst.first(n)) {
        if (preset==ShortcutPreset::Premiere && b.command==Command::Split) b.chord=ImGuiMod_Ctrl|ImGuiKey_K;
        if (preset==ShortcutPreset::Blender && b.command==Command::Duplicate) b.chord=ImGuiMod_Shift|ImGuiKey_D;
    }
    return n;
}
bool CommandPressed(Command command,std::span<const Binding> bindings,bool focused) {
    if (!focused || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive()) return false;
    for (auto b:bindings) if (b.command==command && ImGui::IsKeyChordPressed(b.chord)) return true;
    return false;
}
void Transport(TimeState &s,std::span<const Binding> bindings) {
    bool focused=ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (ImGui::Button(s.playing ? "Pause":"Play") || CommandPressed(Command::PlayPause,bindings,focused)) s.playing=!s.playing;
    ImGui::SameLine();
    if (ImGui::Button("Stop") || CommandPressed(Command::Stop,bindings,focused)) { s.playing=false; s.playhead=s.inOut.first; }
    ImGui::SameLine(); if (ImGui::Button("<") || CommandPressed(Command::PreviousFrame,bindings,focused)) s.playhead=FrameToTick(TickToFrame(s.playhead,s.rate)-1,s.rate);
    ImGui::SameLine(); if (ImGui::Button(">") || CommandPressed(Command::NextFrame,bindings,focused)) s.playhead=FrameToTick(TickToFrame(s.playhead,s.rate)+1,s.rate);
    ImGui::SameLine(); if (ImGui::Button("In") || CommandPressed(Command::SetIn,bindings,focused)) s.inOut.first=(std::min)(s.playhead,s.inOut.last);
    ImGui::SameLine(); if (ImGui::Button("Out") || CommandPressed(Command::SetOut,bindings,focused)) s.inOut.last=(std::max)(s.playhead,s.inOut.first);
    ImGui::SameLine(); ImGui::Checkbox("Loop",&s.loop);
    char text[32]; FormatTimecode(s.playhead,s.rate,s.dropFrame,text); ImGui::SameLine(); ImGui::TextUnformatted(text);
}
void TimeRuler(const char *id,TimeState &s,CanvasState &canvas,std::span<const Marker> markers,std::uint64_t rev,EventBuffer &out,const Theme &t,float height) {
    auto p=ImGui::GetCursorScreenPos(); float width=ImGui::GetContentRegionAvail().x;
    ImGui::InvisibleButton(id,{width,height}); auto *d=ImGui::GetWindowDrawList();
    d->PushClipRect(p,{p.x+width,p.y+height},true);
    double step=1; while (step*canvas.scale.x<65) step*=2;
    for (double second=std::ceil(canvas.origin.x/step)*step;second<canvas.origin.x+width/canvas.scale.x;second+=step) {
        float x=p.x+static_cast<float>((second-canvas.origin.x)*canvas.scale.x);
        d->AddLine({x,p.y+height-8},{x,p.y+height},ImGui::GetColorU32(t.colors.muted));
        char label[32]; FormatTimecode(FromSeconds(second),s.rate,s.dropFrame,label);
        d->AddText({x+3,p.y},ImGui::GetColorU32(t.colors.muted),label);
    }
    for (auto m:markers) { auto x=p.x+static_cast<float>((Seconds(m.tick)-canvas.origin.x)*canvas.scale.x); Diamond(d,{x,p.y+height-5},ImGui::GetColorU32(t.colors.warning)); }
    float x=p.x+static_cast<float>((Seconds(s.playhead)-canvas.origin.x)*canvas.scale.x);
    d->AddLine({x,p.y},{x,p.y+height},ImGui::GetColorU32(t.colors.accent),2);
    d->PopClipRect();
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(0)) s.playhead=FromSeconds(canvas.origin.x+(ImGui::GetIO().MousePos.x-p.x)/canvas.scale.x);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) Action(out,0,rev,EditKind::Marker,{},Value{s.playhead});
}
double Evaluate(std::span<const Keyframe> keys,Tick tick,Extrapolation extrapolation) {
    if (keys.empty()) return 0;
    if (keys.size()==1) return keys[0].value;
    auto first=keys.front().tick,last=keys.back().tick;
    if (extrapolation==Extrapolation::Repeat && last>first && (tick<first||tick>last)) {
        Tick length=last-first; tick=first+((tick-first)%length+length)%length;
    }
    auto linear=[](const Keyframe &a,const Keyframe &b,Tick t) { return a.value+(b.value-a.value)*static_cast<double>(t-a.tick)/static_cast<double>(b.tick-a.tick); };
    if (tick<=first) return extrapolation==Extrapolation::Linear && keys[1].tick!=first ? linear(keys[0],keys[1],tick):keys.front().value;
    if (tick>=last) return extrapolation==Extrapolation::Linear && keys[keys.size()-2].tick!=last ? linear(keys[keys.size()-2],keys.back(),tick):keys.back().value;
    auto b=std::upper_bound(keys.begin(),keys.end(),tick,[](Tick t,const Keyframe &k){return t<k.tick;}); auto a=b-1;
    if (a->interpolation==Interpolation::Constant) return a->value;
    if (a->interpolation==Interpolation::Linear) return linear(*a,*b,tick);
    auto bez=[](double a,double b,double c,double d,double u) { double v=1-u; return v*v*v*a+3*v*v*u*b+3*v*u*u*c+u*u*u*d; };
    double length=Seconds(b->tick-a->tick), target=Seconds(tick-a->tick);
    double x1=std::clamp(a->right.x,0.,length), x2=std::clamp(length+b->left.x,x1,length), lo=0,hi=1;
    for (int i=0;i<40;++i) { double u=(lo+hi)*.5; if (bez(0,x1,x2,length,u)<target) lo=u; else hi=u; }
    return bez(a->value,a->value+a->right.y,b->value+b->left.y,b->value,(lo+hi)*.5);
}
void CurveEditor(const char *id,const CurveProvider &provider,CurveState &s,Selection &selection,EventBuffer &out,const Theme &t,ImVec2 size) {
    auto view=BeginCanvas(id,s.canvas,size,t); DrawGrid(view,s.canvas,{1,1},t);
    if (s.drag.active && (provider.revision!=s.drag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape))) s.drag.Cancel(out);
    auto keys=provider.query ? provider.query(provider.user,{{FromSeconds(view.visible.min.x),FromSeconds(view.visible.max.x)},-view.visible.max.y,-view.visible.min.y}):std::span<const Keyframe>{};
    auto *d=ImGui::GetWindowDrawList(); StableId hit=0; int side=0; Value initial{};
    for (std::size_t i=0;i<keys.size();++i) {
        const auto &k=keys[i]; Point v{Seconds(k.tick),-k.value}; auto p=Screen(v,s.canvas,view.min);
        if (i && keys[i-1].channel==k.channel) {
            auto &prev=keys[i-1]; ImVec2 old=Screen({Seconds(prev.tick),-prev.value},s.canvas,view.min);
            for (int j=1;j<=32;++j) { Tick tick=prev.tick+(k.tick-prev.tick)*j/32;
                auto next=Screen({Seconds(tick),-Evaluate(keys.subspan(i-1,2),tick)},s.canvas,view.min);
                d->AddLine(old,next,ImGui::GetColorU32(t.colors.accent),1.5f); old=next; }
        }
        auto mouse=ImGui::GetIO().MousePos;
        if (selection.Contains(k.id)) {
            for (int h=-1;h<=1;h+=2) {
                auto offset=h<0 ? k.left:k.right; auto hp=Screen({v.x+offset.x,v.y-offset.y},s.canvas,view.min);
                d->AddLine(p,hp,ImGui::GetColorU32(t.colors.muted)); d->AddCircleFilled(hp,4,ImGui::GetColorU32(t.colors.text));
                if (!k.locked && std::hypot(mouse.x-hp.x,mouse.y-hp.y)<7) { hit=k.id; side=h; initial={k.tick,0,0,0,offset.x,offset.y}; }
            }
        }
        Diamond(d,p,ImGui::GetColorU32(selection.Contains(k.id) ? t.colors.warning:t.colors.text));
        if (!k.locked && std::hypot(mouse.x-p.x,mouse.y-p.y)<7) { hit=k.id; side=0; initial={k.tick,0,0,0,k.value}; }
    }
    if (view.hovered && hit && ImGui::IsMouseClicked(0)) {
        selection.Set(hit,ImGui::GetIO().KeyCtrl); s.side=side; s.mouseStart={ImGui::GetIO().MousePos.x,ImGui::GetIO().MousePos.y};
        s.drag.Begin(hit,provider.revision,side ? EditKind::Handle:EditKind::Keyframe,initial,CurrentModifiers(),out);
    }
    if (s.drag.active) {
        auto proposed=s.drag.draft.original; auto mouse=ImGui::GetIO().MousePos;
        double dx=(mouse.x-s.mouseStart.x)/s.canvas.scale.x,dy=-(mouse.y-s.mouseStart.y)/s.canvas.scale.y;
        if (s.side) { proposed.x+=dx; proposed.y+=dy; } else { proposed.first+=FromSeconds(dx); proposed.x+=dy; }
        if (ImGui::IsMouseDown(0) && !(proposed==s.drag.draft.proposed)) s.drag.Update(provider.revision,proposed,out);
        if (ImGui::IsMouseReleased(0)) s.drag.Commit(provider.revision,out);
    }
    EndCanvas();
}
void PropertyGrid(const char *id,const PropertyProvider &p,PropertyState &s,EventBuffer &out) {
    ImGui::PushID(id); ImGui::InputText("Search",s.search,sizeof(s.search));
    if (s.drag.active && (p.revision!=s.drag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape))) s.drag.Cancel(out);
    if (ImGui::BeginTable("properties",3,ImGuiTableFlags_Resizable|ImGuiTableFlags_ScrollY|ImGuiTableFlags_RowBg)) {
        ImGuiListClipper clipper; clipper.Begin(p.count,ImGui::GetFrameHeightWithSpacing());
        while (clipper.Step()) {
            auto rows=p.query ? p.query(p.user,clipper.DisplayStart,clipper.DisplayEnd-clipper.DisplayStart,s.search):std::span<const PropertyView>{};
            for (const auto &row:rows) {
                ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(row.id)));
                ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(row.label);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",row.category);
                ImGui::TableNextColumn(); ImGui::BeginDisabled(Flag(row.flags,PropertyFlags::Locked));
                double draft=s.drag.active && s.drag.draft.target==row.id ? s.draft:row.value;
                bool changed=ImGui::DragScalar("##value",ImGuiDataType_Double,&draft,.01f,nullptr,nullptr,Flag(row.flags,PropertyFlags::Mixed) ? "Mixed":"%.3f");
                if (ImGui::IsItemActivated()) { s.draft=row.value; s.drag.Begin(row.id,p.revision,EditKind::Property,Value{0,0,0,0,row.value},CurrentModifiers(),out); }
                if (changed && s.drag.active) { s.draft=draft; s.drag.Update(p.revision,Value{0,0,0,0,draft},out); }
                if (ImGui::IsItemDeactivated() && s.drag.active && s.drag.draft.target==row.id) s.drag.Commit(p.revision,out);
                ImGui::TableNextColumn();
                if (ImGui::SmallButton(Flag(row.flags,PropertyFlags::Keyed) ? "<>":"+ key")) Action(out,row.id,p.revision,EditKind::Keyframe);
                ImGui::SameLine(); if (ImGui::SmallButton("Reset")) Action(out,row.id,p.revision,EditKind::Reset,Value{0,0,0,0,row.value},Value{0,0,0,0,row.defaultValue});
                ImGui::EndDisabled(); ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}
void AssetBrowser(const char *id,const AssetProvider &p,AssetState &s,Selection &selection,EventBuffer &out,std::span<const char *const> path) {
    ImGui::PushID(id);
    for (auto label:path) { ImGui::TextUnformatted(label); ImGui::SameLine(); ImGui::TextUnformatted("/"); ImGui::SameLine(); }
    if (!path.empty()) ImGui::NewLine();
    ImGui::InputText("Search",s.search,sizeof(s.search)); ImGui::SameLine(); ImGui::Checkbox("Grid",&s.grid);
    int columns=s.grid ? (std::max)(1,static_cast<int>(ImGui::GetContentRegionAvail().x/120)):1;
    if (ImGui::BeginChild("items")) {
        ImGuiListClipper clipper; clipper.Begin((p.count+columns-1)/columns,s.grid ? 112.f:ImGui::GetFrameHeightWithSpacing());
        while (clipper.Step()) {
            auto assets=p.query ? p.query(p.user,clipper.DisplayStart*columns,(clipper.DisplayEnd-clipper.DisplayStart)*columns,s.search):std::span<const AssetView>{};
            int column=0;
            for (const auto &a:assets) {
                if (column++%columns) ImGui::SameLine();
                ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(a.id))); ImGui::BeginGroup();
                if (s.grid && a.thumbnail.GetTexID()) ImGui::Image(a.thumbnail,{104,70});
                else if (s.grid) { auto pos=ImGui::GetCursorScreenPos(); ImGui::Dummy({104,70}); ImGui::GetWindowDrawList()->AddRect(pos,{pos.x+104,pos.y+70},ImGui::GetColorU32(ImGuiCol_Border)); }
                if (ImGui::Selectable(a.label,selection.Contains(a.id),0,{s.grid ? 104.f:0,0})) { selection.Set(a.id,ImGui::GetIO().KeyCtrl,ImGui::GetIO().KeyCtrl); Action(out,a.id,p.revision,EditKind::Select); }
                if (ImGui::BeginDragDropSource()) { ImGui::SetDragDropPayload("IMKIT_ASSET",&a.id,sizeof(a.id)); ImGui::TextUnformatted(a.label); ImGui::EndDragDropSource(); }
                if (a.status!=AssetStatus::Ready) { const char *labels[]={"Ready","Loading","Proxy","Missing","Error"}; ImGui::TextDisabled("%s",labels[static_cast<int>(a.status)]); }
                ImGui::EndGroup(); ImGui::PopID();
            }
        }
    }
    ImGui::EndChild(); ImGui::PopID();
}
bool Splitter(const char *id,float &pane,float total,bool vertical,float minimum) {
    ImGui::InvisibleButton(id,vertical ? ImVec2{6,ImGui::GetContentRegionAvail().y}:ImVec2{ImGui::GetContentRegionAvail().x,6});
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeEW:ImGuiMouseCursor_ResizeNS);
    if (!ImGui::IsItemActive()) return false;
    pane=std::clamp(pane+(vertical ? ImGui::GetIO().MouseDelta.x:ImGui::GetIO().MouseDelta.y),(std::min)(minimum,total*.5f),(std::max)(minimum,total-minimum)); return true;
}
void StatusBar(std::string_view text,const Selection &selection) { ImGui::Separator(); ImGui::TextUnformatted(text.data(),text.data()+text.size()); ImGui::SameLine(); ImGui::Text("%zu selected",selection.count); }
} // namespace imkit::editor
