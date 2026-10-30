#pragma once
#include <imkit/show/controls.h>
#include <array>
#include <span>

// General-purpose parts for LED and light output work: multi-channel colour, gradients, pixel previews, power budgets,
// colour order and test patterns. The library draws and reports input only; fixtures, protocols and persistence stay
// with the host.
namespace imkit::show {
// ---- Light colour -------------------------------------------------------------------------------------------
// A light's channels, each 0..1. `intensity` is the master dimmer applied on top.
struct LightColor {
    float r = 0, g = 0, b = 0;
    float w = 0, a = 0, uv = 0; // White, amber and ultraviolet, for fixtures that have them.
    float intensity = 1;
};
// Colour of a black-body radiator, normalised so the brightest channel is 1. The approximation covers 1667..25000 K;
// `kelvin` is clamped to that range and a non-finite value gives 6500 K.
ImVec4 KelvinToRgb(float kelvin);
// Moves the shared part of the three colour channels onto the white channel (w = min(r, g, b)).
LightColor RgbToRgbw(float r, float g, float b);
// How the light looks on screen: the channels mixed into sRGB-like 0..1 values with the dimmer applied.
ImVec4 MixLightColor(const LightColor& color);

struct LedColorOptions {
    bool white = false, amber = false, uv = false; // Channels beyond RGB.
    bool dimmer = true;         // Intensity track.
    bool cct = false;           // Colour-temperature track; sets RGB from the black-body colour.
    float cctMinimum = 2000, cctMaximum = 10000;
    bool hex = true;            // #RRGGBB entry.
    std::span<const ImVec4> swatches{};
    float width = 0;            // Zero uses about 14 frame heights.
};
// Which control produced the edit.
enum class LedColorField { None, Red, Green, Blue, White, Amber, Uv, Intensity, Kelvin, Area, Hue, Hex, Swatch };
struct LedColorRequest {
    ValueEdit edit{};           // Phases of the gesture; `value` is the edited field's value (0..1, Kelvin for Kelvin).
    LedColorField field = LedColorField::None;
};
// `kelvin` may be null; with `cct` it holds the temperature the track shows (host-owned). A saturation/value area with
// a hue strip, one gradient track per channel, an optional hex entry and swatches.
LedColorRequest LedColorPicker(const char* id, LightColor* color, float* kelvin, LedColorOptions layout = {}, ComponentOptions options = {});

// ---- Gradient ----------------------------------------------------------------------------------------------
struct GradientStop {
    float position = 0;         // 0..1.
    ImVec4 color{1, 1, 1, 1};
};
enum class GradientInterpolation { Linear, Step, Smooth };
// Colour at `t` (0..1). Stops may be in any order; an empty list gives transparent black.
ImVec4 SampleGradient(std::span<const GradientStop> stops, float t, GradientInterpolation mode = GradientInterpolation::Linear);

struct GradientEditorView {
    GradientStop* stops = nullptr;  // Host-owned array; edited in place.
    int* count = nullptr;           // Stops in use.
    int capacity = 0;               // Size of the array.
    GradientInterpolation interpolation = GradientInterpolation::Linear;
};
struct GradientEditorOptions {
    float width = 0;            // Zero fills the available width.
    float height = 0;           // Zero uses 2 frame heights for the bar.
    int minimumStops = 2;
    std::span<const ImVec4> swatches{};
};
struct GradientRequest {
    ValueEdit edit{};           // Position drag of `stop`; `value` is its new position.
    int stop = -1;              // Stop the edit or the colour change belongs to.
    bool added = false;         // Double-click on the bar added a stop (`stop` is its index).
    bool removed = false;       // Delete or the remove button removed `stop`.
    bool colorChanged = false;  // The selected stop's colour changed.
};
// Click a marker to select it, drag to move it, double-click the bar to add a stop, Delete removes. `selected` is the
// selected stop index (host-owned, -1 for none). The colour of the selected stop opens in a popup picker.
GradientRequest GradientEditor(const char* id, const GradientEditorView& view, int* selected, GradientEditorOptions layout = {},
                               ComponentOptions options = {});

// ---- Pixel preview -------------------------------------------------------------------------------------------
struct PixelPreviewView {
    std::span<const ImVec4> colors{};    // One colour per pixel, 0..1 (alpha ignored).
    std::span<const ImVec2> positions{}; // Optional pixel centres in 0..1 of the preview; empty uses the automatic layout.
    int columns = 0;            // Automatic layout: pixels per row; zero draws one row (a strip).
    bool serpentine = false;    // Automatic layout: every other row runs backwards, like zig-zag wiring.
};
struct PixelPreviewOptions {
    ImVec2 size{0, 0};          // Zero fills the width with a height that suits the layout.
    float dotScale = .8f;       // Dot diameter relative to the pixel pitch.
    bool glow = true;
    bool showWiring = false;    // A line through the pixels in index order (up to 2000 pixels).
    bool showIndices = false;   // Pixel numbers, when they fit.
};
struct PixelPreviewRequest {
    int hovered = -1;
    int clicked = -1;
};
PixelPreviewRequest PixelPreview(const char* id, const PixelPreviewView& view, int* selected = nullptr, PixelPreviewOptions layout = {},
                                 ComponentOptions options = {});

// ---- Budget bar ------------------------------------------------------------------------------------------------
struct BudgetSegment {
    const char* label = "";
    float amount = 0;
    ImVec4 color{};             // Alpha <= 0 picks a chart colour.
};
struct BudgetBarView {
    std::span<const BudgetSegment> segments{};
    float limit = 1;            // The capacity the segments are measured against.
    float warnFraction = .8f;   // Warning starts at this share of `limit`.
    const char* title = "";
    const char* unit = "A";
};
struct BudgetBarOptions {
    ImVec2 size{0, 0};          // Zero fills the width, about 1.2 frame heights high.
    const char* format = "%.1f";
};
enum class BudgetStatus { Ok, Warning, Over };
// Stacked segments against a limit, with the warning point marked. The status never relies on colour alone: the label
// reads "Near limit" or "Over limit" and the bar ends in a marker.
BudgetStatus BudgetBar(const char* id, const BudgetBarView& view, BudgetBarOptions layout = {}, ComponentOptions options = {});
// Current drawn by `pixels` LEDs at `brightness` (0..1) showing content of average level `load` (0..1; 1 = full white).
float EstimateLedCurrent(int pixels, float amperesPerPixel, float brightness, float load = 1.f);

// ---- Colour order ----------------------------------------------------------------------------------------------
struct ColorOrderRequest {
    bool changed = false;       // `order` holds a new order.
    bool test = false;          // The user asked to light R, G and B in turn so the order can be checked.
};
// `order` is a host-owned buffer of five chars ("GRB", "GRBW"). The list offers the six RGB permutations, or the
// common RGBW ones when `withWhite`.
ColorOrderRequest ColorOrderPicker(const char* id, char* order, bool withWhite = false, ComponentOptions options = {});
// Writes every permutation of RGB (6) or RGBW (24) to `out`; returns how many fit.
std::size_t ColorOrderChoices(bool withWhite, std::span<std::array<char, 5>> out);

// ---- Test patterns ---------------------------------------------------------------------------------------------
enum class TestPattern { Off, White, Red, Green, Blue, Black, Gradient, Grid, Checker, Chase, Strobe, Numbers, BurnIn };
struct TestPatternOptions {
    bool compact = false;       // Smaller chips.
};
struct TestPatternRequest {
    bool changed = false;
    TestPattern pattern = TestPattern::Off; // The pattern to show now; Off when the active chip was pressed again.
};
// Pick a pattern to show on the output; pressing the active one turns it off. Rendering is the host's job.
TestPatternRequest TestPatternBar(const char* id, TestPattern active, TestPatternOptions layout = {}, ComponentOptions options = {});
const char* TestPatternName(TestPattern pattern);

// ---- Port strip ------------------------------------------------------------------------------------------------
enum class PortState { Idle, Active, Warning, Error, Disabled };
struct PortView {
    const char* label = "";     // "1", "A2", "DMX out".
    const char* detail = "";    // "U1-U3, 510 px".
    PortState state = PortState::Idle;
    float load = -1;            // 0..1 line along the bottom; negative hides it.
};
struct PortStripOptions {
    float portWidth = 0;        // Zero uses about 6 frame heights.
};
// Returns the index of a port that was clicked, or -1. `selected` is the selected port (host-owned, may be null).
int PortStrip(const char* id, std::span<const PortView> ports, int* selected = nullptr, PortStripOptions layout = {}, ComponentOptions options = {});
} // namespace imkit::show
