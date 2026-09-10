#pragma once
#include <imkit/editor_core.h>
namespace imkit::video {
using editor::StableId;
using editor::Tick;
enum class TrackKind { Video, Audio, Caption, Effect, Adjustment, Group };
enum class TrackControl { Visible, Mute, Solo, Locked, Record, Target, Source, Expanded, Height };
enum class Tool { Select, Razor, Ripple, Roll, Slip, Slide, Hand };
struct AudioBucket;
struct TrackView {
    StableId id = 0;
    const char *label = "";
    TrackKind kind = TrackKind::Video;
    float height = 64;
    bool visible = true, mute = false, solo = false, locked = false, record = false, source = false,
         target = true, expanded = true;
};
float TrackExtent(const TrackView &track); // Expanded: at least 64 px; collapsed: 32 px.
struct TrackLayout {
    std::span<const TrackView> tracks;
    double top = 0; // Absolute pixel offset of the first returned track.
};
enum class TransitionKind { None, Dissolve, Fade, Crossfade };
struct EnvelopePoint {StableId id=0;Tick tick=0;double gain=1;bool locked=false;};
// Sorted clip-local points; empty envelope evaluates to unity gain.
double EvaluateEnvelope(std::span<const EnvelopePoint> points,Tick tick);
struct ClipView {
    StableId id = 0, track = 0, linked = 0, group = 0;
    const char *label = "";
    Tick start = 0, duration = 0, sourceIn = 0;
    double speed = 1;
    ImTextureRef thumbnail{};
    bool proxy = false, missing = false, offline = false, locked = false;
    std::span<const float> waveform;
    std::span<const editor::Keyframe> keys; // Sorted by clip-local tick; borrowed draw/edit keys.
    Tick transitionIn = 0, transitionOut = 0;
    std::span<const AudioBucket> audioBuckets;
    TransitionKind transitionInKind=TransitionKind::Dissolve,transitionOutKind=TransitionKind::Dissolve;
    std::span<const EnvelopePoint> envelope;
    StableId keyChannel=0; // Explicit insertion channel, including when keys is empty.
    double keyDefaultValue=0;
    std::span<const editor::Keyframe> keyEvaluation; // Optional full sorted channel for interpolation outside visible keys.
};
struct TransitionEdit {
    Tick inDuration=0,outDuration=0;
    bool valid=false;
};
// Delta changes the selected duration, not the clip position. The other end is preserved.
TransitionEdit EditTransition(const ClipView &clip, bool end, Tick durationDelta);
// Emits a complete Begin/Commit pair; clip/track ownership remains with the host.
void TransitionPicker(const char *id, const ClipView &clip, std::uint64_t revision,
                      editor::EventBuffer &events, bool trackLocked=false);
struct ClipConstraints {
    Tick mediaFirst = 0, mediaLast = editor::TicksPerSecond * 3600, minimumDuration = 1;
};
struct ClipEdit {
    Tick start = 0, duration = 0, sourceIn = 0;
    bool valid = false;
    Tick rippleDelta = 0;
};
ClipEdit EditClip(const ClipView &clip, editor::EditKind kind, Tick delta, ClipConstraints constraints);
// Maximum total duration of a transition centered on an adjacent cut. Each clip
// supplies half of the overlap outside its trimmed source range. Zero rejects
// gaps, different tracks, locked clips and invalid media ranges.
Tick CenteredTransitionLimit(const ClipView &left, const ClipView &right,
                             ClipConstraints leftBounds, ClipConstraints rightBounds);
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
    // Optional indexed variable-height query. Returned rows cover [firstPixel,lastPixel].
    TrackLayout (*layout)(void *, double firstPixel, double lastPixel) = nullptr;
    double totalHeight = 0;
    editor::Range contentRange{}; // Host-maintained total time extent for Fit.
    // Optional host preflight for nonlocal constraints; called only when starting an edit.
    bool (*canBeginEdit)(void *,StableId clip,editor::EditKind kind)=nullptr;
    // Optional indexed existence/lock lookup for active clip-body transactions, including offscreen owners.
    bool (*isEditable)(void *,StableId clip)=nullptr;
    // Optional total-duration limit, including neighboring media handles. Called
    // during transition edits; the host must also validate committed edits.
    Tick (*transitionLimit)(void *,StableId clip,bool outgoing)=nullptr;
};
struct TrackLabels {
    // Borrowed UTF-8 strings. Array order follows Visible through Source in TrackControl.
    std::array<const char *,7> buttons{"V","M","S","L","R","T","P"};
    std::array<const char *,7> names{"Visible","Mute","Solo","Locked","Record armed","Target track","Source patch"};
    const char *controls="Controls", *height="Track height", *on="On", *off="Off";
    const char *expand="Expand track", *collapse="Collapse track";
    std::array<const char *,6> kinds{"Video track","Audio track","Caption track","Effect track","Adjustment track","Group track"};
};
struct TimelineLabels {
    std::array<const char *,7> tools{"Select","Razor","Ripple","Roll","Slip","Slide","Hand"};
    std::array<const char *,7> tooltips{"Select clips","Razor: split clip at cursor","Ripple: trim and shift following clips",
        "Roll: move the boundary between adjacent clips","Slip: change source range without moving clip","Slide: move clip and trim its neighbors","Pan timeline"};
    const char *snap="Snap", *magnet="Magnet", *magnetTooltip="Magnet: snap to timeline targets";
    const char *options="Timeline options", *follow="Follow playhead", *frameGrid="Frame grid";
    std::array<const char *,3> followModes{"Off","Smooth","Page"};
    std::array<const char *,7> snapKinds{"Frame","Playhead","Marker","Clip edge","Keyframe","In/out","Selection edge"};
    const char *fit="Fit", *fitTooltip="Fit timeline";
    const char *unlink="Unlink clip",*ungroup="Remove from group";
    const char *linkSelection="Link selected clips",*groupSelection="Group selected clips";
};
struct TimelineState {
    struct MemberDrag {
        ClipView original{};
        editor::Transaction transaction;
        ClipView previous{},next{};
        editor::Transaction previousTransaction,nextTransaction;
    };
    editor::CanvasState canvas{{0, 0}, {100, 1}};
    editor::Transaction drag;
    editor::TimeState time;
    Tool tool = Tool::Select;
    bool snapping = true, magnet = true;
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
    editor::Transaction heightDrag;
    const IconAtlas *icons = nullptr; // Non-owning host atlas.
    bool snapToFrame = false;
    editor::AutoScroll autoScroll = editor::AutoScroll::Smooth;
    std::span<const editor::Binding> bindings;
    std::uint32_t snapKinds = 0x7f; // Bit positions are editor::SnapKind.
    editor::Transaction transitionDrag;
    bool transitionEnd=false;
    double transitionMouseStart=0;
    editor::Transaction envelopeDrag;
    editor::Point envelopeMouseStart{};
    Tick envelopeContextTick=0;
    editor::Transaction captionDrag;
    bool captionFocus=false;
    editor::Transaction keyDrag;
    double keyMouseStart=0;
    std::span<editor::Transaction> keyCompanions; // Host scratch, stable during a gesture.
    std::size_t keyCompanionCount=0;
    editor::Selection *keySelection=nullptr; // Optional non-owning selection distinct from clips.
    TrackLabels trackLabels;
    TimelineLabels labels;
};
// Resolves both moving edges; ignores every selected clip and filters disabled kinds.
editor::SnapResult ResolveTimelineSnap(const TimelineState &state, Tick delta,
    std::span<const editor::SnapCandidate> candidates, std::span<const StableId> movingIds);
void Timeline(const char *id, const TimelineProvider &provider, TimelineState &state,
              editor::Selection &selection, editor::EventBuffer &events, const Theme &theme,
              ImVec2 size = {0, 300});
enum class MonitorMetadataPreset { Off, Clip, Details };
struct MonitorOptions {
    bool safeArea = true, guides = false, showTimecode = true, transform = false;
    const char *label = "";
    ImVec2 anchor{.5f, .5f};
    bool flipY = false;
    MonitorMetadataPreset metadataPreset=MonitorMetadataPreset::Clip;
    const char *clipName="", *markerComment=""; // Borrowed host UTF-8 strings.
    std::span<const char *const> metadata; // Extra lines for Details, borrowed for this call.
    editor::Rect transformBounds{{.2,.2},{.8,.8}}; // Normalized display coordinates; texture flip does not flip overlays.
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
    StableId gainId = 0, panId = 0; // Explicit property IDs. Zero disables the corresponding control.
    bool locked = false;
    const char *gainLabel = "Gain", *panLabel = "Pan", *muteLabel = "Mute", *soloLabel = "Solo",
               *recordLabel = "Record";
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
    // Optional RGB parade: provide all three width * 256 buffers, or leave all empty.
    std::span<std::uint32_t> redWaveform, greenWaveform, blueWaveform;
};
bool BuildScopes(std::span<const Rgba> pixels, int width, int height, ScopeBuffers output);
void Histogram(const char *id, std::span<const std::uint32_t> bins, ImVec2 size, const Theme &theme);
void ScopeImage(const char *id, std::span<const std::uint32_t> bins, int width, int height, ImVec2 size,
                const Theme &theme);
void ScopeImage(const char *id, std::span<const std::uint32_t> bins, int width, int height, ImVec2 size,
                const Theme &theme, ImVec4 tint);
struct ColorValues {
    float lift[3]{}, gamma[3]{1, 1, 1}, gain[3]{1, 1, 1};
    float temperature = 0, tint = 0, exposure = 0;
};
bool ColorControls(const char *id, ColorValues &hostDraft);
struct ColorPropertyIds {
    StableId lift = 0, gamma = 0, gain = 0, temperature = 0, tint = 0, exposure = 0;
};
struct ColorLabels {
    const char *lift = "Lift", *gamma = "Gamma", *gain = "Gain";
    const char *temperature = "Temperature", *tint = "Tint", *exposure = "Exposure", *level = "Level";
};
struct ColorState {
    editor::Transaction drag;
    std::array<ImVec2, 3> wheelCenters{}; // Last layout, for host overlays and public IO automation.
    float wheelRadius = 0;
};
// RGB edits use Value::x/y/z; scalar edits use x. IDs are explicit, unique and nonzero.
void ColorControls(const char *id, const ColorValues &values, const ColorPropertyIds &ids,
                   std::uint64_t revision, ColorState &state, editor::EventBuffer &events,
                   const ColorLabels &labels = {});
} // namespace imkit::video
