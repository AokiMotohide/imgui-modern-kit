#include <imkit/editor_core.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <string_view>
using namespace imkit::editor;
int main() {
    int failures = 0;
    auto check = [&](bool ok, const char *text) {
        if (!ok) {
            ++failures;
            std::fprintf(stderr, "FAIL %s\n", text);
        }
    };
    char text[32];
    Tick parsed = 0;
    check(FormatTimecode(FrameToTick(1800, {30000, 1001}), {30000, 1001}, true, text) &&
              std::string_view(text) == "00:01:00;02",
          "29.97 minute boundary");
    check(FormatTimecode(FrameToTick(17982, {30000, 1001}), {30000, 1001}, true, text) &&
              std::string_view(text) == "00:10:00;00",
          "29.97 ten minutes");
    check(FormatTimecode(FrameToTick(-3600, {60000, 1001}), {60000, 1001}, true, text) &&
              std::string_view(text) == "-00:01:00;04",
          "negative 59.94");
    check(ParseTimecode(text, {60000, 1001}, true, parsed) && TickToFrame(parsed, {60000, 1001}) == -3600,
          "negative roundtrip");
    check(!ParseTimecode("00:01:00;00", {30000, 1001}, true, parsed), "dropped labels rejected");
    for (FrameRate rate : {FrameRate{24, 1}, {25, 1}, {30, 1}, {24000, 1001}, {30000, 1001}, {60000, 1001}})
        for (Tick frame : {Tick{-100000}, Tick{-1}, Tick{0}, Tick{123456}})
            check(TickToFrame(FrameToTick(frame, rate), rate) == frame, "frame tick roundtrip");
    check(!FormatTimecode(0, {24, 1}, true, text), "unsupported drop rate");
    SnapCandidate candidates[] = {{100, SnapKind::Marker, 1, 1}, {102, SnapKind::Playhead, 2, 2}};
    check(ResolveSnap(101, candidates, 2, 8).tick == 102, "snap priority");
    check(!ResolveSnap(1000, candidates, 2, 8).snapped, "pixel threshold");
    check(ResolveSnap(101, candidates, 2, 8, 2).tick == 100, "exclude drag target");
    std::array<Event, 8> storage{};
    EventBuffer events{storage};
    Transaction tx;
    Value original{9007199254740993LL};
    check(tx.Begin(42, 7, EditKind::Move, original, {}, events), "begin");
    auto proposed = original;
    proposed.first += 10;
    check(tx.Update(7, proposed, events) && tx.Commit(7, events), "update commit");
    check(events.Events().back().original.first == 9007199254740993LL, "integer event precision");
    check(events.Events().back().phase == Phase::Commit, "commit phase");
    events.Clear();
    tx.Begin(42, 7, EditKind::Move, original, {}, events);
    check(!tx.Update(8, proposed, events) && !tx.active && events.Events().back().phase == Phase::Cancel,
          "revision cancellation");
    EventBuffer empty{{}};
    check(!tx.Begin(42, 1, EditKind::Move, {}, {}, empty) && !tx.active && empty.overflow,
          "capacity failure atomicity");
    events.Clear();
    tx.Begin(42, 7, EditKind::Move, original, {}, events);
    tx.Cancel(empty);
    check(tx.active && tx.draft.phase == Phase::Cancel, "cancel retained on overflow");
    check(!tx.Commit(7, events) && !tx.active && events.Events().back().phase == Phase::Cancel,
          "pending cancel cannot become commit");
    events.Clear();
    tx.Begin(42, 7, EditKind::Move, original, {}, events);
    tx.Update(7, proposed, events);
    check(!tx.Commit(7, empty) && tx.active && tx.draft.phase == Phase::Commit,
          "commit retained on overflow");
    check(!tx.Update(7, {}, events) && !tx.active && events.Events().back().phase == Phase::Commit &&
              events.Events().back().proposed == proposed,
          "pending commit retries without accepting new proposed values");
    CanvasState canvas{{2, 3}, {10, 20}};
    auto before = FromScreen({100, 80}, canvas, {});
    ZoomAt(canvas, {100, 80}, {2, 2});
    auto after = FromScreen({100, 80}, canvas, {});
    check(std::abs(before.x - after.x) < 1e-12 && std::abs(before.y - after.y) < 1e-12,
          "cursor anchored zoom");
    check(VisibleRange(canvas, {200, 100}).max.x == canvas.origin.x + 10, "visible range");
    Point polygon[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    check(InPolygon({.5, .5}, polygon) && !InPolygon({2, 2}, polygon), "lasso winding");
    Keyframe keys[2] = {{1, 1, 0, 0}, {2, 1, TicksPerSecond, 1}};
    keys[0].interpolation = Interpolation::Linear;
    check(std::abs(Evaluate(keys, TicksPerSecond / 2) - .5) < 1e-9, "linear curve");
    keys[0].interpolation = Interpolation::Constant;
    check(Evaluate(keys, TicksPerSecond / 2) == 0, "constant curve");
    keys[0].interpolation = Interpolation::Bezier;
    keys[0].right = {1. / 3, 1. / 3};
    keys[1].left = {-1. / 3, -1. / 3};
    check(std::abs(Evaluate(keys, TicksPerSecond / 2) - .5) < 1e-9, "Bezier time inversion");
    check(std::abs(Evaluate(keys, TicksPerSecond * 3 / 2, Extrapolation::Repeat) - .5) < 1e-9,
          "repeat extrapolation");
    Keyframe extrema[] = {{1, 1, 0, 0}, {2, 1, TicksPerSecond, 1}, {3, 1, TicksPerSecond * 2, 0}};
    extrema[1].handles = HandleMode::AutoClamped;
    auto automatic = ResolveHandles(extrema, 1);
    check(automatic.left.y == 0 && automatic.right.y == 0, "auto clamped extrema tangent");
    extrema[1].handles = HandleMode::Vector;
    automatic = ResolveHandles(extrema, 1);
    check(automatic.left.y < 0 && automatic.right.y < 0, "vector handles follow neighbors");
    extrema[1].handles = HandleMode::Aligned;
    automatic = MoveHandle(extrema[1], false, {.5, .5});
    check(std::abs(automatic.left.x - automatic.left.y) < 1e-9 && automatic.left.x < 0,
          "aligned opposite tangent");
    extrema[0].handles = extrema[1].handles = extrema[2].handles = HandleMode::AutoClamped;
    check(std::abs(Evaluate(extrema, TicksPerSecond / 2) - .625) < 1e-9,
          "Bezier uses the next segment neighbor for a clamped extremum");
    automatic = MoveHandle(ResolveHandles(extrema, 1), false, {.25, .5});
    check(automatic.handles == HandleMode::Free && automatic.left.y == 0 && automatic.right.y == .5,
          "manual auto handle edit preserves resolved opposite handle and becomes free");
    auto *context=ImGui::CreateContext();
    auto &io=ImGui::GetIO();
    io.IniFilename=nullptr;
    io.DisplaySize={800,600};
    io.DeltaTime=1.f/60;
    unsigned char *fontPixels=nullptr; int fontWidth=0,fontHeight=0;
    io.Fonts->GetTexDataAsRGBA32(&fontPixels,&fontWidth,&fontHeight);
    TimeState transport;
    std::array<Binding,32> defaults{};
    auto bindingCount=MakeBindings(ShortcutPreset::Premiere,defaults);
    auto frame=[&](std::span<const Binding> bindings) {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize({780,300});
        ImGui::Begin("Transport bindings");
        Transport(transport,bindings);
        ImGui::End(); ImGui::Render();
    };
    auto bindings=std::span(defaults).first(bindingCount);
    frame(bindings); frame(bindings);
    auto press=[&](ImGuiKey key,std::span<const Binding> map) {
        io.AddKeyEvent(key,true); frame(map);
        io.AddKeyEvent(key,false); frame(map);
    };
    press(ImGuiKey_J,bindings);
    check(transport.playing && transport.playbackRate==-1,"J reverse through preset binding");
    press(ImGuiKey_J,bindings);
    check(transport.playbackRate==-2,"repeated reverse accelerates");
    transport.playhead=FrameToTick(18,transport.rate);
    press(ImGuiKey_K,bindings);
    check(!transport.playing && transport.playhead==FrameToTick(18,transport.rate),"K pauses without rewinding");
    press(ImGuiKey_L,bindings);
    check(transport.playing && transport.playbackRate==1,"L changes playback direction");
    transport.playing=false;
    press(ImGuiKey_J,{}); press(ImGuiKey_L,{});
    check(!transport.playing,"empty binding map disables J and L");
    std::array custom{Binding{Command::PlayReverse,ImGuiKey_F6}};
    press(ImGuiKey_F6,custom);
    check(transport.playing && transport.playbackRate==-1,"host remaps reverse command");
    std::array loopBinding{Binding{Command::Loop,ImGuiKey_F7}};
    const bool beforeLoop=transport.loop;
    press(ImGuiKey_F7,loopBinding);
    check(transport.loop!=beforeLoop,"loop command is applied");
    PropertyView property{92371,"Opacity","Video",.75,1};
    PropertyState propertyState;
    propertyState.time=FrameToTick(12,{24,1});
    std::array<Event,16> propertyStorage{};
    EventBuffer propertyEvents{propertyStorage};
    int adds=0,removes=0,toggles=0;
    PropertyProvider propertyProvider{&property,1,1,[](void *user,int first,int count,std::string_view) {
        return first==0 && count>0 ? std::span<const PropertyView>(static_cast<PropertyView*>(user),1)
                                  : std::span<const PropertyView>{};
    }};
    auto propertyFrame=[&] {
        propertyEvents.Clear();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize({780,300});
        ImGui::Begin("Property actions");
        PropertyGrid("inspector",propertyProvider,propertyState,propertyEvents);
        ImGui::End(); ImGui::Render();
        for (const auto &event:propertyEvents.Events()) {
            check(event.target==property.id,"property action preserves explicit ID");
            if (event.kind==EditKind::PropertyKey) {
                check(event.proposed.first==propertyState.time && event.proposed.x==.75,
                      "property key carries exact playhead and value");
                auto action=static_cast<PropertyKeyAction>(event.proposed.offset);
                if (action==PropertyKeyAction::Add) {++adds;property.flags=PropertyFlags::Keyed;}
                if (action==PropertyKeyAction::Remove) {++removes;property.flags=PropertyFlags::None;}
            }
            if (event.kind==EditKind::Toggle) {
                ++toggles;
                unsigned flags=static_cast<unsigned>(property.flags), bit=static_cast<unsigned>(event.proposed.x);
                property.flags=static_cast<PropertyFlags>(event.proposed.y ? flags|bit : flags&~bit);
            }
        }
    };
    propertyFrame(); propertyFrame();
    auto clickProperty=[&](float x,float y,int button=0) {
        io.AddMousePosEvent(x,y); propertyFrame();
        io.AddMouseButtonEvent(button,true); propertyFrame();
        io.AddMouseButtonEvent(button,false); propertyFrame();
    };
    clickProperty(760,57);
    check(adds==1,"property add key button public IO");
    clickProperty(760,57);
    check(removes==1,"property remove key button public IO");
    property.flags=PropertyFlags::Locked;
    clickProperty(760,57);
    check(adds==1 && removes==1,"locked property blocks key changes");
    clickProperty(40,57,1); propertyFrame();
    clickProperty(80,93);
    check(toggles==1 && property.flags==PropertyFlags::None,"locked property can be unlocked from label menu");
    ImGui::DestroyContext(context);
    std::puts(failures ? "FAIL editor core"
                       : "PASS timebase, drop-frame, snap, transaction, canvas and curves");
    return failures ? 1 : 0;
}
