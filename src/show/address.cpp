#include <imkit/show/address.h>
#include "internal.h"
#include <imkit/charts.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace imkit::show {
using namespace impl;

// ---- Pure helpers ----------------------------------------------------------------------------------------------
std::size_t FindOverlaps(std::span<const AddressBlock> blocks, std::span<AddressOverlap> out) {
    std::size_t found = 0;
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        const AddressBlock& a = blocks[i];
        if (a.count <= 0)
            continue;
        for (std::size_t j = i + 1; j < blocks.size(); ++j) {
            const AddressBlock& b = blocks[j];
            if (b.count <= 0 || a.universe != b.universe)
                continue;
            const int first = std::max(a.start, b.start), last = std::min(a.start + a.count - 1, b.start + b.count - 1);
            if (first > last)
                continue;
            if (found < out.size())
                out[found] = {static_cast<int>(i), static_cast<int>(j), a.universe, first, last};
            ++found;
        }
    }
    return found;
}

int FirstFreeRange(std::span<const AddressBlock> blocks, int universe, int slots, int count) {
    if (count <= 0 || slots <= 0 || count > slots)
        return 0;
    static thread_local std::vector<std::pair<int, int>> runs;
    runs.clear();
    for (const AddressBlock& b : blocks)
        if (b.universe == universe && b.count > 0)
            runs.emplace_back(b.start, b.start + b.count - 1);
    std::sort(runs.begin(), runs.end());
    int cursor = 1;
    for (const auto& [first, last] : runs) {
        if (first - cursor >= count)
            return cursor;
        cursor = std::max(cursor, last + 1);
    }
    return slots - cursor + 1 >= count ? cursor : 0;
}

int PixelsPerUniverse(int channelsPerPixel, int slots) {
    if (channelsPerPixel <= 0 || slots <= 0)
        return 0;
    return slots / channelsPerPixel;
}

namespace {
ImVec4 BlockColor(const AddressBlock& b, std::size_t index, const ComponentOptions& options) {
    return b.color.w > 0 ? b.color : ChartColor(options.theme, index);
}
} // namespace

// ---- Address map -----------------------------------------------------------------------------------------------
AddressMapRequest AddressMap(const char* id, const AddressMapView& view, AddressMapState& state, AddressMapOptions o, ComponentOptions options) {
    AddressMapRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const int universes = std::max(1, view.universes), slots = std::max(1, view.slots);
    const float rowH = o.rowHeight > 0 ? o.rowHeight : u * 2.f, gap = u * .35f;
    const float labelW = o.labelWidth > 0 ? o.labelWidth : u * 3.4f, usageW = u * 4.6f;
    const float w = std::max(u * 12.f, ImGui::GetContentRegionAvail().x);
    const float barW = std::max(u * 4.f, w - labelW - usageW);
    const float round = ControlRound(options) * .7f;
    ImGui::PushID(id);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float barX = origin.x + labelW;
    auto rowTop = [&](int row) { return origin.y + static_cast<float>(row) * (rowH + gap); };
    auto slotX = [&](float slot) { return barX + barW * slot / static_cast<float>(slots); };

    // Rows: label, track, grid ticks and usage.
    static thread_local std::vector<std::uint8_t> used;
    for (int r = 0; r < universes; ++r) {
        const float y = rowTop(r);
        draw->AddText({origin.x, y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), FrameText("U%d", r + 1));
        draw->AddRectFilled({barX, y}, {barX + barW, y + rowH}, U32(Recessed(c, .4f)), round);
        draw->AddRect({barX, y}, {barX + barW, y + rowH}, U32(c.border, detail::FrameLineAlpha(.7f)), round, 0, detail::FrameLineWidth(1.f));
        for (int t = slots >= 128 ? 64 : slots / 4; t > 0 && t < slots; t += (slots >= 128 ? 64 : slots / 4)) {
            const float x = slotX(static_cast<float>(t));
            draw->AddLine({x, y + rowH - u * .3f}, {x, y + rowH - 1.f}, U32(c.muted, .5f), 1.f);
        }
        used.assign(static_cast<std::size_t>(slots), 0);
        for (const AddressBlock& b : view.blocks)
            if (b.universe == r)
                for (int s = std::max(1, b.start); s < b.start + b.count && s <= slots; ++s)
                    used[static_cast<std::size_t>(s - 1)] = 1;
        int count = 0;
        for (std::uint8_t v : used)
            count += v;
        const char* text = FrameText("%d/%d", count, slots);
        draw->AddText({barX + barW + u * .6f, y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(count == slots ? c.warning : c.muted), text);
    }

    // Blocks.
    StableId hovered = 0;
    for (std::size_t i = 0; i < view.blocks.size(); ++i) {
        const AddressBlock& b = view.blocks[i];
        if (b.id == 0 || b.count <= 0 || b.universe < 0 || b.universe >= universes)
            continue;
        const float x0 = slotX(static_cast<float>(b.start - 1)), x1 = std::max(x0 + 6.f, slotX(static_cast<float>(b.start - 1 + b.count)));
        const float y = rowTop(b.universe) + 3.f;
        const ImVec2 a{x0, y}, e{std::min(x1, barX + barW), y + rowH - 6.f};
        ImGui::PushID(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(b.id)));
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##block", {std::max(4.f, e.x - a.x), std::max(4.f, e.y - a.y)});
        const bool isHover = ImGui::IsItemHovered();
        if (isHover)
            hovered = b.id;
        if (ImGui::IsItemActivated()) {
            request.select = b.id;
            state.selected = b.id;
            if (o.editable) {
                state.dragging = b.id;
                state.announced = false;
                const float mx = io.MousePos.x;
                state.grabOffset = static_cast<int>(std::floor((mx - barX) / barW * static_cast<float>(slots))) + 1 - b.start;
            }
        }
        const ImVec4 col = BlockColor(b, i, options);
        const bool sel = state.selected == b.id;
        draw->AddRectFilled(a, e, U32(Mix(c.input, col, sel ? .85f : .6f)), round);
        draw->AddRect(a, e, U32(sel ? c.text : col, sel ? .95f : .9f), round, 0, sel ? 2.f : 1.f);
        draw->PushClipRect(a, e, true);
        DrawTextFit(draw, {a.x + u * .3f, a.y + (e.y - a.y - ImGui::GetTextLineHeight()) * .5f}, e.x - a.x - u * .5f, U32(InkFor(c, Mix(c.input, col, .7f))), Safe(b.name));
        draw->PopClipRect();
        SemanticItem(options, FrameText("%s, universe %d, slots %d to %d", Safe(b.name), b.universe + 1, b.start, b.start + b.count - 1),
                     accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, sel);
        ImGui::PopID();
    }

    // Overlaps: hatching, a warning mark and a count, never colour alone.
    static thread_local std::vector<AddressOverlap> overlaps;
    overlaps.assign(std::min<std::size_t>(view.blocks.size() * 2 + 8, 512), AddressOverlap{});
    std::size_t total = 0;
    if (view.blocks.size() <= 512) // FindOverlaps is quadratic; very large maps skip the per-frame check.
        total = FindOverlaps(view.blocks, overlaps);
    const std::size_t shown = std::min(total, overlaps.size());
    for (std::size_t k = 0; k < shown; ++k) {
        const AddressOverlap& ov = overlaps[k];
        if (ov.universe < 0 || ov.universe >= universes)
            continue;
        const float x0 = slotX(static_cast<float>(ov.first - 1)), x1 = std::max(x0 + 4.f, slotX(static_cast<float>(ov.last)));
        const float y0 = rowTop(ov.universe) + 1.f, y1 = rowTop(ov.universe) + rowH - 1.f;
        draw->PushClipRect({x0, y0}, {x1, y1}, true);
        draw->AddRectFilled({x0, y0}, {x1, y1}, U32(c.error, .28f));
        for (float hx = x0 - (y1 - y0); hx < x1; hx += u * .4f)
            draw->AddLine({hx, y1}, {hx + (y1 - y0), y0}, U32(c.error, .9f), 1.5f);
        draw->PopClipRect();
        draw->AddRect({x0, y0}, {x1, y1}, U32(c.error), 2.f, 0, 1.5f);
        if (x1 - x0 >= u * 1.2f)
            GlyphWarning(draw, {(x0 + x1) * .5f, (y0 + y1) * .5f}, u * .75f, U32(c.error));
    }
    const float bottom = rowTop(universes) - gap;
    ImGui::SetCursorScreenPos({origin.x, bottom + gap});
    if (total > 0) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        GlyphWarning(draw, {p.x + u * .4f, p.y + u * .5f}, u * .75f, U32(c.error));
        draw->AddText({p.x + u * 1.0f, p.y + u * .1f}, U32(c.error),
                      FrameText(total == 1 ? "%d overlap" : "%d overlaps", static_cast<int>(total)));
        ImGui::Dummy({w, u * 1.1f});
    } else {
        ImGui::Dummy({w, 1.f});
    }

    // Drag: slot under the pointer picks the new start, the row under it the universe.
    if (state.dragging != 0) {
        const AddressBlock* block = nullptr;
        for (const AddressBlock& b : view.blocks)
            if (b.id == state.dragging)
                block = &b;
        if (!block) {
            state.dragging = 0;
        } else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            request.moved = block->id;
            request.universe = block->universe;
            request.start = block->start;
            request.cancelled = true;
            state.dragging = 0;
            state.announced = false;
        } else {
            const int slotUnder = static_cast<int>(std::floor((io.MousePos.x - barX) / barW * static_cast<float>(slots))) + 1;
            const int row = std::clamp(static_cast<int>(std::floor((io.MousePos.y - origin.y) / (rowH + gap))), 0, universes - 1);
            const int start = std::clamp(slotUnder - state.grabOffset, 1, std::max(1, slots - block->count + 1));
            const bool moved = start != block->start || row != block->universe;
            if ((moved && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.f)) || state.announced) { // A click must not nudge the block.
                request.moved = block->id;
                request.universe = row;
                request.start = start;
                request.started = !state.announced;
                request.changed = true;
                state.announced = true;
                ImGui::SetTooltip("U%d   %d - %d", row + 1, start, start + block->count - 1);
            }
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                request.finished = state.announced;
                state.dragging = 0;
                state.announced = false;
            }
        }
    } else if (hovered != 0) {
        for (const AddressBlock& b : view.blocks)
            if (b.id == hovered) {
                ImGui::SetTooltip("%s\nU%d   %d - %d  (%d)", Safe(b.name), b.universe + 1, b.start, b.start + b.count - 1, b.count);
                break;
            }
    }
    ImGui::PopID();
    return request;
}

// ---- Channel monitor -------------------------------------------------------------------------------------------
ChannelMonitorRequest ChannelMonitor(const char* id, const ChannelMonitorView& view, ChannelMonitorState& state, ChannelMonitorOptions o,
                                     ComponentOptions options) {
    ChannelMonitorRequest request;
    const int n = static_cast<int>(view.values.size());
    if (n == 0)
        return request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const int cols = std::max(1, o.columns);
    const int rows = (n + cols - 1) / cols;
    const float gap = 2.f;
    const float cellH = o.cellHeight > 0 ? o.cellHeight : u * 2.4f;
    ImGui::PushID(id);
    if (o.editable) {
        if (PillButton("zero", Str(options, "show.allZero", "All 0"), c, options, c.text))
            request.allZero = true;
        ImGui::SameLine();
        if (PillButton("full", Str(options, "show.allFull", "Full"), c, options, c.text))
            request.allFull = true;
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%s", Str(options, "show.channelHint", "Drag in a cell to set a value (top = 255)"));
    }
    const float avail = std::max(u * 8.f, ImGui::GetContentRegionAvail().x);
    const float cellW = std::max(2.f, (avail - gap * static_cast<float>(cols - 1)) / static_cast<float>(cols));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const int lo = std::min(state.selected, state.selectedLast < 0 ? state.selected : state.selectedLast);
    const int hi = std::max(state.selected, state.selectedLast < 0 ? state.selected : state.selectedLast);
    for (int i = 0; i < n; ++i) {
        const ImVec2 a{origin.x + static_cast<float>(i % cols) * (cellW + gap), origin.y + static_cast<float>(i / cols) * (cellH + gap)}, b{a.x + cellW, a.y + cellH};
        if (!ImGui::IsRectVisible(a, b))
            continue;
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##channel", {cellW, cellH});
        const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
        const int value = view.values[static_cast<std::size_t>(i)];
        if (hovered)
            request.hovered = i;
        if (ImGui::IsItemActivated()) {
            if (io.KeyShift && state.selected >= 0)
                state.selectedLast = i;
            else {
                state.selected = i;
                state.selectedLast = -1;
            }
            state.announced = false;
        }
        if (o.editable && active) {
            const int target = std::clamp(static_cast<int>(std::lround((1.f - (io.MousePos.y - a.y) / cellH) * 255.f)), 0, 255);
            if (target != value || state.announced) {
                request.index = i;
                request.value = target;
                request.started = !state.announced;
                request.changed = true;
                state.announced = true;
            }
        }
        if (o.editable && ImGui::IsItemDeactivated() && state.announced) {
            request.index = i;
            request.value = value;
            request.finished = true;
            state.announced = false;
        }
        // Colour band of the block that owns the slot.
        ImVec4 band{};
        const int address = view.firstSlot + i;
        for (std::size_t k = 0; k < view.blocks.size(); ++k) {
            const AddressBlock& blk = view.blocks[k];
            if (address >= blk.start && address < blk.start + blk.count) {
                band = BlockColor(blk, k, options);
                break;
            }
        }
        const float level = static_cast<float>(value) / 255.f;
        draw->AddRectFilled(a, b, U32(Recessed(c, .3f)), 3.f);
        if (level > 0) {
            draw->PushClipRect({a.x, b.y - cellH * level}, b, true);
            draw->AddRectFilled(a, b, U32(band.w > 0 ? Mix(c.input, band, .75f) : Mix(c.input, c.accent, .65f)), 3.f);
            draw->PopClipRect();
        }
        if (band.w > 0)
            draw->AddRectFilled({a.x, a.y}, {b.x, a.y + 3.f}, U32(band), 2.f);
        const bool selected = i >= lo && i <= hi && state.selected >= 0;
        draw->AddRect(a, b, U32(selected ? c.accent : c.border, selected ? 1.f : .6f), 3.f, 0, selected ? 2.f : detail::FrameLineWidth(1.f));
        if (cellW >= u * 1.3f) {
            const char* text = o.hex ? FrameText("%02X", value) : FrameText("%d", value);
            const ImVec2 ts = ImGui::CalcTextSize(text);
            draw->AddText({a.x + (cellW - ts.x) * .5f, b.y - ts.y - 2.f}, U32(c.text), text);
        }
        if (cellW >= u * 2.3f)
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .7f, {a.x + 3.f, a.y + 4.f}, U32(c.muted), FrameText("%d", address));
        if (SemanticItem(options, FrameText("Channel %d", address), accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, selected)) {
            state.selected = i;
            state.selectedLast = -1;
        }
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy({avail, static_cast<float>(rows) * (cellH + gap) - gap});
    // Nudge the selected channel with Up and Down.
    if (o.editable && state.selected >= 0 && state.selected < n && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !io.WantTextInput) {
        const int step = io.KeyShift ? 16 : 1;
        int delta = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            delta = step;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            delta = -step;
        if (delta != 0) {
            request.index = state.selected;
            request.value = std::clamp(static_cast<int>(view.values[static_cast<std::size_t>(state.selected)]) + delta, 0, 255);
            request.started = request.changed = request.finished = true;
        }
    }
    if (request.hovered >= 0) {
        const int v = view.values[static_cast<std::size_t>(request.hovered)];
        ImGui::SetTooltip("Ch %d = %d  (%d%%)", view.firstSlot + request.hovered, v, static_cast<int>(std::lround(static_cast<float>(v) / 2.55f)));
    }
    ImGui::PopID();
    return request;
}
} // namespace imkit::show
