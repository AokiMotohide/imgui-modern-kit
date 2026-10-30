#include <imkit/imkit.h>
#include <imkit/show.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string_view>
#include <vector>

namespace {
int failures = 0;
void Check(bool ok, const char* label) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        ++failures;
    }
}
bool Near(double a, double b, double eps = 1e-3) { return std::abs(a - b) <= eps; }
} // namespace

int main() {
    using namespace imkit;
    using namespace imkit::show;

    // ---- Geometry ----------------------------------------------------------------------------------------------
    {
        const SurfaceRect snapped = SnapRect({1.3f, 2.8f, 2.2f, 1.9f}, .5f, 24, 12, 1.f);
        Check(Near(snapped.x, 1.5) && Near(snapped.y, 3.0) && Near(snapped.w, 2.0) && Near(snapped.h, 2.0), "rectangles snap to the step");
        const SurfaceRect inside = SnapRect({23.5f, 11.5f, 4.f, 4.f}, 0.f, 24, 12, 1.f);
        Check(Near(inside.x, 20) && Near(inside.y, 8), "rectangles stay inside the grid");
        const SurfaceRect tiny = SnapRect({0, 0, .1f, .1f}, .5f, 24, 12, 1.f);
        Check(Near(tiny.w, 1) && Near(tiny.h, 1), "rectangles keep the minimum size");
        const SurfaceRect odd = SnapRect({std::numeric_limits<float>::quiet_NaN(), 0, std::numeric_limits<float>::infinity(), 3}, .5f, 24, 12, 1.f);
        Check(odd.w >= 1 && odd.w <= 24 && odd.x >= 0, "non-finite input is tamed");
        const SurfaceRect br = ResizeRect({2, 2, 4, 4}, SurfaceHandle::BottomRight, {1.2f, .8f}, .5f, 1.f, 24, 12);
        Check(Near(br.x, 2) && Near(br.y, 2) && Near(br.w, 5.0) && Near(br.h, 5.0), "bottom-right resize grows the rectangle");
        const SurfaceRect tl = ResizeRect({2, 2, 4, 4}, SurfaceHandle::TopLeft, {1.f, 1.f}, .5f, 1.f, 24, 12);
        Check(Near(tl.x, 3) && Near(tl.y, 3) && Near(tl.w, 3) && Near(tl.h, 3), "top-left resize moves the origin and shrinks");
        const SurfaceRect crush = ResizeRect({2, 2, 4, 4}, SurfaceHandle::Right, {-10.f, 0}, .5f, 1.f, 24, 12);
        Check(Near(crush.w, 1) && Near(crush.x, 2), "resizing stops at the minimum size");
        const SurfaceRect wall = ResizeRect({20, 2, 2, 2}, SurfaceHandle::Right, {10.f, 0}, 0.f, 1.f, 24, 12);
        Check(Near(wall.x + wall.w, 24), "resizing stops at the grid edge");
        Check(HitHandle({100, 100}, {200, 200}, {100, 100}, 12) == static_cast<int>(SurfaceHandle::TopLeft), "top-left handle is hit");
        Check(HitHandle({100, 100}, {200, 200}, {200, 150}, 12) == static_cast<int>(SurfaceHandle::Right), "right handle is hit");
        Check(HitHandle({100, 100}, {200, 200}, {150, 150}, 12) == -1, "the middle is not a handle");
        const SurfaceRect others[] = {{10, 0, 2, 2}, {0, 6, 2, 2}};
        const SurfaceGuides g = AlignGuides({10.1f, 3.f, 2.f, 2.f}, others, .3f);
        Check(g.hasX && Near(g.dx, -.1) && Near(g.x, 10), "a moving rectangle aligns with a neighbour's edge");
        const SurfaceGuides far = AlignGuides({15.f, 3.f, 2.f, 2.f}, others, .3f);
        Check(!far.hasX && !far.hasY, "distant rectangles do not align");
        const SurfaceGuides centre = AlignGuides({0.5f, 6.1f, 1.f, 1.f}, others, .3f);
        Check(centre.hasX && Near(centre.x, 1) , "centres align too");
        SurfaceState state;
        Check(SurfaceSelect(state, 5, false) && state.selectedCount == 1 && SurfaceIsSelected(state, 5), "select replaces");
        Check(SurfaceSelect(state, 6, true) && state.selectedCount == 2, "additive select adds");
        Check(SurfaceSelect(state, 5, true) && state.selectedCount == 1 && !SurfaceIsSelected(state, 5), "additive select toggles an entry off");
        Check(!SurfaceSelect(state, 0, false), "id zero cannot be selected");
        for (StableId id = 100; id < 100 + SurfaceSelectionCapacity + 4; ++id)
            SurfaceSelect(state, id, true);
        Check(state.selectedCount == static_cast<int>(SurfaceSelectionCapacity), "the selection is capped");
        SurfaceClearSelection(state);
        Check(state.selectedCount == 0, "the selection clears");
    }

    // ---- Component -----------------------------------------------------------------------------------------------
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1400, 900};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int fw, fh;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &fw, &fh);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 1024> nodes{};
    std::array<accessibility::ActionRequest, 8> storage{};
    accessibility::ActionQueue actions{storage};
    accessibility::AccessibilityFrame semantics{nodes, &actions};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    float level = .5f;
    bool keyOn = false, lockOn = false;
    std::vector<SurfaceItem> items;
    auto make = [](StableId id, SurfaceKind kind, SurfaceRect rect, const char* label) {
        SurfaceItem it;
        it.id = id;
        it.kind = kind;
        it.rect = rect;
        it.label = label;
        return it;
    };
    items.push_back(make(1, SurfaceKind::Panel, {0, 0, 6, 6}, "Layers"));
    items.push_back(make(2, SurfaceKind::Fader, {1, 1, 2, 4}, "Level"));
    items.back().value = &level;
    items.back().defaultValue = .5f;
    items.push_back(make(3, SurfaceKind::Button, {3.5f, 1, 2, 2}, "Key A"));
    items.back().on = &keyOn;
    items.back().mode = ConsoleMode::Toggle;
    items.push_back(make(4, SurfaceKind::Button, {3.5f, 3.5f, 2, 1.5f}, "Lock"));
    items.back().on = &lockOn;
    items.back().mode = ConsoleMode::Toggle;
    items.back().locked = true;
    items.push_back(make(5, SurfaceKind::Display, {8, 1, 5, 2}, "Clock"));
    items.back().text = "00:00";
    items.push_back(make(6, SurfaceKind::Custom, {8, 4, 4, 2}, "Custom"));
    bool customDrawn = false;
    items.back().draw = [](const SurfaceItem&, ImVec2 size, void* user) {
        *static_cast<bool*>(user) = size.x > 10 && size.y > 10;
        ImGui::Dummy(size);
    };
    items.back().user = &customDrawn;

    SurfaceState state;
    SurfaceRequest request;
    bool sawStarted = false, sawChanged = false, sawFinished = false, sawCancelled = false, sawResize = false;
    int lastCount = 0;
    SurfaceRect lastRect{};
    bool removeSeen = false;
    auto resetSeen = [&] {
        sawStarted = sawChanged = sawFinished = sawCancelled = sawResize = removeSeen = false;
        lastCount = 0;
    };
    auto frame = [&](bool reduced = false) {
        theme.motion.reducedMotion = reduced;
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        SurfaceOptions so;
        so.size = {960, 480};
        so.columns = 24;
        so.rows = 12;
        request = ControlSurface("surface", items, state, so, options);
        const auto& l = request.layout;
        if (l.started || l.changed || l.finished || l.cancelled) {
            sawStarted |= l.started;
            sawChanged |= l.changed;
            sawFinished |= l.finished;
            sawCancelled |= l.cancelled;
            sawResize |= l.resize;
            lastCount = l.count;
            if (l.count > 0)
                lastRect = l.items[0].rect;
            for (int i = 0; i < l.count; ++i)
                for (auto& it : items)
                    if (it.id == l.items[static_cast<std::size_t>(i)].id)
                        it.rect = l.items[static_cast<std::size_t>(i)].rect;
        }
        removeSeen |= request.removeSelected;
        ImGui::End();
        ImGui::Render();
    };
    auto find = [&](std::string_view name) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name)
                return &n;
        return nullptr;
    };
    auto centre = [](const accessibility::SemanticNode& n) { return ImVec2{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f}; };
    auto release = [&] {
        io.AddMouseButtonEvent(0, false);
        for (int i = 0; i < 32; ++i)
            frame();
    };
    auto dragFrom = [&](ImVec2 from, ImVec2 delta, int steps) {
        io.AddMousePosEvent(from.x, from.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        for (int i = 1; i <= steps; ++i) {
            io.AddMousePosEvent(from.x + delta.x * static_cast<float>(i) / static_cast<float>(steps), from.y + delta.y * static_cast<float>(i) / static_cast<float>(steps));
            frame();
        }
    };
    auto tap = [&](ImGuiKey key) {
        io.AddKeyEvent(key, true);
        frame();
        io.AddKeyEvent(key, false);
        frame();
    };

    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid in operate mode");
    Check(customDrawn, "a custom item draws inside its rectangle");
    Check(find("Level") && find("Level")->role == accessibility::SemanticRole::Slider, "operate mode exposes the real fader");
    Check(find("Key A") && find("Key A")->role == accessibility::SemanticRole::Toggle, "operate mode exposes the real toggle key");

    // Operate: a key toggles and the surface reports which item was used.
    if (const auto* node = find("Key A")) {
        const ImVec2 c = centre(*node);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(request.operated == 3 && request.toggled && keyOn, "operating a key reports the item and flips its state");
        release();
    } else
        Check(false, "operate key node");

    // Edit mode: selection.
    state.editing = true;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid in edit mode");
    const auto* levelEdit = find("Level, Fader");
    Check(levelEdit != nullptr && find("Layers, Panel") && find("Clock, Display"), "edit overlays are named by item and kind");
    if (levelEdit) {
        const ImVec2 c = centre(*levelEdit);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        const float before = level;
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(SurfaceIsSelected(state, 2) && state.selectedCount == 1 && request.selectionChanged, "clicking an item selects it");
        Check(Near(level, before) && request.operated == 0, "edit mode does not operate the control");
        release();

        // Move by two cells (the cell is 40 px).
        resetSeen();
        dragFrom(c, {80.f, 0.f}, 6);
        Check(sawStarted && sawChanged && !sawFinished && !sawResize && lastCount == 1, "dragging reports a started and changed move");
        release();
        Check(sawFinished && Near(lastRect.x, 3.0, .01) && Near(lastRect.y, 1.0, .01), "releasing finishes the move on a snapped position");
        Check(Near(items[1].rect.x, 3.0, .01), "the host applied the new position");

        // Resize from the bottom-right handle.
        const auto* moved = find("Level, Fader");
        if (moved) {
            const ImVec2 corner = moved->maximum;
            resetSeen();
            dragFrom(corner, {40.f, 40.f}, 5);
            release();
            Check(sawResize && sawFinished && Near(lastRect.w, 3.0, .01) && Near(lastRect.h, 5.0, .01), "a handle drag resizes by whole cells");
        } else
            Check(false, "moved fader node");

        // Escape cancels a move and restores the original rectangle.
        const SurfaceRect original = items[1].rect;
        const auto* again = find("Level, Fader");
        if (again) {
            const ImVec2 c2{again->minimum.x + 6.f, again->minimum.y + 6.f}; // The top-left corner is clear of the keys.
            resetSeen();
            dragFrom(c2, {0.f, 80.f}, 5);
            Check(sawStarted && items[1].rect.y > original.y, "the drag moves the rectangle while it lasts");
            io.AddKeyEvent(ImGuiKey_Escape, true);
            frame();
            io.AddKeyEvent(ImGuiKey_Escape, false);
            Check(sawCancelled && Near(items[1].rect.y, original.y, .01), "escape cancels and restores the original rectangle");
            release();
            Check(!sawFinished, "a cancelled move does not finish");
        }
    }

    // Keyboard.
    {
        SurfaceClearSelection(state);
        SurfaceSelect(state, 5, false);
        frame();
        const SurfaceRect before = items[4].rect;
        resetSeen();
        tap(ImGuiKey_RightArrow);
        Check(sawStarted && sawFinished && Near(items[4].rect.x, before.x + .5, .01), "an arrow key nudges by one snap step");
        resetSeen();
        tap(ImGuiKey_Delete);
        Check(removeSeen, "Delete requests removal of the selection");
    }

    // Locked items refuse to move.
    if (const auto* lockNode = find("Lock, Button")) {
        const ImVec2 c = centre(*lockNode);
        const SurfaceRect before = items[3].rect;
        resetSeen();
        dragFrom(c, {80.f, 0.f}, 5);
        release();
        Check(Near(items[3].rect.x, before.x) && Near(items[3].rect.y, before.y), "a locked item stays where it is");
    } else
        Check(false, "lock node");

    // Marquee: dragging over empty surface selects what it touches.
    {
        SurfaceClearSelection(state);
        const auto* panel = find("Layers, Panel");
        const auto* clock = find("Clock, Display");
        if (panel && clock) {
            const ImVec2 origin = panel->minimum;
            const ImVec2 from{origin.x + 40.f * 7.f, origin.y + 40.f * 0.2f}, to{origin.x + 40.f * 14.f, origin.y + 40.f * 3.5f};
            dragFrom(from, {to.x - from.x, to.y - from.y}, 5);
            release();
            Check(SurfaceIsSelected(state, 5), "a marquee selects the items it touches");
            Check(!SurfaceIsSelected(state, 1), "and leaves the others alone");
        } else
            Check(false, "marquee nodes");
    }
    ImGui::DestroyContext();
    std::puts(failures ? "show surface tests failed" : "show surface tests passed");
    return failures ? 1 : 0;
}
