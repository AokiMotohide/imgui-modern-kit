#pragma once
#include <imkit/components.h>
#include <span>

namespace imkit {
// Draw-and-request control for pan / tilt / zoom cameras. The library never talks to a camera: the host reads
// the returned PtzRequest each frame, drives its device, and feeds the camera's reported position back through
// PtzView. Nothing is retained between frames except the host-owned PtzState.
struct PtzPreset {
    const char* name = ""; // Optional; shown as a tooltip and used as the accessible name. Borrowed for the frame.
    bool stored = true;    // False draws an empty slot; clicking an empty slot asks the host to store it.
};
struct PtzPosition {
    float pan = 0, tilt = 0; // Degrees.
    float zoom = 1;          // Magnification.
    bool valid = false;      // False shows dashes instead of numbers.
};
// The controls a camera offers. A control it lacks is not drawn and never reports a request, so a pan-only head, a
// zoom-only lens or a camera without presets gets a smaller control instead of dead buttons. Everything defaults to
// true, which keeps the full control.
struct PtzCapabilities {
    bool pan = true;          // Left and right arrows, and the horizontal axis of the pad.
    bool tilt = true;         // Up and down arrows, and the vertical axis of the pad. Diagonals need both.
    bool zoom = true;         // Zoom rocker and zoom bar.
    bool home = true;         // Home well in the centre of the pad. Needs pan or tilt.
    bool presets = true;      // Recall from the preset grid.
    bool presetStore = true;  // Store from the grid; false leaves the slots recall-only and empty slots inert.
    bool speed = true;        // Variable speed; false hides the speed slider and sends full velocity.
};
// Readout text formatted by the host, for devices that report neither degrees nor magnification (normalised
// -1..1 pan and tilt, or a 0..1 zoom). A null field falls back to PtzView::position.
struct PtzReadoutText {
    const char* pan = nullptr;
    const char* tilt = nullptr;
    const char* zoom = nullptr;
};
struct PtzView {
    const char* label = "";  // Heading, for example the camera name.
    const char* status = ""; // Short text next to the connection dot, for example "Connected" or "Offline".
    PtzPosition position{};
    float zoomFraction = -1; // 0..1 zoom position bar; negative hides it.
    std::span<const PtzPreset> presets{};
    int activePreset = -1;   // Preset matching the current position, or -1.
    // False disables every control, ends a move in progress and shows the status as offline.
    bool connected = true;
    PtzCapabilities capabilities{};
    PtzReadoutText readoutText{};
};
struct PtzState {
    float speed = .5f; // 0..1 scale applied to pad and zoom velocity. Edited by the speed slider.
};
struct PtzOptions {
    // Zero derives the pad diameter from the frame height (about 6.5 frame heights, 4.2 when compact).
    float diameter = 0;
    // Pad, zoom and Home only: no heading, readout, speed or presets.
    bool compact = false;
    // Layout switches. A part also needs the camera to offer it (PtzCapabilities) before it is drawn.
    bool readout = true;
    bool speed = true;
    bool presets = true;
    int presetColumns = 4;
};
struct PtzRequest {
    // Pan (x, right positive) and tilt (y, up positive) in [-1, 1] with the speed scale applied.
    ImVec2 velocity{};
    // Zoom velocity in [-1, 1]; positive zooms in.
    float zoom = 0;
    bool moving = false;      // velocity or zoom is non-zero this frame.
    bool stop = false;        // True on the one frame after a move ends, including when the view disconnects.
    bool home = false;
    int recallPreset = -1;
    int storePreset = -1;
    bool speedChanged = false;
};
// Maps a pointer position on the pad to a unit-circle vector (screen axes, y down). Inside the dead zone it is
// zero; beyond it the magnitude rises linearly to 1 at the rim and the vector is clamped there.
ImVec2 PtzPadVector(ImVec2 point, ImVec2 centre, float radius, float deadZone);
// Draws the control and returns what the user asked for this frame. Direction arrows move at full speed while
// held; pressing elsewhere on the pad is an analog joystick. Presets recall on click, store on Shift+click or a
// 0.6 s hold, and an empty slot stores on click. Arrow buttons are focusable (Tab, then hold Space or Enter).
// Only the controls in PtzView::capabilities are drawn; with none of them and no presets it shows a short notice.
PtzRequest PtzControl(const char* id, const PtzView& view, PtzState& state, PtzOptions layout = {},
                      ComponentOptions options = {});
} // namespace imkit
