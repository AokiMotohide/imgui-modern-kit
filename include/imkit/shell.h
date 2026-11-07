#pragma once

#include <imkit/decor.h>
#include <imkit/workflow.h>

namespace imkit {

struct AppBarView {
    const char* product="";
    const char* document="";
    const char* context="";
    const char* status="";
    bool dirty=false;
    FeedbackKind statusKind=FeedbackKind::Info;
    std::span<const Command> actions{};
};

struct WorkspaceHeaderView {
    const char* eyebrow="";
    const char* title="";
    const char* description="";
    const char* status="";
    FeedbackKind statusKind=FeedbackKind::Info;
    std::span<const Command> actions{};
};

struct BottomActionBarView {
    const char* status="";
    FeedbackKind statusKind=FeedbackKind::Info;
    std::span<const Command> actions{};
};

struct DiagnosticsDrawerState {
    bool open=false;
    bool focusPending=false;
};

struct RightSidePanelState {
    bool open=true;
    float width=320.0f;
};

struct RightSidePanelOptions {
    float minimumWidth=240.0f;
    float minimumContentWidth=160.0f;
    float maximumWidthRatio=0.55f;
    float splitterWidth=6.0f;
    float railWidth=24.0f;
    float keyboardStep=16.0f;
    const char* openLabel="Open inspector";
    const char* closeLabel="Close inspector";
    const char* resizeLabel="Resize inspector";
};

struct RightSidePanelLayout {
    float contentWidth=0.0f;
    float handleWidth=0.0f;
    float panelWidth=0.0f;
    bool panelVisible=false;
};

struct ThemePickerState {
    char search[64]{};
    int focused=0;
};

// All actions are requests. The host owns document, status and command state.
StableId AppBar(const char* id, const AppBarView& view, ToolbarState& state,
                ComponentOptions options={});
StableId WorkspaceHeader(const char* id, const WorkspaceHeaderView& view,
                         ToolbarState& state, ComponentOptions options={});
bool InspectorSection(const char* id, const char* label, const char* description,
                      bool& open, ComponentOptions options={});
bool AdvancedSection(const char* id, const char* label, bool& open,
                     ComponentOptions options={});
StableId BottomActionBar(const char* id, const BottomActionBarView& view,
                         ToolbarState& state, ComponentOptions options={});

// The host draws content, then RightSidePanelHandle, then the optional panel.
// toggleRequested is typically a host-scoped keyboard shortcut such as N.
RightSidePanelLayout ResolveRightSidePanelLayout(
    RightSidePanelState& state, float availableWidth,
    bool toggleRequested=false, RightSidePanelOptions options={});
bool RightSidePanelHandle(const char* id, RightSidePanelState& state,
                          ImVec2 available, RightSidePanelOptions options={},
                          ComponentOptions components={});

// Pair EndDiagnosticsDrawer only when BeginDiagnosticsDrawer returns true.
bool BeginDiagnosticsDrawer(const char* id, const char* title,
                            DiagnosticsDrawerState& state, ImVec2 size={},
                            ComponentOptions options={});
void EndDiagnosticsDrawer();

// Theme selection is host-owned and expressed as a stable ThemePreset value.
bool ThemePicker(const char* id, ThemePreset* selected, ThemePickerState& state,
                 ComponentOptions options={});

// ---- External control indicator ------------------------------------------------------------------------------
// Shows that something other than the person at the screen is driving the application: an automation API, a remote
// operator or an agent. A glow frame around the viewport says so at any distance; a pill names who is in control,
// what it is doing and offers Stop. The host owns the connection, the phase and what Stop does.
enum class ExternalControlPhase {
    Off,         // Nothing shown; a visible frame fades out.
    Connected,   // A client is attached but idle.
    Active,      // The client is operating the application.
    InputLocked, // The client operates and local input is held back by the host.
};
struct ExternalControlView {
    ExternalControlPhase phase=ExternalControlPhase::Off;
    const char* title="External control";
    const char* client="";      // Who is in control; empty hides it.
    const char* activity="";    // What it is doing now; empty hides it.
    const char* lockedText="Input locked"; // Shown in the InputLocked phase; empty hides it.
    const char* stopLabel="Stop";          // Empty hides the button.
};
struct ExternalControlStyle {
    GlowFrameStyle connected, active, locked; // The frame for each phase.
    OverlayAnchor pill=OverlayAnchor::TopCenter;
    bool showPill=true;         // False draws the frame alone.
    float pillMargin=10.0f;     // Distance of the pill from the viewport edge, in pixels.
    float lockedTint=0.06f;     // Opacity of a wash of the locked colour over the viewport; 0 for none.
    bool pulseDot=true;         // The dot in the pill follows the frame's brightness.
    // Steadiness against a host that reports its state in bursts.
    float holdSeconds=1.0f;      // A higher phase stays this long after the host lowers it (Off < Connected < Active < InputLocked).
    float pulseInterval=0.5f;    // Pulses closer together than this make one ripple.
    float transitionSeconds=0.3f; // Colour, strength, spread and line blend over this long when the phase changes.
};
// Connected: a quiet steady accent frame. Active: a breathing accent frame that ripples on Pulse(). InputLocked: a
// stronger steady frame in the warning colour. Change any field afterwards.
ExternalControlStyle MakeExternalControlStyle(const Theme& theme);
struct ExternalControlState {
    GlowFrameState glow;
    ExternalControlPhase shown=ExternalControlPhase::Off; // Last phase that was on, kept while the frame fades out.
    double heldUntil=-1;        // The shown phase is kept until then even if the host lowers it.
    GlowFrameStyle drawn, from; // The look drawn last, and the look a transition starts from.
    double transitionStart=-1;
    double lastPulse=-1;
    bool pulseRequested=false;
    float pillWidth=0;          // The widest the pill has been while shown, so changing text does not make it shrink.
    // A ripple at the next draw, for example when a command arrives. Pulses within pulseInterval make one ripple.
    void Pulse() { pulseRequested=true; }
};
struct ExternalControlRequest {
    bool stopRequested=false;   // Stop was pressed; the host decides what stopping means.
    bool needsFrame=false;      // The picture changes over time; a host that sleeps between frames draws another.
};
// Call once per frame after the windows it should cover. `now` is the host's monotonic time in seconds. `viewport`
// null uses the main viewport. The frame draws on the viewport's foreground draw list and never takes input; the pill
// is a small window that takes input only for Stop, so a pill without a Stop label lets clicks through.
ExternalControlRequest ExternalControlIndicator(const char* id, const ExternalControlView& view, ExternalControlState& state,
                                                double now, const ExternalControlStyle& style,
                                                ImGuiViewport* viewport=nullptr, ComponentOptions options={});

} // namespace imkit
