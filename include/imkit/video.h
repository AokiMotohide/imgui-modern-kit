#pragma once
#include <imkit/editor_core.h>
namespace imkit::video {
using editor::StableId;
using editor::Tick;
enum class TrackKind { Video, Audio, Caption, Effect, Adjustment, Group };
enum class Tool { Select, Razor, Ripple, Roll, Slip, Slide, Hand };
struct TrackView {
    StableId id = 0;
    const char *label = "";
    TrackKind kind = TrackKind::Video;
    float height = 64;
    bool visible = true, mute = false, solo = false, locked = false, record = false, source = false,
         target = true, expanded = true;
};
struct ClipView {
    StableId id = 0, track = 0, linked = 0, group = 0;
    const char *label = "";
    Tick start = 0, duration = 0, sourceIn = 0;
    double speed = 1;
    ImTextureRef thumbnail{};
    bool proxy = false, missing = false, offline = false, locked = false;
    std::span<const float> waveform;
    std::span<const editor::Keyframe> keys;
    Tick transitionIn = 0, transitionOut = 0;
};
struct ClipConstraints {
    Tick mediaFirst = 0, mediaLast = editor::TicksPerSecond * 3600, minimumDuration = 1;
};
struct ClipEdit {
    Tick start = 0, duration = 0, sourceIn = 0;
    bool valid = false;
    Tick rippleDelta = 0;
};
ClipEdit EditClip(const ClipView &clip, editor::EditKind kind, Tick delta, ClipConstraints constraints);
struct PairEdit {
    ClipEdit left, right;
    bool valid = false;
};
PairEdit RollClips(const ClipView &left, const ClipView &right, Tick delta, ClipConstraints leftBounds,
                   ClipConstraints rightBounds);
struct TripleEdit {
    ClipEdit previous, current, next;
    bool valid = false;
};
TripleEdit SlideClip(const ClipView &previous, const ClipView &current, const ClipView &next, Tick delta,
                     ClipConstraints previousBounds, ClipConstraints currentBounds,
                     ClipConstraints nextBounds);
struct SplitResult {
    ClipEdit left, right;
    bool valid = false;
};
SplitResult SplitClip(const ClipView &clip, Tick tick, ClipConstraints constraints);
struct TimelineProvider {
    void *user = nullptr;
    std::uint64_t revision = 0;
    int trackCount = 0;
    std::span<const TrackView> (*tracks)(void *, int first, int count) = nullptr;
    std::span<const ClipView> (*clips)(void *, StableId track, editor::Range visible) = nullptr;
    ClipConstraints (*constraints)(void *, StableId clip) = nullptr;
    std::span<const editor::SnapCandidate> (*snap)(void *, editor::Range visible) = nullptr;
    std::span<const editor::Marker> markers;
    struct Neighbors {
        const ClipView *previous = nullptr, *next = nullptr;
    };
    Neighbors (*neighbors)(void *, StableId clip) = nullptr;
    // Includes linked/group members; host marks members of locked tracks as locked.
    std::span<const ClipView> (*selected)(void *, std::span<const StableId>) = nullptr;
};
struct TimelineState {
    struct MemberDrag {
        ClipView original{};
        editor::Transaction transaction;
    };
    editor::CanvasState canvas{{0, 0}, {100, 1}};
    editor::Transaction drag;
    editor::TimeState time;
    Tool tool = Tool::Select;
    bool snapping = true, magnet = false;
    double verticalScroll = 0;
    editor::Point mouseStart{};
    ClipView original{};
    editor::SnapResult guide{};
    float headerWidth = 172, rowHeight = 66;
    StableId hovered = 0;
    editor::CanvasView view{};
    StableId editingCaption = 0;
    char caption[256]{};
    editor::Transaction previousDrag, nextDrag;
    ClipView previousOriginal{}, nextOriginal{};
    std::span<MemberDrag> memberDrags;
    std::size_t memberCount = 0;
};
void Timeline(const char *id, const TimelineProvider &provider, TimelineState &state,
              editor::Selection &selection, editor::EventBuffer &events, const Theme &theme,
              ImVec2 size = {0, 300});
struct MonitorOptions {
    bool safeArea = true, guides = false, showTimecode = true, transform = false;
    const char *label = "";
    ImVec2 anchor{.5f, .5f};
    bool flipY = false;
};
void Monitor(const char *id, ImTextureRef texture, ImVec2 size, const editor::TimeState &time,
             const MonitorOptions &options, const Theme &theme);
struct AudioBucket {
    float minimum = 0, maximum = 0, rms = 0, peak = 0;
};
void BuildAudioBuckets(std::span<const float> interleaved, int channels, int channel,
                       std::span<AudioBucket> output);
struct MeterState {
    float rms = 0, peak = 0, heldPeak = 0, holdRemaining = 0;
};
void UpdateMeter(MeterState &state, std::span<const float> samples, float deltaSeconds,
                 float holdSeconds = 1.5f);
void Waveform(const char *id, std::span<const AudioBucket> buckets, ImVec2 size, const Theme &theme);
void LevelMeter(const char *id, const MeterState &left, const MeterState &right, ImVec2 size,
                const Theme &theme);
struct AudioStripView {
    StableId id = 0;
    const char *label = "";
    double gain = 1, pan = 0;
    bool mute = false, solo = false, record = false;
};
void AudioStrip(const AudioStripView &strip, std::uint64_t revision, editor::PropertyState &state,
                editor::EventBuffer &events);
struct Rgba {
    float r = 0, g = 0, b = 0, a = 1;
};
struct ScopeBuffers {
    std::span<std::uint32_t> red, green, blue, luma; // 256 histogram bins each.
    std::span<std::uint32_t> waveform;               // width * 256 luma bins.
    std::span<std::uint32_t> vectorscope;            // 256 * 256 chroma bins.
};
bool BuildScopes(std::span<const Rgba> pixels, int width, int height, ScopeBuffers output);
void Histogram(const char *id, std::span<const std::uint32_t> bins, ImVec2 size, const Theme &theme);
void ScopeImage(const char *id, std::span<const std::uint32_t> bins, int width, int height, ImVec2 size,
                const Theme &theme);
struct ColorValues {
    float lift[3]{}, gamma[3]{1, 1, 1}, gain[3]{1, 1, 1};
    float temperature = 0, tint = 0, exposure = 0;
};
bool ColorControls(const char *id, ColorValues &hostDraft);
} // namespace imkit::video
