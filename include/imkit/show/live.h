#pragma once
#include <imkit/show/controls.h>
#include <imkit/charts.h>
#include <span>

// Live-operation components: cue lists, GO controls, clip launchers, mixers, transitions, effect racks and the
// monitoring tables behind them. Everything is draw-and-request; the host owns playback, devices and state.
namespace imkit::show {
using StableId = accessibility::StableId;

// ---- Cue list -----------------------------------------------------------------------------------------------
enum class CueState { Idle, Standby, Running, Paused, Done, Broken, Disabled };
enum class CueContinue { None, AutoContinue, AutoFollow };
struct CueView {
    StableId id = 0;
    const char* number = "";
    const char* name = "";
    const char* kind = "";      // Short type word such as "Video", "Audio", "Group".
    CueState state = CueState::Idle;
    CueContinue next = CueContinue::None;
    float preWait = 0;          // Seconds.
    float duration = 0;         // Seconds; zero hides the time.
    float elapsed = -1;         // Seconds since start while running; negative hides the progress line.
    int depth = 0;              // Nesting level for cues inside groups.
    bool group = false;
    bool expanded = true;
    const char* problem = "";   // Shown under the row when state is Broken, for example "Missing file: outro.mov".
};
struct CueListState {
    StableId selected = 0;
    StableId standby = 0;       // The cue GO will fire next; drawn with a playhead marker.
};
struct CueListOptions {
    ImVec2 size{0, 0};          // Zero fills the available width and uses eight rows of height.
    float rowHeight = 0;        // Zero uses about 1.4 frame heights.
    bool showTimes = true;
};
struct CueRequest {
    StableId select = 0;        // A row was clicked or reached with the keyboard.
    StableId activate = 0;      // Double-click or Enter: edit or fire this cue.
    StableId toggleGroup = 0;   // A group chevron was clicked.
    StableId context = 0;       // Right-click.
    StableId move = 0;          // Drag-and-drop: this cue ...
    StableId moveBefore = 0;    // ... was dropped before this one (0 with `move` set means the end of the list).
};
// Rows are virtualised, so a hundred thousand cues cost only the visible rows. Up/Down select with the keyboard.
CueRequest CueList(const char* id, std::span<const CueView> cues, CueListState& state, CueListOptions layout = {},
                   ComponentOptions options = {});

// ---- GO bar -------------------------------------------------------------------------------------------------
struct GoBarView {
    const char* next = "";      // Label of the cue GO will fire.
    bool requireArm = true;     // GO stays disabled until armed.
    bool running = false;       // Anything is playing.
    bool paused = false;
    double remainingSeconds = -1; // Time left on the current cue; negative hides it.
};
struct GoBarRequest {
    bool go = false;
    bool pause = false;         // Toggle pause.
    bool stopAll = false;       // Completed hold.
    bool panic = false;         // Completed hold; the host should black out and silence.
};
GoBarRequest GoBar(const char* id, const GoBarView& view, bool* armed, ComponentOptions options = {});

// ---- Clip launcher ------------------------------------------------------------------------------------------
struct LayerView {
    const char* name = "";
    ImVec4 color{};             // Alpha <= 0 picks a layer colour.
    bool mute = false, solo = false, bypass = false;
    int bus = 0;                // 0 = none, 1 = A, 2 = B.
    float* opacity = nullptr;   // 0..1, edited by the header fader when set.
    const char* blend = "";
    bool selected = false;
};
struct ClipLauncherView {
    std::span<const LayerView> layers{};
    std::span<const char* const> columns{}; // Column titles; the length defines the column count.
    std::span<const PadView> cells{};       // layers.size() x columns.size(), row-major by layer.
};
struct ClipLauncherOptions {
    ImVec2 size{0, 0};          // Zero fills the available width and uses the content height up to ten rows.
    float cellWidth = 0;        // Zero uses about 5 frame heights.
    float cellHeight = 0;       // Zero uses about 2.4 frame heights.
    float headerWidth = 0;      // Zero uses about 9 frame heights.
};
enum class LayerToggle { None, Mute, Solo, Bypass, BusA, BusB };
struct ClipLauncherRequest {
    int layer = -1, column = -1; // Cell involved in `pressed`, `released` or `secondary`.
    bool pressed = false, released = false, secondary = false;
    int triggerColumn = -1;     // A column header was clicked (launch the whole column).
    int stopLayer = -1;         // A layer's stop square was clicked.
    int selectLayer = -1;
    int toggledLayer = -1;
    LayerToggle toggle = LayerToggle::None;
    ValueEdit opacity{};        // Edit of `toggledLayer`'s opacity when `opacityLayer` >= 0.
    int opacityLayer = -1;
};
ClipLauncherRequest ClipLauncher(const char* id, const ClipLauncherView& view, ClipLauncherOptions layout = {},
                                 ComponentOptions options = {});

// ---- Mixer --------------------------------------------------------------------------------------------------
struct MixerStrip {
    const char* name = "";
    ImVec4 color{};
    float* gainDb = nullptr;    // Fader value, -60..+6.
    bool* mute = nullptr;
    bool* solo = nullptr;
    int* bus = nullptr;         // 0 none, 1 A, 2 B.
    std::span<const float> levelsDb{};
    std::span<const float> peaksDb{};
};
struct MixerState {
    float crossfade = .5f;      // 0 = A, 1 = B.
    float masterDb = 0;
};
struct MixerOptions {
    float faderHeight = 0;      // Zero uses about 7.5 frame heights.
    std::span<const float> masterLevelsDb{};
    std::span<const float> masterPeaksDb{};
    bool showCrossfader = true;
};
enum class CrossfadeCurve { Linear, EqualPower, Cut };
// Gains for buses A and B at a crossfader position (0..1), including the curve.
struct CrossfadeGains { float a = 1, b = 0; };
CrossfadeGains CrossfadeLaw(float position, CrossfadeCurve curve);
struct MixerRequest {
    int strip = -1;             // Strip whose fader/toggle changed; -2 master, -3 crossfader.
    ValueEdit edit{};
    bool toggled = false;       // Mute, solo or bus changed on `strip`.
};
MixerRequest Mixer(const char* id, std::span<MixerStrip> strips, MixerState& state, MixerOptions layout = {},
                   ComponentOptions options = {});

// ---- Transition bar -----------------------------------------------------------------------------------------
struct TransitionView {
    float* position = nullptr;  // 0..1 T-bar position, host-owned.
    float* durationSeconds = nullptr;
    bool running = false;       // An automatic transition is in progress.
    const char* from = "Program", *to = "Preview";
};
struct TransitionRequest {
    bool cut = false, autoTransition = false;
    ValueEdit tbar{};
    ValueEdit duration{};
};
TransitionRequest TransitionBar(const char* id, const TransitionView& view, ComponentOptions options = {});

// ---- Effect rack --------------------------------------------------------------------------------------------
struct EffectParam {
    const char* name = "";
    float* value = nullptr;
    float minimum = 0, maximum = 1, defaultValue = 0;
    const char* format = "%.2f";
    const char* unit = "";
};
struct EffectSlot {
    const char* name = "";
    bool* bypass = nullptr;
    float* mix = nullptr;       // 0..1 dry/wet.
    std::span<const EffectParam> params{};
};
struct EffectRackState {
    std::array<bool, 32> expanded{};
};
struct EffectRackRequest {
    int slot = -1;
    int remove = -1, moveFrom = -1, moveTo = -1;
    bool add = false;
    int param = -1;
    ValueEdit edit{};           // Edit of `param` (or of the mix when `param` is -1 and `slot` >= 0).
};
EffectRackRequest EffectRack(const char* id, std::span<const EffectSlot> slots, EffectRackState& state,
                             ComponentOptions options = {});

// ---- Modulator ----------------------------------------------------------------------------------------------
enum class ModShape { Sine, Triangle, Saw, Square, Random };
// Value in -1..1 for a phase in [0, 1). Random is a stepped sample-and-hold keyed by the phase cycle number.
float ModulatorValue(ModShape shape, float phase, unsigned seed = 0);
struct ModulatorView {
    ModShape* shape = nullptr;
    float* rate = nullptr;      // Hz, or divisions per beat when `beatSync`.
    float* phase = nullptr;     // Start offset 0..1.
    float* amount = nullptr;    // 0..1.
    float* offset = nullptr;    // -1..1.
    bool* beatSync = nullptr;
    float playPhase = -1;       // Current 0..1 position shown on the preview; negative hides it.
};
struct ModulatorRequest {
    ValueEdit edit{};
    int field = -1;             // 0 rate, 1 phase, 2 amount, 3 offset.
    bool shapeChanged = false;
};
ModulatorRequest ModulatorEditor(const char* id, const ModulatorView& view, ComponentOptions options = {});

// ---- Mapping table and message monitor ---------------------------------------------------------------------
struct MappingRow {
    StableId id = 0;
    BindingKind kind = BindingKind::Midi;
    const char* source = "";    // "CC 21 ch1", "/layer1/opacity".
    const char* target = "";    // "Layer 1 / Opacity".
    const char* range = "";
    bool enabled = true;
    bool conflict = false;      // Another row listens to the same source.
};
struct MappingState {
    StableId selected = 0;
    StableId learning = 0;      // Row waiting for the next message, or 0.
};
struct MappingRequest {
    StableId select = 0, toggleEnabled = 0, remove = 0, learn = 0;
};
MappingRequest MappingTable(const char* id, std::span<const MappingRow> rows, MappingState& state, ComponentOptions options = {});

struct MessageView {
    double seconds = 0;         // Host clock, shown as mm:ss.mmm relative to the first row.
    BindingKind kind = BindingKind::Midi;
    const char* text = "";      // "CC 21 ch1 = 64", "/layer1/opacity 0.5".
};
struct MessageMonitorState {
    bool paused = false;
    bool midi = true, osc = true, key = true;
    char filter[64]{};
    bool follow = true;
};
struct MessageMonitorRequest {
    int assign = -1;            // Row index whose message should become a mapping source.
    bool clear = false;
};
MessageMonitorRequest MessageMonitor(const char* id, std::span<const MessageView> messages, MessageMonitorState& state,
                                     ImVec2 size = {0, 0}, ComponentOptions options = {});

// ---- Device health ------------------------------------------------------------------------------------------
struct DeviceView {
    const char* name = "";
    const char* status = "";
    LedState led = LedState::On;
    float fps = -1, gpuPercent = -1, temperatureC = -1; // Negative hides the figure.
    int droppedFrames = -1;
    std::span<const float> fpsHistory{};
};
// Returns the index of the device row that was clicked, or -1.
int DeviceHealth(const char* id, std::span<const DeviceView> devices, ComponentOptions options = {});
} // namespace imkit::show
