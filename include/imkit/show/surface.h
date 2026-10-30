#pragma once
#include <imkit/show/image.h>
#include <array>
#include <span>

// ControlSurface: a console-style canvas where an operator (or the person building the show) places faders, knobs, keys,
// pictures and displays freely on a grid. The host owns the layout model and the values; the surface draws it, routes
// input to the real controls in operate mode, and reports selection, move, resize, add and delete requests in edit mode.
namespace imkit::show {
using StableId = accessibility::StableId;

enum class SurfaceKind { Button, Fader, Knob, XYPad, Meter, Display, Image, Panel, Custom };

// Position and size in grid cells.
struct SurfaceRect {
    float x = 0, y = 0, w = 1, h = 1;
};

struct SurfaceItem {
    StableId id = 0;            // Unique and stable; selection and requests refer to it. Zero items are skipped.
    SurfaceKind kind = SurfaceKind::Button;
    SurfaceRect rect{};
    const char* label = "";     // Control label, display caption or panel title.
    const char* text = "";      // Display: the big readout. Button: the corner hint.
    ImVec4 color{};             // Lamp or accent colour; alpha <= 0 uses the theme accent.
    float* value = nullptr;     // Fader, Knob and XYPad (x); Meter level in dB.
    float* value2 = nullptr;    // XYPad (y).
    float minimum = 0, maximum = 1, defaultValue = 0;
    bool* on = nullptr;         // Toggle state of a Button.
    ConsoleMode mode = ConsoleMode::Momentary; // Button and Image behaviour.
    bool solid = false;         // Button: filled with `color` while lit (a hero key such as GO).
    float labelScale = 1.f;     // Button: label size multiplier.
    ImageButtonView image{};    // Image items (and the picture of a Button when it has one).
    const BindingInfo* binding = nullptr;
    bool locked = false;        // Edit mode cannot move or resize it.
    bool disabled = false;      // Inert in operate mode.
    bool hidden = false;
    // Custom: draws inside the item. The cursor is at the item's top-left corner and `size` is its size in pixels.
    void (*draw)(const SurfaceItem& item, ImVec2 size, void* user) = nullptr;
    void* user = nullptr;
};

inline constexpr std::size_t SurfaceSelectionCapacity = 32;

// Host-owned, fixed size. The fields after `selectedCount` are gesture bookkeeping managed by ControlSurface.
struct SurfaceState {
    bool editing = false;       // false: operate, true: edit.
    std::array<StableId, SurfaceSelectionCapacity> selected{};
    int selectedCount = 0;
    int gesture = 0;            // 0 none, 1 move, 2 resize, 3 marquee.
    int handle = -1;
    bool dragging = false;
    bool announced = false;     // `started` was already reported for the current gesture.
    StableId pressed = 0;
    ImVec2 startMouse{0, 0};
    std::array<SurfaceRect, SurfaceSelectionCapacity> startRects{};
};
bool SurfaceIsSelected(const SurfaceState& state, StableId id);
// Replaces the selection (or adds to / removes from it when `additive`). Returns false when the capacity is exceeded.
bool SurfaceSelect(SurfaceState& state, StableId id, bool additive);
void SurfaceClearSelection(SurfaceState& state);

struct SurfaceOptions {
    ImVec2 size{0, 0};          // Viewport in pixels; zero uses the available width and the height that keeps the grid's aspect.
    float cell = 0;             // Pixels per grid cell when `fit` is false; zero uses 1.4 frame heights.
    int columns = 24, rows = 12; // The design grid; the whole grid is shown.
    bool fit = true;            // Scale the grid uniformly to fill the viewport.
    bool showGrid = true;       // Dots at the grid corners while editing.
    bool snap = true;
    float snapStep = .5f;       // Snap granularity in cells.
    float minItemSize = 1.f;    // Smallest width and height in cells.
};

enum class SurfaceHandle { TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left };

// One moved or resized item: the new, snapped rectangle in cells.
struct SurfaceItemRect {
    StableId id = 0;
    SurfaceRect rect{};
};
// The phases of a move or resize gesture, like ValueEdit's: Undo can group everything from `started` to `finished`.
struct SurfaceLayoutEdit {
    bool started = false, changed = false, finished = false, cancelled = false;
    bool resize = false;        // false: move.
    std::array<SurfaceItemRect, SurfaceSelectionCapacity> items{};
    int count = 0;              // Entries of `items` in use. After `cancelled` they hold the original rectangles.
};
struct SurfaceRequest {
    SurfaceLayoutEdit layout{};
    StableId operated = 0;      // Operate mode: the item whose control was used.
    ValueEdit value{};          // Fader, Knob and XYPad edits of `operated`.
    bool pressed = false, released = false, toggled = false; // Button and Image items.
    ImageButtonRequest image{}; // Image items: choose, clear, adjust and drop requests.
    bool selectionChanged = false;
    bool removeSelected = false;    // Delete was pressed.
    bool duplicateSelected = false; // Ctrl+D was pressed.
    bool add = false;           // A palette entry was dropped on the surface.
    SurfaceKind addKind = SurfaceKind::Button;
    ImVec2 addAt{0, 0};         // Snapped grid position of the drop.
};
SurfaceRequest ControlSurface(const char* id, std::span<const SurfaceItem> items, SurfaceState& state, SurfaceOptions layout = {},
                              ComponentOptions options = {});

// Draggable list of item kinds for building a surface; dropping one on the surface reports `add`.
void SurfacePalette(const char* id, ComponentOptions options = {});

// ---- Geometry helpers (pure) --------------------------------------------------------------------------------
// Snaps position and size to `step` (zero keeps them as they are), keeps at least `minSize` and stays inside the grid.
SurfaceRect SnapRect(SurfaceRect rect, float step, int columns, int rows, float minSize);
// The rectangle after dragging `handle` of `start` by `deltaCells`; snapped, at least `minSize`, inside the grid.
SurfaceRect ResizeRect(SurfaceRect start, SurfaceHandle handle, ImVec2 deltaCells, float step, float minSize, int columns, int rows);
// Handle under `point` for the pixel rectangle [min, max], or -1. `handleSize` is the hit size in pixels.
int HitHandle(ImVec2 min, ImVec2 max, ImVec2 point, float handleSize);
// Alignment of `moving` with the nearest edge or centre line of `others` within `threshold` cells.
struct SurfaceGuides {
    bool hasX = false, hasY = false;
    float dx = 0, dy = 0;       // Offset to add to the moving rectangle.
    float x = 0, y = 0;         // Guide line positions in cells.
};
SurfaceGuides AlignGuides(SurfaceRect moving, std::span<const SurfaceRect> others, float threshold);
const char* SurfaceKindName(SurfaceKind kind);
} // namespace imkit::show
