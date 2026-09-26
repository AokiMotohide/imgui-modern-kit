# Build a timeline editor

[日本語](タイムラインの作成.md) · [Documentation index](../README.md) · [How ImKit works](../getting-started/how-it-works.md)

The video timeline is ImKit's professional NLE surface: multi-track editing, fades, transitions, ripple/roll/slide/slip moves, track management, envelope keyframes, and a preview monitor. It is a host-owned, immediate-mode widget. You supply a `TimelineProvider` (your tracks and clips, as borrowed views each frame), a persistent `TimelineState` (pan/zoom/gestures), and an event buffer that the timeline fills with committed edits. The library never decodes media, owns files, or holds your clip list — it draws and reports.

![Multi-track timeline with clips, tracks, and overview](../images/v3-timeline.gif)

## The core pattern

Each frame the relationship between you and ImKit is three steps:

1. **Provide the view.** A `TimelineProvider` returns your `TrackView`s and `ClipView`s (host-owned) and a `revision` you stamp.
2. **Draw.** Call `video::Timeline(id, provider, state, selection, events, theme, size)`.
3. **Apply out.** After `ImGui::Render()`, iterate `events.Events()` and commit only events whose `revision` matches your current frame.

The host owns every clip, track, and undo entry. The timeline emits `editor::Event`s; it does not mutate your data.

## What you need

1. A Dear ImGui context and a window (your own app or the Gallery).
2. The `imkit::video` target linked into your app.
3. A host model: tracks, clips, and an undo history you own.

## What you'll build

- A host-owned model: `TrackView`s and `ClipView`s (ImKit's view types, filled with your data).
- A `TimelineProvider` whose `tracks`/`clips` callbacks return those views.
- A persistent `TimelineState` and a clip `Selection`.
- An `Apply` step that commits trims, splits, fades, transitions, track edits, and ripple deletes.

## The model (host-owned data)

ImKit's `TrackView` and `ClipView` are plain host-owned value types. Fill them from your media, and keep a `revision` you increment whenever the data changes.

```cpp
namespace editor = imkit::editor;
namespace video = imkit::video;

struct Model {
    std::vector<video::TrackView> tracks;
    std::vector<video::ClipView> clips;        // keep clips for a track contiguous
    std::uint64_t revision = 1;
    std::uint32_t nextTrack = 400, nextClip = 800;

    // Persistent: it holds gesture/drag state across frames.
    video::TimelineState state{};
    // Host-owned clip selection; the timeline writes it, you read it back.
    std::vector<editor::StableId> selectedClipIds;
    editor::Selection selection;

    void Init() {
        tracks.push_back(video::TrackView{ nextTrack, "Video A",
                              video::TrackKind::Video, 64, true });
        clips.push_back(MakeClip(nextClip++, 400, "Intro"));
        selection.storage = selectedClipIds.data();
        selection.count = 0;
    }

    video::ClipView MakeClip(std::uint32_t id, std::uint32_t track, const char *label) {
        video::ClipView c;
        c.id = id; c.track = track; c.label = label;
        c.start = editor::FromSeconds(0);
        c.duration = editor::FromSeconds(3);
        c.sourceIn = 0;
        c.speed = 1;
        return c;
    }

    // Media handles and the minimum clip length. The library validates against these.
    video::ClipConstraints Constraints(const video::ClipView &c) const {
        video::ClipConstraints b;
        b.mediaFirst = 0;
        b.mediaLast = editor::FromSeconds(3600);   // from your media
        b.minimumDuration = editor::FromSeconds(0.083);
        return b;
    }

    video::TimelineProvider Provider() {
        video::TimelineProvider p;
        p.user = this;
        p.revision = revision;
        p.trackCount = (int)tracks.size();
        p.tracks = [](void *u, int, int) {
            auto *m = static_cast<Model *>(u);
            return std::span<const video::TrackView>(m->tracks.data(), m->tracks.size());
        };
        // Clips for one track, in order, within the visible range.
        p.clips = [](void *u, editor::StableId track, editor::Range) {
            auto *m = static_cast<Model *>(u);
            int i = 0, n = (int)m->clips.size();
            while (i < n && m->clips[i].track != track) ++i;
            int j = i;
            while (j < n && m->clips[j].track == track) ++j;
            return std::span<const video::ClipView>(m->clips.data() + i, j - i);
        };
        return p;
    }
};
```

The provider borrows your data for the duration of one `Timeline` call. The `revision` you stamp is what makes an event applicable: the timeline tags every committed event with it, and `Apply` below drops anything that does not match the current frame.

## The per-frame draw

Inside your Dear ImGui frame loop (after `NewFrame`):

```cpp
auto provider = model.Provider();
// model.state persists across frames; it carries pan/zoom and in-progress gestures.
// model.selection is persistent; Timeline writes it in place and you read it back.

std::array<editor::Event, 16> eventStorage{};
editor::EventBuffer events{eventStorage};

video::Timeline("timeline", provider, model.state, model.selection,
                events, theme, ImVec2(0, 300));

ImGui::Render();
model.Apply(events.Events());   // commit this frame's events
```

`ImVec2(0, 300)` sizes the widget (height 300 px). The `Timeline` also draws the tool bar, zoom controls, overview strip, and, if you provide `provider.waveform`/`provider.externalDrops`/`provider.drawClipOverlay`, waveforms, external-asset drops, and per-clip overlays.

## Applying events

`Apply` must:

- Ignore any event whose `phase != editor::Phase::Commit` or whose `revision` is stale.
- Map each event's `Value` fields to a model change. The header documents the fields per kind; the public helpers (`EditClip`, `SplitClip`, `RippleDeletePosition`, `RollClips`, `SlideClip`) validate the change against your media before you apply it.
- Increment `revision` when anything changed, so the next frame stamps a new value.

A representative `Apply` (the same shape covers the other kinds):

```cpp
void Model::Apply(std::span<const editor::Event> events) {
    const auto frameRevision = revision;
    bool changed = false;
    auto *clip = [this](editor::StableId id) {
        for (auto &c : clips) if (c.id == id) return &c;
        return nullptr;
    };
    for (auto &e : events) {
        if (e.phase != editor::Phase::Commit || e.revision != frameRevision)
            continue;                          // stale event: drop
        switch (e.kind) {
        // first/last = start / start+duration. delta = proposed - original.
        case editor::EditKind::TrimStart:
        case editor::EditKind::TrimEnd:
        case editor::EditKind::Move: {
            auto *c = clip(e.target);
            if (!c) break;
            const Tick delta = e.proposed.first - e.original.first;
            auto edit = video::EditClip(*c, e.kind, delta, Constraints(*c));
            if (!edit.valid) break;            // host rejects out-of-media edits
            c->start = edit.start; c->duration = edit.duration;
            c->sourceIn = edit.sourceIn;
            if (edit.rippleDelta != 0) RippleFollowing(c->track, c, edit.rippleDelta); // host helper
            changed = true;
            break;
        }
        case editor::EditKind::Split: {        // razor
            auto *c = clip(e.target);
            if (!c) break;
            auto sr = video::SplitClip(*c, e.proposed.first, Constraints(*c));
            if (!sr.valid) break;
            ReplaceClip(*c, sr.left, sr.right); // host helper: two clips take the original's slot
            changed = true;
            break;
        }
        case editor::EditKind::ClipFades: {    // first/last = in/out duration; x/y = curve
            auto *c = clip(e.target);
            if (!c) break;
            c->fades.inDuration  = e.original.first;
            c->fades.outDuration = e.original.last;
            c->fades.inCurve  = static_cast<video::FadeCurve>(e.original.x);
            c->fades.outCurve = static_cast<video::FadeCurve>(e.original.y);
            changed = true;
            break;
        }
        case editor::EditKind::CutTransition: { // target=left, parent=right; first=duration, x=kind
            auto *c = clip(e.target);
            if (!c) break;
            c->outgoingTransition = video::CutTransitionView{
                e.target, e.parent, e.original.first, 0,
                static_cast<video::TransitionKind>(e.original.x), false, ""};
            changed = true;
            break;
        }
        case editor::EditKind::RippleDelete: { // related clips arrive as one batch
            // Rebuild the track from the surviving clips; close the gap.
            // RebuildTrack(e.target, e.operationSize); // host helper: close the gap from the batch
            changed = true;
            break;
        }
        case editor::EditKind::TrackEdit: {    // offset=TrackAction, parent=before-track, x=kind
            auto action = static_cast<video::TrackAction>(e.offset);
            if (action == video::TrackAction::Add) {
                auto t = video::TrackView{ nextTrack, "", video::TrackKind::Video, 64 };
                auto before = e.parent == 0 ? 0 :
                    (int)(std::find_if(tracks.begin(), tracks.end(),
                        [e](const auto &tr){ return tr.id == e.parent; }) - tracks.begin());
                tracks.insert(tracks.begin() + before, t);
            } else if (action == video::TrackAction::Remove) {
                std::erase_if(tracks, [e](const auto &tr){ return tr.id == e.parent; });
            } else { /* Reorder / Duplicate: same shape. */ }
            changed = true;
            break;
        }
        default:
            break;   // Roll/Slide/Slip: video::RollClips / SlideClip / EditClip;
                     // CaptionInsert / AudioEnvelope / Clipboard: host semantics.
        }
    }
    if (changed) ++revision;
}
```

Selection events (`Select`, `BoxSelect`, `LassoSelect`, and track-row edits) are already reflected in the `Selection` you passed; read `selection.count` / `selection.Contains(id)` after `Timeline()` rather than processing those events.

## How to run it

- The main Gallery hosts the timeline (`imkit_gallery` target). Build it and open the Timeline page to see the full surface at scale.
- `tests/video.cpp` is the headless verification: it drives the same `Timeline()` API with a synthetic model and asserts trim/split/transition/ripple behavior. Run it as a test to see the event contract in action.

## What just happened

- Your `Model` owned the tracks, clips, and revision; ImKit never mutated them directly.
- Each frame stamped a provider with the current `revision` and drew the timeline.
- The timeline returned committed events; `Apply` accepted only those matching the frame's revision.
- `revision` incremented, so the next snapshot reflects the edit.

## The rules that keep it safe

- **Revision-gate every event.** A stale event from an earlier frame must be dropped, not applied.
- **Validate against your media.** Use `EditClip`/`SplitClip`/`RollClips`/`SlideClip` before applying; the library's helpers enforce media handles and minimum lengths.
- **Own the state.** `TimelineState` must persist across frames; recreating it resets in-progress gestures.
- **Borrow, don't store.** Provider views are valid only for the current `Timeline` call.
- **Undo is host-owned.** There is no Undo event from the timeline; keep your own undo history and replay it into your model.

## Next

- [Timeline editing (full reference)](../components/timeline-editing.md)
- [Editor API reference](../reference/editor-api.md)
- [Author a custom component](custom-component.md)
