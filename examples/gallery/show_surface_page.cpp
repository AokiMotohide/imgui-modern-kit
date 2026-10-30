#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace imkit::gallery {
using namespace show;

namespace {
SurfaceItem Make(StableId id, SurfaceKind kind, SurfaceRect rect, const char* label) {
    SurfaceItem it;
    it.id = id;
    it.kind = kind;
    it.rect = rect;
    it.label = label;
    return it;
}
} // namespace

void ShowPages::Surface(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    auto& s = surface;
    const float time = static_cast<float>(ImGui::GetTime());

    if (!s.initialized) {
        s.initialized = true;
        s.values.fill(0.f);
        for (int i = 0; i < 4; ++i)
            s.values[static_cast<std::size_t>(i)] = .8f - .1f * static_cast<float>(i);
        s.values[5] = .6f; // Mix
        s.values[6] = .5f;
        s.values[7] = .5f;
        auto add = [&](SurfaceItem it) {
            if (it.id == 0)
                it.id = s.nextId++;
            s.items.push_back(it);
        };
        add(Make(1, SurfaceKind::Panel, {0, 0, 10, 12}, "Layers"));
        static const char* layerNames[4] = {"Layer 1", "Layer 2", "Layer 3", "Layer 4"};
        static const char* muteNames[4] = {"Mute 1", "Mute 2", "Mute 3", "Mute 4"};
        for (int i = 0; i < 4; ++i) {
            SurfaceItem f = Make(10 + static_cast<StableId>(i), SurfaceKind::Fader, {.9f + 2.3f * static_cast<float>(i), 1.7f, 2.2f, 6.8f}, layerNames[i]);
            f.value = &s.values[static_cast<std::size_t>(i)];
            f.defaultValue = .8f;
            add(f);
            SurfaceItem k = Make(20 + static_cast<StableId>(i), SurfaceKind::Button, {.9f + 2.3f * static_cast<float>(i), 8.8f, 2.2f, 2.4f}, muteNames[i]);
            k.on = &s.keys[static_cast<std::size_t>(i)];
            add(k);
        }
        add(Make(2, SurfaceKind::Panel, {10.4f, 0, 6.8f, 6.2f}, "Master"));
        SurfaceItem gain = Make(30, SurfaceKind::Knob, {10.9f, 1.6f, 2.8f, 4.2f}, "Gain");
        gain.value = &s.values[4];
        gain.minimum = -24;
        gain.maximum = 24;
        add(gain);
        SurfaceItem mix = Make(31, SurfaceKind::Knob, {13.8f, 1.6f, 2.8f, 4.2f}, "Mix");
        mix.value = &s.values[5];
        mix.defaultValue = 1.f;
        add(mix);
        SurfaceItem clock = Make(32, SurfaceKind::Display, {10.4f, 6.5f, 6.8f, 2.6f}, "Program");
        clock.text = s.clock.data();
        add(clock);
        SurfaceItem go = Make(33, SurfaceKind::Button, {10.4f, 9.4f, 6.8f, 2.6f}, "GO");
        go.text = "Space";
        go.color = host.theme.semantic.accent;
        go.solid = true;
        go.labelScale = 1.6f;
        add(go);
        SurfaceItem xy = Make(34, SurfaceKind::XYPad, {17.6f, 0, 4.9f, 5.6f}, "Position");
        xy.value = &s.values[6];
        xy.value2 = &s.values[7];
        add(xy);
        SurfaceItem meter = Make(35, SurfaceKind::Meter, {22.9f, 0, 1.1f, 5.6f}, "Out");
        meter.value = &s.values[8];
        meter.minimum = -60;
        meter.maximum = 6;
        add(meter);
        for (int i = 0; i < 3; ++i) {
            SurfaceItem img = Make(40 + static_cast<StableId>(i), SurfaceKind::Image, {17.6f + 2.2f * static_cast<float>(i), 5.9f, 2.0f, 2.8f}, "");
            static const char* names[3] = {"Sunset", "Waves", "Neon"};
            img.label = names[i];
            img.image.image = host.sampleImages[static_cast<std::size_t>(i)];
            img.image.imagePixels = {96, 96};
            img.image.label = names[i];
            img.mode = i == 2 ? ConsoleMode::Toggle : ConsoleMode::Momentary;
            img.on = &s.keys[static_cast<std::size_t>(8 + i)];
            add(img);
        }
        SurfaceItem lock = Make(45, SurfaceKind::Button, {17.6f, 9.2f, 6.4f, 2.8f}, "Locked");
        lock.locked = true;
        lock.on = &s.keys[7];
        lock.mode = ConsoleMode::Toggle;
        add(lock);
        // A capture aid: start in edit mode with two items selected (set IMKIT_GALLERY_SURFACE_EDIT before launching).
        if (std::getenv("IMKIT_GALLERY_SURFACE_EDIT")) {
            s.state.editing = true;
            SurfaceSelect(s.state, 11, false);
            SurfaceSelect(s.state, 45, true);
        }
    }
    s.values[8] = -22.f + 16.f * std::sin(time * 2.7f);
    {
        const int t = static_cast<int>(time);
        std::snprintf(s.clock.data(), s.clock.size(), "%02d:%02d:%02d", t / 3600 % 24, t / 60 % 60, t % 60);
    }

    bool editing = s.state.editing;
    if (Toggle("Edit layout", &editing, o)) {
        s.state.editing = editing;
        if (!editing)
            SurfaceClearSelection(s.state);
    }
    Record(host, "surface-edit");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", editing ? "Click to select, drag to move, handles resize, Delete removes, Ctrl+D duplicates, drag a kind in to add."
                                      : "Operate mode: every control works as it would on a console.");
    if (editing)
        SurfacePalette("palette", o);

    SurfaceOptions so;
    so.columns = 24;
    so.rows = 12;
    const SurfaceRequest r = ControlSurface("console", s.items, s.state, so, o);
    Record(host, "surface");

    if (r.layout.started || r.layout.changed || r.layout.finished || r.layout.cancelled) {
        for (int i = 0; i < r.layout.count; ++i)
            for (SurfaceItem& it : s.items)
                if (it.id == r.layout.items[static_cast<std::size_t>(i)].id)
                    it.rect = r.layout.items[static_cast<std::size_t>(i)].rect;
        if (r.layout.finished) {
            ++s.edits;
            s.last = r.layout.resize ? "resized" : "moved";
        }
        if (r.layout.cancelled)
            s.last = "cancelled";
    }
    if (r.removeSelected) {
        for (int k = 0; k < s.state.selectedCount; ++k) {
            const StableId id = s.state.selected[static_cast<std::size_t>(k)];
            s.items.erase(std::remove_if(s.items.begin(), s.items.end(), [&](const SurfaceItem& it) { return it.id == id; }), s.items.end());
        }
        SurfaceClearSelection(s.state);
        ++s.edits;
        s.last = "removed";
    }
    if (r.duplicateSelected) {
        const std::size_t before = s.items.size();
        for (int k = 0; k < s.state.selectedCount; ++k)
            for (std::size_t i = 0; i < before; ++i)
                if (s.items[i].id == s.state.selected[static_cast<std::size_t>(k)]) {
                    SurfaceItem copy = s.items[i];
                    copy.id = s.nextId++;
                    copy.rect.x = std::min(copy.rect.x + 1.f, static_cast<float>(so.columns) - copy.rect.w);
                    copy.rect.y = std::min(copy.rect.y + 1.f, static_cast<float>(so.rows) - copy.rect.h);
                    s.items.push_back(copy);
                }
        ++s.edits;
        s.last = "duplicated";
    }
    if (r.add) {
        SurfaceItem it = Make(s.nextId++, r.addKind, {r.addAt.x, r.addAt.y, 2.f, 3.f}, SurfaceKindName(r.addKind));
        auto nextValue = [&]() {
            float* p = &s.values[static_cast<std::size_t>(s.spareValue)];
            s.spareValue = 12 + (s.spareValue - 12 + 1) % 28;
            return p;
        };
        auto nextKey = [&]() {
            bool* p = &s.keys[static_cast<std::size_t>(s.spareKey)];
            s.spareKey = 4 + (s.spareKey - 4 + 1) % 3;
            return p;
        };
        switch (r.addKind) {
        case SurfaceKind::Button: it.rect = {r.addAt.x, r.addAt.y, 2.4f, 2.4f}; it.on = nextKey(); it.mode = ConsoleMode::Toggle; break;
        case SurfaceKind::Fader: it.value = nextValue(); it.rect.h = 6.f; it.rect.w = 2.f; break;
        case SurfaceKind::Knob: it.value = nextValue(); it.rect = {r.addAt.x, r.addAt.y, 2.8f, 4.f}; break;
        case SurfaceKind::XYPad: it.value = nextValue(); it.value2 = nextValue(); it.rect = {r.addAt.x, r.addAt.y, 5.f, 4.5f}; break;
        case SurfaceKind::Meter: it.value = nextValue(); it.minimum = -60; it.maximum = 6; it.rect = {r.addAt.x, r.addAt.y, 1.f, 5.f}; break;
        case SurfaceKind::Display: it.text = "00:00"; it.rect = {r.addAt.x, r.addAt.y, 5.f, 2.5f}; break;
        case SurfaceKind::Image: it.image.imagePixels = {96, 96}; it.image.label = "Image"; it.rect = {r.addAt.x, r.addAt.y, 2.6f, 3.f}; break;
        case SurfaceKind::Panel: it.rect = {r.addAt.x, r.addAt.y, 6.f, 4.f}; it.label = "Panel"; break;
        default: break;
        }
        it.rect = SnapRect(it.rect, .5f, so.columns, so.rows, 1.f);
        s.items.insert(s.items.begin() + (r.addKind == SurfaceKind::Panel ? 0 : static_cast<std::ptrdiff_t>(s.items.size())), it);
        SurfaceSelect(s.state, it.id, false);
        ++s.edits;
        s.last = "added";
    }
    if (r.operated != 0) {
        ++s.operated;
        s.last = r.value ? (r.value.finished ? "value committed" : "value") : r.pressed ? "key pressed" : r.released ? "key released" : "image";
    }
    ImGui::TextDisabled("Items: %d   selected: %d   layout edits: %d   operated: %d   last: %s", static_cast<int>(s.items.size()),
                        s.state.selectedCount, s.edits, s.operated, s.last.c_str());
}
} // namespace imkit::gallery
