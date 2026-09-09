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
    return failures ? 1 : 0;
}
