#include <algorithm>
#include <vector>
#include <imkit/video.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <cstdlib>
#include "../examples/gallery/allocation_probe.h"
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
    {
        std::array<video::AudioBucket,8> buckets{};buckets[3]={-.9f,.8f,.5f,.9f};
        video::WaveformView view{buckets,{0,80},video::WaveformStatus::Ready};
        check(video::WaveformPixel(view,{0,80}).minimum==-.9f,"downsample preserves transient");
        check(video::WaveformPixel(view,{30,31}).maximum==.8f,"zoom repeats envelope without holes");
        check(video::WaveformPixel(view,{40,80}).maximum==0,"trim maps to source interval");
        check(video::WaveformPixel(view,{-80,0}).maximum==0,"out of source is silent");
        view.status=video::WaveformStatus::Pending;
        check(video::WaveformPixel(view,{0,80}).peak==0,"pending waveform does not use stale buckets");
    }
    {
        std::array<editor::Keyframe,2> inverted{{{1,10,0,1},{2,10,editor::FromSeconds(1),0}}};
        for(auto &key:inverted) key.interpolation=editor::Interpolation::Linear;
        auto identity=video::ApplyColorCurves({.25f,.5f,.75f,.3f},{});
        check(identity.r==.25f && identity.g==.5f && identity.b==.75f && identity.a==.3f,"empty color curves preserve RGBA");
        auto mapped=video::ApplyColorCurves({.25f,.5f,.75f,.3f},{inverted,{},{}});
        check(std::abs(mapped.r-.75f)<1e-5f && mapped.g==.5f && mapped.b==.75f && mapped.a==.3f,"color curves evaluate channels independently and preserve alpha");
        auto bounded=video::ApplyColorCurves({-1,2,std::numeric_limits<float>::quiet_NaN(),1},{});
        check(bounded.r==0 && bounded.g==1 && bounded.b==0,"color curves bound nonfinite and out of range samples");
    }
    {
        video::ClipView removed;removed.id=1;removed.track=1;removed.start=10;removed.duration=5;
        video::ClipView survivor;survivor.id=2;survivor.track=1;survivor.start=20;survivor.duration=5;
        const std::array deleted{removed};
        check(video::RippleDeletePosition(survivor,deleted)==15,"ripple delete closes preceding removed interval");
        survivor.track=2;check(video::RippleDeletePosition(survivor,deleted)==20,"ripple delete preserves other tracks");
        survivor.track=1;survivor.start=12;check(!video::RippleDeletePosition(survivor,deleted),"ripple delete rejects overlapping survivor");
        survivor.start=20;std::array overlap{removed,removed};check(!video::RippleDeletePosition(survivor,overlap),"ripple delete rejects overlapping removed intervals");
    }
    video::TimelineState snapping;
    {
        video::ClipView left,right;left.track=right.track=1;
        left.duration=10;right.start=10;right.duration=10;right.sourceIn=3;
        video::ClipConstraints bounds{0,20,1};
        check(video::CenteredTransitionLimit(left,right,bounds,bounds)==6,"centered transition uses both source handles");
        left.speed=1.5;
        check(video::CenteredTransitionLimit(left,right,bounds,bounds)==6,"transition handles account for playback speed");
        right.sourceIn=0;
        check(video::CenteredTransitionLimit(left,right,bounds,bounds)==0,"transition cannot read before incoming media");
        right.sourceIn=3;right.start=11;
        check(video::CenteredTransitionLimit(left,right,bounds,bounds)==0,"transition rejects a gap");
        right.start=10;right.locked=true;
        check(video::CenteredTransitionLimit(left,right,bounds,bounds)==0,"transition rejects locked neighbor");
        right.locked=false;left.start=std::numeric_limits<editor::Tick>::max();
        check(video::CenteredTransitionLimit(left,right,bounds,bounds)==0,"transition rejects overflowing cut");
    }
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
    auto extreme=c;extreme.start=std::numeric_limits<editor::Tick>::max()-100;
    check(!video::EditClip(extreme,editor::EditKind::Move,100,bounds).valid,"move rejects overflowing timeline end");
    extreme=c;extreme.speed=1e-20;
    check(video::EditClip(extreme,editor::EditKind::Move,10,bounds).valid,"slow clip saturates unused source handles safely");
    extreme=c;extreme.sourceIn=std::numeric_limits<editor::Tick>::min()+100;
    check(video::EditClip(extreme,editor::EditKind::Slip,10,
          {std::numeric_limits<editor::Tick>::min(),std::numeric_limits<editor::Tick>::max(),1}).sourceIn==extreme.sourceIn+10,
          "wide signed media bounds preserve exact unit-speed source movement");
    video::ClipView exact;exact.duration=1;
    const auto tickMax=std::numeric_limits<editor::Tick>::max();
    const auto exactSlip=video::EditClip(exact,editor::EditKind::Slip,tickMax-2,{0,tickMax,1});
    check(exactSlip.valid && exactSlip.sourceIn==tickMax-2,"unit-speed Slip preserves extreme integer offset exactly");
    exact.duration=tickMax;
    const auto exactSplit=video::SplitClip(exact,tickMax-2,{0,tickMax,1});
    check(exactSplit.valid && exactSplit.right.sourceIn==tickMax-2 && exactSplit.right.duration==2,
          "unit-speed Split preserves cut within two ticks of integer limit");
    check(!video::SplitClip(exact,100,{0,tickMax-1,1}).valid &&
          !video::EditClip(exact,editor::EditKind::Move,0,{0,tickMax-1,1}).valid,
          "unit-speed source bounds reject one tick beyond media without floating rounding");
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
    {
        video::ClipView a,b,d;
        a.start=tickMax-20;a.duration=10;b.start=tickMax-10;b.duration=10;b.sourceIn=10;
        const video::ClipConstraints media{0,40,1};
        const auto nearLimit=video::RollClips(a,b,-5,media,media);
        check(nearLimit.valid && nearLimit.left.duration==5 && nearLimit.right.start==tickMax-15,
              "Roll retains exact adjacent cuts near Tick maximum");
        a.duration=30;b.start=std::numeric_limits<editor::Tick>::min()+9;
        check(!video::RollClips(a,b,0,media,media).valid,"Roll rejects overflowing adjacent cut");
        a.start=tickMax-20;a.duration=10;b.start=tickMax-10;b.duration=20;
        d.start=std::numeric_limits<editor::Tick>::min()+9;d.duration=10;
        check(!video::SlideClip(a,b,d,0,media,media,media).valid,"Slide rejects overflowing middle cut");
        a.start=0;b.start=10;b.duration=10;d.start=20;b.track=1;
        check(!video::RollClips(a,b,0,media,media).valid && !video::SlideClip(a,b,d,0,media,media,media).valid,
              "Roll and Slide reject adjacent positions on different tracks");
        b.track=0;a.duration=std::numeric_limits<editor::Tick>::min();
        check(!video::RollClips(a,b,0,media,media).valid && !video::SlideClip(a,b,d,0,media,media,media).valid,
              "Roll and Slide reject negative durations before boundary arithmetic");
    }
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
    timeline.options.minimumPixelsPerSecond=20;timeline.options.maximumPixelsPerSecond=40;
    timeline.canvas.scale.x=1;frame(full);
    check(timeline.canvas.scale.x==20,"timeline zoom clamps to the configured minimum");
    timeline.canvas.scale.x=100;frame(full);
    check(timeline.canvas.scale.x==40,"timeline zoom clamps to the configured maximum");
    timeline.options.minimumPixelsPerSecond=8;timeline.options.maximumPixelsPerSecond=640;
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
    struct TransitionFixture {video::TrackView track;video::ClipView clip;std::array<video::ClipView,2> related;std::array<video::ClipView,4> neighbors;} transitionFixture;
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
    provider.transitionLimit=[](void *,editor::StableId,bool){return editor::FromSeconds(.8);};
    {
        const float x=timeline.view.min.x+timeline.headerWidth+25,y=timeline.view.min.y+10;
        full.Clear();io.AddMousePosEvent(x,y);frame(full);frame(full);
        io.AddMouseButtonEvent(0,true);frame(full);
        check(timeline.transitionDrag.active,"centered transition begins at half-duration handle");
        full.Clear();io.AddMousePosEvent(x+50,y);frame(full);
        check(timeline.transitionDrag.draft.proposed.first==editor::FromSeconds(.8),"centered transition clamps to host media handles");
        full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
        check(!timeline.transitionDrag.active && full.count==1 && full.Events()[0].phase==editor::Phase::Commit,
              "centered transition commits constrained duration");
    }
    provider.transitionLimit=nullptr;
    // New fade provider suppresses legacy handles and starts from zero duration.
    provider.editing.user=&transitionFixture;
    provider.editing.fades=[](void *u,editor::StableId)->const video::FadeView * {
        return &static_cast<TransitionFixture*>(u)->clip.fades;
    };
    for(int side=0;side<2;++side) {
        frame(full);frame(full);
        const float x=timeline.view.min.x+timeline.headerWidth+(side ? 295.f : 5.f);
        const float y=timeline.view.min.y+11;
        full.Clear();io.AddMousePosEvent(x,y);frame(full);frame(full);
        io.AddMouseButtonEvent(0,true);frame(full);
        check(timeline.fadeDrag.active && !timeline.drag.active && !timeline.transitionDrag.active,"zero fade handle owns pointer before trim");
        full.Clear();io.AddMousePosEvent(x+(side ? -50 : 50),y);frame(full);
        check((side ? timeline.fadeDrag.draft.proposed.last : timeline.fadeDrag.draft.proposed.first)==editor::FromSeconds(.5),"fade drag previews selected end");
        full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
        check(!timeline.fadeDrag.active && full.count==1 && full.Events()[0].kind==editor::EditKind::ClipFades && full.Events()[0].phase==editor::Phase::Commit,"fade commits once without mutating provider");
        check(transitionFixture.clip.fades.inDuration==0 && transitionFixture.clip.fades.outDuration==0,"fade data remains host-owned");
    }
    check(video::EvaluateFade(25,100,{50,0,video::FadeCurve::Linear})==.5,"linear fade evaluation");
    check(video::EvaluateFade(25,100,{50,0,video::FadeCurve::EaseIn})==.25,"ease-in fade evaluation");
    check(video::EvaluateFade(25,100,{50,0,video::FadeCurve::EaseOut})==.75,"ease-out fade evaluation");
    check(video::EvaluateFade(100,100,{0,50})==0 && video::EvaluateFade(0,0,{})==0,"fade-out endpoint and empty clip");
    provider.editing={};
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
    timeline.options.visibleTools=video::TimelineToolBit(video::Tool::Select)|video::TimelineToolBit(video::Tool::Hand);
    timeline.tool=video::Tool::Ripple;frame(full);
    check(timeline.tool==video::Tool::Select,"hidden active tool falls back to the first visible safe tool");
    timeline.bindings=toolBindings;io.AddKeyEvent(ImGuiKey_F3,true);frame(full);io.AddKeyEvent(ImGuiKey_F3,false);frame(full);
    check(timeline.tool==video::Tool::Select,"hidden tool ignores remapped keyboard command");
    timeline.options.visibleTools=0x7f;timeline.bindings={};
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
    selection.storage[1]=902;
    for (const auto command:{editor::Command::Delete,editor::Command::Duplicate}) {
        splitBindings[0].command=command;
        splitCommand(full);
        check(full.count==4 && full.Events()[1].phase==editor::Phase::Commit &&
              full.Events()[3].target==902 && full.Events()[1].kind==
              (command==editor::Command::Delete ? editor::EditKind::Remove : editor::EditKind::Duplicate),
              "remapped clip command includes offscreen selection");
        splitClips[1].locked=true;splitCommand(full);
        check(full.count==0,"locked clip rejects complete keyboard edit");
        splitClips[1].locked=false;splitCommand(small);
        check(small.overflow && small.count==0,"clip keyboard edit reserves complete batch");
    }
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
    selection.Set(transitionFixture.clip.id);selection.Set(902,true);
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+50,clipOrigin.y+25);frame(full);
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(timeline.drag.active && timeline.memberCount==1 && selection.count==2,
          "selected clip body starts complete multi-move without collapsing selection");
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+100,clipOrigin.y+25);frame(full);
    check(timeline.drag.draft.proposed.first-timeline.drag.draft.original.first==
          trimMembers[0].transaction.draft.proposed.first-trimMembers[0].transaction.draft.original.first &&
          timeline.drag.draft.proposed.first>timeline.drag.draft.original.first,
          "multi-clip move preserves spacing including offscreen companion");
    io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+10,clipOrigin.y+25);frame(full);
    check(timeline.drag.draft.proposed.first==0 && trimMembers[0].transaction.draft.proposed.first==0,
          "multi-clip move clamps whole selection at timeline start");
    io.AddKeyEvent(ImGuiKey_Escape,true);frame(full);
    io.AddKeyEvent(ImGuiKey_Escape,false);io.AddMouseButtonEvent(0,false);frame(full);
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
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    provider.isEditable=[](void *,editor::StableId id){return id!=902;};
    full.Clear();frame(full);
    check(full.count==2 && full.Events()[0].phase==editor::Phase::Cancel && full.Events()[1].phase==editor::Phase::Cancel &&
          !timeline.drag.active && !trimMembers[0].transaction.active,
          "indexed offscreen owner invalidation cancels the complete clip edit batch");
    io.AddMouseButtonEvent(0,false);frame(full);provider.isEditable=nullptr;
    transitionFixture.related[1].sourceIn=0;transitionFixture.related[1].track=904;
    for (int i=0;i<4;++i) {
        auto &n=transitionFixture.neighbors[i];n=transitionFixture.clip;n.id=910+i;
        n.track=i<2 ? 900 : 904;n.duration=editor::FromSeconds(i==3 ? .3 : 3);
        n.start=i%2==0 ? -n.duration : (i==1 ? transitionFixture.clip.duration : transitionFixture.related[1].duration);
        n.transitionIn=n.transitionOut=0;
    }
    provider.neighbors=[](void *u,editor::StableId id) {
        auto &f=*static_cast<TransitionFixture*>(u);const int base=id==f.clip.id ? 0 : 2;
        return video::TimelineProvider::Neighbors{&f.neighbors[base],&f.neighbors[base+1]};
    };
    for (auto tool : {video::Tool::Roll,video::Tool::Slide}) {
        timeline.tool=tool;full.Clear();
        io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+100,clipOrigin.y+25);frame(full);
        io.AddMouseButtonEvent(0,true);frame(full);
        check(timeline.drag.active && trimMembers[0].nextTransaction.active &&
              (tool!=video::Tool::Slide || trimMembers[0].previousTransaction.active),"related adjacent tool begins neighbor transactions");
        full.Clear();
        io.AddMousePosEvent(clipOrigin.x+timeline.headerWidth+150,clipOrigin.y+25);frame(full);
        check(trimMembers[0].nextTransaction.draft.proposed.first==transitionFixture.neighbors[3].start+editor::FromSeconds(.2),
              "related adjacent tool uses tightest neighbor duration");
        full.Clear();io.AddMouseButtonEvent(0,false);frame(full);
        check(full.count==(tool==video::Tool::Roll ? 4 : 6) &&
              std::all_of(full.Events().begin(),full.Events().end(),[](const auto &e){return e.phase==editor::Phase::Commit;}),
              "related Roll or Slide commits all participating clips together");
    }
    provider.neighbors=nullptr;
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
    io.AddMouseButtonEvent(0,false);frame(full);
    transitionFixture.related={transitionFixture.clip,transitionFixture.clip};transitionFixture.related[1].id=902;
    provider.selected=[](void *u,std::span<const editor::StableId>){return std::span<const video::ClipView>(static_cast<TransitionFixture*>(u)->related);};
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(full.count==4 && full.Events()[1].kind==editor::EditKind::Split && full.Events()[3].target==902 &&
          full.Events()[1].proposed.first==full.Events()[3].proposed.first,
          "Razor splits related offscreen clip at same timeline cut");
    io.AddMouseButtonEvent(0,false);frame(full);transitionFixture.related[1].locked=true;
    full.Clear();io.AddMouseButtonEvent(0,true);frame(full);
    check(full.count==0,"Razor rejects entire split when related clip is locked");
    io.AddMouseButtonEvent(0,false);frame(full);provider.selected=nullptr;
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
    {
        video::MonitorOptions options;
        ImVec2 controlsOrigin{};
        auto controlsFrame=[&] {
            ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({450,350});
            ImGui::Begin("Monitor controls");controlsOrigin=ImGui::GetCursorScreenPos();
            video::MonitorControls("display",options);ImGui::End();ImGui::Render();
        };
        controlsFrame();controlsFrame();
        auto toggle=[&](int row) {
            io.AddMousePosEvent(controlsOrigin.x+8,controlsOrigin.y+row*ImGui::GetFrameHeightWithSpacing()+8);
            controlsFrame();io.AddMouseButtonEvent(0,true);controlsFrame();io.AddMouseButtonEvent(0,false);controlsFrame();
        };
        toggle(0);check(!options.safeArea,"Monitor controls change host safe-area option");
        toggle(3);check(options.transform && !options.showAnchor.has_value(),"Monitor transform retains legacy anchor default");
        toggle(4);check(options.showAnchor.has_value() && !*options.showAnchor,"Monitor anchor can be hidden independently");
        toggle(3);toggle(4);
        check(!options.transform && options.showAnchor.value_or(false),"Monitor anchor can remain visible without transform bounds");
    }
    {
        const auto theme=MakePrecisionTheme(ColorScheme::Dark);
        const ImTextureID textureId=static_cast<ImTextureID>(0x1234);
        video::MonitorView monitor{editor::ImageView{ImTextureRef(textureId),{1920,1080},{.2f,.1f},{.8f,.9f}}};
        video::MonitorOptions options;options.safeArea=true;options.guides=false;options.showTimecode=false;
        options.metadataPreset=video::MonitorMetadataPreset::Off;options.transform=false;options.showAnchor=false;
        struct Capture {
            bool action=false;
            ImVec2 itemMin{},itemMax{};
            float imageMinX=std::numeric_limits<float>::infinity(),imageMinY=std::numeric_limits<float>::infinity();
            float imageMaxX=-std::numeric_limits<float>::infinity(),imageMaxY=-std::numeric_limits<float>::infinity();
            float uvMinX=std::numeric_limits<float>::infinity(),uvMinY=std::numeric_limits<float>::infinity();
            float uvMaxX=-std::numeric_limits<float>::infinity(),uvMaxY=-std::numeric_limits<float>::infinity();
            std::array<ImVec2,128> overlay{};std::size_t overlayCount=0;
        };
        ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({500,400});ImGui::Begin("Legacy monitor");
        const ImVec2 legacyMin=ImGui::GetCursorScreenPos();
        video::Monitor("legacy",ImTextureRef(textureId),{400,300},{},options,theme);
        const ImVec2 legacyMax=ImGui::GetItemRectMax();ImGui::End();ImGui::Render();
        ImVec2 legacyImageMin{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        ImVec2 legacyImageMax{-std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()};
        ImVec2 legacyUvMin{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
        ImVec2 legacyUvMax{-std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()};
        const auto *legacyDrawData=ImGui::GetDrawData();
        for(int listIndex=0;listIndex<legacyDrawData->CmdListsCount;++listIndex) {
            const ImDrawList *list=legacyDrawData->CmdLists[listIndex];
            for(const auto &command:list->CmdBuffer) if(!command.TexRef._TexData&&command.TexRef._TexID==textureId)
                for(unsigned i=0;i<command.ElemCount;++i) {
                    const auto &vertex=list->VtxBuffer[list->IdxBuffer[command.IdxOffset+i]+command.VtxOffset];
                    legacyImageMin={std::min(legacyImageMin.x,vertex.pos.x),std::min(legacyImageMin.y,vertex.pos.y)};
                    legacyImageMax={std::max(legacyImageMax.x,vertex.pos.x),std::max(legacyImageMax.y,vertex.pos.y)};
                    legacyUvMin={std::min(legacyUvMin.x,vertex.uv.x),std::min(legacyUvMin.y,vertex.uv.y)};
                    legacyUvMax={std::max(legacyUvMax.x,vertex.uv.x),std::max(legacyUvMax.y,vertex.uv.y)};
                }
        }
        check(std::abs(legacyMax.x-legacyMin.x-400)<1e-4f&&std::abs(legacyMax.y-legacyMin.y-300)<1e-4f,
              "legacy Monitor retains full-region stretch item");
        check(legacyImageMin.x==legacyMin.x&&legacyImageMin.y==legacyMin.y&&legacyImageMax.x==legacyMax.x&&legacyImageMax.y==legacyMax.y&&
              legacyUvMin.x==0&&legacyUvMin.y==0&&legacyUvMax.x==1&&legacyUvMax.y==1,
              "legacy Monitor retains full-region texture and UVs");
        const auto drawFrame=[&](video::MonitorView &view,const video::MonitorOptions &drawOptions,ComponentOptions components=ComponentOptions{}) {
            ImGui::NewFrame();if(components.accessibility) components.accessibility->Begin(ImGui::GetFrameCount());
            ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({500,400});
            ImGui::Begin("Aspect monitor");
            Capture capture;capture.itemMin=ImGui::GetCursorScreenPos();capture.itemMax={capture.itemMin.x+400,capture.itemMin.y+300};
            capture.action=video::Monitor("preview",view,{400,300},{},drawOptions,theme,components);
            ImGui::End();ImGui::Render();
            const ImU32 muted=ImGui::GetColorU32(theme.colors.muted);
            const auto *drawData=ImGui::GetDrawData();
            for(int listIndex=0;listIndex<drawData->CmdListsCount;++listIndex) {
                const ImDrawList *list=drawData->CmdLists[listIndex];
                for(const auto &vertex:list->VtxBuffer)
                    if(vertex.col==muted&&capture.overlayCount<capture.overlay.size()) capture.overlay[capture.overlayCount++]=vertex.pos;
                for(const auto &command:list->CmdBuffer) if(!command.TexRef._TexData&&command.TexRef._TexID==textureId)
                    for(unsigned i=0;i<command.ElemCount;++i) {
                        const auto index=list->IdxBuffer[command.IdxOffset+i]+command.VtxOffset;
                        const auto &vertex=list->VtxBuffer[index];
                        capture.imageMinX=std::min(capture.imageMinX,vertex.pos.x);capture.imageMaxX=std::max(capture.imageMaxX,vertex.pos.x);
                        capture.imageMinY=std::min(capture.imageMinY,vertex.pos.y);capture.imageMaxY=std::max(capture.imageMaxY,vertex.pos.y);
                        capture.uvMinX=std::min(capture.uvMinX,vertex.uv.x);capture.uvMaxX=std::max(capture.uvMaxX,vertex.uv.x);
                        capture.uvMinY=std::min(capture.uvMinY,vertex.uv.y);capture.uvMaxY=std::max(capture.uvMaxY,vertex.uv.y);
                    }
            }
            return capture;
        };
        monitor.placement=editor::ImagePlacementMode::Fit;
        auto fit=drawFrame(monitor,options);
        check(std::abs((fit.imageMaxX-fit.imageMinX)-400)<1e-4f&&std::abs((fit.imageMaxY-fit.imageMinY)-225)<1e-4f&&
              std::abs((fit.imageMinY-fit.itemMin.y)-37.5f)<1e-4f,"Monitor Fit uses centered image rectangle");
        monitor.placement=editor::ImagePlacementMode::Fill;
        auto fill=drawFrame(monitor,options);
        check(std::abs(fill.uvMinX-.275f)<1e-5f&&std::abs(fill.uvMaxX-.725f)<1e-5f&&
              std::abs(fill.imageMaxX-fill.imageMinX-400)<1e-4f,"Monitor Fill composes centered crop with source UV");
        auto flippedOptions=options;flippedOptions.flipY=true;
        auto flipped=drawFrame(monitor,flippedOptions);
        check(std::abs(flipped.uvMinY-.1f)<1e-5f&&std::abs(flipped.uvMaxY-.9f)<1e-5f&&
              fill.overlayCount==flipped.overlayCount&&std::equal(fill.overlay.begin(),fill.overlay.begin()+fill.overlayCount,flipped.overlay.begin(),
                  [](ImVec2 a,ImVec2 b){return a.x==b.x&&a.y==b.y;}),"flipY changes texture UV without changing logical overlays");
        monitor.image.pixels={};auto invalid=drawFrame(monitor,options);
        check(!std::isfinite(invalid.imageMinX),"invalid ready Monitor draws neutral canvas without texture");
        monitor.image.pixels={1920,1080};
        for(auto status:{editor::PreviewState::Loading,editor::PreviewState::Empty,editor::PreviewState::Offline,editor::PreviewState::Error}) {
            monitor.status=status;monitor.stateView={"State","Host-owned preview state",status==editor::PreviewState::Error?"Retry":""};
            auto state=drawFrame(monitor,options);check(!std::isfinite(state.imageMinX),"non-ready Monitor does not draw stale texture");
        }
        monitor.stateView={"","","Retry",FeedbackKind::Error};
        std::array<accessibility::SemanticNode,8> stateNodes{};std::array<accessibility::ActionRequest,2> stateActions{};
        accessibility::ActionQueue stateQueue(stateActions);accessibility::AccessibilityFrame stateSemantics(stateNodes,&stateQueue);
        ComponentOptions stateComponents{&theme,nullptr,&stateSemantics};
        auto retry=drawFrame(monitor,options,stateComponents);accessibility::StableId retryId=0;
        for(const auto &node:stateSemantics.Tree().nodes)
            if(node.role==accessibility::SemanticRole::Button&&node.name=="Retry") retryId=node.id;
        check(retryId!=0&&stateQueue.Push({retryId,accessibility::SemanticAction::Press}),"Monitor exposes RetryState semantic action");
        retry=drawFrame(monitor,options,stateComponents);
        check(retry.action,"Monitor returns RetryState action request");
        monitor.status=editor::PreviewState::Ready;
        for(int i=0;i<4;++i) drawFrame(monitor,options);
        gallery::CountAllocations(true);drawFrame(monitor,options);gallery::CountAllocations(false);
        check(gallery::AllocationCount()==0,"steady Monitor frame performs no C++ heap allocation");
    }
    {
        struct EditingFixture {
            std::array<video::TrackView,2> tracks{{{400,"Video A"},{401,"Video B"}}};
            video::ClipView clip;
            std::array<editor::StableId,1> boxIds{410};
            int overlayCalls=0,dropPreviews=0,dropDeliveries=0,dropValue=0,rangePreviews=0;
            editor::StableId dropTrack=0;
            editor::StableId moveDestination=0;
            editor::Tick dropTick=0,previewFirst=0,previewLast=0;
            bool previewValid=true;
        } fixture;
        fixture.clip.id=410;fixture.clip.track=400;fixture.clip.label="Editable";fixture.clip.duration=editor::FromSeconds(3);
        video::TimelineProvider p;p.user=&fixture;p.revision=1;p.trackCount=2;
        p.tracks=[](void *u,int,int){return std::span<const video::TrackView>(static_cast<EditingFixture*>(u)->tracks);};
        p.clips=[](void *u,editor::StableId track,editor::Range){auto &f=*static_cast<EditingFixture*>(u);return track==400 ? std::span<const video::ClipView>(&f.clip,1) : std::span<const video::ClipView>{};};
        p.editing.user=&fixture;
        p.editing.box=[](void *u,editor::Range,double,double){return std::span<const editor::StableId>(static_cast<EditingFixture*>(u)->boxIds);};
        p.editing.destination=[](void *u,editor::StableId,editor::StableId,editor::StableId hovered){
            static_cast<EditingFixture*>(u)->moveDestination=hovered;return hovered;
        };
        p.editing.canMove=[](void *,std::span<const editor::StableId>,editor::Tick,editor::StableId,editor::StableId){return true;};
        p.editing.cut=[](void *,editor::StableId){return video::CutTransitionView{410,411,0,editor::FromSeconds(1),video::TransitionKind::Dissolve};};
        p.drawClipOverlay=[](void *u,editor::StableId,const editor::Value&,ImVec2,ImVec2){++static_cast<EditingFixture*>(u)->overlayCalls;};
        std::array routes{video::TimelineExternalDropRoute{
            "IMKIT_TEST_ASSET",&fixture,
            [](void *,editor::StableId,editor::Tick,const void *data,std::size_t size){return size==sizeof(int) && *static_cast<const int*>(data)==42;},
            [](void *u,editor::StableId track,editor::Tick at,const void *data,std::size_t,bool delivery){
                auto &f=*static_cast<EditingFixture*>(u);++f.dropPreviews;f.dropTrack=track;f.dropTick=at;f.dropValue=*static_cast<const int*>(data);
                if(delivery)++f.dropDeliveries;
            },
            [](void *u,editor::StableId,editor::Tick at,const void *,std::size_t){
                auto &f=*static_cast<EditingFixture*>(u);++f.rangePreviews;
                f.previewFirst=at;f.previewLast=at+editor::FromSeconds(3);
                return video::TimelineExternalDropPreview{{f.previewFirst,f.previewLast},video::TrackKind::Effect,"Three seconds",f.previewValid};
            }}};
        p.externalDrops=routes;
        std::array<editor::StableId,8> clipIds{},trackIds{};
        editor::Selection clipsSelected{clipIds},tracksSelected{trackIds};video::TimelineState state;state.trackSelection=&tracksSelected;
        std::array<editor::Event,16> eventData{};editor::EventBuffer output{eventData};
        ImVec2 shelfOrigin{},assetOrigin{};
        auto render=[&] {
            ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({900,700});
            ImGui::Begin("Extended timeline IO");shelfOrigin=ImGui::GetCursorScreenPos();video::TransitionShelf("external shelf");
            assetOrigin=ImGui::GetCursorScreenPos();ImGui::Button("Asset",{80,24});
            if(ImGui::BeginDragDropSource()) {const int value=42;ImGui::SetDragDropPayload("IMKIT_TEST_ASSET",&value,sizeof(value));ImGui::TextUnformatted("Asset");ImGui::EndDragDropSource();}
            video::Timeline("editing",p,state,clipsSelected,output,MakePrecisionTheme(),{700,300});
            ImGui::End();ImGui::Render();
        };
        auto move=[&](float x,float y){io.AddMousePosEvent(x,y);render();render();};
        render();render();
        const auto origin=state.view.min;
        check(fixture.overlayCalls>0,"host clip overlay is invoked without replacing timeline interaction");
        std::array allBindings{editor::Binding{editor::Command::SelectAll,ImGuiKey_F8}};
        state.bindings=allBindings;
        move(origin.x+state.headerWidth+450,origin.y+140);
        io.AddMouseButtonEvent(0,true);render();io.AddMouseButtonEvent(0,false);render();
        clipsSelected.Clear();
        io.AddKeyEvent(ImGuiKey_F8,true);render();io.AddKeyEvent(ImGuiKey_F8,false);render();
        check(clipsSelected.Contains(410),"remapped Select All uses complete host selection query");
        state.bindings={};clipsSelected.Clear();
        move(origin.x+state.headerWidth+400,origin.y+50);io.AddMouseButtonEvent(0,true);render();
        move(origin.x+state.headerWidth+100,origin.y+10);io.AddMouseButtonEvent(0,false);render();
        check(clipsSelected.Contains(410) && !state.boxSelecting,"public IO box selects an intersecting clip");
        check(state.view.min.x==origin.x && state.view.min.y==origin.y,"box drag does not move host window");
        io.AddKeyEvent(ImGuiMod_Ctrl,true);render();
        move(origin.x+state.headerWidth+400,origin.y+50);io.AddMouseButtonEvent(0,true);render();
        move(origin.x+state.headerWidth+100,origin.y+10);io.AddMouseButtonEvent(0,false);render();
        check(!clipsSelected.Contains(410),"Ctrl rectangle toggles existing selection");
        io.AddKeyEvent(ImGuiMod_Ctrl,false);render();
        move(origin.x+55,origin.y+9);io.AddMouseButtonEvent(0,true);render();io.AddMouseButtonEvent(0,false);render();
        check(tracksSelected.Contains(400) && clipsSelected.count==0,"track selection is independent of clips");
        output.Clear();move(origin.x+55,origin.y+9);io.AddMouseButtonEvent(0,true);render();
        move(origin.x+55,origin.y+75);io.AddMouseButtonEvent(0,false);render();
        check(std::any_of(output.Events().begin(),output.Events().end(),[](const auto &e){return e.phase==editor::Phase::Commit && e.kind==editor::EditKind::TrackEdit && e.proposed.offset==static_cast<int>(video::TrackAction::Reorder) && e.proposed.parent==401;}),"track drag emits reorder request");
        clipsSelected.Clear();output.Clear();
        move(origin.x+state.headerWidth+30,origin.y+20);io.AddMouseButtonEvent(0,true);render();
        move(origin.x+state.headerWidth+90,origin.y+state.rowHeight+20);render();
        io.AddMouseButtonEvent(0,false);render();
        check(fixture.moveDestination==401 && std::any_of(output.Events().begin(),output.Events().end(),[](const auto &e){
                  return e.phase==editor::Phase::Commit && e.kind==editor::EditKind::Move && e.proposed.parent==401;
              }),"clip drag resolves the destination track from its rectangle while active");
        output.Clear();move(shelfOrigin.x+25,shelfOrigin.y+10);io.AddMouseButtonEvent(0,true);render();
        move(origin.x+state.headerWidth+300,origin.y+29);io.AddMouseButtonEvent(0,false);render();
        check(std::any_of(output.Events().begin(),output.Events().end(),[](const auto &e){return e.phase==editor::Phase::Commit && e.kind==editor::EditKind::CutTransition && e.proposed.parent==411 && e.proposed.first==editor::TicksPerSecond;}),"shelf drag inserts shared transition at cut");
        move(assetOrigin.x+20,assetOrigin.y+10);io.AddMouseButtonEvent(0,true);render();
        move(origin.x+state.headerWidth+350,origin.y+state.rowHeight+20);render();
        io.AddMouseButtonEvent(0,false);render();
        check(fixture.dropPreviews>0 && fixture.dropDeliveries==1 && fixture.dropTrack==401 && fixture.dropValue==42,
              "external host payload previews and delivers once on the hovered track");
        check(fixture.rangePreviews>0 && fixture.previewFirst==fixture.dropTick &&
              fixture.previewLast-fixture.previewFirst==editor::FromSeconds(3),
              "external drop range preview uses the exact host-owned candidate duration");
        fixture.previewValid=false;
        move(assetOrigin.x+20,assetOrigin.y+10);io.AddMouseButtonEvent(0,true);render();
        move(origin.x+state.headerWidth+350,origin.y+state.rowHeight+20);render();
        io.AddMouseButtonEvent(0,false);render();
        check(fixture.dropDeliveries==1,"invalid exact-range preview rejects delivery");
    }
    ImGui::DestroyContext(context);
    return failures ? 1 : 0;
}
