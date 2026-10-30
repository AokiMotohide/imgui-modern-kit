#pragma once
#include <imkit/show/controls.h>
#include <span>

// Address-space views for DMX universes and other fixed-size channel ranges: who uses which slots, where blocks
// overlap, and what each channel is carrying. Draw-and-request; the host owns the blocks and the values.
namespace imkit::show {
using StableId = accessibility::StableId;

// A run of slots used by one fixture, group or pixel output. `start` is 1-based, like DMX addresses.
struct AddressBlock {
    StableId id = 0;
    int universe = 0;           // 0-based row.
    int start = 1;
    int count = 1;
    const char* name = "";
    ImVec4 color{};             // Alpha <= 0 picks a chart colour from the block index.
};
struct AddressOverlap {
    int a = -1, b = -1;         // Indices into the block list.
    int universe = 0;
    int first = 0, last = 0;    // The shared slots, 1-based and inclusive.
};
// Writes the overlaps between blocks of the same universe into `out`; returns the number found (counting past the
// capacity of `out`, so a caller can tell the list was cut).
std::size_t FindOverlaps(std::span<const AddressBlock> blocks, std::span<AddressOverlap> out);
// First 1-based slot of a free run of `count` slots in `universe`, or 0 when there is none. Blocks outside the
// universe are ignored.
int FirstFreeRange(std::span<const AddressBlock> blocks, int universe, int slots, int count);
// Whole pixels that fit in one universe: floor(slots / channelsPerPixel). RGB gives 170 of 512, RGBW 128.
int PixelsPerUniverse(int channelsPerPixel, int slots = 512);

struct AddressMapView {
    std::span<const AddressBlock> blocks{};
    int universes = 1;          // Rows shown.
    int slots = 512;            // Slots per universe.
};
struct AddressMapState {
    StableId selected = 0;
    StableId dragging = 0;      // Managed by AddressMap.
    int grabOffset = 0;
    bool announced = false;
};
struct AddressMapOptions {
    float rowHeight = 0;        // Zero uses about 2 frame heights.
    float labelWidth = 0;       // Zero uses about 3.2 frame heights.
    bool editable = true;       // Blocks can be dragged to a new address or universe.
};
struct AddressMapRequest {
    StableId select = 0;        // A block was clicked.
    StableId moved = 0;         // Block being dragged: its new place is `universe` and `start`.
    int universe = 0, start = 1;
    bool started = false, changed = false, finished = false, cancelled = false;
};
// Overlaps are drawn with hatching, a warning mark and text, never colour alone. Drag a block sideways to change its
// start address and up or down to change its universe; Escape cancels.
AddressMapRequest AddressMap(const char* id, const AddressMapView& view, AddressMapState& state, AddressMapOptions layout = {},
                             ComponentOptions options = {});

struct ChannelMonitorView {
    std::span<const unsigned char> values{}; // One value (0..255) per slot, first slot first.
    std::span<const AddressBlock> blocks{};  // Optional colour bands for the slots of one universe; `universe` is ignored.
    int firstSlot = 1;          // Address of values[0].
};
struct ChannelMonitorState {
    int selected = -1;          // Index into `values`.
    int selectedLast = -1;      // End of a shift-click range, or -1.
    bool announced = false;
};
struct ChannelMonitorOptions {
    int columns = 32;
    float cellHeight = 0;       // Zero uses about 2.4 frame heights.
    bool hex = false;           // Values in hexadecimal.
    bool editable = true;       // Drag in a cell to set the value; All 0 and Full keys.
};
struct ChannelMonitorRequest {
    int index = -1;             // Cell being edited by drag or key; -1 for none.
    int value = 0;              // Its new value, 0..255.
    bool started = false, changed = false, finished = false;
    bool allZero = false, allFull = false;
    int hovered = -1;
};
// Shows every slot as a bar with its value. Drag in a cell to set the value (top = 255), Up/Down nudge the selected
// cell, shift-click selects a range.
ChannelMonitorRequest ChannelMonitor(const char* id, const ChannelMonitorView& view, ChannelMonitorState& state,
                                     ChannelMonitorOptions layout = {}, ComponentOptions options = {});
} // namespace imkit::show
