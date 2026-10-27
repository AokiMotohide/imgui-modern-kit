#pragma once
#include <imkit/show/common.h>
#include <imkit/editor_core.h>
#include <array>
#include <span>

// Performance-surface primitives: faders, knobs, pads, meters, readouts and the safety controls a live operator
// needs. Values are host-owned (pointer arguments) and edits are reported through ValueEdit.
namespace imkit::show {
// ---- Fader --------------------------------------------------------------------------------------------------
enum class FaderScale { Linear, Decibel };
// Maps a value to the 0..1 position along the rail and back. Decibel uses a console-style taper (position =
// ((v - min) / (max - min))^3), so 0 dB of a -60..+6 range sits about 75% up. Both clamp and ignore non-finite input.
float FaderPosition(float value, float minimum, float maximum, FaderScale scale);
float FaderValue(float position, float minimum, float maximum, FaderScale scale);
struct FaderOptions {
    ImVec2 size{0, 0};          // Zero derives it from the frame height (vertical 1.8 x 7.5, horizontal full width x 1.8).
    bool vertical = true;
    FaderScale scale = FaderScale::Linear;
    float minimum = 0, maximum = 1;
    float defaultValue = 0;     // Double-click restores this.
    float step = 0;             // Snap; zero is continuous.
    bool bipolar = false;       // Fill starts at the middle of the range instead of the minimum.
    std::span<const float> ticks{}; // Values that get a mark along the rail.
    const char* tickFormat = nullptr; // Optional label format for ticks, for example "%.0f".
    const char* format = "%.1f";
    const char* unit = "";
    bool showValue = true;
    const BindingInfo* binding = nullptr;
};
// Drag the cap (relative) or click the rail (absolute); Shift = fine; double-click = default; Ctrl+click types a
// value; Ctrl+wheel and +/- nudge. A focused fader answers the accessibility Increment/Decrement actions.
ValueEdit Fader(const char* label, float* value, FaderOptions layout = {}, ComponentOptions options = {});

// ---- Knob ---------------------------------------------------------------------------------------------------
struct KnobOptions {
    float diameter = 0;         // Zero derives it from the frame height.
    float minimum = 0, maximum = 1, defaultValue = 0, step = 0;
    bool bipolar = false;
    const char* format = "%.2f";
    const char* unit = "";
    bool showLabel = true, showValue = true;
    const BindingInfo* binding = nullptr;
};
// Vertical drag turns the knob; Shift = fine; double-click = default; Ctrl+click types a value.
ValueEdit Knob(const char* label, float* value, KnobOptions layout = {}, ComponentOptions options = {});

// ---- XY pad -------------------------------------------------------------------------------------------------
struct XYPadOptions {
    ImVec2 size{0, 0};          // Zero draws a square of about 8 frame heights.
    float minimumX = 0, maximumX = 1, minimumY = 0, maximumY = 1;
    float defaultX = .5f, defaultY = .5f;
    int divisions = 4;          // Grid lines per axis; 0 hides the grid.
    const char* format = "%.2f";
};
// Returns the edit of the X axis (`value` holds X); read *y for the other axis. Y grows upwards. Double-click
// restores the defaults.
ValueEdit XYPad(const char* label, float* x, float* y, XYPadOptions layout = {}, ComponentOptions options = {});

// ---- Pads ---------------------------------------------------------------------------------------------------
enum class PadState { Empty, Idle, Loaded, Queued, Playing, Error };
struct PadView {
    const char* label = "";
    const char* caption = "";   // Small second line, for example a key or a duration.
    PadState state = PadState::Idle;
    ImVec4 color{};             // Alpha <= 0 picks a layer colour from the index.
    float progress = -1;        // 0..1 playback progress; negative hides it.
    bool latched = false;       // Toggle pads that are on.
};
struct PadGridOptions {
    int columns = 4;
    float padHeight = 0;        // Zero derives it from the frame height.
    float padWidth = 0;         // Zero stretches the pads to the available width.
    const char* dropPayload = nullptr; // Pads accept this ImGui drag payload type when set.
};
struct PadEvent {
    int index = -1;             // Pad that was pressed, released or dropped on; -1 for none.
    bool pressed = false;       // Primary press this frame (momentary behaviour starts here).
    bool released = false;      // Primary release this frame.
    bool secondary = false;     // Right-click or context request this frame.
    const ImGuiPayload* payload = nullptr; // Set on the frame a drag payload was dropped.
};
PadEvent PadGrid(const char* id, std::span<const PadView> pads, PadGridOptions layout = {}, ComponentOptions options = {});

// ---- Level meter --------------------------------------------------------------------------------------------
// Peak and hold bookkeeping for MeterBar; host-owned and fixed size.
struct PeakHold {
    static constexpr float Silent = -120.f;
    // dB; level is the latest input, peak the falling marker, held a slower envelope.
    std::array<float, 8> level{Silent, Silent, Silent, Silent, Silent, Silent, Silent, Silent};
    std::array<float, 8> peak{Silent, Silent, Silent, Silent, Silent, Silent, Silent, Silent};
    std::array<float, 8> held{Silent, Silent, Silent, Silent, Silent, Silent, Silent, Silent};
    std::array<float, 8> age{};
    void Update(std::span<const float> decibels, float deltaSeconds, float holdSeconds = 1.2f, float fallDbPerSecond = 20.f);
};
struct MeterOptions {
    ImVec2 size{0, 0};          // Zero derives it (vertical 0.9 x 7 frame heights; horizontal full width x 0.9).
    bool vertical = true;
    float minimumDb = -60, maximumDb = 6;
    float warnDb = -12, clipDb = -3; // Zone thresholds.
    std::span<const float> scaleDb{}; // Tick labels, for example {0, -6, -12, -24, -48}.
    bool showPeak = true;
};
// Draws one bar per level (up to 8 channels). Levels and peaks are in dB. Read-only; announced as a Meter.
void MeterBar(const char* id, std::span<const float> levelsDb, std::span<const float> peaksDb, MeterOptions layout = {},
              ComponentOptions options = {});
// Linear amplitude to dB (-inf is clamped to -120).
float AmplitudeToDecibels(float amplitude);

// ---- Timecode and countdown --------------------------------------------------------------------------------
enum class ClockUrgency { Normal, Warning, Critical };
struct TimecodeView {
    const char* label = "";
    editor::Tick time = 0;
    editor::FrameRate rate{30, 1};
    bool dropFrame = false;
    bool countdown = false;     // Shows "-" before the time and colours by urgency.
    bool running = false;       // Dims the separators while stopped.
    ClockUrgency urgency = ClockUrgency::Normal;
    float size = 0;             // Font size in pixels; zero uses about 1.7 frame heights.
};
// Fixed-width digits so the readout never jitters. The caller (or a helper) chooses urgency.
void TimecodeDisplay(const char* id, const TimecodeView& view, ComponentOptions options = {});
// Normal above `warnSeconds`, Warning above `criticalSeconds`, otherwise Critical.
ClockUrgency CountdownUrgency(double secondsRemaining, double warnSeconds = 60, double criticalSeconds = 10);

// ---- Binding badge, safety controls and status lights --------------------------------------------------
// Click the badge to toggle MIDI/OSC Learn; the small cross clears the assignment.
BindingRequest BindingBadge(const char* id, const BindingInfo& info, ComponentOptions options = {});

struct HoldButtonOptions {
    float seconds = .8f;        // How long the button must be held.
    bool destructive = true;    // Error-coloured progress ring.
    ImVec2 size{0, 0};
};
// Returns true on the frame the hold completes. Keyboard: hold Space or Enter. The accessibility Press action
// completes immediately, because assistive technology cannot hold. Returns false while disconnected/disabled.
bool HoldButton(const char* label, HoldButtonOptions layout = {}, ComponentOptions options = {});
// Lock-style toggle that arms a dangerous action (GO, Panic). Returns true on change.
bool ArmSwitch(const char* label, bool* armed, ComponentOptions options = {});

enum class LedState { Off, On, Warning, Error, Live };
// Light with shape and text so state never relies on colour alone: Live is a filled disc ("ON AIR"), Warning a
// triangle, Error a cross, On a ring, Off a dim dot.
void StatusLed(const char* id, LedState state, const char* label, ComponentOptions options = {});
} // namespace imkit::show
