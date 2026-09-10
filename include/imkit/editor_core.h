#pragma once
#include <imkit/theme.h>
#include <imkit/icons.h>
#include <span>
#include <string_view>
#include <cstddef>

namespace imkit::editor {
using StableId = std::uint64_t;
using Tick = std::int64_t;
struct FrameRate {
    int numerator = 24, denominator = 1;
};
// 705600000 ticks per second exactly represents common integer and NTSC rates.
inline constexpr Tick TicksPerSecond = 705600000;
bool Valid(FrameRate rate);
Tick FrameToTick(std::int64_t frame, FrameRate rate);
std::int64_t TickToFrame(Tick tick, FrameRate rate);
double Seconds(Tick tick);
Tick FromSeconds(double seconds);
bool FormatTimecode(Tick tick, FrameRate rate, bool dropFrame, std::span<char> output);
bool ParseTimecode(std::string_view text, FrameRate rate, bool dropFrame, Tick &result);
struct Range {
    Tick first = 0, last = 0;
}; // Half-open.
struct Point {
    double x = 0, y = 0;
};
struct Rect {
    Point min{}, max{};
};
struct Modifiers {
    bool shift = false, control = false, alt = false;
};
Modifiers CurrentModifiers();
enum class Phase { Begin, Update, Commit, Cancel };
enum class EditKind {
    Move,
    Duplicate,
    Remove,
    TrimStart,
    TrimEnd,
    Split,
    Ripple,
    Roll,
    Slip,
    Slide,
    Select,
    BoxSelect,
    LassoSelect,
    Rename,
    Reparent,
    Reorder,
    Link,
    Toggle,
    Property,
    Reset,
    Keyframe,
    Handle,
    Translate,
    Rotate,
    Scale,
    Navigate,
    Range,
    Marker,
    TrackHeight,
    PropertyKey,
    KeyInterpolation,
    KeyHandleMode,
    KeyScale,
    KeyInsert,
    StripSettings,
    LinkGeometry,
    ComponentAdd,
    TransitionDuration, // first=in duration, last=out duration; target=clip ID.
    TransitionType, // first=in kind, last=out kind; target=clip ID.
    AudioEnvelope, // first=local tick, x=gain, parent=clip ID; offset=0 edit/1 insert/2 remove; insert target=clip, otherwise point.
    RippleDelete, // target=removed clip; related clips are emitted as one reserved batch.
    CaptionInsert // target=caption track, first=requested start; host chooses a free interval at or after it.
};
// Exact integer/time fields must never travel through floating point channels.
struct Value {
    Tick first = 0, last = 0, offset = 0;
    StableId parent = 0;
    double x = 0, y = 0, z = 0, w = 0;
    std::array<double,6> affine{}; // Scale events: rotation xyz and shear xy/xz/yz.
    bool hasAffine=false;
    bool operator==(const Value &) const = default;
};
struct Event {
    StableId target = 0;
    std::uint64_t revision = 0;
    Phase phase = Phase::Commit;
    EditKind kind = EditKind::Move;
    Value original{}, proposed{};
    Modifiers modifiers{};
    std::array<char, 256> originalText{}, proposedText{};
};
struct EventBuffer {
    std::span<Event> storage;
    std::size_t count = 0;
    bool overflow = false;
    bool Push(const Event &event);
    void Clear();
    std::span<const Event> Events() const;
};
struct Transaction {
    bool active = false;
    Event draft{};
    bool Begin(StableId id, std::uint64_t revision, EditKind kind, Value original, Modifiers modifiers,
               EventBuffer &events);
    bool Update(std::uint64_t revision, Value proposed, EventBuffer &events);
    bool Commit(std::uint64_t revision, EventBuffer &events);
    void Cancel(EventBuffer &events);
};
struct Selection {
    std::span<StableId> storage;
    std::size_t count = 0;
    StableId active = 0;
    bool Contains(StableId id) const;
    bool Set(StableId id, bool additive = false, bool toggle = false);
    void Clear();
};
enum class SnapKind { Frame, Playhead, Marker, ClipEdge, Keyframe, InOut, Selection };
struct SnapCandidate {
    Tick tick = 0;
    SnapKind kind = SnapKind::Frame;
    int priority = 0;
    StableId id = 0;
};
struct SnapResult {
    Tick tick = 0;
    bool snapped = false;
    SnapCandidate candidate{};
    double distancePixels = 0;
};
SnapResult ResolveSnap(Tick tick, std::span<const SnapCandidate> candidates, double pixelsPerTick,
                       double thresholdPixels = 8, StableId exclude = 0);
struct CanvasState {
    Point origin{};
    Point scale{1, 1};
    bool selecting = false, lasso = false;
    Point selectionStart{};
    std::span<Point> selectionPath;
    std::size_t pathCount = 0;
    bool wheelZoom = true, wheelZoomY = true;
};
Point ToScreen(Point value, const CanvasState &state, Point screenOrigin);
Point FromScreen(Point value, const CanvasState &state, Point screenOrigin);
Rect VisibleRange(const CanvasState &state, Point size);
void ZoomAt(CanvasState &state, Point screenAnchor, Point factors);
void Fit(CanvasState &state, Rect bounds, Point size, double padding = 24);
bool InPolygon(Point point, std::span<const Point> polygon);
struct SelectablePoint {
    StableId id = 0;
    Point position{};
    bool locked = false;
};
struct SelectionProvider {
    void *user = nullptr;
    std::uint64_t revision = 0;
    std::span<const SelectablePoint> (*query)(void *, Rect bounds) = nullptr;
};
struct CanvasView {
    ImVec2 min{}, max{};
    Rect visible{};
    bool hovered = false;
};
// Begin/End are always paired; caller may draw between them. Child owns clipping only.
CanvasView BeginCanvas(const char *id, CanvasState &state, ImVec2 size, const Theme &theme);
void EndCanvas();
void DrawGrid(const CanvasView &view, const CanvasState &state, Point spacing, const Theme &theme);
// Host supplies selectionPath scratch for lasso; exhausted buffers cancel and report overflow.
void CanvasSelection(const CanvasView &view, CanvasState &state, const SelectionProvider &provider,
                     Selection &selection, EventBuffer &events, const Theme &theme, bool lasso = false);
enum class Command {
    PlayPause,
    Stop,
    PreviousFrame,
    NextFrame,
    SetIn,
    SetOut,
    Loop,
    Split,
    Delete,
    Duplicate,
    SelectAll,
    Fit,
    AddKey,
    PreviousKey,
    NextKey,
    Undo,
    Redo,
    PlayReverse,
    PlayForward,
    Pause,
    ToolSelect, ToolRazor, ToolRipple, ToolRoll, ToolSlip, ToolSlide, ToolHand,
    GoToStart, GoToEnd, ClearInOut
};
enum class ShortcutPreset { CapCut, Premiere, Blender };
struct Binding {
    Command command{};
    ImGuiKeyChord chord = 0;
};
std::size_t MakeBindings(ShortcutPreset preset, std::span<Binding> destination);
bool CommandPressed(Command command, std::span<const Binding> bindings, bool focused);
struct Marker {
    StableId id = 0;
    Tick tick = 0;
    const char *label = "";
};
enum class AutoScroll { Off, Smooth, Page };
// Origin/width/head are seconds. Invalid or non-positive width leaves origin unchanged.
double FollowPlayhead(double origin, double width, double head, AutoScroll mode);
struct TransportLabels {
    const char *play="Play", *pause="Pause", *stop="Stop";
    const char *previousFrame="Previous frame", *nextFrame="Next frame";
    const char *in="In", *out="Out", *loop="Loop";
    const char *goToStart="Go to start", *goToEnd="Go to end", *clearInOut="Clear In/Out";
    const char *reverse="Play reverse", *forward="Play forward";
};
struct TimeRulerLabels {
    const char *work="Work range",*inOut="In / Out",*start="Start",*end="End";
    const char *addMarker="Add marker",*removeMarker="Remove marker",*marker="Marker";
};
struct TimeState {
    Tick playhead = 0;
    Range work{0, TicksPerSecond * 10}, inOut{0, TicksPerSecond * 10};
    FrameRate rate{};
    bool playing = false, loop = false, dropFrame = false;
    double playbackRate = 1;
    TransportLabels labels{};
    const IconAtlas *icons=nullptr;
    TimeRulerLabels rulerLabels{};
    Transaction markerDrag;
    StableId contextMarker=0;
    Tick contextTime=0;
    int rangeHandle=0; // 1/2 work start/end, 3/4 in/out start/end.
    Range rangeOriginal{};
};
void TimeRuler(const char *id, TimeState &state, CanvasState &canvas, std::span<const Marker> markers,
               std::uint64_t revision, EventBuffer &events, const Theme &theme, float height = 32);
void Transport(TimeState &state, std::span<const Binding> bindings);
// Optional host-owned atlas; nullptr retains native text buttons.
void Transport(TimeState &state, std::span<const Binding> bindings, const IconAtlas *icons);
enum class Interpolation { Constant, Linear, Bezier };
enum class HandleMode { Auto, AutoClamped, Vector, Aligned, Free };
enum class Extrapolation { Constant, Linear, Repeat };
struct Keyframe {
    StableId id = 0, channel = 0;
    Tick tick = 0;
    double value = 0;
    Point left{-1, 0}, right{1, 0}; // Relative seconds/value offsets.
    Interpolation interpolation = Interpolation::Bezier;
    HandleMode handles = HandleMode::Free;
    bool selected = false, locked = false;
};
double Evaluate(std::span<const Keyframe> sortedKeys, Tick tick,
                Extrapolation extrapolation = Extrapolation::Constant);
Keyframe ResolveHandles(std::span<const Keyframe> sortedKeys, std::size_t index);
Keyframe MoveHandle(const Keyframe &key, bool left, Point proposed);
struct CurveQuery {
    Range time;
    double minimum = 0, maximum = 0;
};
struct CurveProvider {
    void *user = nullptr;
    std::uint64_t revision = 0;
    // Return visible keys plus two neighbors on each side per channel, sorted by channel/time.
    std::span<const Keyframe> (*query)(void *, CurveQuery) = nullptr;
    // Optional complete-channel evaluation, including extrapolation beyond visible keys.
    double (*sample)(void *, StableId channel, Tick, Extrapolation) = nullptr;
    std::optional<Rect> bounds; // Complete key bounds in seconds / negative value coordinates.
    std::span<const Keyframe> (*selected)(void *, std::span<const StableId>) = nullptr;
    const Keyframe *(*neighbor)(void *, StableId channel, Tick, bool next) = nullptr;
    std::span<const SelectablePoint> (*selectionQuery)(void *, Rect) = nullptr;
};
struct CurveLabels {
    const char *add="Add key at playhead",*previous="Previous key",*next="Next key",*remove="Delete selected keys";
    const char *fit="Fit all channels",*ghost="Ghost other channels",*snap="Snap to frame",*scale="Scale key timing";
    const char *box="Box select",*lasso="Lasso select",*extrapolation="Extrapolation";
    const char *interpolation="Interpolation",*handles="Handle mode";
    std::array<const char *,3> extrapolations{"Constant","Linear","Repeat"};
    std::array<const char *,3> interpolations{"Constant","Linear","Bezier"};
    std::array<const char *,5> handleModes{"Auto","Auto Clamped","Vector","Aligned","Free"};
};
struct CurveState {
    CanvasState canvas{{0, -1}, {100, 100}};
    Transaction drag;
    StableId handle = 0;
    StableId contextKey = 0;
    StableId activeChannel = 0;
    bool ghostOtherChannels = true;
    Extrapolation extrapolation = Extrapolation::Constant;
    std::span<const Binding> bindings;
    bool fitRequested = false;
    std::span<Keyframe> previewKeys; // Optional host scratch, at least the query result size.
    std::span<Transaction> companionDrags;
    std::size_t companionCount = 0;
    bool scaleTime = false, scaling = false;
    Tick scalePivot = 0;
    bool lassoSelect = false;
    Tick time = 0;
    bool snapToFrame = false;
    FrameRate rate{};
    int side = 0;
    Point mouseStart{};
    CanvasView view{};
    const IconAtlas *icons=nullptr; // Non-owning host atlas for selection controls.
    CurveLabels labels; // Borrowed UTF-8 display strings.
};
void CurveEditor(const char *id, const CurveProvider &provider, CurveState &state, Selection &selection,
                 EventBuffer &events, const Theme &theme, ImVec2 size = {0, 220});
enum class PropertyKeyAction { Add, Remove, Previous, Next };
// PropertyKey events: target=property ID, first=current tick, offset=action, x=value.
enum class PropertyFlags : unsigned {
    None = 0,
    Mixed = 1,
    Modified = 2,
    Override = 4,
    Favorite = 8,
    Locked = 16,
    Animated = 32,
    Keyed = 64
};
struct PropertyView {
    StableId id = 0;
    const char *label = "", *category = "";
    double value = 0, defaultValue = 0;
    PropertyFlags flags{};
    bool arrayElement=false; // Enables neighbor-based reorder actions.
    IconId icon=IconId::Count; // Optional semantic glyph.
};
struct PropertyProvider {
    void *user = nullptr;
    std::uint64_t revision = 0;
    int count = 0;
    // Host applies search/category filtering; the widget requests only clipped rows.
    std::span<const PropertyView> (*query)(void *, int first, int count, std::string_view search) = nullptr;
    const PropertyView *(*neighbor)(void *,StableId,int direction)=nullptr; // Borrowed sibling, -1/+1; nullptr at boundary.
};
struct PropertyLabels {
    const char *search="Search", *property="Property", *value="Value", *key="Key";
    const char *modifiedSuffix=" (modified)", *overrideSuffix=" (override)", *lockedSuffix=" [locked]";
    const char *favorite="Favorite", *locked="Locked", *overrideValue="Override", *reset="Reset";
    const char *previousKey="Previous key", *nextKey="Next key", *addKey="Add keyframe", *removeKey="Remove keyframe";
    const char *mixed="Mixed",*moveUp="Move up",*moveDown="Move down";
    const char *copy="Copy value",*paste="Paste value";
};
struct PropertyState {
    char search[128]{};
    Transaction drag;
    double draft = 0;
    const IconAtlas *icons = nullptr; // Non-owning host atlas.
    Tick time = 0;
    PropertyLabels labels{};
};
void PropertyGrid(const char *id, const PropertyProvider &provider, PropertyState &state,
                  EventBuffer &events);
enum class AssetStatus { Ready, Loading, Proxy, Missing, Error };
struct AssetView {
    StableId id = 0;
    const char *label = "", *tag = "";
    ImTextureRef thumbnail{};
    AssetStatus status{};
};
struct AssetProvider {
    void *user = nullptr;
    std::uint64_t revision = 0;
    int count = 0;
    std::span<const AssetView> (*query)(void *, int first, int count, std::string_view search) = nullptr;
    // Rebuild the host filter index before visible queries; optional for legacy providers.
    int (*filteredCount)(void *, std::string_view search) = nullptr;
};
struct AssetLabels {
    const char *search="Search",*grid="Grid",*list="List",*tag="Tag";
    std::array<const char *,6> statuses{"All statuses","Ready","Loading","Proxy","Missing","Error"};
    const char *rename="Rename",*duplicate="Duplicate",*remove="Remove";
};
struct AssetState {
    char search[128]{}, rename[256]{};
    bool grid = true;
    StableId renaming = 0;
    char tag[64]{};
    int status = -1; // -1 means all statuses.
    std::span<const StableId> breadcrumbIds;
    const IconAtlas *icons=nullptr;
    AssetLabels labels;
    Transaction renameTransaction;
    bool renameFocus=false;
};
void AssetBrowser(const char *id, const AssetProvider &provider, AssetState &state, Selection &selection,
                  EventBuffer &events, std::span<const char *const> breadcrumbs = {});
bool Splitter(const char *id, float &firstPane, float available, bool vertical = true, float minimum = 80);
void StatusBar(std::string_view text, const Selection &selection);
} // namespace imkit::editor
