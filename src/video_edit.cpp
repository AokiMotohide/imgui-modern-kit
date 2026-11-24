#include "video_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
namespace imkit::video {
double EvaluateEnvelope(std::span<const EnvelopePoint> points,Tick tick) {
    if (points.empty()) return 1;
    auto next=std::upper_bound(points.begin(),points.end(),tick,[](auto t,const auto &point){return t<point.tick;});
    if (next==points.begin()) return next->gain;
    if (next==points.end()) return points.back().gain;
    const auto &previous=*(next-1);
    const double f=(double(tick)-double(previous.tick))/(double(next->tick)-double(previous.tick));
    return previous.gain+(next->gain-previous.gain)*f;
}
TransitionEdit EditTransition(const ClipView &clip, bool end, Tick delta) {
    if (clip.locked || clip.duration<=0 || clip.transitionIn<0 || clip.transitionOut<0 ||
        clip.transitionIn>clip.duration || clip.transitionOut>clip.duration-clip.transitionIn) return {};
    TransitionEdit result{clip.transitionIn,clip.transitionOut,true};
    auto &duration=end ? result.outDuration : result.inDuration;
    const Tick other=end ? result.inDuration : result.outDuration;
    duration+=std::clamp(delta,-duration,clip.duration-other-duration);
    return result;
}
namespace {
bool AddTick(Tick a,Tick b,Tick &out) {
    if ((b>0 && a>std::numeric_limits<Tick>::max()-b) ||
        (b<0 && a<std::numeric_limits<Tick>::min()-b)) return false;
    out=a+b;return true;
}
} // namespace
namespace impl {
Tick AvailableTicks(std::uint64_t sourceTicks,double speed) {
    if (speed==1) return static_cast<Tick>(std::min(sourceTicks,static_cast<std::uint64_t>(std::numeric_limits<Tick>::max())));
    const auto value=std::floor(static_cast<long double>(sourceTicks)/speed);
    return value>=std::ldexp(1.L,63) ? std::numeric_limits<Tick>::max() : static_cast<Tick>(value);
}
} // namespace impl
using impl::AvailableTicks;
float TrackExtent(const TrackView &track) {
    return track.expanded ? (std::max)(64.f,std::isfinite(track.height) ? track.height : 64.f) : 32.f;
}
Tick CenteredTransitionLimit(const ClipView &left,const ClipView &right,
                             ClipConstraints leftBounds,ClipConstraints rightBounds) {
    if (left.track!=right.track || !EditClip(left,editor::EditKind::Move,0,leftBounds).valid ||
        !EditClip(right,editor::EditKind::Move,0,rightBounds).valid ||
        left.start+left.duration!=right.start) return 0;
    const Tick post=AvailableTicks(static_cast<std::uint64_t>(leftBounds.mediaLast)-
                                  static_cast<std::uint64_t>(left.sourceIn),left.speed)-left.duration;
    const Tick pre=AvailableTicks(static_cast<std::uint64_t>(right.sourceIn)-
                                 static_cast<std::uint64_t>(rightBounds.mediaFirst),right.speed);
    return 2*std::min({post,pre,left.duration,right.duration,std::numeric_limits<Tick>::max()/2});
}
std::optional<Tick> RippleDeletePosition(const ClipView &survivor,std::span<const ClipView> removed) {
    const auto add=[](Tick a,Tick b)->std::optional<Tick> {Tick value;return AddTick(a,b,value)?std::optional<Tick>(value):std::nullopt;};
    if(survivor.duration<=0) return std::nullopt;
    const auto survivorEnd=add(survivor.start,survivor.duration);if(!survivorEnd) return std::nullopt;
    Tick shift=0,previousEnd=0;StableId previousTrack=0;bool first=true;
    for(const auto &clip:removed) {
        const auto end=add(clip.start,clip.duration);
        if(clip.duration<=0 || !end || (!first && (clip.track<previousTrack || (clip.track==previousTrack && clip.start<previousEnd)))) return std::nullopt;
        first=false;previousTrack=clip.track;previousEnd=*end;
        if(clip.track!=survivor.track) continue;
        if(clip.start<*survivorEnd && *end>survivor.start) return std::nullopt;
        if(*end<=survivor.start) {auto sum=add(shift,clip.duration);if(!sum) return std::nullopt;shift=*sum;}
    }
    return add(survivor.start,-shift);
}
ClipEdit EditClip(const ClipView &c, editor::EditKind kind, Tick delta, ClipConstraints bounds) {
    ClipEdit result{c.start, c.duration, c.sourceIn, false};
    if (c.locked || bounds.minimumDuration<1 || c.duration < bounds.minimumDuration || c.speed <= 0 || !std::isfinite(c.speed) ||
        bounds.mediaLast < bounds.mediaFirst)
        return result;
    if (c.sourceIn < bounds.mediaFirst || c.sourceIn > bounds.mediaLast ||
        (c.speed==1 ? static_cast<std::uint64_t>(c.duration)>static_cast<std::uint64_t>(bounds.mediaLast)-static_cast<std::uint64_t>(c.sourceIn) :
        static_cast<long double>(c.duration)*c.speed > static_cast<long double>(static_cast<std::uint64_t>(bounds.mediaLast)-static_cast<std::uint64_t>(c.sourceIn))) ||
        c.start>std::numeric_limits<Tick>::max()-c.duration)
        return result;
    Tick before=AvailableTicks(static_cast<std::uint64_t>(c.sourceIn)-static_cast<std::uint64_t>(bounds.mediaFirst),c.speed);
    Tick after=AvailableTicks(static_cast<std::uint64_t>(bounds.mediaLast)-static_cast<std::uint64_t>(c.sourceIn),c.speed)-c.duration;
    const auto advanceSource=[&](Tick amount) {
        if (c.speed==1) return AddTick(c.sourceIn,amount,result.sourceIn);
        const auto mapped=std::round(static_cast<long double>(amount)*c.speed);
        if (!std::isfinite(mapped) || mapped < -std::ldexp(1.L,63) || mapped >= std::ldexp(1.L,63)) return false;
        return AddTick(c.sourceIn,static_cast<Tick>(mapped),result.sourceIn);
    };
    switch (kind) {
    case editor::EditKind::Move:
    case editor::EditKind::Duplicate:
        if (!AddTick(c.start,delta,result.start)) return result;
        break;
    case editor::EditKind::TrimStart:
        delta = std::clamp(delta, -std::min(before,std::numeric_limits<Tick>::max()-c.duration), c.duration - bounds.minimumDuration);
        if (!AddTick(c.start,delta,result.start)) return result;
        result.duration -= delta;
        if (!advanceSource(delta)) return result;
        break;
    case editor::EditKind::TrimEnd:
    case editor::EditKind::Ripple:
        delta = std::clamp(delta, bounds.minimumDuration - c.duration, after);
        result.duration += delta;
        result.rippleDelta = kind == editor::EditKind::Ripple ? delta : 0;
        break;
    case editor::EditKind::Slip:
        delta = std::clamp(delta, -before, after);
        if (!advanceSource(delta)) return result;
        break;
    default:
        return result;
    }
    if (result.start>std::numeric_limits<Tick>::max()-result.duration || result.sourceIn<bounds.mediaFirst ||
        result.sourceIn>bounds.mediaLast) return result;
    result.valid = true;
    return result;
}
PairEdit RollClips(const ClipView &a, const ClipView &b, Tick delta, ClipConstraints ac, ClipConstraints bc) {
    Tick cut=0;
    if (a.track!=b.track || a.duration<=0 || b.duration<=0 ||
        !AddTick(a.start,a.duration,cut) || cut!=b.start || a.locked || b.locked)
        return {};
    auto left = EditClip(a, editor::EditKind::TrimEnd, delta, ac);
    auto right = EditClip(b, editor::EditKind::TrimStart, delta, bc);
    if (!left.valid || !right.valid)
        return {};
    Tick constrained = delta >= 0 ? (std::min)(left.duration - a.duration, right.start - b.start)
                                  : (std::max)(left.duration - a.duration, right.start - b.start);
    left = EditClip(a, editor::EditKind::TrimEnd, constrained, ac);
    right = EditClip(b, editor::EditKind::TrimStart, constrained, bc);
    return {left, right, left.valid && right.valid};
}
SplitResult SplitClip(const ClipView &c, Tick tick, ClipConstraints bounds) {
    if (c.locked || bounds.minimumDuration<1 || c.duration<bounds.minimumDuration ||
        !std::isfinite(c.speed) || c.speed<=0 || bounds.mediaLast<bounds.mediaFirst ||
        c.sourceIn<bounds.mediaFirst || c.sourceIn>bounds.mediaLast ||
        c.start>std::numeric_limits<Tick>::max()-c.duration) return {};
    const auto end=c.start+c.duration;
    if (tick<=c.start || tick>=end) return {};
    const Tick offset=tick-c.start;
    if (offset<bounds.minimumDuration || c.duration-offset<bounds.minimumDuration) return {};
    const auto available=static_cast<std::uint64_t>(bounds.mediaLast)-static_cast<std::uint64_t>(c.sourceIn);
    Tick sourceOffset=offset;
    if (c.speed==1) {
        if (static_cast<std::uint64_t>(c.duration)>available) return {};
    } else {
        const long double sourceDuration=static_cast<long double>(c.duration)*c.speed;
        if (sourceDuration>static_cast<long double>(available)) return {};
        const long double mapped=std::round(static_cast<long double>(offset)*c.speed);
        if (!std::isfinite(mapped) || mapped<0 || mapped>=std::ldexp(1.L,63)) return {};
        sourceOffset=static_cast<Tick>(mapped);
    }
    if (c.sourceIn>std::numeric_limits<Tick>::max()-sourceOffset) return {};
    const auto rightSource=c.sourceIn+sourceOffset;
    if (rightSource>bounds.mediaLast) return {};
    return {{c.start, offset, c.sourceIn, true},
            {tick, c.duration - offset, rightSource, true},true};
}
TripleEdit SlideClip(const ClipView &a, const ClipView &b, const ClipView &c, Tick delta, ClipConstraints ac,
                     ClipConstraints bc, ClipConstraints cc) {
    Tick firstCut=0,secondCut=0;
    if (a.track!=b.track || b.track!=c.track || a.duration<=0 || b.duration<=0 || c.duration<=0 ||
        !AddTick(a.start,a.duration,firstCut) || firstCut!=b.start ||
        !AddTick(b.start,b.duration,secondCut) || secondCut!=c.start || a.locked || b.locked ||
        c.locked)
        return {};
    auto left = EditClip(a, editor::EditKind::TrimEnd, delta, ac),
         right = EditClip(c, editor::EditKind::TrimStart, delta, cc);
    if (!left.valid || !right.valid)
        return {};
    Tick constrained = delta >= 0 ? (std::min)(left.duration - a.duration, right.start - c.start)
                                  : (std::max)(left.duration - a.duration, right.start - c.start);
    left = EditClip(a, editor::EditKind::TrimEnd, constrained, ac);
    right = EditClip(c, editor::EditKind::TrimStart, constrained, cc);
    auto middle = EditClip(b, editor::EditKind::Move, constrained, bc);
    return {left, middle, right, left.valid && middle.valid && right.valid};
}
editor::SnapResult ResolveTimelineSnap(const TimelineState &s, Tick delta,
    std::span<const editor::SnapCandidate> candidates, std::span<const StableId> movingIds) {
    const auto kind = s.drag.draft.kind;
    const bool end = kind == editor::EditKind::TrimEnd || kind == editor::EditKind::Ripple || kind == editor::EditKind::Roll;
    const Tick anchor = s.original.start + (end ? s.original.duration : 0);
    editor::SnapResult best{anchor + delta};
    if (!s.snapping || kind == editor::EditKind::Slip) return best;
    auto consider = [&](editor::SnapCandidate candidate, Tick edge) {
        if (!(s.snapKinds & (1u << static_cast<unsigned>(candidate.kind)))) return;
        if (candidate.id && (candidate.id == s.original.id ||
            std::find(movingIds.begin(),movingIds.end(),candidate.id)!=movingIds.end())) return;
        auto hit = editor::ResolveSnap(edge + delta, {&candidate,1},
            s.canvas.scale.x/editor::TicksPerSecond,8);
        if (hit.snapped && (!best.snapped || hit.distancePixels < best.distancePixels ||
            (hit.distancePixels == best.distancePixels && hit.candidate.priority > best.candidate.priority))) {
            best = hit;
            // tick is the snapped primary anchor; candidate.tick is the visible guide.
            best.tick = anchor + hit.tick - edge;
        }
    };
    auto edge = [&](Tick tick) {
        if (s.snapToFrame && editor::Valid(s.time.rate)) {
            auto frame=editor::TickToFrame(tick+delta,s.time.rate);
            for (auto f : {frame-1,frame,frame+1})
                consider({editor::FrameToTick(f,s.time.rate),editor::SnapKind::Frame,0,0},tick);
        }
        if (s.magnet) for (auto candidate:candidates) consider(candidate,tick);
    };
    edge(anchor);
    if (kind == editor::EditKind::Move || kind == editor::EditKind::Duplicate || kind == editor::EditKind::Slide)
        edge(s.original.start+s.original.duration);
    return best;
}
} // namespace imkit::video
