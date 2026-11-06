---
title: "Build a timeline"
description: "The video timeline shows tracks and clips and lets the user trim, move, split and fade them. It is an immediate-mode widget."
---

The video timeline shows tracks and clips and lets the user trim, move, split and fade them. It is an immediate-mode widget. Each frame you give it a provider that returns your tracks and clips, and it reports the edits the user made as events.

The library does not decode media, open files or keep your clip list.

This tutorial builds a model with one track and two clips, draws it, and applies move, trim and split events.

![The Gallery timeline: clips on several tracks with trim, move and split edits](/imgui-modern-kit/docs-images/v4-timeline-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-timeline.gif)

## The pattern

Each frame has three steps:

1. **Provide.** A `TimelineProvider` returns your `TrackView` and `ClipView` values and a `revision`.
2. **Draw.** Call `video::Timeline(id, provider, state, selection, events, theme, size)`.
3. **Apply.** Pass the returned events to your model. Apply only events whose `revision` matches this frame.

The timeline never changes your data. It emits `editor::Event` values, and you decide what to do with them.

## Before you start

- A Dear ImGui context and a window (your application or the Gallery).
- `imkit::video` linked.
- A model that owns the tracks, clips and Undo history.

## Step 1: Keep the data in your own model

`TrackView` and `ClipView` are plain value types that you fill from your media. Keep a `revision` and increase it whenever the data changes.

```cpp
#include <imgui.h>
#include <imkit/imkit.h>
#include <imkit/video.h>
#include <algorithm>
#include <array>
#include <span>
#include <vector>

namespace editor = imkit::editor;
namespace video = imkit::video;

// Everything in this struct belongs to your application.
struct TimelineModel {
    std::vector<video::TrackView> tracks;
    std::vector<video::ClipView> clips;                  // keep the clips of one track together, in time order
    std::array<editor::StableId, 64> selectedStorage{};
    editor::Selection selection{selectedStorage};        // the timeline writes it, you read it back
    video::TimelineState state;                          // pan, zoom and gestures: keep it across frames
    std::uint64_t revision = 1;                          // increase it whenever the data changes
    editor::StableId nextClip = 900;

    TimelineModel() {
        video::TrackView track;
        track.id = 400;
        track.label = "Video A";
        tracks.push_back(track);
        clips.push_back(MakeClip(800, 400, "Intro", 0, 3));
        clips.push_back(MakeClip(801, 400, "Main", 3, 5));
    }

    static video::ClipView MakeClip(editor::StableId id, editor::StableId track, const char* label,
                                    double startSeconds, double seconds) {
        video::ClipView clip;
        clip.id = id;
        clip.track = track;
        clip.label = label;
        clip.start = editor::FromSeconds(startSeconds);
        clip.duration = editor::FromSeconds(seconds);
        clip.sourceIn = editor::FromSeconds(startSeconds);   // this tutorial uses one long source
        return clip;
    }

    // The limits of the media behind a clip. EditClip and SplitClip check against them.
    static video::ClipConstraints Constraints() {
        video::ClipConstraints limits;
        limits.mediaFirst = 0;
        limits.mediaLast = editor::FromSeconds(3600);
        limits.minimumDuration = editor::FromSeconds(1.0 / 12);
        return limits;
    }

    video::ClipView* Find(editor::StableId id) {
        for (video::ClipView& clip : clips)
            if (clip.id == id) return &clip;
        return nullptr;
    }

    video::TimelineProvider Provider() {
        video::TimelineProvider provider;
        provider.user = this;
        provider.revision = revision;
        provider.trackCount = static_cast<int>(tracks.size());
        provider.tracks = [](void* user, int, int) {
            auto& self = *static_cast<TimelineModel*>(user);
            return std::span<const video::TrackView>(self.tracks);
        };
        provider.clips = [](void* user, editor::StableId track, editor::Range) {
            auto& self = *static_cast<TimelineModel*>(user);
            auto first = std::find_if(self.clips.begin(), self.clips.end(),
                                      [&](const video::ClipView& c) { return c.track == track; });
            auto last = std::find_if_not(first, self.clips.end(),
                                         [&](const video::ClipView& c) { return c.track == track; });
            return std::span<const video::ClipView>(&*first, static_cast<std::size_t>(last - first));
        };
        provider.constraints = [](void*, editor::StableId) { return Constraints(); };
        return provider;
    }
    // Split(), MoveOrTrim() and Apply() are added in step 4.
};
```

The `TimelineState` holds pan, zoom and gestures in progress. Keep one for the life of the timeline, because creating a new one each frame resets the gestures. The `Selection` is also yours. The timeline writes to it and you read it back.

## Step 2: Build the provider

`Provider()` returns callbacks that give the timeline your tracks and the clips of one track. The callbacks run during `Timeline()` and borrow your data for that call only. The `revision` is what makes an event applicable: the timeline copies it into every event, and `Apply` drops any event that does not match.

## Step 3: Draw the timeline

Call this between `NewFrame()` and `Render()`.

```cpp
void DrawTimeline(TimelineModel& model, const imkit::Theme& theme) {
    const video::TimelineProvider provider = model.Provider();

    std::array<editor::Event, 16> eventStorage{};
    editor::EventBuffer events{eventStorage};

    video::Timeline("timeline", provider, model.state, model.selection, events, theme, ImVec2(0, 300));

    model.Apply(events.Events());
}
```

`ImVec2(0, 300)` sets the widget size: the full width and 300 pixels of height. The timeline also draws its tool bar, zoom controls and overview. With `provider.waveform`, `provider.externalDrops` and `provider.drawClipOverlay` set, it draws waveforms, accepts external drops and draws per-clip overlays.

## Step 4: Apply the events

Add these members to `TimelineModel`.

```cpp
    // Cuts a clip in two at the event's tick.
    bool Split(const editor::Event& e) {
        video::ClipView* clip = Find(e.target);
        if (!clip || clip->locked) return false;
        const video::SplitResult split = video::SplitClip(*clip, e.proposed.first, Constraints());
        if (!split.valid) return false;

        video::ClipView second = *clip;                      // the new second half
        second.id = nextClip++;
        second.start = split.right.start;
        second.duration = split.right.duration;
        second.sourceIn = split.right.sourceIn;
        clip->start = split.left.start;
        clip->duration = split.left.duration;
        clip->sourceIn = split.left.sourceIn;
        clips.insert(clips.begin() + (clip - clips.data()) + 1, second);
        return true;
    }

    // Moves or trims every clip of one gesture. Applies all of them or none.
    bool MoveOrTrim(std::span<const editor::Event> batch) {
        struct Planned { editor::StableId id; video::ClipEdit edit; };
        std::vector<Planned> planned;
        for (const editor::Event& e : batch) {
            video::ClipView* clip = Find(e.target);
            if (!clip || clip->locked) return false;
            const editor::Tick delta = e.kind == editor::EditKind::TrimEnd
                ? e.proposed.last - e.original.last          // the end moved
                : e.proposed.first - e.original.first;       // the start moved
            const video::ClipEdit edit = video::EditClip(*clip, e.kind, delta, Constraints());
            if (!edit.valid) return false;                   // outside the media or too short
            planned.push_back({clip->id, edit});
        }
        for (const Planned& p : planned) {
            video::ClipView* clip = Find(p.id);
            clip->start = p.edit.start;
            clip->duration = p.edit.duration;
            clip->sourceIn = p.edit.sourceIn;
        }
        return !planned.empty();
    }

    // Applies the events of one frame. Stale events change nothing.
    void Apply(std::span<const editor::Event> events) {
        const std::uint64_t frameRevision = revision;
        bool changed = false;
        for (std::size_t i = 0; i < events.size();) {
            const std::size_t count = std::max<std::size_t>(1, events[i].operationSize);   // one gesture = one batch
            if (i + count > events.size()) break;                                           // incomplete batch
            const auto batch = events.subspan(i, count);
            i += count;

            const editor::Event& first = batch.front();
            if (first.phase != editor::Phase::Commit || first.revision != frameRevision)
                continue;                                                                   // a preview, or an older frame

            switch (first.kind) {
            case editor::EditKind::Move:
            case editor::EditKind::TrimStart:
            case editor::EditKind::TrimEnd:
                changed |= MoveOrTrim(batch);
                break;
            case editor::EditKind::Split:
                changed |= Split(first);
                break;
            default:
                break;                                                                      // handle the kinds you support
            }
        }
        if (changed) ++revision;                                                            // the next frame stamps a new value
    }
```

| Rule | Reason |
| --- | --- |
| Skip events whose `phase` is not `Commit` | `Begin` and `Update` are previews of a drag in progress |
| Skip events whose `revision` differs from the frame | They were built from older data |
| Walk the events in batches of `operationSize` | Moving several clips is one gesture and one Undo step |
| Apply a batch completely or not at all | A half-applied gesture leaves the timeline inconsistent |
| Let `EditClip` and `SplitClip` check the media | They enforce the media limits and the minimum length |
| Increase `revision` after any change | The next frame then differs from the one the user saw |

Selection events (`Select`, `BoxSelect`, `LassoSelect`) are already reflected in the `Selection` you passed in. Read `selection.count` and `selection.Contains(id)` after `Timeline()` instead of handling them. Other kinds, such as fades, transitions, ripple delete, track edits and keyframes, follow the same shape. The [full reference](../../features/timeline/) lists the fields of each.

## Step 5: Check the logic without a window

`Apply` is plain C++. Feed it events directly:

```cpp
editor::Event move;
move.kind = editor::EditKind::Move;
move.target = 800;
move.revision = model.revision;                       // built for the current frame
move.original = {editor::FromSeconds(0), editor::FromSeconds(3)};
move.proposed = {editor::FromSeconds(1), editor::FromSeconds(4)};
model.Apply({&move, 1});                              // the clip now starts at 1 s, revision becomes 2

model.Apply({&move, 1});                              // stale: ignored, nothing changes
```

The Gallery's Timeline page shows the full surface, and `tests/video.cpp` drives the same API headlessly. Undo is not an event of the timeline. Keep your own history and replay it into the model.

## Rules

- Check the `revision` of every event, and drop stale ones.
- Validate edits against your media with `EditClip`, `SplitClip`, `RollClips` and `SlideClip` before you apply them.
- Keep `TimelineState` across frames. Recreating it resets the gestures in progress.
- Provider data is valid for one `Timeline` call. Do not store the spans.
- Undo belongs to your application.

## Next

- [Timeline editing](../../features/timeline/): the full reference.
- [Editor API](../../api/editor-suite/)
- [Build a custom component](../custom-component/)
