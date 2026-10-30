#include <imkit/show/live.h>
#include "internal.h"
#include <imkit/workflow.h>
#include <cfloat>
#include <vector>

namespace imkit::show {
using namespace impl;

namespace {
constexpr float Pi = 3.14159265358979323846f;

// Small square toggle with a one-letter label (M, S, A, B ...). Returns true on change.
bool MiniToggle(const char* id, const char* letter, bool* value, ImVec4 tone, const ShowColors& c, const ComponentOptions& o,
                const char* accessibleName, float size = 0) {
    auto* draw = ImGui::GetWindowDrawList();
    const float s = size > 0 ? size : Unit() * .78f;
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + s, a.y + s};
    ImGui::InvisibleButton("##t", {s, s});
    bool changed = ImGui::IsItemClicked();
    if (SemanticItem(o, accessibleName, accessibility::SemanticRole::Toggle, accessibility::SemanticAction::Toggle, *value))
        changed = true;
    if (changed)
        *value = !*value;
    const bool hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused();
    const ImVec4 fill = *value ? tone : Mix(c.input, c.text, hovered ? .1f : .04f);
    draw->AddRectFilled(a, b, U32(fill), s * .28f);
    if (!*value)
        draw->AddRect(a, b, U32(c.border), s * .28f, 0, detail::FrameLineWidth(1.f));
    DrawTextCentered(draw, a, b, U32(*value ? InkFor(c, tone) : c.muted), letter);
    if (focused && ImGui::GetIO().NavVisible)
        draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, U32(c.focus), s * .28f + 2.f, 0, 2.f);
    ImGui::PopID();
    return changed;
}

const char* BusName(int bus) { return bus == 1 ? "A" : bus == 2 ? "B" : "-"; }

ImVec4 StateTone(const ShowColors& c, CueState state) {
    switch (state) {
    case CueState::Running: return c.running;
    case CueState::Standby: return c.armed;
    case CueState::Paused: return c.queued;
    case CueState::Done: return c.done;
    case CueState::Broken: return c.broken;
    default: return c.border;
    }
}
} // namespace

// ---- Cue list -------------------------------------------------------------------------------------------------
CueRequest CueList(const char* id, std::span<const CueView> cues, CueListState& state, CueListOptions o, ComponentOptions options) {
    CueRequest request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    const float rowH = o.rowHeight > 0 ? o.rowHeight : u * 1.4f;
    ImVec2 size = o.size;
    if (size.y <= 0)
        size.y = rowH * 8.f + 4.f;
    const float round = options.theme ? options.theme->radius.overlay : u * .4f;

    // Rows hidden by a collapsed group are skipped. The index list is reused between frames.
    static thread_local std::vector<int> visible;
    visible.clear();
    int hideBelow = -1;
    for (int i = 0; i < static_cast<int>(cues.size()); ++i) {
        const CueView& cue = cues[static_cast<std::size_t>(i)];
        if (hideBelow >= 0 && cue.depth > hideBelow)
            continue;
        hideBelow = -1;
        visible.push_back(i);
        if (cue.group && !cue.expanded)
            hideBelow = cue.depth;
    }

    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.surface);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, round);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {2.f, 2.f});
    ImGui::PushStyleColor(ImGuiCol_Header, {0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0, 0, 0, 0});
    const bool open = ImGui::BeginChild("##cues", size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    if (open) {
        auto* draw = ImGui::GetWindowDrawList();
        const float width = ImGui::GetContentRegionAvail().x;
        if (o.showHeader) {
            // Column titles: number and name on the left, time on the right.
            const float hh = u * 1.1f;
            const ImVec2 a = ImGui::GetCursorScreenPos();
            const float fs = ImGui::GetFontSize() * .78f;
            const float ty = a.y + (hh - fs) * .5f;
            draw->AddText(ImGui::GetFont(), fs, {a.x + u * .75f, ty}, U32(c.muted), Str(options, "show.colNumber", "#"));
            draw->AddText(ImGui::GetFont(), fs, {a.x + u * 3.15f, ty}, U32(c.muted), Str(options, "show.colName", "Name"));
            if (o.showTimes) {
                const char* t = Str(options, "show.colTime", "Time");
                draw->AddText(ImGui::GetFont(), fs, {a.x + width - u * .6f - ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0.f, t).x, ty}, U32(c.muted), t);
            }
            draw->AddLine({a.x, a.y + hh}, {a.x + width, a.y + hh}, U32(c.border, .6f), 1.f);
            ImGui::Dummy({width, hh + 2.f});
        }
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(visible.size()), rowH);
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const int index = visible[static_cast<std::size_t>(row)];
                const CueView& cue = cues[static_cast<std::size_t>(index)];
                const bool selected = cue.id != 0 && cue.id == state.selected;
                const bool standby = cue.id != 0 && cue.id == state.standby;
                ImGui::PushID(index);
                const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + rowH};
                ImGui::Selectable("##row", selected, ImGuiSelectableFlags_AllowDoubleClick, {width, rowH});
                const bool hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused();
                const ImVec2 mouse = ImGui::GetIO().MousePos;
                const float indent = u * .75f + static_cast<float>(cue.depth) * u * .8f;
                const ImVec2 chevronMin{a.x + indent - u * .1f, a.y}, chevronMax{a.x + indent + u * .7f, b.y};
                const bool onChevron = cue.group && mouse.x >= chevronMin.x && mouse.x <= chevronMax.x;
                if (ImGui::IsItemClicked() && cue.id != 0) {
                    if (onChevron)
                        request.toggleGroup = cue.id;
                    else
                        request.select = cue.id;
                }
                if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !onChevron)
                    request.activate = cue.id;
                if (focused && !ImGui::IsAnyMouseDown() && !selected && cue.id != 0)
                    request.select = cue.id;
                if (focused && ImGui::IsKeyPressed(ImGuiKey_Enter, false))
                    request.activate = cue.id;
                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    request.context = cue.id;
                if (cue.id != 0 && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
                    ImGui::SetDragDropPayload("IMKIT_CUE", &cue.id, sizeof(StableId));
                    ImGui::TextUnformatted(Safe(cue.name));
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(
                            "IMKIT_CUE", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                        draw->AddLine({a.x, a.y}, {b.x, a.y}, U32(c.accent), 2.f);
                        if (p->IsDelivery() && p->DataSize == sizeof(StableId)) {
                            request.move = *static_cast<const StableId*>(p->Data);
                            request.moveBefore = cue.id;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Row surface.
                const ImVec4 tone = StateTone(c, cue.state);
                ImVec4 fill = c.surface;
                if (selected)
                    fill = Mix(c.surface, c.accent, .16f);
                else if (cue.state == CueState::Running)
                    fill = Mix(c.surface, c.running, .16f);
                else if (standby)
                    fill = Mix(c.surface, c.armed, .1f);
                else if (hovered)
                    fill = Mix(c.surface, c.text, .05f);
                draw->AddRectFilled(a, b, U32(fill), 4.f);
                if (!selected && !standby && !hovered && cue.state != CueState::Running)
                    draw->AddLine({a.x + u * .5f, b.y - .5f}, {b.x - u * .5f, b.y - .5f}, U32(c.border, .3f), 1.f);
                if (cue.state != CueState::Idle)
                    draw->AddRectFilled(a, {a.x + 4.f, b.y}, U32(tone), 2.f);
                if (standby) {
                    // Next to fire: bracketed row and a playhead marker.
                    draw->AddRect({a.x + 1.f, a.y + 1.f}, {b.x - 1.f, b.y - 1.f}, U32(c.armed, .85f), 4.f, 0, 1.5f);
                    const ImVec2 m{a.x + 9.f, a.y + rowH * .5f};
                    draw->AddTriangleFilled({m.x - 3, m.y - 5}, {m.x - 3, m.y + 5}, {m.x + 5, m.y}, U32(c.armed));
                }
                for (int d = 1; d <= cue.depth; ++d) {
                    const float gx = a.x + u * .75f + static_cast<float>(d - 1) * u * .8f + u * .25f;
                    draw->AddLine({gx, a.y}, {gx, b.y}, U32(c.border, .5f), 1.f);
                }
                if (focused && ImGui::GetIO().NavVisible)
                    draw->AddRect(a, b, U32(c.focus), 4.f, 0, 2.f);

                float x = a.x + indent;
                if (cue.group) {
                    detail::DrawChevron(draw, {x + u * .25f, a.y + rowH * .5f}, u * .4f, U32(c.muted),
                                        cue.expanded ? ImGuiDir_Down : ImGuiDir_Right, 1.5f);
                    x += u * .8f;
                } else if (cue.depth > 0) {
                    x += u * .1f;
                }
                const float textY = a.y + (rowH - ImGui::GetTextLineHeight()) * .5f;
                const bool disabled = cue.state == CueState::Disabled;
                const ImU32 ink = U32(disabled ? c.muted : c.text, disabled ? .6f : 1.f);
                draw->AddText({x, textY}, U32(c.muted), Safe(cue.number));
                x += u * 2.4f;
                // Type tag.
                if (cue.kind && cue.kind[0]) {
                    const ImVec2 ts = ImGui::CalcTextSize(cue.kind);
                    const float tagW = ts.x * .82f + u * .5f, tagH = ImGui::GetTextLineHeight() * .9f;
                    const ImVec2 ta{x, a.y + (rowH - tagH) * .5f};
                    draw->AddRectFilled(ta, {ta.x + tagW, ta.y + tagH}, U32(c.input), tagH * .3f);
                    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .82f, {ta.x + u * .25f, ta.y + (tagH - ts.y * .82f) * .5f}, U32(c.muted), cue.kind);
                    x += tagW + u * .4f;
                }
                // Right side, laid out from the edge: time, continue mark, state glyph.
                float right = b.x - u * .6f;
                if (o.showTimes && (cue.duration > 0 || cue.elapsed >= 0 || cue.preWait > 0)) {
                    char text[48];
                    auto clock = [](char* out, std::size_t n, float seconds) {
                        const int total = static_cast<int>(std::max(0.f, seconds));
                        std::snprintf(out, n, "%d:%02d", total / 60, total % 60);
                    };
                    char a1[16], a2[16];
                    if (cue.elapsed >= 0 && cue.duration > 0) {
                        clock(a1, sizeof a1, cue.elapsed);
                        clock(a2, sizeof a2, cue.duration);
                        std::snprintf(text, sizeof text, "%s / %s", a1, a2);
                    } else if (cue.duration > 0) {
                        clock(a1, sizeof a1, cue.duration);
                        if (cue.preWait > 0)
                            std::snprintf(text, sizeof text, "+%.0fs  %s", cue.preWait, a1);
                        else
                            std::snprintf(text, sizeof text, "%s", a1);
                    } else {
                        std::snprintf(text, sizeof text, "+%.0fs", cue.preWait);
                    }
                    const ImVec2 ts = ImGui::CalcTextSize(text);
                    right -= ts.x;
                    draw->AddText({right, textY}, U32(cue.state == CueState::Running ? c.running : c.muted), text);
                    right -= u * .5f;
                }
                if (cue.next != CueContinue::None) {
                    const ImVec2 m{right - u * .25f, a.y + rowH * .5f};
                    draw->AddLine({m.x, m.y - u * .28f}, {m.x, m.y + u * .22f}, U32(c.muted), 1.6f);
                    detail::DrawChevron(draw, {m.x, m.y + u * .1f}, u * .34f, U32(c.muted), ImGuiDir_Down, 1.6f);
                    if (cue.next == CueContinue::AutoFollow)
                        draw->AddLine({m.x - u * .22f, m.y + u * .32f}, {m.x + u * .22f, m.y + u * .32f}, U32(c.muted), 1.6f);
                    right -= u * .8f;
                }
                {
                    const ImVec2 g{right - u * .25f, a.y + rowH * .5f};
                    switch (cue.state) {
                    case CueState::Running: GlyphPlay(draw, g, u * .6f, U32(c.running)); break;
                    case CueState::Paused: GlyphPause(draw, g, u * .6f, U32(c.queued)); break;
                    case CueState::Done: detail::DrawCheck(draw, g, u * .4f, U32(c.done), 1.8f); break;
                    case CueState::Broken: GlyphWarning(draw, g, u * .6f, U32(c.broken)); break;
                    case CueState::Standby: draw->AddCircle(g, u * .15f, U32(c.armed), 12, 1.6f); break;
                    default: break;
                    }
                    if (cue.state != CueState::Idle)
                        right -= u * .7f;
                }
                // Name (and the problem text for broken cues), clipped to what is left.
                const float nameW = std::max(0.f, right - x - u * .3f);
                draw->PushClipRect({x, a.y}, {x + nameW, b.y}, true);
                draw->AddText({x, textY}, ink, Safe(cue.name));
                if (cue.state == CueState::Broken && cue.problem && cue.problem[0]) {
                    const float nw = ImGui::CalcTextSize(Safe(cue.name)).x;
                    draw->AddText({x + nw + u * .6f, textY}, U32(c.broken), cue.problem);
                }
                draw->PopClipRect();
                if (cue.elapsed >= 0 && cue.duration > 0)
                    draw->AddRectFilled({a.x, b.y - 3.f}, {a.x + width * Clamp01(cue.elapsed / cue.duration), b.y}, U32(c.running));

                SemanticItem(options, FrameText("%s %s", Safe(cue.number), Safe(cue.name)), accessibility::SemanticRole::Row,
                             accessibility::SemanticAction::Select, selected);
                ImGui::PopID();
            }
        }
        // Dropping below the last row moves the cue to the end.
        const ImVec2 tail = ImGui::GetCursorScreenPos();
        ImGui::Dummy({width, u * .6f});
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("IMKIT_CUE"))
                if (p->DataSize == sizeof(StableId))
                    request.move = *static_cast<const StableId*>(p->Data);
            ImGui::EndDragDropTarget();
        }
        (void)tail;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    return request;
}

// ---- GO bar ---------------------------------------------------------------------------------------------------
GoBarRequest GoBar(const char* id, const GoBarView& view, bool* armed, ComponentOptions options) {
    GoBarRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float h = u * 2.1f, gap = u * .45f;
    const float round = ControlRound(options);
    const float rightEdge = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    ImGui::PushID(id);
    bool localArmed = true;
    bool* arm = armed ? armed : &localArmed;
    if (view.requireArm) {
        ConsoleButtonOptions ao;
        ao.mode = ConsoleMode::Toggle;
        ao.glyph = ConsoleGlyph::Lock;
        ao.tone = c.armed;
        ao.size = {u * 2.6f, h};
        ConsoleButton(Str(options, "show.arm", "ARM"), arm, ao, options);
        ImGui::SameLine(0, gap);
    } else
        *arm = true;

    // GO: the hero key. Dim and inert until armed.
    const bool canGo = *arm;
    {
        const ImVec2 size{u * 5.4f, h};
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
        ImGui::InvisibleButton("##go", size);
        const bool hovered = ImGui::IsItemHovered() && canGo, held = ImGui::IsItemActive() && canGo, focused = ImGui::IsItemFocused();
        bool go = ImGui::IsItemClicked() && canGo;
        if (SemanticItem(options, Str(options, "show.go", "GO"), accessibility::SemanticRole::Button, accessibility::SemanticAction::Press) && canGo)
            go = true;
        if (canGo && focused && ImGui::IsKeyPressed(ImGuiKey_Space, false))
            go = true;
        request.go = go;
        KeyLook look;
        look.tone = c.accent;
        look.lit = canGo;
        look.solid = true;
        look.enabled = canGo;
        look.labelScale = 1.9f;
        look.hint = canGo ? Str(options, "show.goHint", "Space") : Str(options, "show.armFirst", "Arm first");
        DrawConsoleKey(draw, c, a, b, Str(options, "show.go", "GO"), look, hovered, held, focused, round);
        if (canGo)
            draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, U32(c.accent, .28f), round + 2.f, 0, 2.f);
    }
    ImGui::SameLine(0, gap);

    // Pause / resume: lit while paused.
    {
        bool paused = view.paused;
        ConsoleButtonOptions po;
        po.mode = ConsoleMode::Toggle;
        po.glyph = view.paused ? ConsoleGlyph::Play : ConsoleGlyph::Pause;
        po.tone = c.queued;
        po.size = {u * 2.6f, h};
        if (ConsoleButton(view.paused ? Str(options, "show.resume", "Resume") : Str(options, "show.pause", "Pause"), &paused, po, options).changed)
            request.pause = true;
    }
    ImGui::SameLine(0, gap);
    HoldButtonOptions stop;
    stop.destructive = false;
    stop.key = true;
    stop.hint = Str(options, "show.hold", "hold");
    stop.size = {u * 3.6f, h};
    if (HoldButton(Str(options, "show.stopAll", "Stop all"), stop, options))
        request.stopAll = true;
    ImGui::SameLine(0, gap * .5f);
    HoldButtonOptions panic;
    panic.key = true;
    panic.hint = Str(options, "show.hold", "hold");
    panic.size = {u * 3.6f, h};
    panic.seconds = 1.0f;
    if (HoldButton(Str(options, "show.panic", "Panic"), panic, options))
        request.panic = true;

    // Next cue card: number and name, time left and a progress line. It moves to its own line when the keys leave no room.
    {
        const bool sameLine = FitsOnLine(rightEdge, u * 7.f, gap);
        if (sameLine)
            ImGui::SameLine(0, gap);
        else
            ImGui::Dummy({0, u * .2f});
        const float avail = rightEdge - ImGui::GetCursorScreenPos().x;
        const float w = std::max(u * 7.f, std::min(sameLine ? u * 14.f : u * 24.f, avail));
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
        ReadoutItem({w, h});
        draw->AddRectFilled(a, b, U32(Mix(c.input, c.text, .02f)), round);
        draw->AddRect(a, b, U32(c.border, detail::FrameLineAlpha(.7f)), round, 0, detail::FrameLineWidth(1.f));
        draw->AddRectFilled({a.x + 1.f, a.y + u * .3f}, {a.x + 4.f, b.y - u * .3f}, U32(c.armed), 2.f);
        const float small = ImGui::GetFontSize() * .78f;
        ImFont* font = ImGui::GetFont();
        draw->AddText(font, small, {a.x + u * .6f, a.y + u * .2f}, U32(c.muted), Str(options, "show.next", "Next"));
        const char* number = Safe(view.nextNumber);
        const char* name = view.next && view.next[0] ? view.next : "-";
        float x = a.x + u * .6f;
        const float y2 = a.y + h - u * .25f - ImGui::GetTextLineHeight() * 1.15f;
        if (number[0]) {
            const ImVec2 ns = font->CalcTextSizeA(ImGui::GetFontSize() * 1.15f, FLT_MAX, 0.f, number);
            draw->AddText(font, ImGui::GetFontSize() * 1.15f, {x, y2}, U32(c.muted), number);
            x += ns.x + u * .4f;
        }
        const bool timed = view.remainingSeconds >= 0;
        char left[24] = "";
        float timeW = 0.f;
        if (timed) {
            const int t = static_cast<int>(view.remainingSeconds);
            std::snprintf(left, sizeof left, "%d:%02d", t / 60, t % 60);
            timeW = font->CalcTextSizeA(ImGui::GetFontSize() * 1.3f, FLT_MAX, 0.f, left).x + u * .5f;
            draw->AddText(font, ImGui::GetFontSize() * 1.3f, {b.x - timeW, a.y + h * .5f - ImGui::GetTextLineHeight() * .65f},
                          U32(view.paused ? c.queued : c.running), left);
            draw->AddText(font, small, {b.x - timeW, a.y + u * .2f}, U32(c.muted), Str(options, "show.remaining", "Remaining"));
        }
        draw->PushClipRect(a, {b.x - timeW - u * .3f, b.y}, true);
        draw->AddText(font, ImGui::GetFontSize() * 1.15f, {x, y2}, U32(c.text), name);
        draw->PopClipRect();
        if (timed && view.totalSeconds > 0) {
            const float t = Clamp01(static_cast<float>(1.0 - view.remainingSeconds / view.totalSeconds));
            draw->PushClipRect({a.x, b.y - 3.f}, b, true);
            draw->AddRectFilled(a, {a.x + w * t, b.y}, U32(view.paused ? c.queued : c.running), round);
            draw->PopClipRect();
        }
        AnnotateText(options, Str(options, "show.next", "Next"), FrameText("%s %s", number, name), accessibility::SemanticRole::Status);
    }
    ImGui::PopID();
    return request;
}

// ---- Clip launcher --------------------------------------------------------------------------------------------
ClipLauncherRequest ClipLauncher(const char* id, const ClipLauncherView& view, ClipLauncherOptions o, ComponentOptions options) {
    ClipLauncherRequest request;
    const int layers = static_cast<int>(view.layers.size()), cols = static_cast<int>(view.columns.size());
    if (layers == 0 || cols == 0)
        return request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    const float gap = std::max(3.f, u * .14f);
    const float headW = o.headerWidth > 0 ? o.headerWidth : u * 9.f, titleH = u * 1.3f;
    float cellW = o.cellWidth > 0 ? o.cellWidth : u * 5.f;
    if (o.fitColumns) {
        const float availW = o.size.x > 0 ? o.size.x : ImGui::GetContentRegionAvail().x;
        cellW = std::max(u * 2.2f, (availW - ImGui::GetStyle().WindowPadding.x * 2.f - headW - gap * static_cast<float>(cols + 1)) / static_cast<float>(cols));
    }
    float cellH = o.cellHeight > 0 ? o.cellHeight : u * 2.4f;
    if (o.aspect > 0)
        cellH = std::max(u * 2.4f, cellW / o.aspect);
    const float round = options.theme ? options.theme->radius.control + 2.f : u * .4f;
    ImVec2 size = o.size;
    if (size.y <= 0)
        size.y = titleH + gap + (cellH + gap) * static_cast<float>(std::min(layers, 10)) + ImGui::GetStyle().ScrollbarSize + 4.f;
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0, 0, 0, 0});
    const bool open = ImGui::BeginChild("##launcher", size, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    if (open) {
        auto* draw = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const float gridX = origin.x + headW + gap;
        // Column titles double as scene launchers.
        for (int col = 0; col < cols; ++col) {
            const ImVec2 a{gridX + static_cast<float>(col) * (cellW + gap), origin.y}, b{a.x + cellW, a.y + titleH};
            if (!ImGui::IsRectVisible(a, b))
                continue;
            ImGui::PushID(col);
            ImGui::SetCursorScreenPos(a);
            ImGui::InvisibleButton("##col", {cellW, titleH});
            const bool hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked())
                request.triggerColumn = col;
            SemanticItem(options, FrameText("Launch column %s", Safe(view.columns[static_cast<std::size_t>(col)])),
                         accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
            draw->AddRectFilled(a, b, U32(Mix(c.input, c.text, hovered ? .1f : .03f)), round * .8f);
            GlyphPlay(draw, {a.x + u * .6f, a.y + titleH * .5f}, u * .6f, U32(hovered ? c.accent : c.muted));
            DrawTextFit(draw, {a.x + u * 1.2f, a.y + (titleH - ImGui::GetTextLineHeight()) * .5f}, cellW - u * 1.5f, U32(c.text),
                        Safe(view.columns[static_cast<std::size_t>(col)]));
            ImGui::PopID();
        }
        for (int layer = 0; layer < layers; ++layer) {
            const LayerView& lv = view.layers[static_cast<std::size_t>(layer)];
            const float y = origin.y + titleH + gap + static_cast<float>(layer) * (cellH + gap);
            const ImVec2 la{origin.x, y}, lb{origin.x + headW, y + cellH};
            if (!ImGui::IsRectVisible({origin.x, y}, {gridX + (cellW + gap) * static_cast<float>(cols), y + cellH}))
                continue;
            ImGui::PushID(layer + 100000);
            const ImVec4 tone = lv.color.w > 0 ? lv.color : c.layer(static_cast<std::size_t>(layer));
            // Layer header surface and select hit area.
            ImGui::SetCursorScreenPos(la);
            ImGui::SetNextItemAllowOverlap(); // The toggles and the fader are submitted after and must take the hover.
            ImGui::InvisibleButton("##layer", {headW, cellH}, ImGuiButtonFlags_None);
            if (ImGui::IsItemClicked())
                request.selectLayer = layer;
            SemanticItem(options, Safe(lv.name), accessibility::SemanticRole::Group, accessibility::SemanticAction::Select, lv.selected);
            draw->AddRectFilled(la, lb, U32(Mix(c.raised, c.accent, lv.selected ? .12f : 0.f)), round);
            draw->AddRect(la, lb, U32(lv.selected ? c.accent : c.border, lv.selected ? .9f : .6f), round, 0, detail::FrameLineWidth(1.f));
            draw->AddRectFilled({la.x, la.y + 3}, {la.x + 4.f, lb.y - 3}, U32(tone), 2.f);
            // Stop square.
            {
                const float s = u * .8f;
                ImGui::SetCursorScreenPos({la.x + u * .4f, la.y + u * .25f});
                ImGui::InvisibleButton("##stop", {s, s});
                if (ImGui::IsItemClicked())
                    request.stopLayer = layer;
                SemanticItem(options, FrameText("Stop %s", Safe(lv.name)), accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
                const ImVec2 sc{la.x + u * .4f + s * .5f, la.y + u * .25f + s * .5f};
                const bool sh = ImGui::IsItemHovered();
                draw->AddCircleFilled(sc, s * .5f, U32(Mix(c.input, c.text, sh ? .16f : .07f)), 20);
                GlyphStop(draw, sc, s * .62f, U32(sh ? c.text : c.muted));
            }
            DrawTextFit(draw, {la.x + u * 1.4f, la.y + u * .25f + (u * .8f - ImGui::GetTextLineHeight()) * .5f}, headW - u * 1.4f - u * 3.0f,
                        U32(c.text), Safe(lv.name));
            // M / S / B toggles at the top right.
            {
                float tx = lb.x - u * .35f - u * .78f;
                const float ty = la.y + u * .25f;
                struct T { const char* l; const char* n; bool v; LayerToggle which; ImVec4 tone; };
                const T toggles[3] = {{"B", "Bypass", lv.bypass, LayerToggle::Bypass, c.muted},
                                      {"S", "Solo", lv.solo, LayerToggle::Solo, c.warning},
                                      {"M", "Mute", lv.mute, LayerToggle::Mute, c.error}};
                for (const T& t : toggles) {
                    ImGui::SetCursorScreenPos({tx, ty});
                    bool v = t.v;
                    if (MiniToggle(t.n, t.l, &v, t.tone, c, options, FrameText("%s %s", t.n, Safe(lv.name)))) {
                        request.toggledLayer = layer;
                        request.toggle = t.which;
                    }
                    tx -= u * .86f;
                }
            }
            // Opacity fader along the bottom of the header.
            if (lv.opacity) {
                FaderOptions fo;
                fo.vertical = false;
                fo.showValue = false;
                fo.defaultValue = 1.f;
                fo.size = {headW - u * .8f, u * 1.0f};
                ImGui::SetCursorScreenPos({la.x + u * .4f, lb.y - u * 1.1f});
                const ValueEdit e = Fader(FrameText("Opacity %s", Safe(lv.name)), lv.opacity, fo, options);
                if (e) {
                    request.opacity = e;
                    request.opacityLayer = layer;
                }
            }
            ImGui::PopID();
            // Cells.
            for (int col = 0; col < cols; ++col) {
                const std::size_t index = static_cast<std::size_t>(layer) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(col);
                if (index >= view.cells.size())
                    break;
                const ImVec2 a{gridX + static_cast<float>(col) * (cellW + gap), y}, b{a.x + cellW, a.y + cellH};
                if (!ImGui::IsRectVisible(a, b))
                    continue;
                const PadView& pad = view.cells[index];
                ImGui::PushID(static_cast<int>(index) + 1000000);
                ImGui::SetCursorScreenPos(a);
                ImGui::InvisibleButton("##cell", {cellW, cellH});
                const bool hovered = ImGui::IsItemHovered(), held = ImGui::IsItemActive(), focused = ImGui::IsItemFocused();
                if (ImGui::IsItemActivated() && request.layer < 0) {
                    request.layer = layer;
                    request.column = col;
                    request.pressed = true;
                }
                if (ImGui::IsItemDeactivated() && request.layer < 0) {
                    request.layer = layer;
                    request.column = col;
                    request.released = true;
                }
                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && request.layer < 0) {
                    request.layer = layer;
                    request.column = col;
                    request.secondary = true;
                }
                PadView shown = pad;
                if (shown.color.w <= 0)
                    shown.color = tone;
                impl::DrawPadCell(ImGui::GetWindowDrawList(), c, options, a, b, shown, layer * cols + col, hovered, held, focused, round);
                const char* name = pad.state == PadState::Empty
                                       ? FrameText("Empty cell, %s, %s", Safe(lv.name), Safe(view.columns[static_cast<std::size_t>(col)]))
                                       : FrameText("%s, %s, %s", Safe(pad.label), Safe(lv.name), Safe(view.columns[static_cast<std::size_t>(col)]));
                if (SemanticItem(options, name, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press) && request.layer < 0) {
                    request.layer = layer;
                    request.column = col;
                    request.pressed = request.released = true;
                }
                ImGui::PopID();
            }
        }
        ImGui::SetCursorScreenPos(origin);
        ImGui::Dummy({headW + gap + (cellW + gap) * static_cast<float>(cols), titleH + gap + (cellH + gap) * static_cast<float>(layers)});
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopID();
    return request;
}

// ---- Mixer ----------------------------------------------------------------------------------------------------
CrossfadeGains CrossfadeLaw(float position, CrossfadeCurve curve) {
    const float p = Clamp01(position);
    switch (curve) {
    case CrossfadeCurve::EqualPower: return {std::cos(p * Pi * .5f), std::sin(p * Pi * .5f)};
    case CrossfadeCurve::Cut: return {p < .5f ? 1.f : 0.f, p < .5f ? 0.f : 1.f};
    default: return {1.f - p, p};
    }
}

MixerRequest Mixer(const char* id, std::span<MixerStrip> strips, MixerState& state, MixerOptions o, ComponentOptions options) {
    MixerRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float faderH = o.faderHeight > 0 ? o.faderHeight : u * 7.5f;
    const float stripW = u * 4.4f;
    const float scale[] = {6, 0, -12, -24, -48};
    ImGui::PushID(id);
    ImGui::BeginGroup();
    auto drawStrip = [&](int index, const char* name, ImVec4 tone, float* gain, bool* mute, bool* solo, int* bus,
                         std::span<const float> levels, std::span<const float> peaks, bool master) {
        ImGui::PushID(index + 10);
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::BeginGroup();
        const float width = master ? stripW * 1.15f : stripW;
        // Name tab.
        const float tab = u * 1.0f;
        draw->AddRectFilled(a, {a.x + width, a.y + tab}, U32(Mix(c.raised, tone, .5f)), 6.f, ImDrawFlags_RoundCornersTop);
        DrawTextFit(draw, {a.x + u * .4f, a.y + (tab - ImGui::GetTextLineHeight()) * .5f}, width - u * .8f, U32(InkFor(c, Mix(c.raised, tone, .5f))), name);
        ImGui::Dummy({width, tab + 4.f});
        // Toggles.
        if (mute || solo || bus) {
            bool any = false;
            if (mute) {
                if (MiniToggle("mute", "M", mute, c.error, c, options, FrameText("Mute %s", name))) {
                    request.strip = index;
                    request.toggled = true;
                }
                any = true;
            }
            if (solo) {
                if (any)
                    ImGui::SameLine(0, 3.f);
                if (MiniToggle("solo", "S", solo, c.warning, c, options, FrameText("Solo %s", name))) {
                    request.strip = index;
                    request.toggled = true;
                }
                any = true;
            }
            if (bus) {
                bool a_ = *bus == 1, b_ = *bus == 2;
                if (any)
                    ImGui::SameLine(0, 3.f);
                if (MiniToggle("busA", "A", &a_, c.busA, c, options, FrameText("Bus A %s", name))) {
                    *bus = a_ ? 1 : 0;
                    request.strip = index;
                    request.toggled = true;
                }
                ImGui::SameLine(0, 3.f);
                if (MiniToggle("busB", "B", &b_, c.busB, c, options, FrameText("Bus B %s", name))) {
                    *bus = b_ ? 2 : 0;
                    request.strip = index;
                    request.toggled = true;
                }
            }
        }
        if (gain) {
            const float ticks[] = {6, 0, -6, -12, -24, -48};
            FaderOptions fo;
            fo.scale = FaderScale::Decibel;
            fo.minimum = -60;
            fo.maximum = 6;
            fo.size = {u * 2.3f, faderH};
            fo.ticks = ticks;
            fo.format = "%.1f";
            const ValueEdit e = Fader(name, gain, fo, options);
            if (e) {
                request.strip = master ? -2 : index;
                request.edit = e;
                request.toggled = false;
            }
            ImGui::SameLine(0, 4.f);
            MeterOptions mo;
            mo.size = {u * 1.1f * static_cast<float>(std::max<std::size_t>(1, std::min<std::size_t>(levels.size(), 2))) * .55f + u * .4f,
                       faderH - u * 1.7f};
            if (levels.empty()) {
                const float silent[1] = {-120.f};
                MeterBar(FrameText("%s meter", name), silent, silent, mo, options);
            } else {
                MeterBar(FrameText("%s meter", name), levels, peaks, mo, options);
            }
        }
        ImGui::EndGroup();
        ImGui::PopID();
    };
    for (std::size_t i = 0; i < strips.size(); ++i) {
        MixerStrip& s = strips[i];
        const ImVec4 tone = s.color.w > 0 ? s.color : c.layer(i);
        drawStrip(static_cast<int>(i), Safe(s.name), tone, s.gainDb, s.mute, s.solo, s.bus, s.levelsDb, s.peaksDb, false);
        ImGui::SameLine(0, u * .5f);
    }
    // Divider and master.
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        draw->AddLine({p.x - u * .25f, p.y}, {p.x - u * .25f, p.y + faderH + u * 2.4f}, U32(c.border), 1.f);
    }
    drawStrip(-2, Str(options, "show.master", "Master"), c.accent, &state.masterDb, nullptr, nullptr, nullptr, o.masterLevelsDb,
              o.masterPeaksDb, true);
    ImGui::EndGroup();
    if (o.showCrossfader) {
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.busA, "A");
        ImGui::SameLine();
        FaderOptions fo;
        fo.vertical = false;
        fo.bipolar = true;
        fo.defaultValue = .5f;
        fo.showValue = false;
        const float ticks[] = {0, .5f, 1};
        fo.ticks = ticks;
        fo.size = {std::max(u * 8.f, ImGui::GetContentRegionAvail().x - u * 3.f), u * 1.6f};
        const ValueEdit e = Fader(Str(options, "show.crossfader", "Crossfader"), &state.crossfade, fo, options);
        if (e) {
            request.strip = -3;
            request.edit = e;
            request.toggled = false;
        }
        ImGui::SameLine();
        ImGui::TextColored(c.busB, "B");
    }
    ImGui::PopID();
    (void)scale;
    return request;
}

// ---- Transition bar -------------------------------------------------------------------------------------------
TransitionRequest TransitionBar(const char* id, const TransitionView& view, ComponentOptions options) {
    TransitionRequest request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    ImGui::PushID(id);
    ImGui::BeginGroup();
    if (view.position) {
        FaderOptions fo;
        fo.showValue = false;
        fo.defaultValue = 0;
        fo.size = {u * 2.4f, u * 7.f};
        fo.format = "%.2f";
        request.tbar = Fader(Str(options, "show.tbar", "T-bar"), view.position, fo, options);
    }
    ImGui::EndGroup();
    ImGui::SameLine(0, u * .8f);
    ImGui::BeginGroup();
    ImGui::TextDisabled("%s  >  %s", Safe(view.from), Safe(view.to));
    if (ActionButton(Str(options, "show.cut", "Cut"), ActionVariant::Secondary, {u * 4.f, 0}, options))
        request.cut = true;
    ImGui::BeginDisabled(view.running);
    if (ActionButton(view.running ? Str(options, "show.running", "Running") : Str(options, "show.auto", "Auto"), ActionVariant::Primary,
                     {u * 4.f, 0}, options))
        request.autoTransition = true;
    ImGui::EndDisabled();
    if (view.durationSeconds) {
        KnobOptions ko;
        ko.minimum = 0;
        ko.maximum = 10;
        ko.defaultValue = 1;
        ko.step = .1f;
        ko.format = "%.1f";
        ko.unit = "s";
        ko.diameter = u * 1.6f;
        request.duration = Knob(Str(options, "show.duration", "Duration"), view.durationSeconds, ko, options);
    }
    ImGui::EndGroup();
    (void)c;
    ImGui::PopID();
    return request;
}

// ---- Effect rack ----------------------------------------------------------------------------------------------
EffectRackRequest EffectRack(const char* id, std::span<const EffectSlot> slots, EffectRackState& state, ComponentOptions options) {
    EffectRackRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float round = options.theme ? options.theme->radius.control + 2.f : u * .4f;
    ImGui::PushID(id);
    const float width = ImGui::GetContentRegionAvail().x;
    for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
        const EffectSlot& slot = slots[static_cast<std::size_t>(i)];
        const bool bypassed = slot.bypass && *slot.bypass;
        const bool expanded = i < static_cast<int>(state.expanded.size()) && state.expanded[static_cast<std::size_t>(i)];
        ImGui::PushID(i);
        const ImVec2 a = ImGui::GetCursorScreenPos();
        const float headH = u * 1.5f;
        const float bodyH = expanded ? static_cast<float>(slot.params.size()) * u * 1.15f + u * .5f : 0.f;
        draw->AddRectFilled(a, {a.x + width, a.y + headH + bodyH}, U32(c.raised), round);
        draw->AddRect(a, {a.x + width, a.y + headH + bodyH}, U32(c.border, detail::FrameLineAlpha(.7f)), round, 0, detail::FrameLineWidth(1.f));
        // Grip (drag source), chevron (expand), name.
        ImGui::SetCursorScreenPos({a.x + u * .2f, a.y});
        ImGui::InvisibleButton("##grip", {u * 1.0f, headH});
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
            ImGui::SetDragDropPayload("IMKIT_EFFECT", &i, sizeof(int));
            ImGui::TextUnformatted(Safe(slot.name));
            ImGui::EndDragDropSource();
        }
        for (int dot = 0; dot < 6; ++dot)
            draw->AddCircleFilled({a.x + u * .5f + static_cast<float>(dot % 2) * u * .22f, a.y + headH * .5f + (static_cast<float>(dot / 2) - 1.f) * u * .22f},
                                  1.4f, U32(c.muted, ImGui::IsItemHovered() ? 1.f : .6f), 6);
        SemanticItem(options, FrameText("Move %s", Safe(slot.name)), accessibility::SemanticRole::Button, accessibility::SemanticAction::None);
        ImGui::SetCursorScreenPos({a.x + u * 1.3f, a.y});
        ImGui::InvisibleButton("##expand", {std::max(u, width * .3f), headH});
        if (ImGui::IsItemClicked() && i < static_cast<int>(state.expanded.size())) {
            state.expanded[static_cast<std::size_t>(i)] = !expanded;
            request.slot = i;
        }
        SemanticItem(options, FrameText("%s", Safe(slot.name)), accessibility::SemanticRole::Button, accessibility::SemanticAction::Press, expanded);
        detail::DrawChevron(draw, {a.x + u * 1.7f, a.y + headH * .5f}, u * .4f, U32(c.muted), expanded ? ImGuiDir_Down : ImGuiDir_Right, 1.5f);
        draw->AddText({a.x + u * 2.2f, a.y + (headH - ImGui::GetTextLineHeight()) * .5f}, U32(bypassed ? c.muted : c.text), Safe(slot.name));
        // Right: mix fader, bypass, remove.
        float rx = a.x + width - u * .5f;
        rx -= u * .9f;
        ImGui::SetCursorScreenPos({rx, a.y + (headH - u * .78f) * .5f});
        ImGui::InvisibleButton("##remove", {u * .78f, u * .78f});
        if (ImGui::IsItemClicked())
            request.remove = i;
        SemanticItem(options, FrameText("Remove %s", Safe(slot.name)), accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
        detail::DrawCross(draw, {rx + u * .39f, a.y + headH * .5f}, u * .3f, U32(ImGui::IsItemHovered() ? c.error : c.muted), 1.5f);
        rx -= u * 1.0f;
        if (slot.bypass) {
            bool on = !*slot.bypass;
            ImGui::SetCursorScreenPos({rx, a.y + (headH - u * .78f) * .5f});
            if (MiniToggle("on", "I", &on, c.success, c, options, FrameText("Enable %s", Safe(slot.name)))) {
                *slot.bypass = !on;
                request.slot = i;
            }
            rx -= u * .4f;
        }
        if (slot.mix) {
            FaderOptions fo;
            fo.vertical = false;
            fo.showValue = false;
            fo.defaultValue = 1.f;
            fo.size = {u * 5.f, u * 1.0f};
            rx -= fo.size.x;
            ImGui::SetCursorScreenPos({rx, a.y + (headH - fo.size.y) * .5f});
            const ValueEdit e = Fader(FrameText("Mix %s", Safe(slot.name)), slot.mix, fo, options);
            if (e) {
                request.slot = i;
                request.param = -1;
                request.edit = e;
            }
        }
        // Parameters.
        if (expanded) {
            float y = a.y + headH + u * .15f;
            for (int p = 0; p < static_cast<int>(slot.params.size()); ++p) {
                const EffectParam& param = slot.params[static_cast<std::size_t>(p)];
                draw->AddText({a.x + u * 1.3f, y + (u * 1.0f - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), Safe(param.name));
                if (param.value) {
                    FaderOptions fo;
                    fo.vertical = false;
                    fo.minimum = param.minimum;
                    fo.maximum = param.maximum;
                    fo.defaultValue = param.defaultValue;
                    fo.format = param.format;
                    fo.unit = param.unit;
                    const float labelW = u * 6.f;
                    fo.size = {std::max(u * 6.f, width - labelW - u * 2.f), u * 1.0f};
                    ImGui::SetCursorScreenPos({a.x + labelW + u * 1.3f, y});
                    ImGui::PushID(p);
                    const ValueEdit e = Fader(FrameText("%s %s", Safe(slot.name), Safe(param.name)), param.value, fo, options);
                    ImGui::PopID();
                    if (e) {
                        request.slot = i;
                        request.param = p;
                        request.edit = e;
                    }
                }
                y += u * 1.15f;
            }
        }
        ImGui::SetCursorScreenPos({a.x, a.y + headH + bodyH});
        ImGui::Dummy({width, u * .25f});
        // Reorder target: the whole card.
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("IMKIT_EFFECT"))
                if (pl->DataSize == sizeof(int)) {
                    request.moveFrom = *static_cast<const int*>(pl->Data);
                    request.moveTo = i;
                }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
    }
    if (ActionButton(Str(options, "show.addEffect", "Add effect"), ActionVariant::Secondary, {width, 0}, options))
        request.add = true;
    ImGui::PopID();
    return request;
}

// ---- Modulator ------------------------------------------------------------------------------------------------
float ModulatorValue(ModShape shape, float phase, unsigned seed) {
    if (!std::isfinite(phase))
        return 0;
    float p = phase - std::floor(phase);
    switch (shape) {
    case ModShape::Sine: return std::sin(p * 2.f * Pi);
    case ModShape::Triangle: return p < .25f ? 4.f * p : p < .75f ? 2.f - 4.f * p : 4.f * p - 4.f;
    case ModShape::Saw: return 2.f * p - 1.f;
    case ModShape::Square: return p < .5f ? 1.f : -1.f;
    default: {
        unsigned h = static_cast<unsigned>(p * 8.f) * 2654435761u + seed * 40503u + 12345u;
        h ^= h >> 13;
        h *= 1274126177u;
        h ^= h >> 16;
        return static_cast<float>(h & 0xffff) / 32767.5f - 1.f;
    }
    }
}

ModulatorRequest ModulatorEditor(const char* id, const ModulatorView& view, ComponentOptions options) {
    ModulatorRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    ImGui::BeginGroup();
    const float phase = view.phase ? *view.phase : 0.f, amount = view.amount ? *view.amount : 1.f, offset = view.offset ? *view.offset : 0.f;
    const ModShape shape = view.shape ? *view.shape : ModShape::Sine;
    // Preview: two periods, vertically scaled by amount and shifted by offset.
    const ImVec2 size{u * 10.f, u * 4.2f};
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ReadoutItem(size);
    draw->AddRectFilled(a, b, U32(c.input), u * .3f);
    draw->AddLine({a.x, (a.y + b.y) * .5f}, {b.x, (a.y + b.y) * .5f}, U32(c.border), 1.f);
    draw->AddLine({(a.x + b.x) * .5f, a.y}, {(a.x + b.x) * .5f, b.y}, U32(c.border, .5f), 1.f);
    const int steps = 160;
    auto at = [&](float t) {
        const float v = std::clamp(offset + amount * ModulatorValue(shape, t * 2.f + phase), -1.f, 1.f);
        return ImVec2{a.x + t * size.x, (a.y + b.y) * .5f - v * (size.y * .42f)};
    };
    for (int i = 0; i < steps; ++i)
        draw->AddLine(at(static_cast<float>(i) / steps), at(static_cast<float>(i + 1) / steps), U32(c.accent), 2.f);
    if (view.playPhase >= 0) {
        const float t = std::fmod(view.playPhase, 1.f);
        const ImVec2 p = at(t * .5f);
        const float x = a.x + t * .5f * size.x;
        draw->AddLine({x, a.y}, {x, b.y}, U32(c.text, .35f), 1.f);
        draw->AddCircleFilled({x, p.y}, u * .16f, U32(c.text), 12);
    }
    AnnotateText(options, Str(options, "show.modulatorPreview", "Modulator preview"), "waveform", accessibility::SemanticRole::Status);
    // Shape buttons.
    if (view.shape) {
        ImGui::SameLine(0, u * .6f);
        ImGui::BeginGroup();
        const char* names[] = {"Sine", "Triangle", "Saw", "Square", "Random"};
        for (int s = 0; s < 5; ++s) {
            ImGui::PushID(s);
            const float cell = u * 1.5f;
            const ImVec2 sa = ImGui::GetCursorScreenPos(), sb{sa.x + cell, sa.y + cell * .78f};
            ImGui::InvisibleButton("##shape", {cell, cell * .78f});
            if (ImGui::IsItemClicked()) {
                *view.shape = static_cast<ModShape>(s);
                request.shapeChanged = true;
            }
            const bool on = *view.shape == static_cast<ModShape>(s);
            SemanticItem(options, names[s], accessibility::SemanticRole::Radio, accessibility::SemanticAction::Press, on);
            draw->AddRectFilled(sa, sb, U32(on ? Mix(c.input, c.accent, .25f) : Mix(c.input, c.text, ImGui::IsItemHovered() ? .08f : 0.f)), u * .25f);
            if (on)
                draw->AddRect(sa, sb, U32(c.accent), u * .25f, 0, 1.f);
            for (int i = 0; i < 24; ++i) {
                const auto pt = [&](int k) {
                    const float t = static_cast<float>(k) / 24.f;
                    return ImVec2{sa.x + cell * .15f + t * cell * .7f,
                                  sa.y + cell * .39f - ModulatorValue(static_cast<ModShape>(s), t) * cell * .24f};
                };
                draw->AddLine(pt(i), pt(i + 1), U32(on ? c.text : c.muted), 1.4f);
            }
            ImGui::PopID();
            if (s % 2 == 0)
                ImGui::SameLine(0, 3.f);
        }
        ImGui::EndGroup();
    }
    // Knobs.
    auto knob = [&](const char* name, float* v, float mn, float mx, float def, const char* fmt, const char* unit, int field) {
        if (!v)
            return;
        KnobOptions ko;
        ko.minimum = mn;
        ko.maximum = mx;
        ko.defaultValue = def;
        ko.format = fmt;
        ko.unit = unit;
        ko.bipolar = mn < 0;
        ko.diameter = u * 1.7f;
        const ValueEdit e = Knob(name, v, ko, options);
        if (e) {
            request.edit = e;
            request.field = field;
        }
        ImGui::SameLine();
    };
    const bool sync = view.beatSync && *view.beatSync;
    knob(Str(options, "show.rate", "Rate"), view.rate, sync ? 1.f : .05f, sync ? 16.f : 20.f, 1.f, sync ? "%.0f" : "%.2f", sync ? "/beat" : "Hz", 0);
    knob(Str(options, "show.phase", "Phase"), view.phase, 0.f, 1.f, 0.f, "%.2f", "", 1);
    knob(Str(options, "show.amount", "Amount"), view.amount, 0.f, 1.f, 1.f, "%.2f", "", 2);
    knob(Str(options, "show.offset", "Offset"), view.offset, -1.f, 1.f, 0.f, "%.2f", "", 3);
    if (view.beatSync) {
        MiniToggle("sync", "Sync", view.beatSync, c.accent, c, options, Str(options, "show.beatSync", "Beat sync"), u * 1.1f);
    }
    ImGui::NewLine();
    ImGui::EndGroup();
    ImGui::PopID();
    return request;
}

// ---- Mapping table and message monitor ------------------------------------------------------------------------
namespace {
const char* KindWord(BindingKind kind) {
    switch (kind) {
    case BindingKind::Midi: return "MIDI";
    case BindingKind::Osc: return "OSC";
    case BindingKind::Key: return "KEY";
    case BindingKind::Modulator: return "MOD";
    default: return "";
    }
}
} // namespace

MappingRequest MappingTable(const char* id, std::span<const MappingRow> rows, MappingState& state, ComponentOptions options) {
    MappingRequest request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    ImGui::PushID(id);
    if (ImGui::BeginTable("##map", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable,
                          {0, (u * 1.2f + ImGui::GetStyle().CellPadding.y * 2.f) * 7.f})) {
        ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, u * 1.4f);
        ImGui::TableSetupColumn(Str(options, "show.source", "Source"), ImGuiTableColumnFlags_WidthStretch, 1.1f);
        ImGui::TableSetupColumn(Str(options, "show.target", "Target"), ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn(Str(options, "show.range", "Range"), ImGuiTableColumnFlags_WidthFixed, u * 5.f);
        ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, u * 5.f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(rows.size()), u * 1.2f + ImGui::GetStyle().CellPadding.y * 2.f);
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                const MappingRow& row = rows[static_cast<std::size_t>(i)];
                ImGui::PushID(i);
                ImGui::TableNextRow(ImGuiTableRowFlags_None, u * 1.2f);
                const bool learning = row.id != 0 && row.id == state.learning;
                ImGui::TableSetColumnIndex(0);
                bool on = row.enabled;
                if (MiniToggle("on", "I", &on, c.success, c, options, FrameText("Enable %s", Safe(row.target)), u * .8f))
                    request.toggleEnabled = row.id;
                ImGui::TableSetColumnIndex(1);
                ImGui::AlignTextToFramePadding();
                const bool selected = row.id != 0 && row.id == state.selected;
                if (ImGui::Selectable(FrameText("%s %s##sel", KindWord(row.kind), Safe(row.source)), selected,
                                      ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, {0, u * 1.2f}))
                    request.select = row.id;
                SemanticItem(options, FrameText("%s %s to %s", KindWord(row.kind), Safe(row.source), Safe(row.target)),
                             accessibility::SemanticRole::Row, accessibility::SemanticAction::Select, selected);
                ImGui::TableSetColumnIndex(2);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(Safe(row.target));
                ImGui::TableSetColumnIndex(3);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("%s", Safe(row.range));
                ImGui::TableSetColumnIndex(4);
                if (row.conflict) {
                    GlyphWarning(ImGui::GetWindowDrawList(), {ImGui::GetCursorScreenPos().x + u * .4f, ImGui::GetCursorScreenPos().y + u * .5f}, u * .6f, U32(c.warning));
                    ImGui::Dummy({u * .8f, u * .8f});
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", Str(options, "show.conflict", "Another mapping listens to the same source."));
                    ImGui::SameLine();
                }
                bool learn = learning;
                if (MiniToggle("learn", learning ? "..." : "L", &learn, c.warning, c, options, FrameText("Learn %s", Safe(row.target)), u * .8f))
                    request.learn = row.id;
                ImGui::SameLine();
                if (ImGui::SmallButton("x"))
                    request.remove = row.id;
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
    return request;
}

MessageMonitorRequest MessageMonitor(const char* id, std::span<const MessageView> messages, MessageMonitorState& state, ImVec2 size,
                                     ComponentOptions options) {
    MessageMonitorRequest request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    ImGui::PushID(id);
    if (ActionButton(state.paused ? Str(options, "show.resume", "Resume") : Str(options, "show.pause", "Pause"), ActionVariant::Secondary,
                     {0, 0}, options))
        state.paused = !state.paused;
    ImGui::SameLine();
    if (FilterChip("midi", "MIDI", state.midi, options))
        state.midi = !state.midi;
    ImGui::SameLine();
    if (FilterChip("osc", "OSC", state.osc, options))
        state.osc = !state.osc;
    ImGui::SameLine();
    if (FilterChip("key", "KEY", state.key, options))
        state.key = !state.key;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(u * 8.f);
    ImGui::InputTextWithHint("##filter", Str(options, "show.filter", "Filter"), state.filter, sizeof(state.filter));
    ImGui::SameLine();
    if (ActionButton(Str(options, "show.clear", "Clear"), ActionVariant::Ghost, {0, 0}, options))
        request.clear = true;
    if (state.paused) {
        ImGui::SameLine();
        StatusLed("paused", LedState::Warning, Str(options, "show.paused", "Paused"), options);
    }
    static thread_local std::vector<int> shown;
    shown.clear();
    for (int i = 0; i < static_cast<int>(messages.size()); ++i) {
        const MessageView& m = messages[static_cast<std::size_t>(i)];
        if ((m.kind == BindingKind::Midi && !state.midi) || (m.kind == BindingKind::Osc && !state.osc) ||
            (m.kind == BindingKind::Key && !state.key))
            continue;
        if (state.filter[0] && !std::strstr(Safe(m.text), state.filter))
            continue;
        shown.push_back(i);
    }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.surface);
    if (ImGui::BeginChild("##messages", size, ImGuiChildFlags_Borders)) {
        const double origin = messages.empty() ? 0.0 : messages.front().seconds;
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(shown.size()), ImGui::GetTextLineHeightWithSpacing());
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const int index = shown[static_cast<std::size_t>(row)];
                const MessageView& m = messages[static_cast<std::size_t>(index)];
                ImGui::PushID(index);
                const double t = m.seconds - origin;
                ImGui::TextDisabled("%02d:%02d.%03d", static_cast<int>(t) / 60, static_cast<int>(t) % 60, static_cast<int>((t - std::floor(t)) * 1000.0));
                ImGui::SameLine(u * 5.f);
                ImGui::TextColored(m.kind == BindingKind::Osc ? c.layer(2) : m.kind == BindingKind::Midi ? c.accent : c.muted, "%s", KindWord(m.kind));
                ImGui::SameLine(u * 8.f);
                if (ImGui::Selectable(FrameText("%s##m", Safe(m.text)), false, ImGuiSelectableFlags_AllowDoubleClick) &&
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    request.assign = index;
                ImGui::PopID();
            }
        }
        if (state.follow && !state.paused && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 2.f)
            ImGui::SetScrollHereY(1.f);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopID();
    return request;
}

// ---- Master bar -----------------------------------------------------------------------------------------------
MasterBarRequest MasterBar(const char* id, const MasterBarView& view, ComponentOptions options) {
    MasterBarRequest request;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    const float h = u * 2.f, gap = u * .45f;
    const float rightEdge = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    ImGui::PushID(id);
    if (view.master) {
        FaderOptions fo;
        fo.vertical = false;
        fo.defaultValue = 1.f;
        fo.format = "%.2f";
        fo.size = {u * 8.f, h};
        request.master = Fader(Str(options, "show.master", "Master"), view.master, fo, options);
    }
    // The keys follow on the same line while they fit and wrap to the next line otherwise.
    bool placed = view.master != nullptr;
    auto follow = [&](float width) {
        if (placed && FitsOnLine(rightEdge, width, gap))
            ImGui::SameLine(0, gap);
        placed = true;
    };
    follow(u * 4.2f);
    if (view.blackout) {
        bool on = true;
        ConsoleButtonOptions bo;
        bo.mode = ConsoleMode::Toggle;
        bo.tone = c.error;
        bo.solid = true;
        bo.size = {u * 4.2f, h};
        bo.hint = Str(options, "show.release", "release");
        if (ConsoleButton(Str(options, "show.blackout", "Blackout"), &on, bo, options).changed)
            request.blackout = true;
    } else {
        HoldButtonOptions ho;
        ho.key = true;
        ho.seconds = .6f;
        ho.size = {u * 4.2f, h};
        ho.hint = Str(options, "show.hold", "hold");
        if (HoldButton(Str(options, "show.blackout", "Blackout"), ho, options))
            request.blackout = true;
    }
    follow(u * 3.f);
    {
        bool on = view.freeze;
        ConsoleButtonOptions fo;
        fo.mode = ConsoleMode::Toggle;
        fo.tone = c.queued;
        fo.size = {u * 3.f, h};
        if (ConsoleButton(Str(options, "show.freeze", "Freeze"), &on, fo, options).changed)
            request.freeze = true;
    }
    follow(u * 3.f);
    {
        ConsoleButtonOptions fo;
        fo.mode = ConsoleMode::Flash;
        fo.tone = c.accent;
        fo.size = {u * 3.f, h};
        const ConsoleEvent e = ConsoleButton(Str(options, "show.flash", "Flash"), nullptr, fo, options);
        request.flash = ImGui::IsItemActive() || e.pressed;
        follow(u * 3.f);
        fo.tone = c.warning;
        const ConsoleEvent s = ConsoleButton(Str(options, "show.strobe", "Strobe"), nullptr, fo, options);
        request.strobe = ImGui::IsItemActive() || s.pressed;
    }
    ImGui::PopID();
    return request;
}

// ---- Device health --------------------------------------------------------------------------------------------
int DeviceHealth(const char* id, std::span<const DeviceView> devices, ComponentOptions options) {
    int clicked = -1;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float round = options.theme ? options.theme->radius.control + 2.f : u * .4f;
    ImGui::PushID(id);
    const float width = ImGui::GetContentRegionAvail().x;
    for (int i = 0; i < static_cast<int>(devices.size()); ++i) {
        const DeviceView& d = devices[static_cast<std::size_t>(i)];
        ImGui::PushID(i);
        const ImVec2 a = ImGui::GetCursorScreenPos();
        const float h = u * 2.4f;
        ImGui::InvisibleButton("##device", {width, h});
        const bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            clicked = i;
        const ImVec2 b{a.x + width, a.y + h};
        draw->AddRectFilled(a, b, U32(Mix(c.raised, c.text, hovered ? .05f : 0.f)), round);
        draw->AddRect(a, b, U32(d.led == LedState::Error ? c.error : d.led == LedState::Warning ? c.warning : c.border, d.led == LedState::Error || d.led == LedState::Warning ? .9f : .6f), round, 0, detail::FrameLineWidth(1.f));
        SemanticItem(options, FrameText("%s, %s", Safe(d.name), Safe(d.status)), accessibility::SemanticRole::Row, accessibility::SemanticAction::Select);
        // Status light (shape coded) and names.
        const ImVec2 lc{a.x + u * .8f, a.y + h * .5f};
        const float r = u * .2f;
        switch (d.led) {
        case LedState::Live: draw->AddCircleFilled(lc, r, U32(c.error), 14); break;
        case LedState::On: draw->AddCircle(lc, r, U32(c.success), 14, 2.f); draw->AddCircleFilled(lc, r * .45f, U32(c.success), 10); break;
        case LedState::Warning: GlyphWarning(draw, lc, r * 2.4f, U32(c.warning)); break;
        case LedState::Error: draw->AddCircleFilled(lc, r * 1.1f, U32(c.error), 14); detail::DrawCross(draw, lc, r * 1.1f, U32(c.surface), 1.8f); break;
        default: draw->AddCircleFilled(lc, r * .7f, U32(c.border), 10); break;
        }
        draw->AddText({a.x + u * 1.5f, a.y + h * .5f - ImGui::GetTextLineHeight()}, U32(c.text), Safe(d.name));
        draw->AddText({a.x + u * 1.5f, a.y + h * .5f}, U32(c.muted), Safe(d.status));
        // Figures, right to left, with a sparkline of the frame rate in front of them.
        float x = b.x - u * .6f;
        auto chip = [&](const char* text, ImVec4 tone) {
            const ImVec2 ts = ImGui::CalcTextSize(text);
            x -= ts.x + u * .7f;
            draw->AddRectFilled({x, a.y + h * .5f - ts.y * .7f}, {x + ts.x + u * .5f, a.y + h * .5f + ts.y * .7f}, U32(c.input), ts.y * .5f);
            draw->AddText({x + u * .25f, a.y + h * .5f - ts.y * .5f}, U32(tone), text);
            x -= u * .2f;
        };
        if (d.droppedFrames >= 0)
            chip(FrameText("%d dropped", d.droppedFrames), d.droppedFrames > 0 ? c.warning : c.muted);
        if (d.temperatureC >= 0)
            chip(FrameText("%.0f C", d.temperatureC), d.temperatureC > 80 ? c.error : d.temperatureC > 70 ? c.warning : c.text);
        if (d.gpuPercent >= 0)
            chip(FrameText("GPU %.0f%%", d.gpuPercent), d.gpuPercent > 95 ? c.warning : c.text);
        if (d.fps >= 0)
            chip(FrameText("%.1f fps", d.fps), c.text);
        if (!d.fpsHistory.empty() && x - a.x > u * 12.f) {
            const float sw = u * 5.f;
            const ImVec2 save = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos({x - sw - u * .3f, a.y + u * .35f});
            SparklineView sv;
            sv.values = d.fpsHistory;
            SparklineOptions so;
            so.size = {sw, h - u * .7f};
            so.area = false;
            so.kind = d.led == LedState::Error ? FeedbackKind::Error : d.led == LedState::Warning ? FeedbackKind::Warning : FeedbackKind::Info;
            Sparkline("fps", sv, so, options);
            ImGui::SetCursorScreenPos(save);
        }
        ImGui::PopID();
        ImGui::Dummy({0, u * .15f});
    }
    ImGui::PopID();
    return clicked;
}
} // namespace imkit::show
