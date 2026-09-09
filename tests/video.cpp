#include <vector>
#include <imkit/video.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <cstdlib>
using namespace imkit;
int main() {
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    int failures = 0;
    auto check = [&](bool ok, const char *s) {
        if (!ok) {
            ++failures;
            std::fprintf(stderr, "FAIL %s\n", s);
        }
    };
    video::TimelineState snapping;
    snapping.original.id=1;
    snapping.original.start=editor::FromSeconds(1);
    snapping.original.duration=editor::FromSeconds(2);
    snapping.drag.draft.kind=editor::EditKind::Move;
    std::array targets{editor::SnapCandidate{editor::FromSeconds(3.04),editor::SnapKind::Marker,2,88}};
    auto hit=video::ResolveTimelineSnap(snapping,0,targets,{});
    check(hit.snapped && hit.tick==editor::FromSeconds(1.04) && hit.candidate.tick==targets[0].tick,
          "moving end snaps and retains guide target separately from start");
    snapping.magnet=false;
    check(!video::ResolveTimelineSnap(snapping,0,targets,{}).snapped,"magnet off disables target attraction");
    snapping.magnet=true;
    std::array<editor::StableId,1> moving{88};
    check(!video::ResolveTimelineSnap(snapping,0,targets,moving).snapped,"moving selection never attracts itself");
    snapping.snapKinds &= ~(1u<<static_cast<unsigned>(editor::SnapKind::Marker));
    check(!video::ResolveTimelineSnap(snapping,0,targets,{}).snapped,"disabled target kind is ignored");
    snapping.snapToFrame=true;
    snapping.magnet=false;
    auto frameHit=video::ResolveTimelineSnap(snapping,editor::FromSeconds(.01),{},{});
    check(frameHit.snapped && frameHit.tick==snapping.original.start,"independent frame grid resolves nearest frame");
    snapping.snapping=false;
    check(!video::ResolveTimelineSnap(snapping,0,targets,{}).snapped,"snap master disables grid and targets");
    video::ClipView c;
    c.start = 100;
    c.duration = 50;
    c.sourceIn = 20;
    video::ClipConstraints bounds{0, 100, 2};
    auto trim = video::EditClip(c, editor::EditKind::TrimStart, -40, bounds);
    check(trim.valid && trim.start == 80 && trim.duration == 70 && trim.sourceIn == 0,
          "trim media lower bound");
    trim = video::EditClip(c, editor::EditKind::TrimEnd, -100, bounds);
    check(trim.duration == 2, "minimum duration");
    trim = video::EditClip(c, editor::EditKind::Ripple, 100, bounds);
    check(trim.duration == 80 && trim.rippleDelta == 30, "ripple constrained delta");
    auto slip = video::EditClip(c, editor::EditKind::Slip, 100, bounds);
    check(slip.start == 100 && slip.duration == 50 && slip.sourceIn == 50, "slip preserves timeline");
    auto split = video::SplitClip(c, 120, bounds);
    check(split.valid && split.left.duration == 20 && split.right.sourceIn == 40 &&
              split.right.duration == 30,
          "split source mapping");
    check(!video::SplitClip(c, 100, bounds).valid, "split rejects edge");
    auto invalidSplit=c;
    invalidSplit.speed=std::numeric_limits<double>::infinity();
    check(!video::SplitClip(invalidSplit,120,bounds).valid,"split rejects nonfinite speed");
    invalidSplit=c;invalidSplit.sourceIn=90;
    check(!video::SplitClip(invalidSplit,120,bounds).valid,"split rejects source range beyond media");
    check(!video::SplitClip(c,std::numeric_limits<editor::Tick>::min(),bounds).valid,
          "split rejects extreme cut before subtracting timeline ticks");
    invalidSplit=c;invalidSplit.start=std::numeric_limits<editor::Tick>::max()-10;
    check(!video::SplitClip(invalidSplit,std::numeric_limits<editor::Tick>::max()-5,bounds).valid,
          "split rejects overflowing timeline end");
    check(!video::SplitClip(c,120,{0,100,0}).valid,"split rejects invalid minimum duration");
    video::ClipView right = c;
    right.start = 150;
    auto roll = video::RollClips(c, right, 100, bounds, bounds);
    check(roll.valid && roll.left.start + roll.left.duration == roll.right.start &&
              roll.left.duration == 80 && roll.right.duration == 20,
          "roll shared constraint");
    auto third = right;
    third.start = 200;
    auto slide = video::SlideClip(c, right, third, 100, bounds, bounds, bounds);
    check(slide.valid && slide.current.duration == 50 &&
              slide.previous.start + slide.previous.duration == slide.current.start &&
              slide.current.start + slide.current.duration == slide.next.start,
          "slide keeps middle media and neighbor boundaries");
    third.start = 201;
    check(!video::SlideClip(c, right, third, 1, bounds, bounds, bounds).valid, "slide rejects gaps");
    c.locked = true;
    check(!video::EditClip(c, editor::EditKind::Move, 1, bounds).valid, "locked math");
    float pcm[] = {-1, .2f, 1, .4f, 0, .6f, 0, .8f};
    std::array<video::AudioBucket, 2> buckets;
    video::BuildAudioBuckets(pcm, 2, 0, buckets);
    check(buckets[0].minimum == -1 && buckets[0].maximum == 1 && buckets[0].rms == 1 && buckets[1].rms == 0,
          "PCM channel buckets");
    const float invalidPCM[]={std::numeric_limits<float>::quiet_NaN(),.5f};
    video::AudioBucket sanitized;
    video::BuildAudioBuckets(invalidPCM,1,0,{&sanitized,1});
    check(sanitized.minimum==0 && sanitized.maximum==.5f && std::isfinite(sanitized.rms),
          "first nonfinite PCM sample does not contaminate min/max");
    video::MeterState meter;
    video::UpdateMeter(meter, pcm, .1f);
    check(meter.heldPeak == 1, "meter peak");
    video::UpdateMeter(meter, {}, .5f);
    check(meter.heldPeak == 1, "peak hold");
    video::UpdateMeter(meter, {}, 2);
    check(meter.heldPeak < 1, "peak decay");
    std::array<std::uint32_t, 256> r{}, g{}, b{}, l{};
    std::array<std::uint32_t, 512> waveform{};
    std::array<std::uint32_t, 65536> vector{};
    video::Rgba pixels[] = {{1, 0, 0, 1}, {0, 1, 0, 1}};
    check(video::BuildScopes(pixels, 2, 1, {r, g, b, l, waveform, vector}), "scope shape");
    check(r[255] == 1 && r[0] == 1 && b[0] == 2, "scope channel bins");
    check(!video::BuildScopes(pixels, 3, 1, {r, g, b, l, waveform, vector}),
          "scope rejects mismatched dimensions");
    std::array<std::uint32_t,512> redWave{},greenWave{},blueWave{};
    check(video::BuildScopes(pixels,2,1,{r,g,b,l,waveform,vector,redWave,greenWave,blueWave}) &&
              redWave[255*2] == 1 && redWave[1] == 1 && greenWave[255*2+1] == 1 &&
              blueWave[0] == 1 && blueWave[1] == 1,
          "RGB waveform preserves channel and horizontal pixel position");
    check(!video::BuildScopes(pixels,2,1,{r,g,b,l,waveform,vector,redWave,{},blueWave}),
          "partial RGB waveform buffers rejected before write");
    auto *context = ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {800,600};
    io.DeltaTime = 1.f / 60;
    unsigned char *fontPixels; int fontWidth, fontHeight;
    io.Fonts->GetTexDataAsRGBA32(&fontPixels, &fontWidth, &fontHeight);
    video::TimelineState timeline;
    timeline.original.id = 1;
    timeline.original.duration = 100;
    video::TimelineProvider provider;
    provider.revision = 7;
    std::array<editor::Event,8> fullStorage;
    std::array<editor::Event,1> smallStorage;
    editor::EventBuffer full{fullStorage}, small{smallStorage};
    editor::Selection selection{};
    auto frame = [&](editor::EventBuffer &events) {
        ImGui::NewFrame();
        ImGui::SetNextWindowSize({780,580});
        ImGui::Begin("Atomic timeline events");
        video::Timeline("timeline", provider, timeline, selection, events,
                        imkit::MakePrecisionTheme(imkit::ColorScheme::Dark), {600,300});
        ImGui::End();
        ImGui::Render();
    };
    timeline.drag.Begin(1,7,editor::EditKind::Move,{0,100},{},full);
    timeline.nextDrag.Begin(2,7,editor::EditKind::Move,{100,200},{},full);
    full.Clear();
    frame(small);
    check(small.overflow && small.count == 0 && timeline.drag.active && timeline.nextDrag.active,
          "insufficient terminal capacity publishes no partial clip commit");
    frame(full);
    check(full.count == 2 && !timeline.drag.active && !timeline.nextDrag.active &&
              full.Events()[0].phase == editor::Phase::Commit &&
              full.Events()[1].phase == editor::Phase::Commit,
          "clip commit retries atomically after release frame");
    full.Clear(); small.Clear();
    timeline.drag.Begin(1,7,editor::EditKind::Move,{0,100},{},full);
    timeline.nextDrag.Begin(2,7,editor::EditKind::Move,{100,200},{},full);
    full.Clear(); provider.revision = 8;
    frame(small);
    check(small.overflow && small.count == 0 && timeline.drag.active && timeline.nextDrag.active,
          "insufficient cancel capacity preserves every clip transaction");
    frame(full);
    check(full.count == 2 && !timeline.drag.active && !timeline.nextDrag.active &&
              full.Events()[0].phase == editor::Phase::Cancel &&
              full.Events()[1].phase == editor::Phase::Cancel,
          "revision cancellation retries as one complete batch");
    struct LayoutFixture {
        std::array<video::TrackView,3> tracks;
        int calls=0;
        double firstPixel=0,lastPixel=0;
        editor::Range queriedTime{};
    } layoutFixture;
    for (int i=0;i<3;++i) {
        layoutFixture.tracks[i].id=100+i;
        layoutFixture.tracks[i].label="Track";
        layoutFixture.tracks[i].height=200-i*20.f;
    }
    provider.user=&layoutFixture;provider.trackCount=3;provider.totalHeight=540;
    provider.layout=[](void *u,double first,double last) {
        auto &f=*static_cast<LayoutFixture *>(u); ++f.calls;f.firstPixel=first;f.lastPixel=last;
        std::size_t a=0,b=0;double top=0,end=0;
        while (a<f.tracks.size() && top+video::TrackExtent(f.tracks[a])<=first)
            top+=video::TrackExtent(f.tracks[a++]);
        b=a;end=top;
        while (b<f.tracks.size() && end<last) end+=video::TrackExtent(f.tracks[b++]);
        return video::TrackLayout{std::span<const video::TrackView>(f.tracks).subspan(a,b-a),top};
    };
    timeline.verticalScroll=100; full.Clear();frame(full);
    check(layoutFixture.calls==1 && layoutFixture.firstPixel==100 && layoutFixture.lastPixel>100,
          "variable height timeline queries only the visible pixel interval");
    check(video::TrackExtent(layoutFixture.tracks[0])==200,"expanded track uses host height");
    timeline.verticalScroll=0;full.Clear();frame(full);
    auto clickTrack=[&](ImVec2 p) {
        full.Clear();io.AddMousePosEvent(p.x,p.y);frame(full);
        io.AddMouseButtonEvent(0,true);frame(full);
        io.AddMouseButtonEvent(0,false);frame(full);
    };
    clickTrack({timeline.view.min.x+10,timeline.view.min.y+10});
    check(full.count==2 && full.Events()[0].phase==editor::Phase::Begin && full.Events()[1].phase==editor::Phase::Commit &&
              full.Events()[0].target==100 &&
              full.Events()[1].proposed.x==static_cast<double>(video::TrackControl::Expanded) &&
              full.Events()[1].proposed.y==0,"track collapse emits the explicit host control");
    layoutFixture.tracks[0].expanded=false;
    check(video::TrackExtent(layoutFixture.tracks[0])==32,"collapsed row has compact extent");
    layoutFixture.tracks[0].expanded=true;
    float sourceX=timeline.view.min.x+4;
    for (const char *label:{"V","M","S","L","R","T"})
        sourceX+=ImGui::CalcTextSize(label).x+ImGui::GetStyle().FramePadding.x*2+2;
    clickTrack({sourceX+5,timeline.view.min.y+35});
    check(full.count==2 && full.Events()[1].proposed.x==static_cast<double>(video::TrackControl::Source) &&
              full.Events()[1].proposed.y==1,"source patch button emits a distinct control");
    timeline.trackLabels.controls="\xe6\x93\x8d\xe4\xbd\x9c"; // Host UTF-8 menu label.
    timeline.headerWidth=100;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    full.Clear();frame(full);
    clickTrack({timeline.view.min.x+15,timeline.view.min.y+35});
    full.Clear();
    io.AddKeyEvent(ImGuiKey_DownArrow,true);frame(full);
    io.AddKeyEvent(ImGuiKey_DownArrow,false);frame(full);
    io.AddKeyEvent(ImGuiKey_Enter,true);frame(full);
    io.AddKeyEvent(ImGuiKey_Enter,false);frame(full);
    check(full.count==2 && full.Events()[0].phase==editor::Phase::Begin && full.Events()[1].phase==editor::Phase::Commit &&
              full.Events()[0].target==100 &&
        full.Events()[0].kind==editor::EditKind::Toggle,
        "narrow track control menu emits host event through public IO");
    small.Clear();io.AddMousePosEvent(timeline.view.min.x+10,timeline.view.min.y+10);frame(small);
    io.AddMouseButtonEvent(0,true);frame(small);io.AddMouseButtonEvent(0,false);frame(small);
    check(small.overflow && small.count==0,"track toggle shortage emits neither Begin nor Commit");
    full.Clear();timeline.heightDrag.Begin(100,provider.revision,editor::EditKind::TrackHeight,{0,0,0,0,200},{},full);
    full.Clear();frame(full);
    check(!timeline.heightDrag.active && full.count==1 && full.Events()[0].phase==editor::Phase::Cancel,
          "closed track height popup cancels active transaction");
    full.Clear();timeline.heightDrag.Begin(100,provider.revision,editor::EditKind::TrackHeight,{0,0,0,0,200},{},full);
    small.Clear();small.Push({});frame(small);
    check(timeline.heightDrag.active && timeline.heightDrag.draft.phase==editor::Phase::Cancel && small.overflow,
          "closed track height popup retains Cancel when event buffer is full");
    full.Clear();frame(full);
    check(!timeline.heightDrag.active && full.count==1 && full.Events()[0].phase==editor::Phase::Cancel,
          "track height retries its pending Cancel once capacity returns");
    timeline.headerWidth=180;
    provider.contentRange={editor::FromSeconds(-10),editor::FromSeconds(10)};
    provider.clips=[](void *u,editor::StableId,editor::Range range) {
        static_cast<LayoutFixture*>(u)->queriedTime=range;
        return std::span<const video::ClipView>{};
    };
    std::array fitBindings{editor::Binding{editor::Command::Fit,ImGuiKey_F8}};
    timeline.bindings=fitBindings;
    const double oldY=timeline.canvas.scale.y;
    full.Clear();io.AddKeyEvent(ImGuiKey_F8,true);frame(full);
    io.AddKeyEvent(ImGuiKey_F8,false);frame(full);
    const double expectedScale=(600.-timeline.headerWidth-48)/20;
    check(std::abs(timeline.canvas.scale.x-expectedScale)<1e-9 &&
              std::abs(timeline.canvas.origin.x-(-10-24/expectedScale))<1e-9,
          "host remapped Fit frames negative and positive timeline extent with padding");
    check(timeline.canvas.scale.y==oldY,"timeline Fit preserves vertical scale");
    check(layoutFixture.queriedTime.first<=provider.contentRange.first &&
              layoutFixture.queriedTime.last>=provider.contentRange.last,
          "visible query contains the fitted timeline range");
    timeline.bindings={};timeline.canvas.origin.x=7;
    io.AddKeyEvent(ImGuiKey_F8,true);frame(full);io.AddKeyEvent(ImGuiKey_F8,false);frame(full);
    check(timeline.canvas.origin.x==7,"empty bindings disable Fit shortcut");
    struct TransitionFixture {video::TrackView track;video::ClipView clip;std::array<video::ClipView,2> related;} transitionFixture;
    transitionFixture.track.id=900;transitionFixture.track.label="Video";
    transitionFixture.clip.id=901;transitionFixture.clip.track=900;transitionFixture.clip.label="Transition";
    transitionFixture.clip.duration=editor::FromSeconds(3);
    transitionFixture.clip.transitionIn=editor::FromSeconds(.5);
    transitionFixture.clip.transitionOut=editor::FromSeconds(.5);
    provider={};provider.user=&transitionFixture;provider.trackCount=1;provider.revision=1;
    provider.tracks=[](void *u,int,int){return std::span<const video::TrackView>(&static_cast<TransitionFixture*>(u)->track,1);};
    provider.clips=[](void *u,editor::StableId,editor::Range){return std::span<const video::ClipView>(&static_cast<TransitionFixture*>(u)->clip,1);};
    timeline={};full.Clear();frame(full);frame(full);
    for (int side=0;side<2;++side) {
        const auto stableOrigin=timeline.view.min;
        const float x=timeline.view.min.x+timeline.headerWidth+(side ? 250.f : 50.f);
        const float y=timeline.view.min.y+10;
        full.Clear();io.AddMousePosEvent(x,y);frame(full);frame(full);
        io.AddMouseButtonEvent(0,true);frame(full);
        check(timeline.transitionDrag.active && timeline.transitionEnd==(side==1),"transition handle begins the selected end");
        full.Clear();io.AddMousePosEvent(x+(side ? -50.f : 50.f),y);frame(full);
        check(timeline.transitionDrag.draft.proposed.first==editor::FromSeconds(side ? .5 : 1.) &&
              timeline.transitionDrag.draft.proposed.last==editor::FromSeconds(side ? 1. : .5),"transition handle changes only its duration");
        check(timeline.view.min.x==stableOrigin.x && timeline.view.min.y==stableOrigin.y,"transition drag does not move the ImGui window");
        full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
        check(full.count==1 && full.Events()[0].kind==editor::EditKind::TransitionDuration &&
              full.Events()[0].phase==editor::Phase::Commit && !timeline.transitionDrag.active,"transition duration commits without moving the clip");
    }
    auto transitionEdit=video::EditTransition(transitionFixture.clip,false,editor::FromSeconds(100));
    check(transitionEdit.valid && transitionEdit.inDuration==editor::FromSeconds(2.5) &&
          transitionEdit.outDuration==editor::FromSeconds(.5),"transition clamp preserves opposite end");
    transitionFixture.clip.locked=true;
    check(!video::EditTransition(transitionFixture.clip,true,1).valid,"locked transition calculation rejects edit");
    transitionFixture.clip.locked=false;
    for (int reason=0;reason<3;++reason) {
        const float x=timeline.view.min.x+timeline.headerWidth+50,y=timeline.view.min.y+10;
        full.Clear();io.AddMousePosEvent(x,y);frame(full);frame(full);
        io.AddMouseButtonEvent(0,true);frame(full);
        check(timeline.transitionDrag.active,"transition cancellation setup");
        if (reason==0) transitionFixture.track.locked=true;
        if (reason==1) ++provider.revision;
        if (reason==2) io.AddKeyEvent(ImGuiKey_Escape,true);
        editor::EventBuffer noSpace{};frame(noSpace);
        check(noSpace.overflow && timeline.transitionDrag.active,"transition Cancel survives buffer shortage");
        full.Clear();frame(full);
        check(!timeline.transitionDrag.active && full.count==1 && full.Events()[0].phase==editor::Phase::Cancel,
              "transition Cancel retries after lock revision or Escape");
        transitionFixture.track.locked=false;
        io.AddKeyEvent(ImGuiKey_Escape,false);io.AddMouseButtonEvent(0,false);full.Clear();frame(full);
    }
    editor::Keyframe clipKey;clipKey.id=1901;clipKey.tick=editor::FromSeconds(1);clipKey.value=.7;
    transitionFixture.clip.keys={&clipKey,1};
    editor::StableId keyIds[4];editor::Selection keySelection{keyIds};timeline.keySelection=&keySelection;
    full.Clear();frame(full);
    const float keyX=timeline.view.min.x+timeline.headerWidth+100;
    const float keyY=timeline.view.min.y+video::TrackExtent(transitionFixture.track)-16;
    io.AddMousePosEvent(keyX,keyY);frame(full);io.AddMouseButtonEvent(0,true);full.Clear();frame(full);
    check(timeline.keyDrag.active && keySelection.Contains(clipKey.id) && !timeline.drag.active,"clip key hit begins key edit without clip movement");
    io.AddMousePosEvent(keyX+50,keyY);full.Clear();frame(full);
    check(timeline.keyDrag.draft.proposed.first==editor::FromSeconds(1.5) && clipKey.tick==editor::FromSeconds(1),"clip key preview preserves host data");
    full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
    check(full.count==1 && full.Events()[0].kind==editor::EditKind::Keyframe && full.Events()[0].target==clipKey.id &&
          full.Events()[0].proposed.parent==transitionFixture.clip.id && full.Events()[0].phase==editor::Phase::Commit,
          "clip key commits local time with owner ID");
    editor::Keyframe clipKeys[]={clipKey,clipKey};clipKeys[1].id=1902;clipKeys[1].tick=editor::FromSeconds(2);
    transitionFixture.clip.keys=clipKeys;keySelection.Set(clipKeys[1].id,true);
    editor::Transaction clipKeyCompanions[1];timeline.keyCompanions=clipKeyCompanions;
    full.Clear();io.AddMousePosEvent(keyX,keyY);frame(full);io.AddMouseButtonEvent(0,true);frame(full);
    check(timeline.keyCompanionCount==1 && full.count==2,"clip multi-key Begin covers complete selection");
    full.Clear();io.AddMousePosEvent(keyX+300,keyY);frame(full);
    check(timeline.keyDrag.draft.proposed.first==editor::FromSeconds(2) &&
          clipKeyCompanions[0].draft.proposed.first==editor::FromSeconds(3),"clip multi-key clamp preserves spacing");
    small.Clear();io.AddMouseButtonEvent(0,false);frame(small);
    check(small.overflow && small.count==0 && timeline.keyDrag.active && clipKeyCompanions[0].active,
          "clip multi-key terminal shortage retains all targets");
    full.Clear();frame(full);
    check(full.count==2 && full.Events()[0].phase==editor::Phase::Commit && full.Events()[1].phase==editor::Phase::Commit &&
          !timeline.keyDrag.active && !clipKeyCompanions[0].active,"clip multi-key commits complete retry batch");
    full.Clear();io.AddMousePosEvent(keyX,keyY);io.AddKeyEvent(ImGuiMod_Ctrl,true);frame(full);
    io.AddMouseButtonEvent(0,true);frame(full);
    check(!keySelection.Contains(clipKeys[0].id) && keySelection.Contains(clipKeys[1].id) &&
          !timeline.keyDrag.active && full.count==0 && !full.overflow,
          "Ctrl-click deselects clip key without starting a drag or reporting shortage");
    io.AddMousePosEvent(keyX+30,keyY);frame(full);io.AddMouseButtonEvent(0,false);frame(full);
    check(full.count==0 && !timeline.drag.active,"deselected key cannot move remaining keys or the clip");
    io.AddKeyEvent(ImGuiMod_Ctrl,false);frame(full);keySelection.Set(clipKeys[0].id,true);
    std::array keyBindings{editor::Binding{editor::Command::Duplicate,ImGuiKey_F7},editor::Binding{editor::Command::Delete,ImGuiKey_F6}};
    timeline.bindings=keyBindings;
    full.Clear();io.AddKeyEvent(ImGuiKey_F7,true);frame(full);io.AddKeyEvent(ImGuiKey_F7,false);frame(full);
    check(full.count==4 && full.Events()[1].kind==editor::EditKind::Duplicate && full.Events()[3].kind==editor::EditKind::Duplicate &&
          full.Events()[1].proposed.first-clipKeys[0].tick==full.Events()[3].proposed.first-clipKeys[1].tick,
          "remapped clip key Duplicate emits complete equally-offset pairs");
    clipKeys[1].locked=true;full.Clear();io.AddKeyEvent(ImGuiKey_F6,true);frame(full);io.AddKeyEvent(ImGuiKey_F6,false);frame(full);
    check(full.count==0,"one locked clip key rejects whole Delete command");clipKeys[1].locked=false;
    small.Clear();io.AddKeyEvent(ImGuiKey_F6,true);frame(small);io.AddKeyEvent(ImGuiKey_F6,false);frame(small);
    check(small.overflow && small.count==0,"clip key Delete refuses partial event capacity");
    full.Clear();io.AddKeyEvent(ImGuiKey_F6,true);frame(full);io.AddKeyEvent(ImGuiKey_F6,false);frame(full);
    check(full.count==4 && full.Events()[1].kind==editor::EditKind::Remove && full.Events()[3].kind==editor::EditKind::Remove,
          "remapped clip key Delete commits all selected keys");timeline.bindings={};
    std::array navigationBindings{editor::Binding{editor::Command::AddKey,ImGuiKey_F8},
        editor::Binding{editor::Command::PreviousKey,ImGuiKey_F9},editor::Binding{editor::Command::NextKey,ImGuiKey_F10}};
    timeline.bindings=navigationBindings;transitionFixture.clip.keyChannel=7100;
    timeline.time.playhead=editor::FromSeconds(1.5);
    auto keyCommand=[&](ImGuiKey key,editor::EventBuffer &events) {
        events.Clear();io.AddKeyEvent(key,true);frame(events);io.AddKeyEvent(key,false);frame(events);
    };
    keyCommand(ImGuiKey_F9,full);
    check(full.count==2 && full.Events()[1].kind==editor::EditKind::Navigate &&
          timeline.time.playhead==clipKeys[0].tick,"remapped previous clip key moves playhead");
    keyCommand(ImGuiKey_F10,full);
    check(full.count==2 && timeline.time.playhead==clipKeys[1].tick,"remapped next clip key moves playhead");
    timeline.time.playhead=editor::FromSeconds(1.5);keyCommand(ImGuiKey_F8,full);
    check(full.count==2 && full.Events()[1].kind==editor::EditKind::KeyInsert && full.Events()[1].target==7100 &&
          full.Events()[1].proposed.parent==transitionFixture.clip.id && full.Events()[1].proposed.first==editor::FromSeconds(1.5),
          "clip insertion emits explicit channel and local time");
    keyCommand(ImGuiKey_F8,small);check(small.overflow && small.count==0,"clip insertion refuses partial event pair");
    transitionFixture.track.locked=true;keyCommand(ImGuiKey_F8,full);
    check(full.count==0,"locked track rejects clip key insertion");transitionFixture.track.locked=false;
    editor::Keyframe outsideKeys[]={{7501,7100,editor::FromSeconds(-1),0},{7502,7100,editor::FromSeconds(4),1}};
    outsideKeys[0].interpolation=editor::Interpolation::Linear;
    transitionFixture.clip.keyEvaluation=outsideKeys;
    timeline.time.playhead=editor::FromSeconds(1.5);keyCommand(ImGuiKey_F8,full);
    check(full.count==2 && full.Events()[1].proposed.x==.5,"clip key insertion evaluates off-clip context instead of clipped visible keys");
    transitionFixture.clip.keyEvaluation={};
    timeline.time.playhead=clipKeys[0].tick;keyCommand(ImGuiKey_F8,full);
    check(full.count==0,"clip insertion preserves existing key at current time");timeline.bindings={};
    transitionFixture.clip.keys={};timeline.keySelection=nullptr;
    transitionFixture.track.kind=video::TrackKind::Caption;
    auto beginCaption=[&] {
        io.DeltaTime=.4f;frame(full);io.DeltaTime=1.f/60;
        full.Clear();io.AddMousePosEvent(timeline.view.min.x+timeline.headerWidth+130,timeline.view.min.y+24);frame(full);
        for (int click=0;click<2;++click) {
            io.AddMouseButtonEvent(0,true);frame(full);io.AddMouseButtonEvent(0,false);frame(full);
        }
        check(timeline.captionDrag.active && timeline.captionDrag.draft.originalText[0]=='T',"caption double click begins text transaction");
        full.Clear();frame(full);
    };
    beginCaption();
    io.AddInputCharactersUTF8("\xe5\xad\x97\xe5\xb9\x95");frame(full);
    full.Clear();io.AddKeyEvent(ImGuiKey_Enter,true);frame(full);io.AddKeyEvent(ImGuiKey_Enter,false);frame(full);
    check(!timeline.captionDrag.active && full.count==1 && full.Events()[0].phase==editor::Phase::Commit &&
          std::string_view(full.Events()[0].proposedText.data())=="\xe5\xad\x97\xe5\xb9\x95" &&
          std::string_view(full.Events()[0].originalText.data())=="Transition","caption Enter commits original and UTF8 proposal");
    beginCaption();io.AddInputCharactersUTF8("cancel");frame(full);
    full.Clear();io.AddKeyEvent(ImGuiKey_Escape,true);frame(full);io.AddKeyEvent(ImGuiKey_Escape,false);frame(full);
    check(!timeline.captionDrag.active && full.count==1 && full.Events()[0].phase==editor::Phase::Cancel &&
          full.Events()[0].proposedText==full.Events()[0].originalText,"caption Escape restores original text in Cancel");
    transitionFixture.track.kind=video::TrackKind::Video;
    timeline.tool=video::Tool::Hand;full.Clear();frame(full);
    const auto panOrigin=timeline.view.min;const double timeOrigin=timeline.canvas.origin.x;
    const auto selectedClips=selection.count;
    io.AddMousePosEvent(panOrigin.x+timeline.headerWidth+130,panOrigin.y+25);frame(full);
    io.AddMouseButtonEvent(0,true);frame(full);
    io.AddMousePosEvent(panOrigin.x+timeline.headerWidth+170,panOrigin.y+25);frame(full);
    io.AddMouseButtonEvent(0,false);frame(full);
    check(timeline.canvas.origin.x<timeOrigin && timeline.view.min.x==panOrigin.x && timeline.view.min.y==panOrigin.y &&
          full.count==0 && selection.count==selectedClips,"Hand pan scrolls time without moving window or selecting clip");
    std::array toolBindings{editor::Binding{editor::Command::ToolSelect,ImGuiKey_F1},
        editor::Binding{editor::Command::ToolRazor,ImGuiKey_F2},editor::Binding{editor::Command::ToolRipple,ImGuiKey_F3},
        editor::Binding{editor::Command::ToolRoll,ImGuiKey_F4},editor::Binding{editor::Command::ToolSlip,ImGuiKey_F5},
        editor::Binding{editor::Command::ToolSlide,ImGuiKey_F6},editor::Binding{editor::Command::ToolHand,ImGuiKey_F7}};
    timeline.bindings=toolBindings;
    for (int i=0;i<7;++i) {
        const auto key=static_cast<ImGuiKey>(ImGuiKey_F1+i);full.Clear();
        io.AddKeyEvent(key,true);frame(full);io.AddKeyEvent(key,false);frame(full);
        check(static_cast<int>(timeline.tool)==i && full.count==0,"remapped tool command changes mode without edit events");
    }
    timeline.bindings={};io.AddKeyEvent(ImGuiKey_C,true);frame(full);io.AddKeyEvent(ImGuiKey_C,false);frame(full);
    check(timeline.tool==video::Tool::Hand,"unbound default tool key does not bypass host binding map");
    timeline.tool=video::Tool::Select;
    timeline.canvas.origin.x=0;full.Clear();frame(full);
    const auto clipOrigin=timeline.view.min;
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+100,clipOrigin.y+25);frame(full);
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(full.overflow && !timeline.drag.active,"clip selection scratch shortage prevents edit Begin");
    io.AddMouseButtonEvent(0,false);frame(full);
    editor::StableId clipIds[4];selection.storage=clipIds;selection.Clear();
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+140,clipOrigin.y+25);frame(full);
    check(timeline.drag.active && timeline.view.min.x==clipOrigin.x && timeline.view.min.y==clipOrigin.y,
          "clip body drag keeps parent window stationary");
    io.AddMouseButtonEvent(0,false);frame(full);
    check(!timeline.drag.active && full.Events().back().phase==editor::Phase::Commit,"clip body drag commits normally");
    full.Clear();io.AddKeyEvent(ImGuiMod_Ctrl,true);frame(full);
    io.AddMouseButtonEvent(0,true);frame(full);
    check(!selection.Contains(transitionFixture.clip.id) && !timeline.drag.active && full.count==0 && !full.overflow,
          "Ctrl-click deselects clip without starting edit transaction");
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+160,clipOrigin.y+25);frame(full);
    io.AddMouseButtonEvent(0,false);frame(full);io.AddKeyEvent(ImGuiMod_Ctrl,false);frame(full);
    check(full.count==0 && !timeline.drag.active,"deselected clip emits no Update or Commit after mouse movement");
    std::array splitClips{transitionFixture.clip,transitionFixture.clip};splitClips[1].id=902;
    auto savedUser=provider.user;
    provider.user=&splitClips;
    provider.selected=[](void *u,std::span<const editor::StableId>){return std::span<const video::ClipView>(*static_cast<std::array<video::ClipView,2>*>(u));};
    auto savedTracks=provider.tracks;auto savedClips=provider.clips;
    provider.tracks=nullptr;provider.clips=nullptr;
    selection.storage[0]=901;selection.storage[1]=902;selection.count=2;
    std::array splitBindings{editor::Binding{editor::Command::Split,ImGuiKey_F9}};
    timeline.bindings=splitBindings;timeline.time.playhead=editor::FromSeconds(1);
    auto splitCommand=[&](editor::EventBuffer &buffer) {
        buffer.Clear();io.AddKeyEvent(ImGuiKey_F9,true);frame(buffer);
        io.AddKeyEvent(ImGuiKey_F9,false);frame(buffer);
    };
    splitCommand(full);
    check(full.count==4 && full.Events()[1].phase==editor::Phase::Commit &&
          full.Events()[3].target==902 && full.Events()[3].proposed.first==timeline.time.playhead,
          "remapped Split atomically emits selected offscreen clip transactions");
    splitClips[1].locked=true;splitCommand(full);
    check(full.count==0,"locked selected clip prevents partial Split");
    splitClips[1].locked=false;splitCommand(small);
    check(small.overflow && small.count==0,"Split reserves entire selection event batch");
    selection.storage[1]=999;splitCommand(full);
    check(full.overflow && full.count==0,"incomplete selected query prevents partial Split");
    provider.selected=nullptr;provider.user=savedUser;provider.tracks=savedTracks;provider.clips=savedClips;
    selection.Clear();timeline.bindings={};
    selection.storage[0]=transitionFixture.clip.id;selection.storage[1]=902;selection.count=2;
    provider.selected=[](void *u,std::span<const editor::StableId>){
        return std::span<const video::ClipView>(&static_cast<TransitionFixture*>(u)->clip,1);
    };
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(full.overflow && full.count==0 && !timeline.drag.active,
          "incomplete clip selection query rejects entire move before Begin");
    io.AddMouseButtonEvent(0,false);frame(full);provider.selected=nullptr;
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(full.overflow && full.count==0 && !timeline.drag.active,
          "multiple selected clips require complete provider resolution");
    io.AddMouseButtonEvent(0,false);frame(full);selection.Clear();
    transitionFixture.related={transitionFixture.clip,transitionFixture.clip};
    transitionFixture.related[1].id=902;transitionFixture.related[1].duration=editor::FromSeconds(.3);
    provider.selected=[](void *u,std::span<const editor::StableId>){
        return std::span<const video::ClipView>(static_cast<TransitionFixture*>(u)->related);
    };
    provider.constraints=[](void *,editor::StableId){return video::ClipConstraints{0,editor::FromSeconds(100),editor::FromSeconds(.1)};};
    std::array<video::TimelineState::MemberDrag,2> trimMembers;
    timeline.memberDrags=trimMembers;timeline.snapping=false;
    selection.Set(transitionFixture.clip.id);full.Clear();
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+2,clipOrigin.y+25);frame(full);
    io.AddMouseButtonEvent(0,true);frame(full);
    check(timeline.drag.active && timeline.memberCount==1 && timeline.drag.draft.kind==editor::EditKind::TrimStart,
          "linked trim begins primary and offscreen member transactions");
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+52,clipOrigin.y+25);frame(full);
    check(timeline.drag.draft.proposed.first==editor::FromSeconds(.2) &&
          trimMembers[0].transaction.draft.proposed.first==editor::FromSeconds(.2) &&
          trimMembers[0].transaction.draft.proposed.last==editor::FromSeconds(.3),
          "linked trim shares shortest member minimum-duration constraint");
    full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
    check(full.count==2 && full.Events()[0].phase==editor::Phase::Commit && full.Events()[1].phase==editor::Phase::Commit,
          "linked trim commits all members together");
    timeline.tool=video::Tool::Slip;
    transitionFixture.related[1].sourceIn=editor::FromSeconds(99.5);
    full.Clear();io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+100,clipOrigin.y+25);frame(full);
    io.AddMouseButtonEvent(0,true);frame(full);
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+150,clipOrigin.y+25);frame(full);
    check(timeline.drag.active && timeline.drag.draft.proposed.first==transitionFixture.clip.start &&
          timeline.drag.draft.proposed.last==transitionFixture.clip.start+transitionFixture.clip.duration &&
          timeline.drag.draft.proposed.offset==editor::FromSeconds(.2) &&
          trimMembers[0].transaction.draft.proposed.offset==editor::FromSeconds(99.7),
          "linked Slip preserves placement and shares tightest source upper bound");
    full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
    check(full.count==2 && full.Events()[0].kind==editor::EditKind::Slip && full.Events()[1].kind==editor::EditKind::Slip,
          "linked Slip commits source changes for every member");
    timeline.tool=video::Tool::Select;
    provider.selected=nullptr;provider.constraints=nullptr;timeline.memberCount=0;timeline.memberDrags={};selection.Clear();
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+160,clipOrigin.y+25);frame(full);
    timeline.tool=video::Tool::Razor;full.Clear();frame(full);
    small.Clear();io.AddMouseButtonEvent(0,true);frame(small);
    check(small.overflow && small.count==0 && !timeline.drag.active,"Razor rejects insufficient buffer without partial transaction");
    io.AddMouseButtonEvent(0,false);frame(full);full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(full.count==2 && full.Events()[0].phase==editor::Phase::Begin && full.Events()[1].phase==editor::Phase::Commit &&
          full.Events()[0].kind==editor::EditKind::Split && full.Events()[1].target==transitionFixture.clip.id &&
          full.Events()[1].revision==provider.revision && full.Events()[1].original.first==transitionFixture.clip.start &&
          full.Events()[1].proposed.first>transitionFixture.clip.start,
          "Razor emits complete split transaction with original clip and proposed cut");
    io.AddMouseButtonEvent(0,false);frame(full);timeline.tool=video::Tool::Select;
    timeline.tool=video::Tool::Ripple;
    provider.canBeginEdit=[](void *,editor::StableId,editor::EditKind kind){return kind!=editor::EditKind::Ripple;};
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(full.count==0 && !full.overflow && !timeline.drag.active,"host nonlocal preflight rejects Ripple before Begin");
    io.AddMouseButtonEvent(0,false);frame(full);provider.canBeginEdit=nullptr;
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(timeline.drag.active && full.Events()[0].phase==editor::Phase::Begin,"optional preflight preserves permitted Ripple start");
    io.AddMouseButtonEvent(0,false);frame(full);timeline.tool=video::Tool::Select;
    video::EnvelopePoint envelope[]={{2901,editor::FromSeconds(.5),.5},{2902,editor::FromSeconds(1.5),1}};
    check(video::EvaluateEnvelope(envelope,editor::FromSeconds(1))==.75 && video::EvaluateEnvelope({},0)==1,
          "volume envelope linearly interpolates gain and defaults to unity");
    transitionFixture.clip.envelope=envelope;transitionFixture.track.kind=video::TrackKind::Audio;
    timeline.canvas.origin.x=0;full.Clear();frame(full);
    const float envelopeTop=std::min(timeline.view.min.y+video::TrackExtent(transitionFixture.track)-9,
        timeline.view.min.y+4+2*ImGui::GetFontSize()+7);
    const float envelopeBottom=timeline.view.min.y+video::TrackExtent(transitionFixture.track)-9;
    const float envelopeX=timeline.view.min.x+timeline.headerWidth+150;
    const float envelopeY=(envelopeTop+envelopeBottom)*.5f;
    io.AddMousePosEvent(envelopeX,envelopeY);frame(full);full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(timeline.envelopeDrag.active && !timeline.drag.active,"volume envelope point owns its drag");
    io.AddMousePosEvent(envelopeX+20,envelopeY-6);full.Clear();frame(full);
    check(timeline.envelopeDrag.draft.proposed.x>1 && timeline.envelopeDrag.draft.proposed.first>envelope[1].tick &&
          envelope[1].gain==1,"volume envelope updates gain and local time without host mutation");
    full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
    check(full.count==1 && full.Events()[0].kind==editor::EditKind::AudioEnvelope &&
          full.Events()[0].target==2902 && full.Events()[0].phase==editor::Phase::Commit,"volume envelope emits typed Commit");
    auto envelopeMenu=[&](float x,float y) {
        full.Clear();io.AddMousePosEvent(x,y);frame(full);io.AddMouseButtonEvent(1,true);frame(full);
        io.AddMouseButtonEvent(1,false);frame(full);io.AddMousePosEvent(790,590);frame(full);
        io.AddKeyEvent(ImGuiKey_Home,true);frame(full);io.AddKeyEvent(ImGuiKey_Home,false);frame(full);
        io.AddKeyEvent(ImGuiKey_Enter,true);frame(full);io.AddKeyEvent(ImGuiKey_Enter,false);frame(full);
    };
    envelopeMenu(timeline.view.min.x+timeline.headerWidth+210,timeline.view.min.y+25);
    check(full.count==2 && full.Events()[1].kind==editor::EditKind::AudioEnvelope && full.Events()[1].proposed.offset==1 &&
          full.Events()[1].target==transitionFixture.clip.id,"clip menu emits envelope insertion request");
    envelopeMenu(envelopeX,envelopeY);
    check(full.count==2 && full.Events()[1].kind==editor::EditKind::AudioEnvelope && full.Events()[1].proposed.offset==2 &&
          full.Events()[1].target==2902,"point menu emits envelope removal request");
    transitionFixture.clip.envelope={};transitionFixture.track.kind=video::TrackKind::Video;
    ImVec2 pickerOrigin{};
    auto pickerFrame=[&](editor::EventBuffer &events) {
        ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({780,580});
        ImGui::Begin("Transition picker");pickerOrigin=ImGui::GetCursorScreenPos();
        video::TransitionPicker("types",transitionFixture.clip,1,events);
        ImGui::End();ImGui::Render();
    };
    auto chooseTransition=[&](int side,int option,editor::EventBuffer &events) {
        events.Clear();pickerFrame(events);pickerFrame(events);
        io.AddMousePosEvent(pickerOrigin.x+80,pickerOrigin.y+ImGui::GetFrameHeight()*.5f+side*ImGui::GetFrameHeightWithSpacing());pickerFrame(events);
        io.AddMouseButtonEvent(0,true);pickerFrame(events);io.AddMouseButtonEvent(0,false);pickerFrame(events);
        io.AddMousePosEvent(770,570);pickerFrame(events);
        auto key=[&](ImGuiKey k){io.AddKeyEvent(k,true);pickerFrame(events);io.AddKeyEvent(k,false);pickerFrame(events);};
        key(ImGuiKey_Home);for (int i=0;i<option;++i) key(ImGuiKey_DownArrow);key(ImGuiKey_Enter);
    };
    chooseTransition(0,2,full);
    check(full.count==2 && full.Events()[0].phase==editor::Phase::Begin &&
          full.Events()[1].phase==editor::Phase::Commit && full.Events()[1].kind==editor::EditKind::TransitionType &&
          full.Events()[1].proposed.first==2 && full.Events()[1].proposed.last==1,
          "transition picker changes in kind with a complete typed transaction");
    chooseTransition(1,3,full);
    check(full.count==2 && full.Events()[1].proposed.first==1 && full.Events()[1].proposed.last==3,
          "transition picker preserves the other end");
    chooseTransition(0,2,small);
    check(small.overflow && small.count==0,"transition picker rejects partial Begin Commit delivery");
    transitionFixture.clip.locked=true;chooseTransition(0,2,full);
    check(full.count==0,"locked transition picker emits no edit");transitionFixture.clip.locked=false;
    {
        std::vector<editor::Keyframe> denseKeys(100000);
        for (std::size_t i=0;i<denseKeys.size();++i) denseKeys[i]={10000+i,7100,editor::FromSeconds(double(i)),double(i)};
        transitionFixture.clip.keys=denseKeys;transitionFixture.clip.duration=editor::FromSeconds(100000);
        transitionFixture.clip.envelope={};transitionFixture.track.kind=video::TrackKind::Video;
        timeline={};timeline.canvas.scale.x=100;full.Clear();frame(full);frame(full);
        check(ImGui::GetDrawData()->TotalVtxCount<10000,"100k inline keys emit geometry only for visible interval");
        transitionFixture.clip.keys={};
        std::vector<video::EnvelopePoint> denseEnvelope(100000);
        for (std::size_t i=0;i<denseEnvelope.size();++i) denseEnvelope[i]={200000+i,editor::FromSeconds(double(i)),double(i%2)};
        transitionFixture.clip.envelope=denseEnvelope;transitionFixture.track.kind=video::TrackKind::Audio;
        full.Clear();frame(full);frame(full);
        check(ImGui::GetDrawData()->TotalVtxCount<10000,"100k envelope points emit visible geometry with boundary neighbors");
        transitionFixture.clip.envelope={};
    }
    video::ColorValues hostColors;
    video::ColorPropertyIds colorIds{101,307,509,701,907,1103};
    video::ColorState colorState;
    std::uint64_t colorRevision=9;
    int colorCommits=0;
    bool colorCancelled=false;
    auto colorFrame = [&] {
        full.Clear();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({780,580});
        ImGui::Begin("Three-way color");
        video::ColorControls("grade",hostColors,colorIds,colorRevision,colorState,full);
        ImGui::End();
        ImGui::Render();
        for (const auto &event : full.Events()) {
            colorCancelled |= event.phase == editor::Phase::Cancel;
            if (event.phase != editor::Phase::Commit) continue;
            float *rgb = event.target==colorIds.lift ? hostColors.lift :
                         event.target==colorIds.gamma ? hostColors.gamma : hostColors.gain;
            rgb[0]=static_cast<float>(event.proposed.x); rgb[1]=static_cast<float>(event.proposed.y);
            rgb[2]=static_cast<float>(event.proposed.z); ++colorCommits;
        }
    };
    colorFrame(); colorFrame();
    for (int wheel=0; wheel<3; ++wheel) {
        const auto center=colorState.wheelCenters[wheel];
        io.AddMousePosEvent(center.x,center.y); colorFrame();
        io.AddMouseButtonEvent(0,true); colorFrame();
        io.AddMousePosEvent(center.x+colorState.wheelRadius*.6f,center.y); colorFrame();
        check(colorState.drag.active && colorCommits==wheel,"wheel preview remains uncommitted");
        io.AddMouseButtonEvent(0,false); colorFrame();
        const float *rgb=wheel==0 ? hostColors.lift : wheel==1 ? hostColors.gamma : hostColors.gain;
        const float neutral=wheel==0 ? 0.f : 1.f;
        const float pointerX=(std::floor(center.x+colorState.wheelRadius*.6f)-center.x)/colorState.wheelRadius;
        check(colorCommits==wheel+1 && std::abs(rgb[0]-neutral-pointerX)<.001f &&
                  std::abs(rgb[1]-neutral+pointerX*.5f)<.001f &&
                  std::abs(rgb[2]-neutral+pointerX*.5f)<.001f,
              "three-way wheel pointer gesture applies typed RGB proposal");
    }
    auto center=colorState.wheelCenters[0];
    io.AddMousePosEvent(center.x,center.y); colorFrame();
    io.AddMouseButtonEvent(0,true); colorFrame();
    ++colorRevision; colorFrame();
    io.AddMouseButtonEvent(0,false); colorFrame();
    check(colorCancelled && colorCommits==3,"color gesture revision change cancels without host mutation");
    video::AudioStripView strip;
    strip.id=29; strip.gainId=1307; strip.panId=9011; strip.label="Mix";
    editor::PropertyState audioState;
    ImVec2 faderPoint{},panPoint{},mutePoint{};
    int audioCommits=0;
    auto audioFrame = [&] {
        full.Clear();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({780,580});
        ImGui::Begin("Audio channel");
        auto p=ImGui::GetCursorScreenPos();
        float top=p.y+ImGui::GetTextLineHeightWithSpacing();
        const auto &style=ImGui::GetStyle();
        float panX=p.x+32+style.ItemInnerSpacing.x+ImGui::CalcTextSize(strip.gainLabel).x+style.ItemSpacing.x;
        faderPoint={p.x+16,top+20};
        panPoint={panX+ImGui::GetFontSize()*8,
                  top+ImGui::GetFrameHeight()*.5f};
        mutePoint={panX+8,
                   top+ImGui::GetFrameHeightWithSpacing()+ImGui::GetFrameHeight()*.5f};
        video::AudioStrip(strip,1,audioState,full);
        ImGui::End(); ImGui::Render();
        for (const auto &event:full.Events()) {
            check(event.target==strip.gainId || event.target==strip.panId || event.target==strip.id,
                  "audio events use explicit property IDs rather than neighboring IDs");
            if (event.phase!=editor::Phase::Commit) continue;
            ++audioCommits;
            if (event.target==strip.gainId) strip.gain=event.proposed.x;
            if (event.target==strip.panId) strip.pan=event.proposed.x;
            if (event.target==strip.id && event.proposed.x==static_cast<double>(video::TrackControl::Mute))
                strip.mute=event.proposed.y!=0;
        }
    };
    audioFrame(); audioFrame();
    auto clickAudio=[&](ImVec2 p) {
        io.AddMousePosEvent(p.x,p.y); audioFrame();
        io.AddMouseButtonEvent(0,true); audioFrame();
        io.AddMouseButtonEvent(0,false); audioFrame();
    };
    clickAudio(faderPoint);
    check(strip.gain>2 && audioCommits==1,"audio fader host commit through public IO");
    clickAudio(panPoint);
    check(strip.pan>0 && audioCommits==2,"audio pan host commit through public IO");
    clickAudio(mutePoint);
    check(strip.mute && audioCommits==3,"audio mute track control event");
    strip.locked=true;
    clickAudio(faderPoint); clickAudio(mutePoint);
    check(audioCommits==3 && strip.mute,"locked audio controls emit no edits");
    ImGui::DestroyContext(context);
    return failures ? 1 : 0;
}
