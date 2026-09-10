#include <imkit/video.h>
#include "transaction_support.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
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
bool TransitionBadge(ImDrawList *draw,const IconAtlas *atlas,TransitionKind kind,bool outgoing,ImVec2 position,float size) {
    if (!atlas || kind==TransitionKind::None) return false;
    const float pixels=size*std::max(ImGui::GetIO().DisplayFramebufferScale.x,ImGui::GetIO().DisplayFramebufferScale.y);
    std::size_t level=0;
    while (level+1<IconPixelSizes.size() && IconPixelSizes[level]<pixels) ++level;
    if (!atlas->textures[level].GetTexID()) return false;
    const auto icon=kind==TransitionKind::Dissolve ? IconId::Dissolve : kind==TransitionKind::Fade ?
        (outgoing ? IconId::FadeOut : IconId::FadeIn) : IconId::Crossfade;
    const auto region=GetIconRegion(icon,IconPixelSizes[level]);
    draw->AddImage(atlas->textures[level],position,{position.x+size,position.y+size},region.uv0,region.uv1,ImGui::GetColorU32(ImGuiCol_Text));
    return true;
}
void EnvelopeAction(editor::EventBuffer &out,const ClipView &clip,std::uint64_t revision,
                    StableId target,Tick tick,double gain,Tick action) {
    if (out.storage.size()-out.count<2) {out.overflow=true;return;}
    editor::Value original;original.parent=clip.id;original.first=tick;original.x=gain;
    editor::Transaction edit;edit.Begin(target,revision,editor::EditKind::AudioEnvelope,original,editor::CurrentModifiers(),out);
    edit.draft.proposed.offset=action;edit.Commit(revision,out);
}
void EndClipKeys(TimelineState &s,std::uint64_t revision,bool cancel,editor::EventBuffer &out) {
    if (!s.keyDrag.active) return;
    cancel |= revision!=s.keyDrag.draft.revision || s.keyDrag.draft.phase==editor::Phase::Cancel;
    s.keyDrag.draft.phase=cancel ? editor::Phase::Cancel : editor::Phase::Commit;
    for (auto &member:s.keyCompanions.first(s.keyCompanionCount)) member.draft.phase=s.keyDrag.draft.phase;
    if (out.storage.size()-out.count<1+s.keyCompanionCount) {out.overflow=true;return;}
    if (cancel) s.keyDrag.Cancel(out);else s.keyDrag.Commit(revision,out);
    for (auto &member:s.keyCompanions.first(s.keyCompanionCount)) {
        if (cancel) member.Cancel(out);else member.Commit(revision,out);
    }
    s.keyCompanionCount=0;
}
const editor::Keyframe *FindClipKey(std::span<const editor::Keyframe> keys,Tick tick,StableId id) {
    auto key=std::lower_bound(keys.begin(),keys.end(),tick,[](const auto &key,Tick value){return key.tick<value;});
    for (;key!=keys.end() && key->tick==tick;++key) if (key->id==id) return &*key;
    return nullptr;
}
editor::Value Value(const ClipView &c) {
    return {c.start, c.start + c.duration, c.sourceIn, c.track, c.speed};
}
editor::Value Value(const ClipEdit &e, StableId track, double speed) {
    return {e.start, e.start + e.duration, e.sourceIn, track, speed};
}
void Toggle(editor::EventBuffer &out, const TrackView &track, std::uint64_t revision, int field, bool value) {
    if (out.count>out.storage.size() || out.storage.size()-out.count<2) {out.overflow=true;return;}
    editor::Value original{0,0,0,0,static_cast<double>(field),value ? 1. : 0.};
    editor::Transaction edit;edit.Begin(track.id,revision,editor::EditKind::Toggle,original,editor::CurrentModifiers(),out);
    edit.draft.proposed.y=value ? 0. : 1.;edit.Commit(revision,out);
}
std::size_t ActiveDrags(const TimelineState &s) {
    std::size_t count = s.drag.active + s.previousDrag.active + s.nextDrag.active;
    for (const auto &member : s.memberDrags.first(s.memberCount))
        count += member.transaction.active+member.previousTransaction.active+member.nextTransaction.active;
    return count;
}
bool ReserveEvents(editor::EventBuffer &out, std::size_t count) {
    if (out.count > out.storage.size() || count > out.storage.size() - out.count) {
        out.overflow = true;
        return false;
    }
    return true;
}
void EndDrags(TimelineState &s, std::uint64_t revision, bool cancel, editor::EventBuffer &out) {
    // A gesture is one host batch. Never publish a partial terminal batch.
    if (!ReserveEvents(out, ActiveDrags(s))) {
        s.drag.draft.phase = cancel ? editor::Phase::Cancel : editor::Phase::Commit;
        return;
    }
    auto finish = [&](editor::Transaction &tx) {
        if (cancel) tx.Cancel(out);
        else tx.Commit(revision, out);
    };
    finish(s.previousDrag);
    finish(s.drag);
    finish(s.nextDrag);
    for (auto &member : s.memberDrags.first(s.memberCount)) {
        finish(member.previousTransaction);finish(member.transaction);finish(member.nextTransaction);
    }
}
void SplitBatch(const TimelineProvider &p,std::span<const ClipView> clips,std::span<const StableId> ids,
                Tick tick,editor::EventBuffer &out) {
    bool available=true;
    for (const auto id:ids)
        if (std::none_of(clips.begin(),clips.end(),[&](const auto &clip){return clip.id==id;})) {
            out.overflow=true;available=false;
        }
    std::size_t count=0;
    const auto eligible=[&](const ClipView &clip) {
        return tick>clip.start &&
            static_cast<long double>(tick)-clip.start<clip.duration;
    };
    for (std::size_t i=0;i<clips.size();++i) {
        const auto &clip=clips[i];
        if (!eligible(clip) || std::any_of(clips.begin(),clips.begin()+i,
            [&](const auto &other){return other.id==clip.id;})) continue;
        ++count;
        available &= SplitClip(clip,tick,p.constraints ? p.constraints(p.user,clip.id) : ClipConstraints{}).valid;
        if (p.canBeginEdit && !p.canBeginEdit(p.user,clip.id,editor::EditKind::Split)) available=false;
    }
    if (available && ReserveEvents(out,count*2))
        for (std::size_t i=0;i<clips.size();++i) {
            const auto &clip=clips[i];
            if (!eligible(clip) || std::any_of(clips.begin(),clips.begin()+i,
                [&](const auto &other){return other.id==clip.id;})) continue;
            editor::Transaction edit;
            edit.Begin(clip.id,p.revision,editor::EditKind::Split,Value(clip),editor::CurrentModifiers(),out);
            edit.draft.proposed.first=tick;edit.Commit(p.revision,out);
        }
}
Tick AvailableTicks(std::uint64_t sourceTicks,double speed) {
    if (speed==1) return static_cast<Tick>(std::min(sourceTicks,static_cast<std::uint64_t>(std::numeric_limits<Tick>::max())));
    const auto value=std::floor(static_cast<long double>(sourceTicks)/speed);
    return value>=std::ldexp(1.L,63) ? std::numeric_limits<Tick>::max() : static_cast<Tick>(value);
}
bool AddTick(Tick a,Tick b,Tick &out) {
    if ((b>0 && a>std::numeric_limits<Tick>::max()-b) ||
        (b<0 && a<std::numeric_limits<Tick>::min()-b)) return false;
    out=a+b;return true;
}
} // namespace
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
void Timeline(const char *id, const TimelineProvider &p, TimelineState &s, editor::Selection &selection,
              editor::EventBuffer &out, const Theme &theme, ImVec2 size) {
    ImGui::PushID(id);
    const float timelineWidth=size.x>0?size.x:ImGui::GetContentRegionAvail().x;
    constexpr editor::Command toolCommands[]={editor::Command::ToolSelect,editor::Command::ToolRazor,
        editor::Command::ToolRipple,editor::Command::ToolRoll,editor::Command::ToolSlip,editor::Command::ToolSlide,editor::Command::ToolHand};
    const auto &tools=s.labels.tools;
    for (int i = 0; i < 7; ++i) {
        if (i)
            ImGui::SameLine();
        if (s.icons) {
            const IconId glyphs[]={IconId::SelectPointer,IconId::Razor,IconId::RippleEdit,IconId::RollingEdit,IconId::SlipEdit,IconId::SlideEdit,IconId::HandPan};
            const auto &tips=s.labels.tooltips;
            const bool active=static_cast<int>(s.tool)==i;
            if (active) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            if (IconButton(tools[i],*s.icons,glyphs[i],tips[i])) s.tool=static_cast<Tool>(i);
            if (active) {
                const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine({a.x+4,b.y-2},{b.x-4,b.y-2},ImGui::GetColorU32(theme.colors.text),2);
            }
            if (active) ImGui::PopStyleColor();
        } else if (ImGui::Selectable(tools[i], static_cast<int>(s.tool) == i, 0, {56, 24}))
            s.tool = static_cast<Tool>(i);
        if (ImGui::IsItemHovered() || ImGui::IsItemFocused()) {
            const auto binding=std::find_if(s.bindings.begin(),s.bindings.end(),[&](const auto &b){return b.command==toolCommands[i] && b.chord;});
            if (binding==s.bindings.end()) ImGui::SetTooltip("%s",s.labels.tooltips[i]);
            else {
                const auto chord=binding->chord;
                ImGui::SetTooltip("%s (%s%s%s%s%s)",s.labels.tooltips[i],
                    chord & ImGuiMod_Ctrl ? "Ctrl+" : "",chord & ImGuiMod_Shift ? "Shift+" : "",
                    chord & ImGuiMod_Alt ? "Alt+" : "",chord & ImGuiMod_Super ? "Super+" : "",
                    ImGui::GetKeyName(static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_)));
            }
        }
    }
    ImGui::SameLine();
    ImGui::Checkbox(s.labels.snap, &s.snapping);
    ImGui::SameLine();
    if (s.icons) {
        const bool active=s.magnet;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (IconButton("magnet", *s.icons, IconId::Magnet, s.labels.magnetTooltip)) s.magnet=!s.magnet;
        if (active) ImGui::PopStyleColor();
    } else ImGui::Checkbox(s.labels.magnet, &s.magnet);
    ImGui::SameLine();
    if (ImGui::Button(s.labels.options)) ImGui::OpenPopup("snap-options");
    if (ImGui::BeginPopup("snap-options")) {
        ImGui::Checkbox("Stereo waveforms",&s.waveformOptions.stereo);
        ImGui::SliderFloat("Waveform display gain",&s.waveformOptions.gain,.25f,4.f);
        int follow=static_cast<int>(s.autoScroll);
        if (ImGui::Combo(s.labels.follow,&follow,s.labels.followModes.data(),static_cast<int>(s.labels.followModes.size())))
            s.autoScroll=static_cast<editor::AutoScroll>(follow);
        if(s.icons) {Icon(*s.icons,IconId::SnapToFrame,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
        ImGui::Checkbox(s.labels.frameGrid, &s.snapToFrame);
        const auto &names=s.labels.snapKinds;
        for (unsigned i=1;i<7;++i) {
            constexpr IconId glyphs[]{IconId::SnapToFrame,IconId::Timecode,IconId::Marker,IconId::SnapToClipEdge,IconId::Keyframe,IconId::WorkRange,IconId::SnapToSelection};
            if(s.icons) {Icon(*s.icons,glyphs[i],{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            bool enabled=(s.snapKinds & (1u<<i))!=0;
            if (ImGui::Checkbox(names[i],&enabled)) s.snapKinds ^= 1u<<i;
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    bool fit=s.icons ? IconButton("fit",*s.icons,IconId::FitView,s.labels.fitTooltip) : ImGui::Button(s.labels.fit);
    fit |= editor::CommandPressed(editor::Command::Fit,s.bindings,
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
    ImGui::SameLine();
    const bool fitSelected=ImGui::SmallButton("Fit selection");
    auto fitRange=p.contentRange;
    if (fitSelected && p.selected && selection.count) {
        const auto selected=p.selected(p.user,selection.storage.first(selection.count));
        if(!selected.empty()) {
            fitRange={selected.front().start,selected.front().start+selected.front().duration};
            for(const auto &c:selected) {fitRange.first=std::min(fitRange.first,c.start);fitRange.last=std::max(fitRange.last,c.start+c.duration);}
            fit=true;
        }
    }
    if (fit && fitRange.last>fitRange.first) {
        const double width=timelineWidth-s.headerWidth;
        if (width>48) {
            const double first=editor::Seconds(fitRange.first),last=editor::Seconds(fitRange.last);
            s.canvas.scale.x=(width-48)/(last-first);
            s.canvas.origin.x=first-24/s.canvas.scale.x;
        }
    }
    if (s.time.playing && !fit)
        s.canvas.origin.x=editor::FollowPlayhead(s.canvas.origin.x,
            (timelineWidth-s.headerWidth)/s.canvas.scale.x,
            editor::Seconds(s.time.playhead),s.autoScroll);
    const auto &input=ImGui::GetIO();
    const bool overTracks=ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && input.MousePos.x>=s.view.min.x+s.headerWidth &&
        input.MousePos.x<s.view.max.x && input.MousePos.y>=s.view.min.y && input.MousePos.y<s.view.max.y;
    if(overTracks) {
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {s.canvas.origin.x-=input.MouseDelta.x/s.canvas.scale.x;}
        if(input.KeyCtrl && input.MouseWheel && !ImGui::IsAnyItemActive())
            editor::ZoomAt(s.canvas,{input.MousePos.x-s.view.min.x-s.headerWidth,0},{std::pow(1.15,input.MouseWheel),1});
        s.canvas.origin.x-=input.MouseWheelH*32/s.canvas.scale.x;
    }
    if (p.contentRange.last>p.contentRange.first) {
        const auto a=ImGui::GetCursorScreenPos();
        const float width=std::max(1.f,timelineWidth-s.headerWidth);
        ImGui::SetCursorScreenPos({a.x+s.headerWidth,a.y});
        ImGui::InvisibleButton("overview",{width,14*ImGui::GetFontSize()/14});
        const double first=editor::Seconds(p.contentRange.first),length=editor::Seconds(p.contentRange.last-p.contentRange.first);
        const double viewWidth=width/s.canvas.scale.x;
        const float edgeLeft=a.x+s.headerWidth+float((s.canvas.origin.x-first)/length*width);
        const float edgeRight=edgeLeft+float(viewWidth/length*width);
        if(ImGui::IsItemActivated()) {
            const float mx=input.MousePos.x;
            s.overviewDrag=std::abs(mx-edgeLeft)<7 ? -1 : std::abs(mx-edgeRight)<7 ? 1 : 0;
            if(!s.overviewDrag && (mx<edgeLeft || mx>edgeRight)) s.canvas.origin.x=first+std::clamp(double(mx-a.x-s.headerWidth)/width,0.,1.)*length-viewWidth*.5;
        }
        if(ImGui::IsItemActive()) {
            const double delta=input.MouseDelta.x/width*length;
            if(s.overviewDrag<0) {const double end=s.canvas.origin.x+viewWidth;s.canvas.origin.x=std::min(end-.001,s.canvas.origin.x+delta);s.canvas.scale.x=std::clamp(width/(end-s.canvas.origin.x),1e-9,1e9);}
            else if(s.overviewDrag>0) s.canvas.scale.x=std::clamp(width/std::max(.001,viewWidth+delta),1e-9,1e9);
            else s.canvas.origin.x+=delta;
        }
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("Drag: navigate / Ctrl+wheel: zoom");
        auto *d=ImGui::GetWindowDrawList();const auto lo=ImGui::GetItemRectMin(),hi=ImGui::GetItemRectMax();
        d->AddRectFilled(lo,hi,ImGui::GetColorU32(theme.colors.input),3);
        const float left=lo.x+float(std::clamp((s.canvas.origin.x-first)/length,0.,1.)*width);
        const float right=lo.x+float(std::clamp((s.canvas.origin.x+width/s.canvas.scale.x-first)/length,0.,1.)*width);
        d->AddRectFilled({left,lo.y+2},{std::max(left+3,right),hi.y-2},ImGui::GetColorU32(theme.colors.selection),2);
    }
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + s.headerWidth);
    s.time.icons=s.icons;
    editor::TimeRuler("time", s.time, s.canvas, p.markers, p.revision, out, theme);
    s.canvas.wheelZoom = ImGui::GetIO().KeyCtrl;
    s.canvas.wheelZoomY = false;
    auto passiveCanvas=s.canvas;passiveCanvas.wheelZoom=false;
    auto view = editor::BeginCanvas("tracks", passiveCanvas, size, theme);
    view.visible=editor::VisibleRange(s.canvas,{view.max.x-view.min.x,view.max.y-view.min.y});
    s.view = view;
    auto *draw = ImGui::GetWindowDrawList();
    const auto &io = ImGui::GetIO();
    detail::ResumeTerminal(s.transitionDrag,p.revision,out);
    if (s.transitionDrag.active && (p.revision!=s.transitionDrag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        s.transitionDrag.Cancel(out);
    bool transitionSeen=false;
    detail::ResumeTerminal(s.captionDrag,p.revision,out);
    if (s.captionDrag.active && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        s.captionDrag.draft.proposedText=s.captionDrag.draft.originalText;s.captionDrag.Cancel(out);
    }
    if (!s.captionDrag.active) s.editingCaption=0;
    bool captionSeen=false;
    if (s.keyDrag.active && (s.keyDrag.draft.phase==editor::Phase::Cancel || s.keyDrag.draft.phase==editor::Phase::Commit ||
        s.keyDrag.draft.revision!=p.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        EndClipKeys(s,p.revision,ImGui::IsKeyPressed(ImGuiKey_Escape),out);
    detail::ResumeTerminal(s.envelopeDrag,p.revision,out);
    if (s.envelopeDrag.active && ImGui::IsKeyPressed(ImGuiKey_Escape)) s.envelopeDrag.Cancel(out);
    bool envelopeSeen=false;
    bool keySeen=false;
    const bool keyCommands=ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && !io.WantTextInput &&
        !s.keyDrag.active && !s.envelopeDrag.active && !s.drag.active && !s.transitionDrag.active && !s.captionDrag.active;
    for (int i=0;i<7;++i) if (editor::CommandPressed(toolCommands[i],s.bindings,keyCommands && !s.heightDrag.active))
        s.tool=static_cast<Tool>(i);
    if (editor::CommandPressed(editor::Command::Split,s.bindings,keyCommands && !s.heightDrag.active) &&
        selection.count && p.selected) {
        const auto ids=selection.storage.first(selection.count);
        SplitBatch(p,p.selected(p.user,ids),ids,s.time.playhead,out);
    }
    const bool duplicateKeys=editor::CommandPressed(editor::Command::Duplicate,s.bindings,keyCommands);
    const bool removeKeys=editor::CommandPressed(editor::Command::Delete,s.bindings,keyCommands);
    const bool addKey=editor::CommandPressed(editor::Command::AddKey,s.bindings,keyCommands);
    const bool previousKey=editor::CommandPressed(editor::Command::PreviousKey,s.bindings,keyCommands);
    const bool nextKey=editor::CommandPressed(editor::Command::NextKey,s.bindings,keyCommands);
    bool heightEditorSeen=false;
    detail::ResumeTerminal(s.heightDrag,p.revision,out);
    if (s.heightDrag.active && ImGui::IsKeyPressed(ImGuiKey_Escape)) s.heightDrag.Cancel(out);
    if (s.tool==Tool::Hand && view.max.x>view.min.x+s.headerWidth) {
        const auto cursor=ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos({view.min.x+s.headerWidth,view.min.y});
        ImGui::InvisibleButton("hand-pan",{view.max.x-view.min.x-s.headerWidth,view.max.y-view.min.y});
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            s.canvas.origin.x-=io.MouseDelta.x/s.canvas.scale.x;
            s.verticalScroll=std::max(0.,s.verticalScroll-io.MouseDelta.y);
        }
        ImGui::SetCursorScreenPos(cursor);ImGui::Dummy({0,0});
    }
    // Track rows are indexed independently from time; no all-track query or clip scan.
    if (view.hovered && io.MouseWheel && !io.KeyCtrl)
        s.verticalScroll = (std::max)(0., s.verticalScroll - io.MouseWheel * s.rowHeight);
    const double totalHeight = p.layout ? p.totalHeight : p.trackCount*s.rowHeight;
    if (totalHeight>0)
        s.verticalScroll=std::clamp(s.verticalScroll,0.,(std::max)(0.,totalHeight-(view.max.y-view.min.y)));
    const int first = (std::max)(0, static_cast<int>(s.verticalScroll / s.rowHeight));
    const int count = (std::max)(
        0, (std::min)(p.trackCount - first, static_cast<int>((view.max.y - view.min.y) / s.rowHeight) + 2));
    TrackLayout layout;
    if (p.layout) layout=p.layout(p.user,s.verticalScroll,s.verticalScroll+view.max.y-view.min.y);
    else {
        layout.tracks=p.tracks ? p.tracks(p.user, first, count) : std::span<const TrackView>{};
        layout.top=first*s.rowHeight;
    }
    auto tracks=layout.tracks;
    editor::Range range{editor::FromSeconds(s.canvas.origin.x),
                        editor::FromSeconds(s.canvas.origin.x +
                                            (view.max.x - view.min.x - s.headerWidth) / s.canvas.scale.x)};
    s.hovered = 0;
    if (s.drag.active) {
        bool cancel = p.revision != s.drag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                      s.drag.draft.phase == editor::Phase::Cancel;
        if (!cancel && p.isEditable) {
            const auto valid=[&](const editor::Transaction &tx){return !tx.active || p.isEditable(p.user,tx.draft.target);};
            cancel=!valid(s.drag) || !valid(s.previousDrag) || !valid(s.nextDrag);
            for (const auto &member:s.memberDrags.first(s.memberCount)) cancel|=!valid(member.transaction) || !valid(member.previousTransaction) || !valid(member.nextTransaction);
        }
        if (cancel || s.drag.draft.phase == editor::Phase::Commit)
            EndDrags(s, p.revision, cancel, out);
    }
    double rowTop=layout.top;
    for (const auto &track : tracks) {
        const float rowHeight=p.layout ? TrackExtent(track) : s.rowHeight;
        float y = view.min.y + static_cast<float>(rowTop-s.verticalScroll);
        rowTop+=rowHeight;
        draw->AddRectFilled({view.min.x, y}, {view.max.x, y + rowHeight - 2},
                            ImGui::GetColorU32(theme.editor.trackHeader));
        ImGui::SetCursorScreenPos({view.min.x + 4, y + 3});
        ImGui::PushID(reinterpret_cast<void *>(static_cast<std::uintptr_t>(track.id)));
        if (ImGui::SmallButton(track.expanded ? "v" : ">"))
            Toggle(out,track,p.revision,static_cast<int>(TrackControl::Expanded),track.expanded);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",track.expanded ? s.trackLabels.collapse : s.trackLabels.expand);
        ImGui::SameLine(0,4);
        if (s.icons) {
            constexpr IconId kinds[]={IconId::Video,IconId::Audio,IconId::Text,IconId::EffectTrack,IconId::AdjustmentTrack,IconId::Layers};
            const auto kind=static_cast<unsigned>(track.kind);
            if (kind<std::size(kinds)) {
                Icon(*s.icons,kinds[kind],{ImGui::GetFontSize()});
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",s.trackLabels.kinds[kind]);
                ImGui::SameLine(0,4);
            }
        }
        const auto nameMin=ImGui::GetCursorScreenPos();
        const float nameWidth=std::max(1.f,view.min.x+s.headerWidth-4-nameMin.x);
        ImGui::InvisibleButton("track-name",{nameWidth,ImGui::GetFontSize()},ImGuiButtonFlags_MouseButtonRight);
        const ImVec4 nameClip{nameMin.x,y,view.min.x+s.headerWidth-4,y+rowHeight};
        draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),nameMin,ImGui::GetColorU32(theme.colors.text),track.label,nullptr,0,&nameClip);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",track.label);
        if (ImGui::BeginPopupContextItem("track layout")) {
            if(track.kind==TrackKind::Caption) {
                if(s.icons) {Icon(*s.icons,IconId::CaptionAdd,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
                if(ImGui::MenuItem(s.labels.addCaption,nullptr,false,!track.locked) && ReserveEvents(out,2)) {
                    editor::Transaction action;action.Begin(track.id,p.revision,editor::EditKind::CaptionInsert,{},editor::CurrentModifiers(),out);
                    action.draft.proposed.first=s.time.playhead;action.Commit(p.revision,out);
                }
            }
            float height=s.heightDrag.active && s.heightDrag.draft.target==track.id ?
                         static_cast<float>(s.heightDrag.draft.proposed.x) : track.height;
            if(s.icons) {Icon(*s.icons,IconId::TrackHeight,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            bool edited=ImGui::SliderFloat(s.trackLabels.height,&height,64,240,"%.0f px");
            if (ImGui::IsItemActivated())
                s.heightDrag.Begin(track.id,p.revision,editor::EditKind::TrackHeight,
                                   {0,0,0,0,track.height},editor::CurrentModifiers(),out);
            if (edited && s.heightDrag.active)
                s.heightDrag.Update(p.revision,{0,0,0,0,height},out);
            if (s.heightDrag.active && s.heightDrag.draft.target==track.id) heightEditorSeen=true;
            if (ImGui::IsItemDeactivated() && s.heightDrag.active)
                s.heightDrag.Commit(p.revision,out);
            ImGui::EndPopup();
        }
        if (track.expanded) {
        ImGui::SetCursorScreenPos({view.min.x + 4, y + 30});
        const auto &labels=s.trackLabels.buttons;
        const auto &tooltips=s.trackLabels.names;
        const bool values[] = {track.visible, track.mute,   track.solo,
                               track.locked,  track.record, track.target, track.source};
        const IconId controlIcons[]={track.visible ? IconId::Eye : IconId::EyeOff,
            track.mute ? IconId::Mute : IconId::Volume,IconId::Solo,
            track.locked ? IconId::Lock : IconId::Unlock,IconId::Record,IconId::Target,IconId::SourcePatch};
        const float controlIconSize=20*ImGui::GetFontSize()/14;
        float controlsWidth=12;
        for (int f=0;f<7;++f) controlsWidth+=(s.icons && controlIcons[f]!=IconId::Count ? controlIconSize :
            ImGui::CalcTextSize(labels[f]).x)+2*ImGui::GetStyle().FramePadding.x;
        if (controlsWidth>s.headerWidth-8) {
            if (ImGui::SmallButton(s.trackLabels.controls)) ImGui::OpenPopup("track controls");
            if (ImGui::BeginPopup("track controls")) {
                for (int f=0;f<7;++f) {
                    ImGui::PushID(f);
                    if (s.icons) {
                        if (controlIcons[f]!=IconId::Count) Icon(*s.icons,controlIcons[f],{ImGui::GetFontSize()});
                        else ImGui::Dummy({ImGui::GetFontSize(),ImGui::GetFontSize()});
                        ImGui::SameLine();
                    }
                    if (ImGui::MenuItem(tooltips[f],nullptr,values[f])) Toggle(out,track,p.revision,f,values[f]);
                    ImGui::PopID();
                }
                ImGui::EndPopup();
            }
        } else for (int f = 0; f < 7; ++f) {
            if (f)
                ImGui::SameLine(0, 2);
            if (values[f]) ImGui::PushStyleColor(ImGuiCol_Button,theme.colors.accent);
            ImGui::PushID(f);
            const bool pressed=s.icons && controlIcons[f]!=IconId::Count ?
                IconButton("control",*s.icons,controlIcons[f],tooltips[f],{controlIconSize}) :
                s.icons ? ImGui::Button(labels[f],{0,controlIconSize+2*ImGui::GetStyle().FramePadding.y}) : ImGui::SmallButton(labels[f]);
            if (pressed) Toggle(out,track,p.revision,f,values[f]);
            ImGui::PopID();
            if (values[f]) {
                auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
                draw->AddLine({a.x+2,b.y-1},{b.x-2,b.y-1},ImGui::GetColorU32(theme.colors.text),2);
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s: %s",tooltips[f],values[f] ? s.trackLabels.on : s.trackLabels.off);
        }
        }
        ImGui::PopID();
        auto clips = p.clips ? p.clips(p.user, track.id, range) : std::span<const ClipView>{};
        draw->PushClipRect({view.min.x + s.headerWidth, y}, {view.max.x, y + rowHeight - 2}, true);
        for (const auto &clip : clips) {
            auto value = Value(clip);
            if (s.drag.active && s.drag.draft.target == clip.id)
                value = s.drag.draft.proposed;
            for (const auto &member : s.memberDrags.first(s.memberCount)) {
                if (member.transaction.active && member.original.id == clip.id) value=member.transaction.draft.proposed;
                if (member.previousTransaction.active && member.previous.id==clip.id) value=member.previousTransaction.draft.proposed;
                if (member.nextTransaction.active && member.next.id==clip.id) value=member.nextTransaction.draft.proposed;
            }
            if (s.previousDrag.active && s.previousOriginal.id == clip.id)
                value = s.previousDrag.draft.proposed;
            if (s.nextDrag.active && s.nextOriginal.id == clip.id)
                value = s.nextDrag.draft.proposed;
            float x =
                view.min.x + s.headerWidth +
                static_cast<float>((editor::Seconds(value.first) - s.canvas.origin.x) * s.canvas.scale.x);
            float end = x + static_cast<float>(editor::Seconds(value.last - value.first) * s.canvas.scale.x);
            ImVec2 a{x, y + 4}, b{end, y + rowHeight - 7};
            bool selected = selection.Contains(clip.id);
            auto color = track.kind == TrackKind::Audio     ? theme.editor.audioClip
                         : track.kind == TrackKind::Caption ? theme.editor.captionClip
                         : track.kind == TrackKind::Effect ? theme.editor.effectClip
                         : track.kind == TrackKind::Adjustment ? theme.editor.adjustmentClip
                         : track.kind == TrackKind::Group ? theme.editor.groupClip
                                                            : theme.editor.videoClip;
            color.w *= selected ? .8f : .35f;
            draw->AddRectFilled(a, b, ImGui::GetColorU32(color), 4);
            draw->AddRect(a, b, ImGui::GetColorU32(selected ? theme.colors.focus : theme.colors.border), 4, 0,
                          selected ? 2.f : 1.f);
            const float nameY=y+4+((clip.transitionIn || clip.transitionOut) ? ImGui::GetFontSize() : 3.f);
            const ImVec4 textBounds{x+4,a.y+1,end-4,b.y-1};
            auto text=[&](ImVec2 position,ImU32 color,const char *label,float right) {
                const ImVec4 bounds{textBounds.x,textBounds.y,std::min(textBounds.z,right),textBounds.w};
                if (bounds.z>bounds.x && bounds.w>bounds.y)
                    draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),position,color,label,nullptr,0,&bounds);
            };
            char duration[48]{},metadata[96]{};
            editor::FormatTimecode(value.last-value.first,s.time.rate,false,duration);
            std::snprintf(metadata,sizeof(metadata),"%s  %.3gx",duration,value.x);
            const float metadataWidth=ImGui::CalcTextSize(metadata).x;
            const float metadataRight=end-(clip.proxy ? ImGui::GetFontSize()+16 : 12);
            const bool showMetadata=metadataRight-x>metadataWidth+100 && nameY+ImGui::GetFontSize()<b.y;
            const float nameRight=showMetadata ? metadataRight-metadataWidth-10 : metadataRight;
            if (track.expanded && clip.thumbnail.GetTexID() && end-x>8 && b.y>nameY+ImGui::GetFontSize()+7)
                draw->AddImage(clip.thumbnail, {x+3,nameY+ImGui::GetFontSize()+4}, {std::min(end-3,x+65),b.y-3});
            float nameLeft=x+6;
            if (s.icons && (clip.linked || clip.group)) {
                const float iconSize=ImGui::GetFontSize();
                const float pixels=iconSize*std::max(io.DisplayFramebufferScale.x,io.DisplayFramebufferScale.y);
                std::size_t level=0;
                while (level+1<IconPixelSizes.size() && IconPixelSizes[level]<pixels) ++level;
                const auto badge=[&](IconId id) {
                    if (nameLeft+iconSize+12>nameRight || !s.icons->textures[level].GetTexID()) return;
                    const auto region=GetIconRegion(id,IconPixelSizes[level]);
                    draw->AddImage(s.icons->textures[level],{nameLeft,nameY},{nameLeft+iconSize,nameY+iconSize},
                        region.uv0,region.uv1,ImGui::GetColorU32(theme.colors.text));
                    nameLeft+=iconSize+3;
                };
                if (clip.linked) badge(IconId::Link);
                if (clip.group) badge(IconId::Layers);
            }
            if (s.editingCaption == clip.id && s.captionDrag.active) {
                captionSeen=true;
                if (track.locked || clip.locked) {
                    s.captionDrag.draft.proposedText=s.captionDrag.draft.originalText;s.captionDrag.Cancel(out);
                }
                if (s.captionDrag.active && s.captionDrag.draft.phase!=editor::Phase::Cancel &&
                    s.captionDrag.draft.phase!=editor::Phase::Commit) {
                    ImGui::SetCursorScreenPos({x+4,y+5});ImGui::SetNextItemWidth(std::max(30.f,end-x-8));
                    if (s.captionFocus) {ImGui::SetKeyboardFocusHere();s.captionFocus=false;}
                    const bool accept=ImGui::InputText("##caption",s.caption,sizeof(s.caption),
                        ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll);
                    auto update=s.captionDrag.draft;update.phase=editor::Phase::Update;
                    std::snprintf(update.proposedText.data(),update.proposedText.size(),"%s",s.caption);
                    if (update.proposedText!=s.captionDrag.draft.proposedText && out.Push(update)) s.captionDrag.draft=update;
                    if (accept) {s.captionDrag.draft.proposedText=update.proposedText;s.captionDrag.Commit(p.revision,out);}
                }
            } else {
                text({nameLeft,nameY},ImGui::GetColorU32(theme.colors.text),clip.label,nameRight);
                if (showMetadata) text({metadataRight-metadataWidth,nameY},ImGui::GetColorU32(theme.colors.muted),metadata,metadataRight);
            }
            if (clip.missing || clip.offline)
                draw->AddLine(a, b, ImGui::GetColorU32(clip.offline ? theme.editor.error : theme.editor.missing), 2);
            if (track.locked || clip.locked)
                text({x+6,b.y-ImGui::GetFontSize()-3},ImGui::GetColorU32(theme.editor.locked),"Locked",end-4);
            if (clip.proxy)
                text({end-ImGui::GetFontSize()-8,y+7},ImGui::GetColorU32(theme.editor.proxy),"P",end-8);
            if (clip.linked || clip.group)
                draw->AddLine({x + 3, b.y - 3}, {end - 3, b.y - 3}, ImGui::GetColorU32(theme.colors.text));
            const float waveformTop=std::min(b.y-2,a.y+((clip.transitionIn || clip.transitionOut) ? 2.f : 1.f)*ImGui::GetFontSize()+7);
            const float waveformCenter=(waveformTop+b.y)*.5f;
            const float waveformAmplitude=std::max(0.f,(b.y-waveformTop)*.5f);
            if (track.expanded && p.waveform.query && clip.audioSource && std::isfinite(clip.speed) && clip.speed>0) {
                const float left=std::max(a.x,view.min.x+s.headerWidth),right=std::min(b.x,view.max.x);
                if (right>left && end>x) {
                    const auto sourceAt=[&](float px) {return static_cast<Tick>(std::clamp(
                        static_cast<long double>(clip.sourceIn)+(static_cast<long double>(px)-x)/(end-x)*clip.duration*clip.speed,
                        static_cast<long double>(std::numeric_limits<Tick>::min()),static_cast<long double>(std::numeric_limits<Tick>::max())));};
                    const editor::Range range{sourceAt(left),sourceAt(right)};
                    const int channels=s.waveformOptions.stereo ? std::clamp(clip.audioChannels,1,2) : 1;
                    for (int channel=0;channel<channels;++channel) {
                        const auto data=p.waveform.query(p.waveform.user,{clip.audioSource,range,channel,static_cast<int>(std::ceil(right-left))});
                        const float height=(b.y-waveformTop)/channels;
                        DrawWaveform(data,range,{left,waveformTop+channel*height},{right,waveformTop+(channel+1)*height},s.waveformOptions,theme);
                    }
                }
            } else if (track.expanded && !clip.audioBuckets.empty()) {
                const editor::Range range{0,editor::TicksPerSecond};
                DrawWaveform({clip.audioBuckets,range,WaveformStatus::Ready},range,{x,waveformTop},{end,b.y},s.waveformOptions,theme);
            } else for (std::size_t j=0;track.expanded && j<clip.waveform.size();++j) {
                const float wx=x+(end-x)*static_cast<float>(j)/clip.waveform.size();
                const float amplitude=std::clamp(clip.waveform[j],0.f,1.f)*waveformAmplitude;
                draw->AddLine({wx,waveformCenter-amplitude},{wx,waveformCenter+amplitude},ImGui::GetColorU32(theme.colors.text));
            }
            const bool keyOwner=selection.count==1 ? selected : s.keyDrag.draft.original.parent==clip.id;
            if (keyOwner && (addKey || previousKey || nextKey)) {
                const auto distance=std::uint64_t(s.time.playhead)-std::uint64_t(clip.start);
                const Tick local=s.time.playhead<clip.start ? Tick{-1} :
                    distance>std::uint64_t(INT64_MAX) ? Tick{INT64_MAX} : static_cast<Tick>(distance);
                const editor::Keyframe *neighbor=nullptr;
                bool existing=false;
                for (const auto &key:clip.keys) {
                    existing |= key.tick==local;
                    if (key.tick<0 || key.tick>clip.duration) continue;
                    if ((previousKey && key.tick<local && (!neighbor || key.tick>neighbor->tick)) ||
                        (!previousKey && nextKey && key.tick>local && (!neighbor || key.tick<neighbor->tick))) neighbor=&key;
                }
                if (addKey && !clip.locked && !track.locked && clip.keyChannel && local>=0 && local<=clip.duration && !existing) {
                    if (out.storage.size()-out.count<2) out.overflow=true;
                    else {
                        editor::Value proposal;proposal.first=local;proposal.parent=clip.id;
                        const auto evaluation=clip.keyEvaluation.empty() ? clip.keys : clip.keyEvaluation;
                        proposal.x=evaluation.empty() ? clip.keyDefaultValue : editor::Evaluate(evaluation,local);
                        editor::Transaction action;action.Begin(clip.keyChannel,p.revision,editor::EditKind::KeyInsert,{},editor::CurrentModifiers(),out);
                        action.draft.proposed=proposal;action.Commit(p.revision,out);
                    }
                } else if (!addKey && neighbor) {
                    if (out.storage.size()-out.count<2) out.overflow=true;
                    else {
                        editor::Value original;original.first=s.time.playhead;original.parent=clip.id;
                        editor::Transaction action;action.Begin(neighbor->id,p.revision,editor::EditKind::Navigate,original,editor::CurrentModifiers(),out);
                        action.draft.proposed.first=clip.start+neighbor->tick;action.Commit(p.revision,out);
                        s.time.playhead=clip.start+neighbor->tick;
                    }
                }
            }
            if ((duplicateKeys || removeKeys) && s.keySelection && s.keyDrag.draft.original.parent==clip.id) {
                std::size_t count=0;bool allowed=!clip.locked && !track.locked;
                Tick delta=std::max(Tick{0},editor::FrameToTick(1,s.time.rate));
                for (const auto &key:clip.keys) if (s.keySelection->Contains(key.id)) {
                    ++count;allowed &= !key.locked && key.tick>=0 && key.tick<=clip.duration;
                    delta=std::min(delta,std::max(Tick{0},clip.duration-key.tick));
                }
                if (count>0 && allowed) {
                    if (out.storage.size()-out.count<count*2) out.overflow=true;
                    else for (const auto &key:clip.keys) if (s.keySelection->Contains(key.id)) {
                        editor::Value original;original.first=key.tick;original.x=key.value;original.parent=clip.id;
                        editor::Transaction action;action.Begin(key.id,p.revision,removeKeys ? editor::EditKind::Remove : editor::EditKind::Duplicate,
                            original,editor::CurrentModifiers(),out);
                        if (!removeKeys) action.draft.proposed.first+=delta;
                        action.Commit(p.revision,out);
                    }
                }
            }
            bool envelopeHit=false;
            if (track.expanded && !clip.envelope.empty()) {
                const float top=waveformTop,bottom=b.y-2,height=std::max(1.f,bottom-top);
                auto proposedPoint=[&](const EnvelopePoint &point) {
                    auto result=point;
                    if (s.envelopeDrag.active && s.envelopeDrag.draft.target==point.id &&
                        s.envelopeDrag.draft.original.parent==clip.id && s.envelopeDrag.draft.phase!=editor::Phase::Cancel) {
                        result.tick=s.envelopeDrag.draft.proposed.first;result.gain=s.envelopeDrag.draft.proposed.x;
                    }
                    return result;
                };
                auto screen=[&](const EnvelopePoint &point) {return ImVec2{x+float(editor::Seconds(point.tick)*s.canvas.scale.x),
                    bottom-float(std::clamp(point.gain,0.,2.)*.5)*height};};
                if (s.envelopeDrag.active && s.envelopeDrag.draft.original.parent==clip.id) {
                    const auto tick=s.envelopeDrag.draft.original.first;
                    auto point=std::lower_bound(clip.envelope.begin(),clip.envelope.end(),tick,
                        [](const auto &p,Tick t){return p.tick<t;});
                    while (point!=clip.envelope.end() && point->tick==tick && point->id!=s.envelopeDrag.draft.target) ++point;
                    if (point!=clip.envelope.end() && point->tick==tick && point->id==s.envelopeDrag.draft.target) {
                        envelopeSeen=true;
                        const auto index=static_cast<std::size_t>(point-clip.envelope.begin());
                        if (clip.locked || track.locked || point->locked) s.envelopeDrag.Cancel(out);
                        else if (s.envelopeDrag.draft.phase!=editor::Phase::Cancel && s.envelopeDrag.draft.phase!=editor::Phase::Commit) {
                            auto value=s.envelopeDrag.draft.original;
                            const Tick low=index ? clip.envelope[index-1].tick : 0;
                            const Tick high=index+1<clip.envelope.size() ? clip.envelope[index+1].tick : clip.duration;
                            if (low>high) s.envelopeDrag.Cancel(out);
                            else {
                                const Tick delta=editor::FromSeconds((io.MousePos.x-s.envelopeMouseStart.x)/s.canvas.scale.x);
                                value.first+=std::clamp(delta,low-value.first,high-value.first);
                                value.x=std::clamp(value.x-(io.MousePos.y-s.envelopeMouseStart.y)*2/height,0.,2.);
                                if (!(value==s.envelopeDrag.draft.proposed)) s.envelopeDrag.Update(p.revision,value,out);
                            }
                        }
                    }
                }
                const auto firstTick=editor::FromSeconds((view.min.x+s.headerWidth-5-x)/s.canvas.scale.x);
                const auto lastTick=editor::FromSeconds((view.max.x+5-x)/s.canvas.scale.x);
                auto first=std::lower_bound(clip.envelope.begin(),clip.envelope.end(),firstTick,[](const auto &p,Tick tick){return p.tick<tick;});
                auto last=std::upper_bound(first,clip.envelope.end(),lastTick,[](Tick tick,const auto &p){return tick<p.tick;});
                if (first!=clip.envelope.begin()) --first;
                if (last!=clip.envelope.end()) ++last;
                ImVec2 previous{};bool havePrevious=false;
                for (auto current=first;current!=last;++current) {
                    const auto &point=*current;
                    const auto position=screen(proposedPoint(point));
                    if (havePrevious) draw->AddLine(previous,position,ImGui::GetColorU32(theme.editor.scope),2);
                    previous=position;havePrevious=true;
                    draw->AddCircleFilled(position,3,ImGui::GetColorU32(theme.editor.scope));
                    if (position.x<view.min.x+s.headerWidth || position.x>view.max.x || position.y<view.min.y || position.y>view.max.y) continue;
                    const auto cursor=ImGui::GetCursorScreenPos();ImGui::SetCursorScreenPos({position.x-5,position.y-5});
                    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(clip.id)));
                    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(point.id)));
                    ImGui::InvisibleButton("envelope",{10,10});const bool hovered=ImGui::IsItemHovered();
                    if (ImGui::BeginPopupContextItem("envelope-actions")) {
                        if (ImGui::MenuItem(s.labels.removeEnvelope,nullptr,false,!point.locked && !clip.locked && !track.locked && !s.envelopeDrag.active))
                            EnvelopeAction(out,clip,p.revision,point.id,point.tick,point.gain,2);
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();ImGui::PopID();ImGui::SetCursorScreenPos(cursor);ImGui::Dummy({0,0});
                    envelopeHit |= hovered;
                    if (hovered) {
                        ImGui::SetTooltip("%s: %.2f",s.labels.envelope,proposedPoint(point).gain);
                        if (ImGui::IsMouseClicked(0) && !clip.locked && !track.locked && !point.locked &&
                            !s.drag.active && !s.keyDrag.active && !s.transitionDrag.active && !s.envelopeDrag.active) {
                            editor::Value original;original.first=point.tick;original.x=point.gain;original.parent=clip.id;
                            if (s.envelopeDrag.Begin(point.id,p.revision,editor::EditKind::AudioEnvelope,original,editor::CurrentModifiers(),out)) {
                                s.envelopeMouseStart={io.MousePos.x,io.MousePos.y};envelopeSeen=true;
                            }
                        }
                    }
                }
            }
            bool keyHit=false;
            if (s.keyDrag.active && s.keyDrag.draft.original.parent==clip.id) {
                keySeen=true;
                bool valid=!clip.locked && !track.locked;
                auto validate=[&](const editor::Transaction &drag) {
                    const auto found=FindClipKey(clip.keys,drag.draft.original.first,drag.draft.target);
                    valid &= found && !found->locked;
                };
                Tick minTick=s.keyDrag.draft.original.first,maxTick=minTick;
                validate(s.keyDrag);
                for (const auto &member:s.keyCompanions.first(s.keyCompanionCount)) {
                    validate(member);minTick=std::min(minTick,member.draft.original.first);maxTick=std::max(maxTick,member.draft.original.first);
                }
                if (!valid || minTick<0 || maxTick>clip.duration) EndClipKeys(s,p.revision,true,out);
                else if (s.keyDrag.draft.phase!=editor::Phase::Cancel && s.keyDrag.draft.phase!=editor::Phase::Commit) {
                    Tick delta=editor::FromSeconds((io.MousePos.x-s.keyMouseStart)/s.canvas.scale.x);
                    delta=std::clamp(delta,-minTick,clip.duration-maxTick);
                    if (s.snapping && s.snapToFrame) {
                        const Tick candidate=s.keyDrag.draft.original.first+delta;
                        delta=std::clamp(editor::FrameToTick(editor::TickToFrame(candidate,s.time.rate),s.time.rate)-s.keyDrag.draft.original.first,-minTick,clip.duration-maxTick);
                    }
                    if (out.storage.size()-out.count<1+s.keyCompanionCount) out.overflow=true;
                    else {
                        auto update=[&](editor::Transaction &drag) {auto value=drag.draft.original;value.first+=delta;
                            if (!(value==drag.draft.proposed)) drag.Update(p.revision,value,out);};
                        update(s.keyDrag);for (auto &member:s.keyCompanions.first(s.keyCompanionCount)) update(member);
                    }
                }
            }
            const auto firstTick=editor::FromSeconds((view.min.x+s.headerWidth-7-x)/s.canvas.scale.x);
            const auto lastTick=editor::FromSeconds((view.max.x+7-x)/s.canvas.scale.x);
            auto visibleFirst=std::lower_bound(clip.keys.begin(),clip.keys.end(),firstTick,[](const auto &key,Tick tick){return key.tick<tick;});
            auto visibleLast=std::upper_bound(visibleFirst,clip.keys.end(),lastTick,[](Tick tick,const auto &key){return tick<key.tick;});
            auto drawKey=[&](const editor::Keyframe &key) {
                Tick tick=key.tick;
                if (s.keyDrag.active && s.keyDrag.draft.original.parent==clip.id && s.keyDrag.draft.phase!=editor::Phase::Cancel) {
                    if (s.keyDrag.draft.target==key.id) tick=s.keyDrag.draft.proposed.first;
                    for (const auto &member:s.keyCompanions.first(s.keyCompanionCount))
                        if (member.draft.target==key.id) tick=member.draft.proposed.first;
                }
                if (tick<0 || tick>clip.duration) return;
                const float kx=x+float(editor::Seconds(tick)*s.canvas.scale.x),ky=b.y-9;
                const bool selectedKey=s.keySelection && s.keySelection->Contains(key.id);
                draw->AddQuadFilled({kx,ky-4},{kx+4,ky},{kx,ky+4},{kx-4,ky},
                    ImGui::GetColorU32(selectedKey ? theme.editor.selectedKey : theme.editor.key));
                if (selectedKey) draw->AddCircle({kx,ky},7,ImGui::GetColorU32(theme.colors.focus));
                bool hovered=false;
                if (kx>=view.min.x+s.headerWidth && kx<=view.max.x && ky>=view.min.y && ky<=view.max.y) {
                    const auto cursor=ImGui::GetCursorScreenPos();
                    ImGui::SetCursorScreenPos({kx-6,ky-6});
                    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(clip.id)));
                    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(key.id)));
                    ImGui::InvisibleButton("key",{12,12});hovered=ImGui::IsItemHovered();
                    ImGui::PopID();ImGui::PopID();ImGui::SetCursorScreenPos(cursor);ImGui::Dummy({0,0});
                }
                keyHit |= hovered;
                if (hovered) {
                    ImGui::SetTooltip("%s",s.labels.dragKey);
                    if (ImGui::IsMouseClicked(0) && !key.locked && !clip.locked && !track.locked &&
                        !s.keyDrag.active && !s.drag.active && !s.transitionDrag.active && !s.captionDrag.active) {
                        bool selected=true;
                        if (s.keySelection && (!s.keySelection->Contains(key.id) || io.KeyCtrl))
                            selected=s.keySelection->Set(key.id,io.KeyCtrl,io.KeyCtrl);
                        if (selected && s.keySelection && !s.keySelection->Contains(key.id))
                            return; // Ctrl-click removed this key; selection changes do not begin edits.
                        std::size_t count=0;bool valid=selected;
                        for (const auto &member:clip.keys) if (member.id!=key.id && s.keySelection && s.keySelection->Contains(member.id)) {
                            ++count;valid &= !member.locked && member.tick>=0 && member.tick<=clip.duration;
                        }
                        if (!selected || count>s.keyCompanions.size() || out.storage.size()-out.count<count+1) out.overflow=true;
                        else if (valid) {
                            auto begin=[&](editor::Transaction &drag,const editor::Keyframe &k) {
                                editor::Value original;original.first=k.tick;original.x=k.value;original.parent=clip.id;
                                drag.Begin(k.id,p.revision,editor::EditKind::Keyframe,original,editor::CurrentModifiers(),out);
                            };
                            begin(s.keyDrag,key);s.keyCompanionCount=0;
                            for (const auto &member:clip.keys) if (member.id!=key.id && s.keySelection && s.keySelection->Contains(member.id))
                                begin(s.keyCompanions[s.keyCompanionCount++],member);
                            s.keyMouseStart=io.MousePos.x;keySeen=true;
                        }
                    }
                }
            };
            for (auto key=visibleFirst;key!=visibleLast;++key) drawKey(*key);
            if (s.keyDrag.active && s.keyDrag.draft.original.parent==clip.id) {
                auto drawMoved=[&](const editor::Transaction &drag) {
                    const auto key=FindClipKey(clip.keys,drag.draft.original.first,drag.draft.target);
                    if (key && (key->tick<firstTick || key->tick>lastTick)) drawKey(*key);
                };
                drawMoved(s.keyDrag);for (const auto &drag:s.keyCompanions.first(s.keyCompanionCount)) drawMoved(drag);
            }
            Tick transitionIn=clip.transitionIn,transitionOut=clip.transitionOut;
            const bool editingTransition=s.transitionDrag.active && s.transitionDrag.draft.target==clip.id;
            if (editingTransition) {
                transitionSeen=true;
                if (track.locked || clip.locked) s.transitionDrag.Cancel(out);
                else if (s.transitionDrag.draft.phase!=editor::Phase::Cancel &&
                         s.transitionDrag.draft.phase!=editor::Phase::Commit) {
                    auto proposed=s.transitionDrag.draft.original;
                    const auto kind=s.transitionEnd ? clip.transitionOutKind : clip.transitionInKind;
                    const bool centered=p.transitionLimit && (kind==TransitionKind::Dissolve || kind==TransitionKind::Crossfade);
                    const Tick delta=editor::FromSeconds((io.MousePos.x-s.transitionMouseStart)/s.canvas.scale.x*(centered ? 2. : 1.));
                    auto original=clip;original.transitionIn=proposed.first;original.transitionOut=proposed.last;
                    auto edit=EditTransition(original,s.transitionEnd,s.transitionEnd ? -delta : delta);
                    if (edit.valid && p.transitionLimit) {
                        auto &duration=s.transitionEnd ? edit.outDuration : edit.inDuration;
                        duration=std::min(duration,std::max<Tick>(0,p.transitionLimit(p.user,clip.id,s.transitionEnd)));
                    }
                    if (!edit.valid) s.transitionDrag.Cancel(out);
                    else {
                        proposed.first=edit.inDuration;proposed.last=edit.outDuration;
                        if (!(proposed==s.transitionDrag.draft.proposed)) s.transitionDrag.Update(p.revision,proposed,out);
                    }
                }
                if (s.transitionDrag.draft.phase!=editor::Phase::Cancel) {
                    transitionIn=s.transitionDrag.draft.proposed.first;
                    transitionOut=s.transitionDrag.draft.proposed.last;
                }
            }
            bool transitionHit=false;
            for (int side=0;side<2;++side) {
                const auto type=side ? clip.transitionOutKind : clip.transitionInKind;
                const bool centered=p.transitionLimit && (type==TransitionKind::Dissolve || type==TransitionKind::Crossfade);
                const double extent=centered ? .5 : 1.;
                const float handleX=side ? end-float(editor::Seconds(transitionOut)*s.canvas.scale.x*extent) :
                                          x+float(editor::Seconds(transitionIn)*s.canvas.scale.x*extent);
                const float handleScale=ImGui::GetFontSize()/14.f;
                const float radius=4*handleScale;
                const ImVec2 handle{handleX,a.y+6*handleScale};
                const char *badge=type==TransitionKind::None ? "-" : type==TransitionKind::Dissolve ? "D" :
                                  type==TransitionKind::Fade ? "F" : "X";
                if ((side ? transitionOut : transitionIn)>0) {
                    const ImVec2 position{handleX+(side ? -22.f : 6.f)*handleScale,a.y};
                    if (!TransitionBadge(draw,s.icons,type,side!=0,position,16*handleScale))
                        draw->AddText(position,ImGui::GetColorU32(theme.colors.text),badge);
                }
                draw->AddLine(side ? ImVec2{handleX,b.y} : a,
                              side ? ImVec2{end,a.y} : ImVec2{handleX,b.y},ImGui::GetColorU32(theme.colors.text),2);
                draw->AddRect({handle.x-radius,handle.y-radius},{handle.x+radius,handle.y+radius},
                              ImGui::GetColorU32(theme.editor.marker),1,0,editingTransition ? 2.f : 1.f);
                bool hovered=false;
                if (handleX>=view.min.x+s.headerWidth && handleX<=view.max.x && handle.y>=view.min.y && handle.y<=view.max.y) {
                    const auto cursor=ImGui::GetCursorScreenPos();
                    ImGui::SetCursorScreenPos({handle.x-6*handleScale,handle.y-6*handleScale});
                    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(clip.id)));
                    ImGui::InvisibleButton(side ? "transition-out" : "transition-in",{12*handleScale,12*handleScale});
                    hovered=ImGui::IsItemHovered();ImGui::PopID();ImGui::SetCursorScreenPos(cursor);ImGui::Dummy({0,0});
                }
                transitionHit |= hovered;
                if (hovered) {
                    ImGui::BeginTooltip();
                    if (s.icons) {Icon(*s.icons,IconId::TransitionDuration,{16*handleScale});ImGui::SameLine();}
                    ImGui::TextUnformatted(side ? s.labels.transitions.outDuration : s.labels.transitions.inDuration);
                    ImGui::EndTooltip();
                    if (ImGui::IsMouseClicked(0) && !track.locked && !clip.locked && !s.drag.active && !s.transitionDrag.active) {
                        editor::Value original;original.first=clip.transitionIn;original.last=clip.transitionOut;
                        if (s.transitionDrag.Begin(clip.id,p.revision,editor::EditKind::TransitionDuration,original,editor::CurrentModifiers(),out)) {
                            s.transitionEnd=side==1;s.transitionMouseStart=io.MousePos.x;transitionSeen=true;
                        }
                    }
                }
            }
            bool hit=false;
            const ImVec2 hitMin{std::max(x,view.min.x+s.headerWidth),std::max(a.y,view.min.y)};
            const ImVec2 hitMax{std::min(end,view.max.x),std::min(b.y,view.max.y)};
            if (hitMax.x>hitMin.x && hitMax.y>hitMin.y) {
                const auto cursor=ImGui::GetCursorScreenPos();ImGui::SetCursorScreenPos(hitMin);
                ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(clip.id)));
                ImGui::InvisibleButton("clip-body",{hitMax.x-hitMin.x,hitMax.y-hitMin.y});
                hit=ImGui::IsItemHovered();
                ImGui::PopID();
                ImGui::SetCursorScreenPos(cursor);ImGui::Dummy({0,0});
            }
            if (hit) {
                s.hovered = clip.id;
                if (!io.MouseDown[0]) {
                    ImGui::BeginTooltip();ImGui::TextUnformatted(clip.label);
                    if(s.tool==Tool::Select && (std::abs(io.MousePos.x-x)<7 || std::abs(io.MousePos.x-end)<7)) {
                        const bool start=std::abs(io.MousePos.x-x)<std::abs(io.MousePos.x-end);
                        if(s.icons) {Icon(*s.icons,start?IconId::TrimStart:IconId::TrimEnd,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
                        ImGui::TextUnformatted(start?s.labels.trimStart:s.labels.trimEnd);
                    }
                    if(s.icons) {Icon(*s.icons,IconId::PlaybackSpeed,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
                    ImGui::Text("%s: %s   %s: %.3gx",s.labels.duration,duration,s.labels.speed,value.x);
                    if (clip.linked) ImGui::Text("%s: %llu",s.labels.linked,static_cast<unsigned long long>(clip.linked));
                    if (clip.group) ImGui::Text("%s: %llu",s.labels.group,static_cast<unsigned long long>(clip.group));
                    if (clip.proxy) {
                        if (s.icons) {Icon(*s.icons,IconId::ProxyMedia,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
                        ImGui::TextUnformatted(s.labels.proxy);
                    }
                    if (clip.missing) ImGui::TextUnformatted(s.labels.missing);
                    if (clip.offline) {
                        if(s.icons) {Icon(*s.icons,IconId::Disconnected,{ImGui::GetFontSize()});ImGui::SameLine();}
                        ImGui::TextUnformatted(s.labels.offline);
                    }
                    if (track.locked || clip.locked) ImGui::TextUnformatted(s.labels.locked);
                    ImGui::EndTooltip();
                }
            }
            ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(clip.id)));
            if (hit && !envelopeHit && !keyHit && ImGui::IsMouseClicked(1)) {
                s.envelopeContextTick=std::clamp(editor::FromSeconds((io.MousePos.x-x)/s.canvas.scale.x),Tick{0},std::max(Tick{0},clip.duration));
                ImGui::OpenPopup("transition-picker");
            }
            if (ImGui::BeginPopup("transition-picker")) {
                if(track.kind==TrackKind::Audio && s.icons) {Icon(*s.icons,IconId::VolumeEnvelope,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
                if (track.kind==TrackKind::Audio && ImGui::MenuItem(s.labels.addEnvelope,nullptr,false,!clip.locked && !track.locked && !s.envelopeDrag.active))
                    EnvelopeAction(out,clip,p.revision,clip.id,s.envelopeContextTick,EvaluateEnvelope(clip.envelope,s.envelopeContextTick),1);
                const auto relationItem=[&](const char *label,int relation,bool enabled) {
                    if (s.icons) {
                        Icon(*s.icons,relation ? IconId::Layers : IconId::Link,
                             {ImGui::GetFontSize(),ImGui::GetStyleColorVec4(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled)});
                        ImGui::SameLine();
                    }
                    return ImGui::MenuItem(label,nullptr,false,enabled);
                };
                    for (int relation=0;relation<2;++relation) {
                        const auto set=relation ? clip.group : clip.linked;
                        if (relationItem(relation ? s.labels.ungroup : s.labels.unlink,relation,
                            set && !track.locked && !clip.locked && !s.drag.active)) {
                            if (ReserveEvents(out,2)) {
                                editor::Value original;original.parent=set;original.offset=relation;
                                editor::Transaction edit;
                                edit.Begin(clip.id,p.revision,editor::EditKind::Link,original,editor::CurrentModifiers(),out);
                                edit.draft.proposed.parent=0;edit.Commit(p.revision,out);
                            }
                        }
                    }
                    for (int relation=0;relation<2;++relation)
                        if (relationItem(relation ? s.labels.groupSelection : s.labels.linkSelection,relation,
                            selection.count>1 && selection.Contains(clip.id) && p.selected && !s.drag.active && !track.locked && !clip.locked)) {
                            const auto ids=selection.storage.first(selection.count);
                            const auto members=p.selected(p.user,ids);
                            bool complete=true,editable=true;
                            for (const auto id:ids)
                                complete &= std::count_if(members.begin(),members.end(),[&](const auto &m){return m.id==id;})==1;
                            for (const auto &member:members) editable &= !member.locked;
                            if (!complete) out.overflow=true;
                            else if (editable && ReserveEvents(out,members.size()*2))
                                for (const auto &member:members) {
                                    editor::Value original;original.parent=relation ? member.group : member.linked;original.offset=relation+2;
                                    editor::Transaction edit;
                                    edit.Begin(member.id,p.revision,editor::EditKind::Link,original,editor::CurrentModifiers(),out);
                                    edit.draft.proposed.parent=0;edit.Commit(p.revision,out);
                                }
                        }
                if(s.icons) {Icon(*s.icons,IconId::RippleDelete,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
                if(ImGui::MenuItem(s.labels.rippleDelete,nullptr,false,selection.Contains(clip.id) && p.selected && !clip.locked && !track.locked && !s.drag.active)) {
                    const auto ids=selection.storage.first(selection.count);const auto members=p.selected(p.user,ids);
                    bool complete=!members.empty(),editable=true;
                    for(auto id:ids) complete &= std::count_if(members.begin(),members.end(),[&](const auto &m){return m.id==id;})==1;
                    for(const auto &member:members) editable &= !member.locked && (!p.isEditable || p.isEditable(p.user,member.id));
                    if(!complete) out.overflow=true;
                    else if(editable && ReserveEvents(out,members.size()*2)) for(const auto &member:members) {
                        editor::Transaction action;action.Begin(member.id,p.revision,editor::EditKind::RippleDelete,{member.start,member.duration},editor::CurrentModifiers(),out);
                        action.Commit(p.revision,out);
                    }
                }
                TransitionPicker("types",clip,p.revision,out,track.locked,{s.icons,s.labels.transitions});
                ImGui::EndPopup();
            }
            ImGui::PopID();
            if (hit && s.tool!=Tool::Hand && !transitionHit && !s.transitionDrag.active && !s.captionDrag.active &&
                track.kind==TrackKind::Caption && ImGui::IsMouseDoubleClicked(0) && !track.locked && !clip.locked) {
                EndDrags(s,p.revision,true,out);
                if (std::strlen(clip.label)>=sizeof(s.caption)) out.overflow=true;
                else {
                    editor::Event begin{clip.id,p.revision,editor::Phase::Begin,editor::EditKind::Rename};
                    std::snprintf(begin.originalText.data(),begin.originalText.size(),"%s",clip.label);
                    begin.proposedText=begin.originalText;begin.modifiers=editor::CurrentModifiers();
                    if (out.Push(begin)) {
                        s.captionDrag.draft=begin;s.captionDrag.active=true;s.editingCaption=clip.id;
                        s.captionFocus=true;captionSeen=true;std::snprintf(s.caption,sizeof(s.caption),"%s",clip.label);
                    }
                }
            }
            if (hit && s.tool!=Tool::Hand && !envelopeHit && !s.envelopeDrag.active && !keyHit && !s.keyDrag.active && !transitionHit && !s.transitionDrag.active && s.editingCaption != clip.id && ImGui::IsMouseClicked(0) && !track.locked &&
                !clip.locked && !s.drag.active) {
                if ((!selection.Contains(clip.id) || io.KeyCtrl) && !selection.Set(clip.id,io.KeyCtrl,io.KeyCtrl)) {
                    out.overflow=true;continue;
                }
                if (!selection.Contains(clip.id)) continue; // Toggle-off is selection only.
                auto kind = io.KeyAlt ? editor::EditKind::Duplicate : editor::EditKind::Move;
                if (io.MousePos.x - x < 7)
                    kind = editor::EditKind::TrimStart;
                else if (end - io.MousePos.x < 7)
                    kind = editor::EditKind::TrimEnd;
                if (s.tool == Tool::Ripple)
                    kind = editor::EditKind::Ripple;
                if (s.tool == Tool::Slip)
                    kind = editor::EditKind::Slip;
                if (s.tool == Tool::Roll)
                    kind = editor::EditKind::Roll;
                if (s.tool == Tool::Slide)
                    kind = editor::EditKind::Slide;
                if (p.canBeginEdit && !p.canBeginEdit(p.user,clip.id,s.tool==Tool::Razor ? editor::EditKind::Split : kind)) continue;
                if (s.tool == Tool::Razor) {
                    auto split = Value(clip);
                    split.first = editor::FromSeconds(
                        s.canvas.origin.x + (io.MousePos.x - view.min.x - s.headerWidth) / s.canvas.scale.x);
                    const std::span<const StableId> ids(&clip.id,1);
                    const auto members=p.selected ? p.selected(p.user,ids) : std::span<const ClipView>(&clip,1);
                    SplitBatch(p,members,ids,split.first,out);
                } else if (s.tool != Tool::Hand) {
                    auto neighbors =
                        p.neighbors ? p.neighbors(p.user, clip.id) : TimelineProvider::Neighbors{};
                    bool adjacent = kind == editor::EditKind::Roll || kind == editor::EditKind::Slide;
                    bool available = !adjacent || (neighbors.next && !neighbors.next->locked &&
                                                   clip.start + clip.duration == neighbors.next->start);
                    if (kind == editor::EditKind::Slide)
                        available = available && neighbors.previous && !neighbors.previous->locked &&
                                    neighbors.previous->start + neighbors.previous->duration == clip.start;
                    auto members = (kind == editor::EditKind::Move || kind == editor::EditKind::Duplicate ||
                                           kind == editor::EditKind::TrimStart || kind == editor::EditKind::TrimEnd || kind == editor::EditKind::Slip || kind == editor::EditKind::Ripple || adjacent) &&
                                           p.selected
                                       ? p.selected(p.user, selection.storage.first(selection.count))
                                       : std::span<const ClipView>{};
                    // A selection query must resolve the complete edit set, even offscreen.
                    // Reject incomplete or ambiguous host scratch before publishing any Begin.
                    if (kind == editor::EditKind::Move || kind == editor::EditKind::Duplicate ||
                                           kind == editor::EditKind::TrimStart || kind == editor::EditKind::TrimEnd || kind == editor::EditKind::Slip || kind == editor::EditKind::Ripple || adjacent) {
                        if (!p.selected && selection.count > 1) {
                            out.overflow = true;
                            available = false;
                        }
                        if (p.selected) {
                            for (const auto id : selection.storage.first(selection.count))
                                if (std::count_if(members.begin(), members.end(),
                                                  [&](const auto &member) { return member.id == id; }) != 1) {
                                    out.overflow = true;
                                    available = false;
                                }
                            for (std::size_t i = 0; i < members.size(); ++i)
                                if (std::any_of(members.begin(), members.begin() + i,
                                                [&](const auto &member) { return member.id == members[i].id; })) {
                                    out.overflow = true;
                                    available = false;
                                }
                        }
                    }
                    std::size_t memberCount = 0;
                    for (const auto &member : members)
                        if (member.id != clip.id) {
                            ++memberCount;
                            available &= !member.locked && (!p.canBeginEdit || p.canBeginEdit(p.user,member.id,kind));
                        }
                    if (memberCount > s.memberDrags.size()) {
                        out.overflow = true;
                        available = false;
                    }
                    std::size_t memberNeighbors=0;
                    if (adjacent && available) {
                        std::size_t index=0;
                        for (const auto &member:members) if (member.id!=clip.id) {
                            auto &entry=s.memberDrags[index];entry={};entry.original=member;
                            const auto n=p.neighbors ? p.neighbors(p.user,member.id) : TimelineProvider::Neighbors{};
                            if (!n.next || n.next->locked || member.start+member.duration!=n.next->start) {available=false;break;}
                            entry.next=*n.next;++memberNeighbors;
                            if (kind==editor::EditKind::Slide) {
                                if (!n.previous || n.previous->locked || n.previous->start+n.previous->duration!=member.start) {available=false;break;}
                                entry.previous=*n.previous;++memberNeighbors;
                            }
                            for (const auto id : {entry.original.id,entry.previous.id,entry.next.id}) if (id) {
                                if ((p.isEditable && !p.isEditable(p.user,id)) || (p.canBeginEdit && !p.canBeginEdit(p.user,id,kind))) available=false;
                                if (id==clip.id || (neighbors.next && id==neighbors.next->id) ||
                                    (kind==editor::EditKind::Slide && neighbors.previous && id==neighbors.previous->id)) available=false;
                                for (std::size_t j=0;j<index;++j) {
                                    const auto &prior=s.memberDrags[j];
                                    if (id==prior.original.id || id==prior.previous.id || id==prior.next.id) available=false;
                                }
                            }
                            ++index;
                        }
                    }
                    const std::size_t required = 1 + (adjacent ? 1 : 0) +
                                                 (kind == editor::EditKind::Slide ? 1 : 0) + memberCount+memberNeighbors;
                    if (available && ReserveEvents(out, required)) {
                        s.memberCount = 0;
                        s.original = clip;
                        s.mouseStart = {io.MousePos.x, io.MousePos.y};
                        s.drag.Begin(clip.id, p.revision, kind, Value(clip), editor::CurrentModifiers(), out);
                        if (adjacent) {
                            s.nextOriginal = *neighbors.next;
                            s.nextDrag.Begin(neighbors.next->id, p.revision, kind, Value(*neighbors.next),
                                             editor::CurrentModifiers(), out);
                        }
                        if (kind == editor::EditKind::Slide) {
                            s.previousOriginal = *neighbors.previous;
                            s.previousDrag.Begin(neighbors.previous->id, p.revision, kind,
                                                 Value(*neighbors.previous), editor::CurrentModifiers(), out);
                        }
                        {
                            for (const auto &member : members)
                                if (member.id != clip.id) {
                                    auto &drag = s.memberDrags[s.memberCount++];
                                    if (!adjacent) drag={};
                                    drag.original = member;
                                    if (adjacent) drag.nextTransaction.Begin(drag.next.id,p.revision,kind,Value(drag.next),editor::CurrentModifiers(),out);
                                    if (kind==editor::EditKind::Slide) drag.previousTransaction.Begin(drag.previous.id,p.revision,kind,Value(drag.previous),editor::CurrentModifiers(),out);
                                    drag.transaction.Begin(member.id, p.revision, kind, Value(member),
                                                           editor::CurrentModifiers(), out);
                                }
                        }
                    }
                }
            }
        }
        // Overlay cut-spanning bounds after all visible clip bodies so either
        // side remains visible. This reuses the borrowed visible query only.
        if (p.transitionLimit) for (const auto &clip:clips) {
            for (int side=0;side<2;++side) {
                const auto kind=side ? clip.transitionOutKind : clip.transitionInKind;
                if (kind!=TransitionKind::Dissolve && kind!=TransitionKind::Crossfade) continue;
                Tick duration=side ? clip.transitionOut : clip.transitionIn;
                if (s.transitionDrag.active && s.transitionDrag.draft.target==clip.id &&
                    s.transitionDrag.draft.phase!=editor::Phase::Cancel)
                    duration=side ? s.transitionDrag.draft.proposed.last : s.transitionDrag.draft.proposed.first;
                if (duration<=0) continue;
                const double cut=editor::Seconds(clip.start)+(side ? editor::Seconds(clip.duration) : 0.);
                const float center=view.min.x+s.headerWidth+float((cut-s.canvas.origin.x)*s.canvas.scale.x);
                const float half=float(editor::Seconds(duration)*s.canvas.scale.x*.5);
                const float bottom=y+rowHeight-8,top=bottom-10;
                const auto color=ImGui::GetColorU32(theme.editor.marker);
                draw->AddRect({center-half,top},{center+half,bottom},color);
                draw->AddLine({center-half,bottom},{center+half,top},color);
                draw->AddLine({center-half,top},{center+half,bottom},color);
            }
        }
        draw->PopClipRect();
    }
    if (s.heightDrag.active && !heightEditorSeen && s.heightDrag.draft.phase!=editor::Phase::Commit &&
        s.heightDrag.draft.phase!=editor::Phase::Cancel) s.heightDrag.Cancel(out);
    if (s.envelopeDrag.active) {
        if (!envelopeSeen) s.envelopeDrag.Cancel(out);
        else if (!out.overflow && !ImGui::IsMouseDown(0) && s.envelopeDrag.draft.phase!=editor::Phase::Cancel)
            s.envelopeDrag.Commit(p.revision,out);
    }
    if (s.keyDrag.active) {
        if (!keySeen) EndClipKeys(s,p.revision,true,out);
        else if (!out.overflow && !ImGui::IsMouseDown(0) && s.keyDrag.draft.phase!=editor::Phase::Cancel) EndClipKeys(s,p.revision,false,out);
    }
    if (s.captionDrag.active && !captionSeen) {
        s.captionDrag.draft.proposedText=s.captionDrag.draft.originalText;s.captionDrag.Cancel(out);
    }
    if (s.transitionDrag.active) {
        if (!transitionSeen) s.transitionDrag.Cancel(out);
        else if (!out.overflow && !ImGui::IsMouseDown(0) && s.transitionDrag.draft.phase!=editor::Phase::Cancel)
            s.transitionDrag.Commit(p.revision,out);
    }
    if (s.drag.active && s.drag.draft.phase != editor::Phase::Cancel &&
        s.drag.draft.phase != editor::Phase::Commit && ReserveEvents(out, ActiveDrags(s))) {
        Tick delta = editor::FromSeconds((io.MousePos.x - s.mouseStart.x) / s.canvas.scale.x);
        s.guide = {};
        if (s.snapping) {
            auto candidates=p.snap && s.magnet ? p.snap(p.user,range) : std::span<const editor::SnapCandidate>{};
            s.guide=ResolveTimelineSnap(s,delta,candidates,selection.storage.first(selection.count));
            Tick anchor=s.original.start;
            if (s.drag.draft.kind==editor::EditKind::TrimEnd || s.drag.draft.kind==editor::EditKind::Ripple ||
                s.drag.draft.kind==editor::EditKind::Roll) anchor+=s.original.duration;
            if (s.guide.snapped) delta=s.guide.tick-anchor;
        }
        auto constraints = p.constraints ? p.constraints(p.user, s.original.id) : ClipConstraints{};
        const bool trimMembers=s.drag.draft.kind==editor::EditKind::TrimStart || s.drag.draft.kind==editor::EditKind::TrimEnd ||
            s.drag.draft.kind==editor::EditKind::Slip || s.drag.draft.kind==editor::EditKind::Ripple;
        if (trimMembers && s.memberCount) {
            const auto constrain=[&](const ClipView &clip,ClipConstraints limits) {
                const auto edit=EditClip(clip,s.drag.draft.kind,delta,limits);
                if (!edit.valid) {delta=0;return;}
                Tick applied=s.drag.draft.kind==editor::EditKind::TrimStart ? edit.start-clip.start : edit.duration-clip.duration;
                if (s.drag.draft.kind==editor::EditKind::Slip) {
                    const auto before=AvailableTicks(static_cast<std::uint64_t>(clip.sourceIn)-static_cast<std::uint64_t>(limits.mediaFirst),clip.speed);
                    const auto after=AvailableTicks(static_cast<std::uint64_t>(limits.mediaLast)-static_cast<std::uint64_t>(clip.sourceIn),clip.speed)-clip.duration;
                    applied=std::clamp(delta,-before,after);
                }
                delta=delta>=0 ? std::min(delta,applied) : std::max(delta,applied);
            };
            constrain(s.original,constraints);
            for (const auto &member:s.memberDrags.first(s.memberCount))
                constrain(member.original,p.constraints ? p.constraints(p.user,member.original.id) : ClipConstraints{});
        }
        const bool relatedAdjacent=s.drag.draft.kind==editor::EditKind::Roll || s.drag.draft.kind==editor::EditKind::Slide;
        if (relatedAdjacent && s.memberCount) {
            const auto limits=[&](StableId id){return p.constraints ? p.constraints(p.user,id) : ClipConstraints{};};
            const auto constrain=[&](const ClipView &previous,const ClipView &current,const ClipView &next) {
                Tick applied=0;
                if (s.drag.draft.kind==editor::EditKind::Roll) {
                    const auto pair=RollClips(current,next,delta,limits(current.id),limits(next.id));
                    if (pair.valid) applied=pair.left.duration-current.duration;
                } else {
                    const auto triple=SlideClip(previous,current,next,delta,limits(previous.id),limits(current.id),limits(next.id));
                    if (triple.valid) applied=triple.current.start-current.start;
                }
                delta=delta>=0 ? std::min(delta,applied) : std::max(delta,applied);
            };
            constrain(s.previousOriginal,s.original,s.nextOriginal);
            for (const auto &member:s.memberDrags.first(s.memberCount)) constrain(member.previous,member.original,member.next);
        }
        auto edit = EditClip(s.original, s.drag.draft.kind, delta, constraints);
        auto proposed = edit.valid ? Value(edit, s.original.track, s.original.speed) : s.drag.draft.original;
        if (s.drag.draft.kind == editor::EditKind::Roll && s.nextDrag.active) {
            auto pair =
                RollClips(s.original, s.nextOriginal, delta, constraints,
                          p.constraints ? p.constraints(p.user, s.nextOriginal.id) : ClipConstraints{});
            if (pair.valid) {
                proposed = Value(pair.left, s.original.track, s.original.speed);
                auto next = Value(pair.right, s.nextOriginal.track, s.nextOriginal.speed);
                if (ImGui::IsMouseDown(0) && !(next == s.nextDrag.draft.proposed))
                    s.nextDrag.Update(p.revision, next, out);
            }
        }
        if (s.drag.draft.kind == editor::EditKind::Slide && s.previousDrag.active && s.nextDrag.active) {
            auto triple = SlideClip(
                s.previousOriginal, s.original, s.nextOriginal, delta,
                p.constraints ? p.constraints(p.user, s.previousOriginal.id) : ClipConstraints{}, constraints,
                p.constraints ? p.constraints(p.user, s.nextOriginal.id) : ClipConstraints{});
            if (triple.valid) {
                proposed = Value(triple.current, s.original.track, s.original.speed);
                auto previous = Value(triple.previous, s.previousOriginal.track, s.previousOriginal.speed),
                     next = Value(triple.next, s.nextOriginal.track, s.nextOriginal.speed);
                if (ImGui::IsMouseDown(0)) {
                    if (!(previous == s.previousDrag.draft.proposed))
                        s.previousDrag.Update(p.revision, previous, out);
                    if (!(next == s.nextDrag.draft.proposed))
                        s.nextDrag.Update(p.revision, next, out);
                }
            }
        }
        if (ImGui::IsMouseDown(0) && !(proposed == s.drag.draft.proposed))
            s.drag.Update(p.revision, proposed, out);
        for (auto &member : s.memberDrags.first(s.memberCount)) {
            auto value = Value(member.original);
            if (relatedAdjacent) {
                const auto limits=[&](StableId id){return p.constraints ? p.constraints(p.user,id) : ClipConstraints{};};
                auto update=[&](editor::Transaction &tx,const ClipEdit &edit,const ClipView &source) {
                    const auto proposed=Value(edit,source.track,source.speed);
                    if (ImGui::IsMouseDown(0) && !(proposed==tx.draft.proposed)) tx.Update(p.revision,proposed,out);
                };
                if (s.drag.draft.kind==editor::EditKind::Roll) {
                    const auto pair=RollClips(member.original,member.next,delta,limits(member.original.id),limits(member.next.id));
                    if (pair.valid) {value=Value(pair.left,member.original.track,member.original.speed);update(member.nextTransaction,pair.right,member.next);}
                } else {
                    const auto triple=SlideClip(member.previous,member.original,member.next,delta,limits(member.previous.id),limits(member.original.id),limits(member.next.id));
                    if (triple.valid) {value=Value(triple.current,member.original.track,member.original.speed);update(member.previousTransaction,triple.previous,member.previous);update(member.nextTransaction,triple.next,member.next);}
                }
            } else if (trimMembers) {
                const auto edit=EditClip(member.original,s.drag.draft.kind,delta,
                    p.constraints ? p.constraints(p.user,member.original.id) : ClipConstraints{});
                if (edit.valid) value=Value(edit,member.original.track,member.original.speed);
            } else {
                value.first += delta;
                value.last += delta;
            }
            if (ImGui::IsMouseDown(0) && !(value == member.transaction.draft.proposed))
                member.transaction.Update(p.revision, value, out);
        }
        if (s.guide.snapped) {
            float x =
                view.min.x + s.headerWidth +
                static_cast<float>((editor::Seconds(s.guide.candidate.tick) - s.canvas.origin.x) * s.canvas.scale.x);
            draw->AddLine({x, view.min.y}, {x, view.max.y}, ImGui::GetColorU32(theme.editor.snapGuide), 2);
        }
    }
    if (s.drag.active && !ImGui::IsMouseDown(0))
        EndDrags(s, p.revision, s.drag.draft.phase == editor::Phase::Cancel, out);
    float playhead =
        view.min.x + s.headerWidth +
        static_cast<float>((editor::Seconds(s.time.playhead) - s.canvas.origin.x) * s.canvas.scale.x);
    draw->AddLine({playhead, view.min.y}, {playhead, view.max.y}, ImGui::GetColorU32(theme.colors.accent), 2);
    if(s.drag.active && ImGui::IsMouseDown(0)) {
        ImGui::BeginTooltip();
        ImGui::Text("%+.3f s / %.3f s",editor::Seconds(s.drag.draft.proposed.first-s.drag.draft.original.first),
            editor::Seconds(s.drag.draft.proposed.last-s.drag.draft.proposed.first));
        ImGui::EndTooltip();
    }
    editor::EndCanvas();
    ImGui::PopID();
}
void TransitionPicker(const char *id, const ClipView &clip, std::uint64_t revision,
                      editor::EventBuffer &events, bool trackLocked) {
    TransitionPicker(id,clip,revision,events,trackLocked,{});
}
void TransitionPicker(const char *id,const ClipView &clip,std::uint64_t revision,
                      editor::EventBuffer &events,bool trackLocked,const TransitionPickerOptions &options) {
    ImGui::PushID(id);
    ImGui::BeginDisabled(clip.locked || trackLocked);
    const auto &names=options.labels.kinds;
    if (options.icons) {
        Icon(*options.icons,IconId::Transition,{16*ImGui::GetFontSize()/14});ImGui::SameLine();
        ImGui::TextUnformatted(options.labels.title);
    }
    editor::Value original;original.first=static_cast<Tick>(clip.transitionInKind);
    original.last=static_cast<Tick>(clip.transitionOutKind);
    for (int side=0;side<2;++side) {
        int selected=static_cast<int>(side ? clip.transitionOutKind : clip.transitionInKind);
        bool changed=false;
        if (ImGui::BeginCombo(side ? options.labels.out : options.labels.in,names[selected])) {
            const IconId icons[]{IconId::Transition,IconId::Dissolve,side ? IconId::FadeOut : IconId::FadeIn,IconId::Crossfade};
            for (int i=0;i<4;++i) {
                if (options.icons && i) {Icon(*options.icons,icons[i],{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
                if (ImGui::Selectable(names[i],selected==i)) {selected=i;changed=true;}
                if (selected==i) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (changed) {
            if (events.storage.size()-events.count<2) events.overflow=true;
            else {
                auto proposed=original;
                (side ? proposed.last : proposed.first)=selected;
                editor::Transaction edit;
                edit.Begin(clip.id,revision,editor::EditKind::TransitionType,original,editor::CurrentModifiers(),events);
                edit.draft.proposed=proposed;edit.Commit(revision,events);
            }
        }
    }
    ImGui::EndDisabled();ImGui::PopID();
}
void MonitorControls(const char *id,MonitorOptions &options,const IconAtlas *icons,const MonitorLabels &labels) {
    ImGui::PushID(id);
    auto toggle=[&](IconId icon,const char *label,bool &value) {
        if (icons) {Icon(*icons,icon,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
        ImGui::Checkbox(label,&value);
    };
    toggle(IconId::SafeArea,labels.safeArea,options.safeArea);
    toggle(IconId::Guides,labels.guides,options.guides);
    ImGui::Checkbox(labels.timecode,&options.showTimecode);
    toggle(IconId::TransformBounds,labels.bounds,options.transform);
    bool anchor=options.showAnchor.value_or(options.transform);
    const bool before=anchor;toggle(IconId::AnchorPoint,labels.anchor,anchor);
    if (anchor!=before) options.showAnchor=anchor;
    if (icons) {Icon(*icons,IconId::MetadataOverlay,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
    int preset=static_cast<int>(options.metadataPreset);
    if (ImGui::Combo(labels.metadata,&preset,labels.presets.data(),3)) options.metadataPreset=static_cast<MonitorMetadataPreset>(preset);
    ImGui::PopID();
}
void Monitor(const char *id, ImTextureRef texture, ImVec2 size, const editor::TimeState &time,
             const MonitorOptions &o, const Theme &theme) {
    ImGui::PushID(id);
    auto p = ImGui::GetCursorScreenPos();
    if (texture.GetTexID())
        ImGui::Image(texture, size, o.flipY ? ImVec2{0,1} : ImVec2{0,0}, o.flipY ? ImVec2{1,0} : ImVec2{1,1});
    else
        ImGui::Dummy(size);
    auto *d = ImGui::GetWindowDrawList();
    d->PushClipRect(p, {p.x + size.x, p.y + size.y}, true);
    if (!texture.GetTexID())
        d->AddRectFilled(p, {p.x + size.x, p.y + size.y}, ImGui::GetColorU32(theme.colors.canvas));
    auto color = ImGui::GetColorU32(theme.colors.muted);
    if (o.safeArea)
        for (float inset : {.05f, .1f})
            d->AddRect({p.x + size.x * inset, p.y + size.y * inset},
                       {p.x + size.x * (1 - inset), p.y + size.y * (1 - inset)}, color);
    if (o.guides)
        for (int i = 1; i < 3; ++i) {
            d->AddLine({p.x + size.x * i / 3, p.y}, {p.x + size.x * i / 3, p.y + size.y}, color);
            d->AddLine({p.x, p.y + size.y * i / 3}, {p.x + size.x, p.y + size.y * i / 3}, color);
        }
    if (o.transform) {
        const auto &bounds=o.transformBounds;
        const ImVec2 minimum{p.x+float(size.x*bounds.min.x),p.y+float(size.y*bounds.min.y)};
        const ImVec2 maximum{p.x+float(size.x*bounds.max.x),p.y+float(size.y*bounds.max.y)};
        if (std::isfinite(minimum.x) && std::isfinite(minimum.y) && std::isfinite(maximum.x) && std::isfinite(maximum.y) &&
            minimum.x<maximum.x && minimum.y<maximum.y)
            d->AddRect(minimum,maximum,ImGui::GetColorU32(theme.colors.accent),0.f,2.f,ImDrawFlags_None);
    }
    if (o.showAnchor.value_or(o.transform)) {
        const ImVec2 anchor{p.x+size.x*o.anchor.x,p.y+size.y*o.anchor.y};
        if (std::isfinite(anchor.x) && std::isfinite(anchor.y)) d->AddCircle(anchor,5,color);
    }
    const float padding=std::max(3.f,ImGui::GetFontSize()*.25f);
    const float lineHeight=ImGui::GetFontSize()+2*padding;
    auto overlayText=[&](float top,const char *label) {
        if (!label || !*label || size.x<=4*padding || size.y<lineHeight+2*padding) return;
        const ImVec2 minimum{p.x+padding,top};
        const ImVec2 maximum{std::min(p.x+size.x-padding,minimum.x+ImGui::CalcTextSize(label).x+2*padding),top+lineHeight};
        auto background=theme.colors.surface;background.w=1;
        d->AddRectFilled(minimum,maximum,ImGui::GetColorU32(background),theme.metrics.radius);
        const ImVec4 bounds{minimum.x+padding,minimum.y+padding,maximum.x-padding,maximum.y-padding};
        d->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{bounds.x,bounds.y},ImGui::GetColorU32(theme.colors.text),label,nullptr,0,&bounds);
    };
    overlayText(p.y+padding,o.label);
    if (o.metadataPreset!=MonitorMetadataPreset::Off) {
        float top=p.y+padding+((o.label && *o.label) ? lineHeight+padding : 0);
        const float bottom=p.y+size.y-padding-(o.showTimecode ? lineHeight+padding : 0);
        auto line=[&](const char *label) {
            if (label && *label && top+lineHeight<=bottom) {
                overlayText(top,label);top+=lineHeight+padding;
            }
        };
        line(o.clipName);line(o.markerComment);
        if (o.metadataPreset==MonitorMetadataPreset::Details)
            for (const char *label:o.metadata) {
                if (top+lineHeight>bottom) break;
                line(label);
            }
    }
    if (o.showTimecode && (!o.label || !*o.label || size.y>=2*lineHeight+3*padding)) {
        char label[32]{};
        editor::FormatTimecode(time.playhead, time.rate, time.dropFrame, label);
        overlayText(p.y+size.y-lineHeight-padding,label);
    }
    d->PopClipRect();
    ImGui::PopID();
}
void BuildAudioBuckets(std::span<const float> pcm, int channels, int channel, std::span<AudioBucket> out) {
    std::fill(out.begin(), out.end(), AudioBucket{});
    if (channels <= 0 || channel < 0 || channel >= channels || out.empty())
        return;
    auto frames = pcm.size() / channels;
    for (std::size_t i = 0; i < out.size(); ++i) {
        auto first = i * frames / out.size(), last = (i + 1) * frames / out.size();
        if (first == last)
            continue;
        auto &b = out[i];
        float firstSample = pcm[first * channels + channel];
        b.minimum = b.maximum = std::isfinite(firstSample) ? firstSample : 0.f;
        double sum = 0;
        for (auto j = first; j < last; ++j) {
            float v = pcm[j * channels + channel];
            if (!std::isfinite(v))
                v = 0;
            b.minimum = (std::min)(b.minimum, v);
            b.maximum = (std::max)(b.maximum, v);
            b.peak = (std::max)(b.peak, std::abs(v));
            sum += v * v;
        }
        b.rms = static_cast<float>(std::sqrt(sum / (last - first)));
    }
}
void UpdateMeter(MeterState &s, std::span<const float> samples, float dt, float hold) {
    AudioBucket bucket;
    BuildAudioBuckets(samples, 1, 0, {&bucket, 1});
    s.rms = bucket.rms;
    s.peak = bucket.peak;
    s.holdRemaining = (std::max)(0.f, s.holdRemaining - dt);
    if (s.peak >= s.heldPeak) {
        s.heldPeak = s.peak;
        s.holdRemaining = hold;
    } else if (s.holdRemaining == 0)
        s.heldPeak = (std::max)(s.peak, s.heldPeak - dt * .5f);
}
AudioBucket WaveformPixel(const WaveformView &view, editor::Range range) {
    AudioBucket result{};
    if (view.status!=WaveformStatus::Ready || view.buckets.empty() || view.range.last<=view.range.first || range.last<=range.first) return result;
    const long double extent=static_cast<long double>(view.range.last)-view.range.first;
    const auto index=[&](Tick t) {return (static_cast<long double>(t)-view.range.first)*view.buckets.size()/extent;};
    const auto first=static_cast<std::size_t>(std::clamp(std::floor(index(range.first)),0.L,static_cast<long double>(view.buckets.size())));
    const auto last=static_cast<std::size_t>(std::clamp(std::ceil(index(range.last)),0.L,static_cast<long double>(view.buckets.size())));
    for (auto i=first;i<last;++i) {
        const auto &v=view.buckets[i];
        if (std::isfinite(v.minimum)) result.minimum=std::min(result.minimum,v.minimum);
        if (std::isfinite(v.maximum)) result.maximum=std::max(result.maximum,v.maximum);
        if (std::isfinite(v.peak)) result.peak=std::max(result.peak,v.peak);
        if (std::isfinite(v.rms)) result.rms=std::max(result.rms,v.rms);
    }
    return result;
}
void DrawWaveform(const WaveformView &view, editor::Range range, ImVec2 min, ImVec2 max,
                  const WaveformOptions &options, const Theme &theme) {
    if (max.x<=min.x || max.y<=min.y) return;
    auto *d=ImGui::GetWindowDrawList();
    const float left=std::max(min.x,d->GetClipRectMin().x),right=std::min(max.x,d->GetClipRectMax().x);
    if(right<=left) return;
    const auto at=[&](float x) {return static_cast<Tick>(static_cast<long double>(range.first)+
        (static_cast<long double>(range.last)-range.first)*(x-min.x)/(max.x-min.x));};
    range={at(left),at(right)};min.x=left;max.x=right;
    d->PushClipRect(min,max,true);
    const float center=(min.y+max.y)*.5f, amplitude=(max.y-min.y)*.46f;
    if (options.zeroLine) d->AddLine({min.x,center},{max.x,center},ImGui::GetColorU32(theme.colors.muted));
    if (view.status!=WaveformStatus::Ready) {
        d->AddText(min,ImGui::GetColorU32(view.status==WaveformStatus::Error ? theme.colors.destructive : theme.colors.muted),
            view.status==WaveformStatus::Error ? "!" : "...");
    } else {
        const int pixels=std::max(1,static_cast<int>(std::ceil(max.x-min.x)));
        const auto time=[&](int i) {return range.first+static_cast<Tick>((static_cast<long double>(range.last)-range.first)*i/pixels);};
        const float gain=std::isfinite(options.gain) ? std::clamp(options.gain,0.f,64.f) : 1.f;
        for (int i=0;i<pixels;++i) {
            const auto v=WaveformPixel(view,{time(i),time(i+1)});
            const float low=std::clamp(v.minimum*gain,-1.f,1.f), high=std::clamp(v.maximum*gain,-1.f,1.f);
            if (low!=0 || high!=0) d->AddRectFilled({min.x+i,center-high*amplitude},
                {min.x+i+1, std::max(center-low*amplitude,center-high*amplitude+1)},ImGui::GetColorU32(theme.colors.text));
        }
    }
    d->PopClipRect();
}
void Waveform(const char *id, std::span<const AudioBucket> buckets, ImVec2 size, const Theme &theme) {
    if (size.x<=0 || size.y<=0) return;
    const auto p=ImGui::GetCursorScreenPos();ImGui::InvisibleButton(id,size);
    const editor::Range range{0,static_cast<Tick>(buckets.size())*editor::TicksPerSecond};
    DrawWaveform({buckets,range,WaveformStatus::Ready},range,p,{p.x+size.x,p.y+size.y},{},theme);
}
void LevelMeter(const char *id, const MeterState &left, const MeterState &right, ImVec2 size,
                const Theme &t) {
    auto p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    auto *d = ImGui::GetWindowDrawList();
    int i = 0;
    for (auto s : {left, right}) {
        float x = p.x + i++ * size.x * .5f, w = size.x * .45f;
        auto level = [](float value) {
            return std::clamp((20 * std::log10((std::max)(value, 1e-6f)) + 60) / 60, 0.f, 1.f);
        };
        d->AddRectFilled({x, p.y}, {x + w, p.y + size.y}, ImGui::GetColorU32(t.colors.input));
        d->AddRectFilled({x, p.y + size.y * (1 - level(s.rms))}, {x + w, p.y + size.y},
                         ImGui::GetColorU32(s.peak >= 1 ? t.colors.destructive : t.colors.success));
        float hold = p.y + size.y * (1 - level(s.heldPeak));
        d->AddLine({x, hold}, {x + w, hold}, ImGui::GetColorU32(t.colors.warning), 2);
    }
}
void AudioStrip(const AudioStripView &v, std::uint64_t revision, editor::PropertyState &s,
                editor::EventBuffer &out) {
    detail::ResumeTerminal(s.drag, revision, out);
    if (s.drag.active && (v.locked || ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                         (s.drag.draft.target != v.gainId && s.drag.draft.target != v.panId)))
        s.drag.Cancel(out);
    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(v.id)));
    ImGui::BeginGroup();
    ImGui::TextUnformatted(v.label);
    ImGui::BeginDisabled(v.locked);
    auto process = [&](StableId target,double original,double draft,bool changed) {
        if (!target) return;
        if (ImGui::IsItemActivated() && !s.drag.active)
            s.drag.Begin(target,revision,editor::EditKind::Property,{0,0,0,0,original},
                         editor::CurrentModifiers(),out);
        if (s.drag.active && s.drag.draft.target==target) {
            if (changed) s.drag.Update(revision,{0,0,0,0,draft},out);
            if (ImGui::IsItemDeactivated()) s.drag.Commit(revision,out);
        }
    };
    double gain=s.drag.active && s.drag.draft.target==v.gainId ? s.drag.draft.proposed.x : v.gain;
    double minimum=0,maximum=4;
    ImGui::BeginDisabled(!v.gainId);
    bool changed=ImGui::VSliderScalar(v.gainLabel,{32,100},ImGuiDataType_Double,&gain,&minimum,&maximum,"%.2f");
    process(v.gainId,v.gain,gain,changed);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginGroup();
    double pan=s.drag.active && s.drag.draft.target==v.panId ? s.drag.draft.proposed.x : v.pan;
    minimum=-1;maximum=1;
    ImGui::SetNextItemWidth(ImGui::GetFontSize()*10);
    ImGui::BeginDisabled(!v.panId || v.panId==v.gainId);
    changed=ImGui::SliderScalar(v.panLabel,ImGuiDataType_Double,&pan,&minimum,&maximum,"%.2f");
    process(v.panId,v.pan,pan,changed);
    ImGui::EndDisabled();
    const bool flags[]={v.mute,v.solo,v.record};
    const TrackControl controls[]={TrackControl::Mute,TrackControl::Solo,TrackControl::Record};
    const char *labels[]={v.muteLabel,v.soloLabel,v.recordLabel};
    for (int i=0;i<3;++i) {
        bool value=flags[i];
        ImGui::PushID(i);
        if(s.icons) {const IconId icons[]{IconId::Mute,IconId::Solo,IconId::Microphone};Icon(*s.icons,icons[i],{ImGui::GetFontSize()});ImGui::SameLine();}
        if (ImGui::Checkbox(labels[i],&value))
            out.Push({v.id,revision,editor::Phase::Commit,editor::EditKind::Toggle,
                      {0,0,0,0,static_cast<double>(controls[i]),flags[i] ? 1. : 0.},
                      {0,0,0,0,static_cast<double>(controls[i]),value ? 1. : 0.},editor::CurrentModifiers()});
        ImGui::PopID();
    }
    ImGui::EndGroup();
    ImGui::EndDisabled();
    ImGui::EndGroup();
    ImGui::PopID();
}
bool BuildScopes(std::span<const Rgba> pixels, int w, int h, ScopeBuffers out) {
    if (w <= 0 || h <= 0 || pixels.size() != static_cast<std::size_t>(w) * h || out.red.size() != 256 ||
        out.green.size() != 256 || out.blue.size() != 256 || out.luma.size() != 256 ||
        out.waveform.size() != static_cast<std::size_t>(w) * 256 || out.vectorscope.size() != 65536)
        return false;
    const auto waveSize = static_cast<std::size_t>(w) * 256;
    const bool rgb = !out.redWaveform.empty() || !out.greenWaveform.empty() || !out.blueWaveform.empty();
    if (rgb && (out.redWaveform.size() != waveSize || out.greenWaveform.size() != waveSize ||
                out.blueWaveform.size() != waveSize))
        return false;
    for (auto span : {out.red, out.green, out.blue, out.luma, out.waveform, out.vectorscope,
                     out.redWaveform, out.greenWaveform, out.blueWaveform})
        std::fill(span.begin(), span.end(), 0);
    auto bin = [](float value) {
        return static_cast<int>(std::clamp(std::isfinite(value) ? value : 0.f, 0.f, 1.f) * 255);
    };
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        auto p = pixels[i];
        p.r = std::isfinite(p.r) ? p.r : 0.f;
        p.g = std::isfinite(p.g) ? p.g : 0.f;
        p.b = std::isfinite(p.b) ? p.b : 0.f;
        float y = .2126f * p.r + .7152f * p.g + .0722f * p.b;
        ++out.red[bin(p.r)];
        ++out.green[bin(p.g)];
        ++out.blue[bin(p.b)];
        ++out.luma[bin(y)];
        ++out.waveform[bin(y) * w + i % w];
        if (rgb) {
            ++out.redWaveform[bin(p.r) * w + i % w];
            ++out.greenWaveform[bin(p.g) * w + i % w];
            ++out.blueWaveform[bin(p.b) * w + i % w];
        }
        ++out.vectorscope[bin(.5f + (p.r - y) * .635f) * 256 + bin(.5f + (p.b - y) * .539f)];
    }
    return true;
}
void Histogram(const char *id, std::span<const std::uint32_t> bins, ImVec2 size, const Theme &t) {
    auto p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    if (bins.empty())
        return;
    float maximum = static_cast<float>((std::max)(1u, *std::max_element(bins.begin(), bins.end())));
    auto *d = ImGui::GetWindowDrawList();
    for (std::size_t i = 0; i < bins.size(); ++i) {
        float x = p.x + size.x * i / bins.size();
        d->AddLine({x, p.y + size.y}, {x, p.y + size.y * (1 - bins[i] / maximum)},
                   ImGui::GetColorU32(t.colors.accent));
    }
}
void ScopeImage(const char *id, std::span<const std::uint32_t> bins, int w, int h, ImVec2 size,
                const Theme &t) {
    ScopeImage(id, bins, w, h, size, t, t.colors.success);
}
void ScopeImage(const char *id, std::span<const std::uint32_t> bins, int w, int h, ImVec2 size,
                const Theme &, ImVec4 tint) {
    auto p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    if (w <= 0 || h <= 0 || bins.size() != static_cast<std::size_t>(w) * h)
        return;
    auto *d = ImGui::GetWindowDrawList();
    auto max = *std::max_element(bins.begin(), bins.end());
    if (!max)
        return;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (bins[y * w + x]) {
                auto c = tint;
                c.w *= static_cast<float>(std::log1p(bins[y * w + x]) / std::log1p(max));
                d->AddRectFilled({p.x + x * size.x / w, p.y + (h - 1 - y) * size.y / h},
                                 {p.x + (x + 1) * size.x / w, p.y + (h - y) * size.y / h},
                                 ImGui::GetColorU32(c));
            }
}
namespace {
editor::Value ColorValue(const float *rgb) { return {0,0,0,0,rgb[0],rgb[1],rgb[2]}; }
void ColorEvent(StableId target, editor::Value original, editor::Value proposed, bool changed,
                std::uint64_t revision, ColorState *state, editor::EventBuffer *events) {
    if (!state || !events || !target)
        return;
    auto &drag = state->drag;
    if (ImGui::IsItemActivated() && !drag.active)
        drag.Begin(target, revision, editor::EditKind::Property, original, editor::CurrentModifiers(), *events);
    if (drag.active && drag.draft.target == target) {
        if (changed)
            drag.Update(revision, proposed, *events);
        if (ImGui::IsItemDeactivated())
            drag.Commit(revision, *events);
    }
}
bool Wheel(float *rgb, float diameter, ImVec2 &center) {
    constexpr float pi = 3.14159265358979323846f;
    const auto p = ImGui::GetCursorScreenPos();
    float radius = diameter * .5f - 4;
    center = {p.x + diameter * .5f, p.y + diameter * .5f};
    ImGui::InvisibleButton("wheel", {diameter,diameter});
    const float mean = (rgb[0] + rgb[1] + rgb[2]) / 3;
    float x = rgb[0] - mean, y = (rgb[1] - rgb[2]) / std::sqrt(3.f);
    bool changed = false;
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(0)) {
        auto mouse = ImGui::GetIO().MousePos;
        x = (mouse.x - center.x) / radius;
        y = (center.y - mouse.y) / radius;
        float length = std::hypot(x,y);
        if (length > 1) { x /= length; y /= length; }
        if (ImGui::IsMouseDoubleClicked(0)) x = y = 0;
        float candidate[] = {mean + x, mean - .5f*x + std::sqrt(3.f)*.5f*y,
                             mean - .5f*x - std::sqrt(3.f)*.5f*y};
        for (int i=0; i<3; ++i) {
            changed |= rgb[i] != candidate[i];
            rgb[i] = candidate[i];
        }
    }
    auto *draw = ImGui::GetWindowDrawList();
    for (int ring=0; ring<8; ++ring)
        for (int sector=0; sector<48; ++sector) {
            float a = sector * 2*pi/48, b = (sector+1) * 2*pi/48;
            float inner = radius*ring/8, outer = radius*(ring+1)/8;
            float hue = (sector+.5f)/48, saturation = (ring+.5f)/8;
            ImVec4 color;
            ImGui::ColorConvertHSVtoRGB(hue,saturation,.85f,color.x,color.y,color.z);
            color.w = 1;
            auto point = [&](float angle, float r) {
                return ImVec2{center.x+std::cos(angle)*r, center.y-std::sin(angle)*r};
            };
            draw->AddQuadFilled(point(a,inner),point(a,outer),point(b,outer),point(b,inner),
                                ImGui::GetColorU32(color));
        }
    const auto border = ImGui::GetColorU32(ImGuiCol_Border);
    draw->AddCircle(center,radius,border,64,1.5f);
    draw->AddLine({center.x-4,center.y},{center.x+4,center.y},border);
    draw->AddLine({center.x,center.y-4},{center.x,center.y+4},border);
    float length = std::hypot(x,y);
    if (length>1) { x/=length; y/=length; }
    ImVec2 thumb{center.x+x*radius,center.y-y*radius};
    draw->AddCircleFilled(thumb,5,ImGui::GetColorU32(ImGuiCol_WindowBg));
    draw->AddCircle(thumb,5,ImGui::GetColorU32(ImGuiCol_Text),16,2);
    if (ImGui::IsItemFocused())
        draw->AddRect(p,{p.x+diameter,p.y+diameter},ImGui::GetColorU32(ImGuiCol_NavCursor),4.f,2.f,ImDrawFlags_None);
    return changed;
}
bool ColorPanel(const char *id, ColorValues &draft, const ColorValues &original,
                const ColorPropertyIds &ids, std::uint64_t revision, ColorState *state,
                editor::EventBuffer *events, const ColorLabels &labels) {
    ImGui::PushID(id);
    bool changed = false;
    float *rgb[] = {draft.lift,draft.gamma,draft.gain};
    const float *source[] = {original.lift,original.gamma,original.gain};
    const StableId targets[] = {ids.lift,ids.gamma,ids.gain};
    const char *names[] = {labels.lift,labels.gamma,labels.gain};
    const int columns = ImGui::GetContentRegionAvail().x < ImGui::GetFontSize()*30 ? 1 : 3;
    if (ImGui::BeginTable("wheels",columns)) {
        for (int i=0; i<3; ++i) {
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            ImGui::TextUnformatted(names[i]);
            ImGui::BeginDisabled(state && !targets[i]);
            float diameter = std::clamp(ImGui::GetContentRegionAvail().x, 64.f, ImGui::GetFontSize()*8);
            ImVec2 center;
            bool edited = Wheel(rgb[i],diameter,center);
            ColorEvent(targets[i],ColorValue(source[i]),ColorValue(rgb[i]),edited,revision,state,events);
            changed |= edited;
            if (state) { state->wheelCenters[i]=center; state->wheelRadius=diameter*.5f-4; }
            ImGui::SetNextItemWidth(diameter);
            float mean = (rgb[i][0]+rgb[i][1]+rgb[i][2])/3, before = mean;
            edited = ImGui::SliderFloat(labels.level,&mean,i==0 ? -1.f : .01f,i==0 ? 1.f : 4.f);
            if (edited) for (int c=0;c<3;++c) rgb[i][c] += mean-before;
            ColorEvent(targets[i],ColorValue(source[i]),ColorValue(rgb[i]),edited,revision,state,events);
            changed |= edited;
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    float *scalars[] = {&draft.temperature,&draft.tint,&draft.exposure};
    float values[] = {original.temperature,original.tint,original.exposure};
    StableId scalarIds[] = {ids.temperature,ids.tint,ids.exposure};
    const char *scalarNames[] = {labels.temperature,labels.tint,labels.exposure};
    for (int i=0; i<3; ++i) {
        ImGui::PushID(i+3);
        ImGui::BeginDisabled(state && !scalarIds[i]);
        bool edited = ImGui::SliderFloat(scalarNames[i],scalars[i],i==2 ? -10.f : -1.f,i==2 ? 10.f : 1.f);
        ColorEvent(scalarIds[i],{0,0,0,0,values[i]},{0,0,0,0,*scalars[i]},edited,revision,state,events);
        changed |= edited;
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}
}
Rgba ApplyColorCurves(Rgba input,const ColorCurveSet &curves) {
    auto evaluate=[](float value,std::span<const editor::Keyframe> keys) {
        const double normalized=std::isfinite(value) ? std::clamp(double(value),0.,1.) : 0.;
        const double result=keys.empty() ? normalized : editor::Evaluate(keys,editor::FromSeconds(normalized));
        return std::isfinite(result) ? float(std::clamp(result,0.,1.)) : 0.f;
    };
    return {evaluate(input.r,curves.red),evaluate(input.g,curves.green),evaluate(input.b,curves.blue),input.a};
}
bool ColorControls(const char *id, ColorValues &draft) {
    const ColorValues original = draft;
    return ColorPanel(id,draft,original,{},0,nullptr,nullptr,{});
}
void ColorControls(const char *id, const ColorValues &values, const ColorPropertyIds &ids,
                   std::uint64_t revision, ColorState &state, editor::EventBuffer &events,
                   const ColorLabels &labels) {
    detail::ResumeTerminal(state.drag,revision,events);
    if (state.drag.active && ImGui::IsKeyPressed(ImGuiKey_Escape)) state.drag.Cancel(events);
    auto draft = values;
    if (state.drag.active) {
        const auto &event = state.drag.draft;
        float *rgb = event.target == ids.lift ? draft.lift : event.target == ids.gamma ? draft.gamma :
                     event.target == ids.gain ? draft.gain : nullptr;
        if (rgb) { rgb[0]=static_cast<float>(event.proposed.x); rgb[1]=static_cast<float>(event.proposed.y);
                   rgb[2]=static_cast<float>(event.proposed.z); }
        else if (event.target == ids.temperature) draft.temperature=static_cast<float>(event.proposed.x);
        else if (event.target == ids.tint) draft.tint=static_cast<float>(event.proposed.x);
        else if (event.target == ids.exposure) draft.exposure=static_cast<float>(event.proposed.x);
        else state.drag.Cancel(events);
    }
    ColorPanel(id,draft,values,ids,revision,&state,&events,labels);
}
} // namespace imkit::video
