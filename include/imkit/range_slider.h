#pragma once
#include <imkit/components.h>

namespace imkit {
struct RangeSliderOptions {
    // Zero uses the current item width (SetNextItemWidth / PushItemWidth), like the native sliders.
    float width = 0;
    // Smallest allowed distance between the two handles, in value units.
    float minGap = 0;
    // Values snap to multiples of step from the minimum; zero is continuous (integers always snap to 1).
    float step = 0;
    // Short marks under the rail, evenly spaced including both ends; fewer than two draws none.
    int ticks = 0;
    // Draws the current low and high value at the ends of the row.
    bool valueLabels = true;
};
// Two-handle range slider. The host owns both values; the call edits them in place and returns true on the
// frame they change. Drag a handle, drag the filled span to move both, or click the rail to move the nearest
// handle. A focused handle also answers +/- (Shift = x10); the arrow keys stay with Dear ImGui navigation.
// Handles are announced as two accessibility Group nodes with numeric ranges and Increment/Decrement actions.
// Values are kept within [minimum, maximum] with low <= high; non-finite input is replaced by the nearest bound.
bool RangeSlider(const char* label, float* low, float* high, float minimum, float maximum,
                 const char* format = "%.1f", RangeSliderOptions layout = {}, ComponentOptions options = {});
bool RangeSliderInt(const char* label, int* low, int* high, int minimum, int maximum, const char* format = "%d",
                    RangeSliderOptions layout = {}, ComponentOptions options = {});
} // namespace imkit
