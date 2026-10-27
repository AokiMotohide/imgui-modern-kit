#pragma once
#include <imkit/components.h>

// Shared vocabulary of the optional Show module (live, schedule, rules, beat and script components).
// Every component draws and reports input only: values, clocks, devices and persistence stay with the host.
namespace imkit::show {
// What a continuous-value control reports for one frame. A click is started, changed and finished in the same
// frame; a drag is started once, changed while the value moves, and finished on release. The vocabulary matches
// the editor modules' Begin/Update/Commit/Cancel, so hosts can group Undo steps, thin out OSC output and route
// MIDI Learn without extra state.
struct ValueEdit {
    bool started = false;   // A gesture began this frame.
    bool changed = false;   // The value moved this frame; `value` holds the new value.
    bool finished = false;  // The gesture ended this frame and the value is final.
    bool cancelled = false; // Escape restored `original`; no `finished` follows for this gesture.
    double original = 0;    // Value when the gesture began.
    double value = 0;       // Current value (first axis for two-axis controls).
    explicit operator bool() const { return started || changed || finished || cancelled; }
};

enum class BindingKind { None, Midi, Osc, Key, Modulator };
// Describes the control assignment the host holds for a parameter. Strings are borrowed for the frame.
struct BindingInfo {
    BindingKind kind = BindingKind::None;
    const char* source = ""; // For example "CC 21", "/layer1/opacity" or "LFO 1".
    const char* range = "";  // Optional mapped range such as "0-100%".
    bool learning = false;   // True while the host waits for the next incoming message.
};
enum class BindingRequest { None, ToggleLearn, Clear };

// Semantic colours the Show components share. They derive from the theme's semantic tokens, so every preset,
// contrast mode and density works without extra tokens. A null theme falls back to the current ImGui style.
struct ShowColors {
    ImVec4 surface{}, raised{}, input{}, text{}, muted{}, border{}, accent{}, onAccent{}, focus{};
    ImVec4 success{}, warning{}, error{};
    ImVec4 busA{}, busB{};
    // Live status vocabulary.
    ImVec4 running{}, queued{}, armed{}, broken{}, done{};
    ImVec4 layer(std::size_t index) const;
    const Theme* theme = nullptr;
};
ShowColors MakeShowColors(const ComponentOptions& options);
} // namespace imkit::show
