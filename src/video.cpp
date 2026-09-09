#include <imkit/video.h>
#include "transaction_support.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
namespace imkit::video {
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
editor::Value Value(const ClipView &c) {
    return {c.start, c.start + c.duration, c.sourceIn, c.track, c.speed};
}
editor::Value Value(const ClipEdit &e, StableId track, double speed) {
    return {e.start, e.start + e.duration, e.sourceIn, track, speed};
}
void Toggle(editor::EventBuffer &out, const TrackView &track, std::uint64_t revision, int field, bool value) {
    out.Push({track.id,
              revision,
              editor::Phase::Commit,
              editor::EditKind::Toggle,
              {0, 0, 0, 0, static_cast<double>(field), value ? 1. : 0.},
              {0, 0, 0, 0, static_cast<double>(field), value ? 0. : 1.},
              editor::CurrentModifiers()});
}
std::size_t ActiveDrags(const TimelineState &s) {
    std::size_t count = s.drag.active + s.previousDrag.active + s.nextDrag.active;
    for (const auto &member : s.memberDrags.first(s.memberCount))
        count += member.transaction.active;
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
    for (auto &member : s.memberDrags.first(s.memberCount))
        finish(member.transaction);
}
} // namespace
float TrackExtent(const TrackView &track) {
    return track.expanded ? (std::max)(64.f,std::isfinite(track.height) ? track.height : 64.f) : 32.f;
}
ClipEdit EditClip(const ClipView &c, editor::EditKind kind, Tick delta, ClipConstraints bounds) {
    ClipEdit result{c.start, c.duration, c.sourceIn, false};
    if (c.locked || c.duration < bounds.minimumDuration || c.speed <= 0 || !std::isfinite(c.speed) ||
        bounds.mediaLast < bounds.mediaFirst)
        return result;
    if (c.sourceIn < bounds.mediaFirst || c.sourceIn > bounds.mediaLast ||
        c.duration * c.speed > bounds.mediaLast - c.sourceIn)
        return result;
    Tick before = static_cast<Tick>(std::floor((c.sourceIn - bounds.mediaFirst) / c.speed));
    Tick after = static_cast<Tick>(std::floor((bounds.mediaLast - c.sourceIn) / c.speed)) - c.duration;
    switch (kind) {
    case editor::EditKind::Move:
    case editor::EditKind::Duplicate:
        result.start += delta;
        break;
    case editor::EditKind::TrimStart:
        delta = std::clamp(delta, -before, c.duration - bounds.minimumDuration);
        result.start += delta;
        result.duration -= delta;
        result.sourceIn += static_cast<Tick>(std::llround(delta * c.speed));
        break;
    case editor::EditKind::TrimEnd:
    case editor::EditKind::Ripple:
        delta = std::clamp(delta, bounds.minimumDuration - c.duration, after);
        result.duration += delta;
        result.rippleDelta = kind == editor::EditKind::Ripple ? delta : 0;
        break;
    case editor::EditKind::Slip:
        delta = std::clamp(delta, -before, after);
        result.sourceIn += static_cast<Tick>(std::llround(delta * c.speed));
        break;
    default:
        return result;
    }
    result.valid = true;
    return result;
}
PairEdit RollClips(const ClipView &a, const ClipView &b, Tick delta, ClipConstraints ac, ClipConstraints bc) {
    if (a.start + a.duration != b.start || a.locked || b.locked)
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
    Tick offset = tick - c.start;
    if (c.locked || offset < bounds.minimumDuration || c.duration - offset < bounds.minimumDuration)
        return {};
    return {{c.start, offset, c.sourceIn, true},
            {tick, c.duration - offset, c.sourceIn + static_cast<Tick>(std::llround(offset * c.speed)), true},
            true};
}
TripleEdit SlideClip(const ClipView &a, const ClipView &b, const ClipView &c, Tick delta, ClipConstraints ac,
                     ClipConstraints bc, ClipConstraints cc) {
    if (a.start + a.duration != b.start || b.start + b.duration != c.start || a.locked || b.locked ||
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
    const char *tools[] = {"Select", "Razor", "Ripple", "Roll", "Slip", "Slide", "Hand"};
    for (int i = 0; i < 7; ++i) {
        if (i)
            ImGui::SameLine();
        if (i==0 && s.icons) {
            const bool active=s.tool==Tool::Select;
            if (active) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            if (IconButton("select-tool",*s.icons,IconId::SelectPointer,"Select clips")) s.tool=Tool::Select;
            if (active) ImGui::PopStyleColor();
        } else if (ImGui::Selectable(tools[i], static_cast<int>(s.tool) == i, 0, {56, 24}))
            s.tool = static_cast<Tool>(i);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &s.snapping);
    ImGui::SameLine();
    if (s.icons) {
        const bool active=s.magnet;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (IconButton("magnet", *s.icons, IconId::Magnet, "Magnet: snap to timeline targets")) s.magnet=!s.magnet;
        if (active) ImGui::PopStyleColor();
    } else ImGui::Checkbox("Magnet", &s.magnet);
    ImGui::SameLine();
    if (ImGui::Button("Timeline options")) ImGui::OpenPopup("snap-options");
    if (ImGui::BeginPopup("snap-options")) {
        int follow=static_cast<int>(s.autoScroll);
        if (ImGui::Combo("Follow playhead",&follow,"Off\0Smooth\0Page\0"))
            s.autoScroll=static_cast<editor::AutoScroll>(follow);
        ImGui::Checkbox("Frame grid", &s.snapToFrame);
        const char *names[]={"Frame", "Playhead", "Marker", "Clip edge", "Keyframe", "In/out", "Selection edge"};
        for (unsigned i=1;i<7;++i) {
            bool enabled=(s.snapKinds & (1u<<i))!=0;
            if (ImGui::Checkbox(names[i],&enabled)) s.snapKinds ^= 1u<<i;
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    bool fit=s.icons ? IconButton("fit",*s.icons,IconId::FitView,"Fit timeline") : ImGui::Button("Fit");
    fit |= editor::CommandPressed(editor::Command::Fit,s.bindings,
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
    if (fit && p.contentRange.last>p.contentRange.first) {
        const double width=timelineWidth-s.headerWidth;
        if (width>48) {
            const double first=editor::Seconds(p.contentRange.first),last=editor::Seconds(p.contentRange.last);
            s.canvas.scale.x=(width-48)/(last-first);
            s.canvas.origin.x=first-24/s.canvas.scale.x;
        }
    }
    if (s.time.playing && !fit)
        s.canvas.origin.x=editor::FollowPlayhead(s.canvas.origin.x,
            (timelineWidth-s.headerWidth)/s.canvas.scale.x,
            editor::Seconds(s.time.playhead),s.autoScroll);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + s.headerWidth);
    editor::TimeRuler("time", s.time, s.canvas, p.markers, p.revision, out, theme);
    s.canvas.wheelZoom = ImGui::GetIO().KeyCtrl;
    s.canvas.wheelZoomY = false;
    auto view = editor::BeginCanvas("tracks", s.canvas, size, theme);
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
    detail::ResumeTerminal(s.heightDrag,p.revision,out);
    if (s.heightDrag.active && ImGui::IsKeyPressed(ImGuiKey_Escape)) s.heightDrag.Cancel(out);
    if (view.hovered && s.tool == Tool::Hand && ImGui::IsMouseDragging(0))
        s.canvas.origin.x -= io.MouseDelta.x / s.canvas.scale.x;
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
        if (cancel || s.drag.draft.phase == editor::Phase::Commit)
            EndDrags(s, p.revision, cancel, out);
    }
    double rowTop=layout.top;
    for (const auto &track : tracks) {
        const float rowHeight=p.layout ? TrackExtent(track) : s.rowHeight;
        float y = view.min.y + static_cast<float>(rowTop-s.verticalScroll);
        rowTop+=rowHeight;
        draw->AddRectFilled({view.min.x, y}, {view.max.x, y + rowHeight - 2},
                            ImGui::GetColorU32(theme.colors.surface));
        ImGui::SetCursorScreenPos({view.min.x + 4, y + 3});
        ImGui::PushID(reinterpret_cast<void *>(static_cast<std::uintptr_t>(track.id)));
        if (ImGui::SmallButton(track.expanded ? "v" : ">"))
            Toggle(out,track,p.revision,static_cast<int>(TrackControl::Expanded),track.expanded);
        ImGui::SameLine(0,4);
        ImGui::TextUnformatted(track.label);
        if (ImGui::BeginPopupContextItem("track layout")) {
            float height=s.heightDrag.active && s.heightDrag.draft.target==track.id ?
                         static_cast<float>(s.heightDrag.draft.proposed.x) : track.height;
            bool edited=ImGui::SliderFloat("Track height",&height,64,240,"%.0f px");
            if (ImGui::IsItemActivated())
                s.heightDrag.Begin(track.id,p.revision,editor::EditKind::TrackHeight,
                                   {0,0,0,0,track.height},editor::CurrentModifiers(),out);
            if (edited && s.heightDrag.active)
                s.heightDrag.Update(p.revision,{0,0,0,0,height},out);
            if (ImGui::IsItemDeactivated() && s.heightDrag.active)
                s.heightDrag.Commit(p.revision,out);
            ImGui::EndPopup();
        }
        if (track.expanded) {
        ImGui::SetCursorScreenPos({view.min.x + 4, y + 30});
        const char *labels[] = {"V", "M", "S", "L", "R", "T", "P"};
        const char *tooltips[]={"Visible","Mute","Solo","Locked","Record armed","Target track","Source patch"};
        const bool values[] = {track.visible, track.mute,   track.solo,
                               track.locked,  track.record, track.target, track.source};
        float controlsWidth=12;
        for (auto label:labels) controlsWidth+=ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x;
        if (controlsWidth>s.headerWidth-8) {
            if (ImGui::SmallButton("Controls")) ImGui::OpenPopup("track controls");
            if (ImGui::BeginPopup("track controls")) {
                for (int f=0;f<7;++f)
                    if (ImGui::MenuItem(tooltips[f],nullptr,values[f])) Toggle(out,track,p.revision,f,values[f]);
                ImGui::EndPopup();
            }
        } else for (int f = 0; f < 7; ++f) {
            if (f)
                ImGui::SameLine(0, 2);
            if (values[f]) ImGui::PushStyleColor(ImGuiCol_Button,theme.colors.accent);
            if (ImGui::SmallButton(labels[f]))
                Toggle(out, track, p.revision, f, values[f]);
            if (values[f]) {
                auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
                draw->AddLine({a.x+2,b.y-1},{b.x-2,b.y-1},ImGui::GetColorU32(theme.colors.text),2);
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s: %s",tooltips[f],values[f] ? "On" : "Off");
        }
        }
        ImGui::PopID();
        auto clips = p.clips ? p.clips(p.user, track.id, range) : std::span<const ClipView>{};
        draw->PushClipRect({view.min.x + s.headerWidth, y}, {view.max.x, y + rowHeight - 2}, true);
        for (const auto &clip : clips) {
            auto value = Value(clip);
            if (s.drag.active && s.drag.draft.target == clip.id)
                value = s.drag.draft.proposed;
            for (const auto &member : s.memberDrags.first(s.memberCount))
                if (member.transaction.active && member.original.id == clip.id)
                    value = member.transaction.draft.proposed;
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
            auto color = track.kind == TrackKind::Audio     ? theme.colors.success
                         : track.kind == TrackKind::Caption ? theme.colors.warning
                                                            : theme.colors.accent;
            color.w = selected ? .8f : .35f;
            draw->AddRectFilled(a, b, ImGui::GetColorU32(color), 4);
            draw->AddRect(a, b, ImGui::GetColorU32(selected ? theme.colors.focus : theme.colors.border), 4, 0,
                          selected ? 2.f : 1.f);
            if (track.expanded && clip.thumbnail.GetTexID())
                draw->AddImage(clip.thumbnail, {x + 3, y + 23}, {(std::min)(end - 3, x + 65), b.y - 3});
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
            } else draw->AddText({x+6,y+7},ImGui::GetColorU32(theme.colors.text),clip.label);
            if (clip.missing || clip.offline)
                draw->AddLine(a, b, ImGui::GetColorU32(theme.colors.destructive), 2);
            if (track.locked || clip.locked)
                draw->AddText({x + 6, b.y - 18}, ImGui::GetColorU32(theme.colors.muted), "Locked");
            if (clip.proxy)
                draw->AddText({end - 20, y + 7}, ImGui::GetColorU32(theme.colors.warning), "P");
            if (clip.linked || clip.group)
                draw->AddLine({x + 3, b.y - 3}, {end - 3, b.y - 3}, ImGui::GetColorU32(theme.colors.text));
            for (std::size_t j = 0; track.expanded && j < clip.waveform.size(); ++j) {
                float wx = x + (end - x) * static_cast<float>(j) / clip.waveform.size(),
                      amplitude = clip.waveform[j] * (rowHeight - 28) * .4f;
                draw->AddLine({wx, y + 40 - amplitude}, {wx, y + 40 + amplitude},
                              ImGui::GetColorU32(theme.colors.text));
            }
            for (std::size_t j = 0; track.expanded && j < clip.audioBuckets.size(); ++j) {
                float wx = x + (end-x) * static_cast<float>(j) / clip.audioBuckets.size();
                float centerY = (a.y+b.y)*.5f + 5;
                float amplitude = (b.y-a.y-26)*.5f;
                draw->AddLine({wx,centerY-amplitude*clip.audioBuckets[j].maximum},
                              {wx,centerY-amplitude*clip.audioBuckets[j].minimum},
                              ImGui::GetColorU32(theme.colors.text));
            }
            for (const auto &key : clip.keys) {
                float kx = x + static_cast<float>(editor::Seconds(key.tick) * s.canvas.scale.x);
                float ky = b.y - 9;
                draw->AddQuadFilled({kx, ky - 4}, {kx + 4, ky}, {kx, ky + 4}, {kx - 4, ky},
                                    ImGui::GetColorU32(theme.colors.warning));
            }
            Tick transitionIn=clip.transitionIn,transitionOut=clip.transitionOut;
            const bool editingTransition=s.transitionDrag.active && s.transitionDrag.draft.target==clip.id;
            if (editingTransition) {
                transitionSeen=true;
                if (track.locked || clip.locked) s.transitionDrag.Cancel(out);
                else if (s.transitionDrag.draft.phase!=editor::Phase::Cancel &&
                         s.transitionDrag.draft.phase!=editor::Phase::Commit) {
                    auto proposed=s.transitionDrag.draft.original;
                    const Tick delta=editor::FromSeconds((io.MousePos.x-s.transitionMouseStart)/s.canvas.scale.x);
                    auto original=clip;original.transitionIn=proposed.first;original.transitionOut=proposed.last;
                    const auto edit=EditTransition(original,s.transitionEnd,s.transitionEnd ? -delta : delta);
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
                const float handleX=side ? end-float(editor::Seconds(transitionOut)*s.canvas.scale.x) :
                                          x+float(editor::Seconds(transitionIn)*s.canvas.scale.x);
                const ImVec2 handle{handleX,a.y+6};
                const auto type=side ? clip.transitionOutKind : clip.transitionInKind;
                const char *badge=type==TransitionKind::None ? "-" : type==TransitionKind::Dissolve ? "D" :
                                  type==TransitionKind::Fade ? "F" : "X";
                if ((side ? transitionOut : transitionIn)>0)
                    draw->AddText({handleX+(side ? -14.f : 6.f),a.y+2},ImGui::GetColorU32(theme.colors.text),badge);
                draw->AddLine(side ? ImVec2{handleX,b.y} : a,
                              side ? ImVec2{end,a.y} : ImVec2{handleX,b.y},ImGui::GetColorU32(theme.colors.text),2);
                draw->AddRect({handle.x-4,handle.y-4},{handle.x+4,handle.y+4},
                              ImGui::GetColorU32(theme.editor.marker),1,0,editingTransition ? 2.f : 1.f);
                const bool hovered=view.hovered && handleX>=view.min.x+s.headerWidth &&
                    std::abs(io.MousePos.x-handle.x)<=6 && std::abs(io.MousePos.y-handle.y)<=6;
                transitionHit |= hovered;
                if (hovered) {
                    ImGui::SetTooltip(side ? "Transition out duration" : "Transition in duration");
                    if (ImGui::IsMouseClicked(0) && !track.locked && !clip.locked && !s.drag.active && !s.transitionDrag.active) {
                        editor::Value original;original.first=clip.transitionIn;original.last=clip.transitionOut;
                        if (s.transitionDrag.Begin(clip.id,p.revision,editor::EditKind::TransitionDuration,original,editor::CurrentModifiers(),out)) {
                            s.transitionEnd=side==1;s.transitionMouseStart=io.MousePos.x;transitionSeen=true;
                        }
                    }
                }
            }
            bool hit = view.hovered && io.MousePos.x >= (std::max)(x, view.min.x + s.headerWidth) &&
                       io.MousePos.x < end && io.MousePos.y >= a.y && io.MousePos.y < b.y;
            if (hit)
                s.hovered = clip.id;
            ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(clip.id)));
            if (hit && ImGui::IsMouseClicked(1)) ImGui::OpenPopup("transition-picker");
            if (ImGui::BeginPopup("transition-picker")) {
                TransitionPicker("types",clip,p.revision,out,track.locked);
                ImGui::EndPopup();
            }
            ImGui::PopID();
            if (hit && !transitionHit && !s.transitionDrag.active && !s.captionDrag.active &&
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
            if (hit && !transitionHit && !s.transitionDrag.active && s.editingCaption != clip.id && ImGui::IsMouseClicked(0) && !track.locked &&
                !clip.locked && !s.drag.active) {
                if (!selection.Contains(clip.id) || io.KeyCtrl)
                    selection.Set(clip.id, io.KeyCtrl, io.KeyCtrl);
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
                if (s.tool == Tool::Razor) {
                    auto split = Value(clip);
                    split.first = editor::FromSeconds(
                        s.canvas.origin.x + (io.MousePos.x - view.min.x - s.headerWidth) / s.canvas.scale.x);
                    out.Push({clip.id, p.revision, editor::Phase::Commit, editor::EditKind::Split,
                              Value(clip), split, editor::CurrentModifiers()});
                } else if (s.tool != Tool::Hand) {
                    auto neighbors =
                        p.neighbors ? p.neighbors(p.user, clip.id) : TimelineProvider::Neighbors{};
                    bool adjacent = kind == editor::EditKind::Roll || kind == editor::EditKind::Slide;
                    bool available = !adjacent || (neighbors.next && !neighbors.next->locked &&
                                                   clip.start + clip.duration == neighbors.next->start);
                    if (kind == editor::EditKind::Slide)
                        available = available && neighbors.previous && !neighbors.previous->locked &&
                                    neighbors.previous->start + neighbors.previous->duration == clip.start;
                    auto members = (kind == editor::EditKind::Move || kind == editor::EditKind::Duplicate) &&
                                           p.selected
                                       ? p.selected(p.user, selection.storage.first(selection.count))
                                       : std::span<const ClipView>{};
                    std::size_t memberCount = 0;
                    for (const auto &member : members)
                        if (member.id != clip.id) {
                            ++memberCount;
                            available &= !member.locked;
                        }
                    if (memberCount > s.memberDrags.size()) {
                        out.overflow = true;
                        available = false;
                    }
                    const std::size_t required = 1 + (adjacent ? 1 : 0) +
                                                 (kind == editor::EditKind::Slide ? 1 : 0) + memberCount;
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
                                    drag.original = member;
                                    drag.transaction.Begin(member.id, p.revision, kind, Value(member),
                                                           editor::CurrentModifiers(), out);
                                }
                        }
                    }
                }
            }
        }
        draw->PopClipRect();
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
            value.first += delta;
            value.last += delta;
            if (ImGui::IsMouseDown(0) && !(value == member.transaction.draft.proposed))
                member.transaction.Update(p.revision, value, out);
        }
        if (s.guide.snapped) {
            float x =
                view.min.x + s.headerWidth +
                static_cast<float>((editor::Seconds(s.guide.candidate.tick) - s.canvas.origin.x) * s.canvas.scale.x);
            draw->AddLine({x, view.min.y}, {x, view.max.y}, ImGui::GetColorU32(theme.colors.warning), 2);
        }
    }
    if (s.drag.active && !ImGui::IsMouseDown(0))
        EndDrags(s, p.revision, s.drag.draft.phase == editor::Phase::Cancel, out);
    float playhead =
        view.min.x + s.headerWidth +
        static_cast<float>((editor::Seconds(s.time.playhead) - s.canvas.origin.x) * s.canvas.scale.x);
    draw->AddLine({playhead, view.min.y}, {playhead, view.max.y}, ImGui::GetColorU32(theme.colors.accent), 2);
    editor::EndCanvas();
    ImGui::PopID();
}
void TransitionPicker(const char *id, const ClipView &clip, std::uint64_t revision,
                      editor::EventBuffer &events, bool trackLocked) {
    ImGui::PushID(id);
    ImGui::BeginDisabled(clip.locked || trackLocked);
    const char *names[]={"None","Dissolve","Fade","Crossfade"};
    editor::Value original;original.first=static_cast<Tick>(clip.transitionInKind);
    original.last=static_cast<Tick>(clip.transitionOutKind);
    for (int side=0;side<2;++side) {
        int selected=static_cast<int>(side ? clip.transitionOutKind : clip.transitionInKind);
        if (ImGui::Combo(side ? "Transition out" : "Transition in",&selected,names,4)) {
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
        d->AddRect({p.x + size.x * .2f, p.y + size.y * .2f}, {p.x + size.x * .8f, p.y + size.y * .8f},
                   ImGui::GetColorU32(theme.colors.accent), 0.f, 2.f, ImDrawFlags_None);
        d->AddCircle({p.x + size.x * o.anchor.x, p.y + size.y * o.anchor.y}, 5, color);
    }
    d->AddText({p.x + 8, p.y + 8}, color, o.label);
    if (o.showTimecode) {
        char label[32];
        editor::FormatTimecode(time.playhead, time.rate, time.dropFrame, label);
        d->AddText({p.x + 8, p.y + size.y - 24}, color, label);
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
void Waveform(const char *id, std::span<const AudioBucket> buckets, ImVec2 size, const Theme &theme) {
    auto p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    auto *d = ImGui::GetWindowDrawList();
    for (std::size_t i = 0; i < buckets.size(); ++i) {
        float x = p.x + size.x * i / buckets.size();
        d->AddLine({x, p.y + size.y * (.5f - .5f * buckets[i].maximum)},
                   {x, p.y + size.y * (.5f - .5f * buckets[i].minimum)},
                   ImGui::GetColorU32(theme.colors.success));
    }
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
