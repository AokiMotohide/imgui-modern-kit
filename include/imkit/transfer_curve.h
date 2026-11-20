#pragma once
#include <imkit/components.h>
#include <span>

namespace imkit {
// One knot of a piecewise cubic Bezier transfer curve. All coordinates are absolute values in 0..1.
// The segment between knot i and knot i+1 uses pos(i), out(i), in(i+1), pos(i+1).
struct TransferCurveNode {
    ImVec2 pos{0.f, 0.f};
    ImVec2 in{0.f, 0.f};
    ImVec2 out{0.f, 0.f};
};
// Optional display mapping of the y axis for one series (for example y^(1/gamma)). Both functions must be given
// together; `inverse` turns a dragged display value back into a stored value. Null draws and edits y unchanged.
using TransferCurveMapFn = float (*)(float y, void* user);
struct TransferCurveSeries {
    std::span<TransferCurveNode> nodes; // Host-owned; edited in place.
    ImU32 color = 0xFFFFFFFFu;
    bool dashed = false;
    TransferCurveMapFn toDisplay = nullptr;
    TransferCurveMapFn fromDisplay = nullptr;
    void* user = nullptr;
};
struct TransferCurveState {
    int series = 0; // Selected series; the host stores it.
    int node = -1;  // Selected node of that series; -1 is none.
};
struct TransferCurveOptions {
    // Zero uses the available width / the same height.
    ImVec2 size{0.f, 0.f};
    // Keeps every series non-decreasing after an edit (TransferCurveClampMonotone).
    bool monotone = false;
    // Grid lines per axis (0 draws none).
    int gridDivisions = 4;
    // Distance in pixels within which a click selects a node, a handle or another series' curve.
    float pickRadius = 8.f;
};
struct TransferCurveInteraction {
    bool activated = false;     // A drag started.
    bool changed = false;       // A value changed this frame.
    bool committed = false;     // A drag ended after a change, or a key step happened.
    bool canceled = false;      // Esc during a drag restored the values from before the drag.
    bool seriesChanged = false; // state.series changed by a click.
    bool nodeChanged = false;   // state.node changed (click, Esc, or a series switch).
};
// Draws a 0..1 frame with every series as a cubic Bezier, the selected series' knots, and the in/out handles of the
// selected knot only. Drag a knot (its handles follow) or a handle; x stays between the neighbouring knots and the
// first and last knot keep their x. Click near another series' curve to select it. With keyboard focus the arrow keys
// move the selected knot by 0.01 (Shift: 0.04) and Esc clears the knot selection. Returns true on the frame a value or
// the selection changes; `interaction` gives the detail.
bool TransferCurveEditor(const char* id, std::span<TransferCurveSeries> series, TransferCurveState& state,
                         TransferCurveOptions layout = {}, TransferCurveInteraction* interaction = nullptr,
                         ComponentOptions options = {});
// Clamps handles to their segment box ([x0,x1] x [y0,y1]) and raises knot y values so the curve never decreases.
// Returns true when anything changed. Use it as the host rule after an edit when `monotone` is off.
bool TransferCurveClampMonotone(std::span<TransferCurveNode> nodes);
} // namespace imkit
