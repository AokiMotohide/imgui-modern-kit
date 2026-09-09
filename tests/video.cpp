#include <imkit/video.h>
#include <array>
#include <cmath>
#include <cstdio>
using namespace imkit;
int main() {
    int failures = 0;
    auto check = [&](bool ok, const char *s) {
        if (!ok) {
            ++failures;
            std::fprintf(stderr, "FAIL %s\n", s);
        }
    };
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
    ImGui::DestroyContext(context);
    return failures ? 1 : 0;
}
