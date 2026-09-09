#include <imkit/video.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace imkit;
int main() {
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
    check(full.count==1 && full.Events()[0].target==100 &&
              full.Events()[0].proposed.x==static_cast<double>(video::TrackControl::Expanded) &&
              full.Events()[0].proposed.y==0,"track collapse emits the explicit host control");
    layoutFixture.tracks[0].expanded=false;
    check(video::TrackExtent(layoutFixture.tracks[0])==32,"collapsed row has compact extent");
    layoutFixture.tracks[0].expanded=true;
    float sourceX=timeline.view.min.x+4;
    for (const char *label:{"V","M","S","L","R","T"})
        sourceX+=ImGui::CalcTextSize(label).x+ImGui::GetStyle().FramePadding.x*2+2;
    clickTrack({sourceX+5,timeline.view.min.y+35});
    check(full.count==1 && full.Events()[0].proposed.x==static_cast<double>(video::TrackControl::Source) &&
              full.Events()[0].proposed.y==1,"source patch button emits a distinct control");
    timeline.headerWidth=100;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    full.Clear();frame(full);
    clickTrack({timeline.view.min.x+15,timeline.view.min.y+35});
    full.Clear();
    io.AddKeyEvent(ImGuiKey_DownArrow,true);frame(full);
    io.AddKeyEvent(ImGuiKey_DownArrow,false);frame(full);
    io.AddKeyEvent(ImGuiKey_Enter,true);frame(full);
    io.AddKeyEvent(ImGuiKey_Enter,false);frame(full);
    check(full.count==1 && full.Events()[0].target==100 &&
        full.Events()[0].kind==editor::EditKind::Toggle,
        "narrow track control menu emits host event through public IO");
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
    struct TransitionFixture {video::TrackView track;video::ClipView clip;} transitionFixture;
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
        const float x=timeline.view.min.x+timeline.headerWidth+(side ? 250.f : 50.f);
        const float y=timeline.view.min.y+10;
        full.Clear();io.AddMousePosEvent(x,y);frame(full);frame(full);
        io.AddMouseButtonEvent(0,true);frame(full);
        check(timeline.transitionDrag.active && timeline.transitionEnd==(side==1),"transition handle begins the selected end");
        full.Clear();io.AddMousePosEvent(x+(side ? -50.f : 50.f),y);frame(full);
        check(timeline.transitionDrag.draft.proposed.first==editor::FromSeconds(side ? .5 : 1.) &&
              timeline.transitionDrag.draft.proposed.last==editor::FromSeconds(side ? 1. : .5),"transition handle changes only its duration");
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
