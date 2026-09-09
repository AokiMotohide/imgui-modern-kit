#include <imkit/video.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace imkit::video {
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
} // namespace
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
void Timeline(const char *id, const TimelineProvider &p, TimelineState &s, editor::Selection &selection,
              editor::EventBuffer &out, const Theme &theme, ImVec2 size) {
    ImGui::PushID(id);
    const char *tools[] = {"Select", "Razor", "Ripple", "Roll", "Slip", "Slide", "Hand"};
    for (int i = 0; i < 7; ++i) {
        if (i)
            ImGui::SameLine();
        if (ImGui::Selectable(tools[i], static_cast<int>(s.tool) == i, 0, {56, 24}))
            s.tool = static_cast<Tool>(i);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &s.snapping);
    ImGui::SameLine();
    ImGui::Checkbox("Magnet", &s.magnet);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + s.headerWidth);
    editor::TimeRuler("time", s.time, s.canvas, p.markers, p.revision, out, theme);
    s.canvas.wheelZoom = ImGui::GetIO().KeyCtrl;
    s.canvas.wheelZoomY = false;
    auto view = editor::BeginCanvas("tracks", s.canvas, size, theme);
    s.view = view;
    auto *draw = ImGui::GetWindowDrawList();
    const auto &io = ImGui::GetIO();
    if (view.hovered && s.tool == Tool::Hand && ImGui::IsMouseDragging(0))
        s.canvas.origin.x -= io.MouseDelta.x / s.canvas.scale.x;
    if (s.time.playing) {
        double visibleSeconds = (view.max.x - view.min.x - s.headerWidth) / s.canvas.scale.x,
               head = editor::Seconds(s.time.playhead);
        if (head > s.canvas.origin.x + visibleSeconds * .9)
            s.canvas.origin.x = head - visibleSeconds * .9;
        if (head < s.canvas.origin.x)
            s.canvas.origin.x = head;
    }
    // Track rows are indexed independently from time; no all-track query or clip scan.
    if (view.hovered && io.MouseWheel && !io.KeyCtrl)
        s.verticalScroll = (std::max)(0., s.verticalScroll - io.MouseWheel * s.rowHeight);
    const int first = (std::max)(0, static_cast<int>(s.verticalScroll / s.rowHeight));
    const int count = (std::max)(
        0, (std::min)(p.trackCount - first, static_cast<int>((view.max.y - view.min.y) / s.rowHeight) + 2));
    auto tracks = p.tracks ? p.tracks(p.user, first, count) : std::span<const TrackView>{};
    editor::Range range{editor::FromSeconds(s.canvas.origin.x),
                        editor::FromSeconds(s.canvas.origin.x +
                                            (view.max.x - view.min.x - s.headerWidth) / s.canvas.scale.x)};
    s.hovered = 0;
    if (s.drag.active && (p.revision != s.drag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape))) {
        s.drag.Cancel(out);
        s.previousDrag.Cancel(out);
        s.nextDrag.Cancel(out);
        for (auto &member : s.memberDrags.first(s.memberCount))
            member.transaction.Cancel(out);
    }
    int index = first;
    for (const auto &track : tracks) {
        float y = view.min.y + index++ * s.rowHeight - static_cast<float>(s.verticalScroll);
        draw->AddRectFilled({view.min.x, y}, {view.max.x, y + s.rowHeight - 2},
                            ImGui::GetColorU32(theme.colors.surface));
        ImGui::SetCursorScreenPos({view.min.x + 4, y + 3});
        ImGui::PushID(reinterpret_cast<void *>(static_cast<std::uintptr_t>(track.id)));
        ImGui::TextUnformatted(track.label);
        ImGui::SetCursorScreenPos({view.min.x + 4, y + 28});
        const char *labels[] = {"V", "M", "S", "L", "R", "T"};
        const bool values[] = {track.visible, track.mute,   track.solo,
                               track.locked,  track.record, track.target};
        for (int f = 0; f < 6; ++f) {
            if (f)
                ImGui::SameLine(0, 2);
            if (ImGui::SmallButton(labels[f]))
                Toggle(out, track, p.revision, f, values[f]);
        }
        ImGui::PopID();
        auto clips = p.clips ? p.clips(p.user, track.id, range) : std::span<const ClipView>{};
        draw->PushClipRect({view.min.x + s.headerWidth, y}, {view.max.x, y + s.rowHeight - 2}, true);
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
            ImVec2 a{x, y + 4}, b{end, y + s.rowHeight - 7};
            bool selected = selection.Contains(clip.id);
            auto color = track.kind == TrackKind::Audio     ? theme.colors.success
                         : track.kind == TrackKind::Caption ? theme.colors.warning
                                                            : theme.colors.accent;
            color.w = selected ? .8f : .35f;
            draw->AddRectFilled(a, b, ImGui::GetColorU32(color), 4);
            draw->AddRect(a, b, ImGui::GetColorU32(selected ? theme.colors.focus : theme.colors.border), 4, 0,
                          selected ? 2.f : 1.f);
            if (clip.thumbnail.GetTexID())
                draw->AddImage(clip.thumbnail, {x + 3, y + 23}, {(std::min)(end - 3, x + 65), b.y - 3});
            if (s.editingCaption == clip.id) {
                ImGui::SetCursorScreenPos({x + 4, y + 5});
                ImGui::SetNextItemWidth((std::max)(30.f, end - x - 8));
                if (ImGui::InputText("##caption", s.caption, sizeof(s.caption),
                                     ImGuiInputTextFlags_EnterReturnsTrue)) {
                    editor::Event e{clip.id, p.revision, editor::Phase::Commit, editor::EditKind::Rename};
                    std::snprintf(e.originalText.data(), e.originalText.size(), "%s", clip.label);
                    std::snprintf(e.proposedText.data(), e.proposedText.size(), "%s", s.caption);
                    if (out.Push(e))
                        s.editingCaption = 0;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                    s.editingCaption = 0;
            } else
                draw->AddText({x + 6, y + 7}, ImGui::GetColorU32(theme.colors.text), clip.label);
            if (clip.missing || clip.offline)
                draw->AddLine(a, b, ImGui::GetColorU32(theme.colors.destructive), 2);
            if (track.locked || clip.locked)
                draw->AddText({x + 6, b.y - 18}, ImGui::GetColorU32(theme.colors.muted), "Locked");
            if (clip.proxy)
                draw->AddText({end - 20, y + 7}, ImGui::GetColorU32(theme.colors.warning), "P");
            if (clip.linked || clip.group)
                draw->AddLine({x + 3, b.y - 3}, {end - 3, b.y - 3}, ImGui::GetColorU32(theme.colors.text));
            for (std::size_t j = 0; j < clip.waveform.size(); ++j) {
                float wx = x + (end - x) * static_cast<float>(j) / clip.waveform.size(),
                      amplitude = clip.waveform[j] * (s.rowHeight - 28) * .4f;
                draw->AddLine({wx, y + 40 - amplitude}, {wx, y + 40 + amplitude},
                              ImGui::GetColorU32(theme.colors.text));
            }
            for (const auto &key : clip.keys) {
                float kx = x + static_cast<float>(editor::Seconds(key.tick) * s.canvas.scale.x);
                float ky = b.y - 9;
                draw->AddQuadFilled({kx, ky - 4}, {kx + 4, ky}, {kx, ky + 4}, {kx - 4, ky},
                                    ImGui::GetColorU32(theme.colors.warning));
            }
            if (clip.transitionIn)
                draw->AddLine(
                    a, {x + static_cast<float>(editor::Seconds(clip.transitionIn) * s.canvas.scale.x), b.y},
                    ImGui::GetColorU32(theme.colors.text), 2);
            if (clip.transitionOut)
                draw->AddLine(
                    {end - static_cast<float>(editor::Seconds(clip.transitionOut) * s.canvas.scale.x), b.y},
                    {end, a.y}, ImGui::GetColorU32(theme.colors.text), 2);
            bool hit = view.hovered && io.MousePos.x >= (std::max)(x, view.min.x + s.headerWidth) &&
                       io.MousePos.x < end && io.MousePos.y >= a.y && io.MousePos.y < b.y;
            if (hit)
                s.hovered = clip.id;
            if (hit && track.kind == TrackKind::Caption && ImGui::IsMouseDoubleClicked(0) && !track.locked &&
                !clip.locked) {
                s.drag.Cancel(out);
                s.editingCaption = clip.id;
                std::snprintf(s.caption, sizeof(s.caption), "%s", clip.label);
            }
            if (hit && s.editingCaption != clip.id && ImGui::IsMouseClicked(0) && !track.locked &&
                !clip.locked) {
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
                    if (available && out.storage.size() - out.count >= 3) {
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
                        if ((kind == editor::EditKind::Move || kind == editor::EditKind::Duplicate) &&
                            p.selected) {
                            auto members = p.selected(p.user, selection.storage.first(selection.count));
                            for (const auto &member : members)
                                if (member.id != clip.id && !member.locked &&
                                    s.memberCount < s.memberDrags.size() &&
                                    out.count + 3 < out.storage.size() / 2) {
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
    if (s.drag.active) {
        Tick delta = editor::FromSeconds((io.MousePos.x - s.mouseStart.x) / s.canvas.scale.x);
        s.guide = {};
        if (s.snapping && p.snap) {
            auto candidates = p.snap(p.user, range);
            Tick anchor = s.drag.draft.kind == editor::EditKind::TrimEnd ||
                                  s.drag.draft.kind == editor::EditKind::Ripple
                              ? s.original.start + s.original.duration
                              : s.original.start;
            s.guide = editor::ResolveSnap(anchor + delta, candidates,
                                          s.canvas.scale.x / editor::TicksPerSecond, 8, s.original.id);
            if (s.guide.snapped)
                delta = s.guide.tick - anchor;
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
        if (ImGui::IsMouseReleased(0)) {
            s.previousDrag.Commit(p.revision, out);
            s.drag.Commit(p.revision, out);
            s.nextDrag.Commit(p.revision, out);
            for (auto &member : s.memberDrags.first(s.memberCount))
                member.transaction.Commit(p.revision, out);
        }
        if (s.guide.snapped) {
            float x =
                view.min.x + s.headerWidth +
                static_cast<float>((editor::Seconds(s.guide.tick) - s.canvas.origin.x) * s.canvas.scale.x);
            draw->AddLine({x, view.min.y}, {x, view.max.y}, ImGui::GetColorU32(theme.colors.warning), 2);
        }
    }
    float playhead =
        view.min.x + s.headerWidth +
        static_cast<float>((editor::Seconds(s.time.playhead) - s.canvas.origin.x) * s.canvas.scale.x);
    draw->AddLine({playhead, view.min.y}, {playhead, view.max.y}, ImGui::GetColorU32(theme.colors.accent), 2);
    editor::EndCanvas();
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
        b.minimum = b.maximum = pcm[first * channels + channel];
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
    editor::PropertyView properties[] = {{v.id, "Gain", "Audio", v.gain, 1},
                                         {v.id + 1, "Pan", "Audio", v.pan, 0}};
    editor::PropertyProvider p{properties, revision, 2, [](void *u, int first, int count, std::string_view) {
                                   return std::span<const editor::PropertyView>(
                                              static_cast<editor::PropertyView *>(u), 2)
                                       .subspan(first, (std::min)(count, 2 - first));
                               }};
    editor::PropertyGrid(v.label, p, s, out);
}
bool BuildScopes(std::span<const Rgba> pixels, int w, int h, ScopeBuffers out) {
    if (w <= 0 || h <= 0 || pixels.size() != static_cast<std::size_t>(w) * h || out.red.size() != 256 ||
        out.green.size() != 256 || out.blue.size() != 256 || out.luma.size() != 256 ||
        out.waveform.size() != static_cast<std::size_t>(w) * 256 || out.vectorscope.size() != 65536)
        return false;
    for (auto span : {out.red, out.green, out.blue, out.luma, out.waveform, out.vectorscope})
        std::fill(span.begin(), span.end(), 0);
    auto bin = [](float value) {
        return static_cast<int>(std::clamp(std::isfinite(value) ? value : 0.f, 0.f, 1.f) * 255);
    };
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        auto p = pixels[i];
        float y = .2126f * p.r + .7152f * p.g + .0722f * p.b;
        ++out.red[bin(p.r)];
        ++out.green[bin(p.g)];
        ++out.blue[bin(p.b)];
        ++out.luma[bin(y)];
        ++out.waveform[bin(y) * w + i % w];
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
                auto c = t.colors.success;
                c.w = static_cast<float>(std::log1p(bins[y * w + x]) / std::log1p(max));
                d->AddRectFilled({p.x + x * size.x / w, p.y + (h - 1 - y) * size.y / h},
                                 {p.x + (x + 1) * size.x / w, p.y + (h - y) * size.y / h},
                                 ImGui::GetColorU32(c));
            }
}
bool ColorControls(const char *id, ColorValues &draft) {
    ImGui::PushID(id);
    bool changed = ImGui::ColorEdit3("Lift", draft.lift);
    changed |= ImGui::ColorEdit3("Gamma", draft.gamma);
    changed |= ImGui::ColorEdit3("Gain", draft.gain);
    changed |= ImGui::SliderFloat("Temperature", &draft.temperature, -1, 1);
    changed |= ImGui::SliderFloat("Tint", &draft.tint, -1, 1);
    changed |= ImGui::SliderFloat("Exposure", &draft.exposure, -10, 10);
    ImGui::PopID();
    return changed;
}
} // namespace imkit::video
