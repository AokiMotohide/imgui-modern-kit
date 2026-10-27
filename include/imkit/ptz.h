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
struct PtzView {
    const char* label = "";  // Heading, for example the camera name.
    const char* status = ""; // Short text next to the connection dot, for example "Connected" or "Offline".
    PtzPosition position{};
    float zoomFraction = -1; // 0..1 zoom position bar; negative hides it.
    std::span<const PtzPreset> presets{};
    int activePreset = -1;   // Preset matching the current position, or -1.
    // False disables every control, ends a move in progress and shows the status as offline.
    bool connected = true;
};
struct PtzState {
    float speed = .5f; // 0..1 scale applied to pad and zoom velocity. Edited by the speed slider.
    // Internal bookkeeping for the move in progress; hosts only need to keep the struct alive.
    int held = 0;
    bool padValid = false;
};
struct PtzOptions {
    // Zero derives the pad diameter from the frame height (about 6.5 frame heights, 4.2 when compact).
    float diameter = 0;
    // Pad, zoom and Home only: no heading, readout, speed or presets.
    bool compact = false;
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
PtzRequest PtzControl(const char* id, const PtzView& view, PtzState& state, PtzOptions layout = {},
                      ComponentOptions options = {});
} // namespace imkit
