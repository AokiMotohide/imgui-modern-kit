#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::gallery {
using namespace show;

void ShowPages::Led(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    auto& s = led;
    const float u = ImGui::GetFrameHeight();
    const float time = static_cast<float>(ImGui::GetTime());

    if (!s.initialized) {
        s.initialized = true;
        s.stops[0] = {0.f, {1.f, .25f, .1f, 1.f}};
        s.stops[1] = {.5f, {1.f, .85f, .2f, 1.f}};
        s.stops[2] = {1.f, {.2f, .4f, 1.f, 1.f}};
        s.blockNames = {"Strip A  50 px", "Strip B  50 px", "PAR 1  RGBW", "Bar 2  RGB x4", "Wall 1  160 px", "Spot  16 ch"};
        auto block = [&](StableId id, int universe, int start, int count, std::size_t name) {
            AddressBlock b;
            b.id = id;
            b.universe = universe;
            b.start = start;
            b.count = count;
            b.name = s.blockNames[name].c_str();
            s.blocks.push_back(b);
        };
        block(1, 0, 1, 150, 0);
        block(2, 0, 151, 150, 1);
        block(3, 0, 301, 8, 2);
        block(4, 0, 305, 12, 3); // Overlaps PAR 1 on purpose.
        block(5, 1, 1, 480, 4);
        block(6, 2, 1, 16, 5);
        static const char* portNames[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};
        static const char* portDetails[8] = {"U1  300 ch", "U2  480 ch", "U3  16 ch", "idle", "no link", "U4  510 ch", "hot  72 C", "off"};
        const PortState states[8] = {PortState::Active, PortState::Active, PortState::Active, PortState::Idle, PortState::Error, PortState::Active, PortState::Warning, PortState::Disabled};
        const float loads[8] = {.55f, .8f, .1f, -1.f, -1.f, .96f, .74f, -1.f};
        for (std::size_t i = 0; i < s.ports.size(); ++i) {
            s.ports[i].label = portNames[i];
            s.ports[i].detail = portDetails[i];
            s.ports[i].state = states[i];
            s.ports[i].load = loads[i];
        }
        // A static snapshot of the first fixtures, so that edits made in the channel monitor stay put.
        for (std::size_t i = 0; i < 100; ++i) {
            const ImVec4 c = SampleGradient(std::span<const GradientStop>(s.stops.data(), 3), static_cast<float>(i) / 100.f);
            s.channels[i * 3] = static_cast<unsigned char>(c.x * 200.f);
            s.channels[i * 3 + 1] = static_cast<unsigned char>(c.y * 200.f);
            s.channels[i * 3 + 2] = static_cast<unsigned char>(c.z * 200.f);
        }
    }

    // Animate the pixels from the gradient, and the channels from the first fixtures.
    for (std::size_t i = 0; i < s.pixels.size(); ++i) {
        const float t = std::fmod(static_cast<float>(i) / static_cast<float>(s.pixels.size()) + time * .08f, 1.f);
        s.pixels[i] = SampleGradient(std::span<const GradientStop>(s.stops.data(), static_cast<std::size_t>(s.stopCount)), t, GradientInterpolation::Linear);
    }
    if (BeginGrid(host, "show-led-grid", 2)) {
        const bool section1 = BeginSection(host, "Colour", "RGB with white and amber, a dimmer and colour temperature. Edits report their phases.");
        if (section1) {
            LedColorOptions co;
            co.white = true;
            co.amber = true;
            co.cct = true;
            static const ImVec4 swatches[] = {{1, .1f, .1f, 1}, {1, .6f, .1f, 1}, {1, .9f, .2f, 1}, {.2f, .9f, .4f, 1}, {.2f, .6f, 1, 1}, {.7f, .3f, 1, 1}, {1, 1, 1, 1}};
            co.swatches = swatches;
            const LedColorRequest r = LedColorPicker("light", &s.color, &s.kelvin, co, o);
            Record(host, "led-color");
            if (r.edit)
                s.last = std::string("colour edit: ") + (r.edit.finished ? "finished" : r.edit.started ? "started" : "changed");
            ImGui::Spacing();
            ImGui::TextDisabled("Colour order sent to the strip");
            const ColorOrderRequest co2 = ColorOrderPicker("order", s.order, s.orderWhite, o);
            Record(host, "led-order");
            if (co2.changed)
                s.last = std::string("order: ") + s.order;
            if (co2.test)
                s.last = "order test requested";
        }
        EndSection(section1);

        const bool section2 = BeginSection(host, "Test patterns", "Pressing the active pattern turns it off. The host draws the pattern on the output.");
        if (section2) {
            const TestPatternRequest r = TestPatternBar("patterns", s.pattern, {}, o);
            Record(host, "led-patterns");
            if (r.changed) {
                s.pattern = r.pattern;
                s.last = std::string("pattern: ") + TestPatternName(r.pattern);
            }
            ImGui::TextDisabled("Active: %s", TestPatternName(s.pattern));
        }
        EndSection(section2);

        const bool section3 = BeginSection(host, "Gradient and pixels", "Double-click the bar to add a stop; drag a marker to move it. The strip and the grid use the gradient.");
        if (section3) {
            GradientEditorView gv;
            gv.stops = s.stops.data();
            gv.count = &s.stopCount;
            gv.capacity = static_cast<int>(s.stops.size());
            GradientEditorOptions go;
            static const ImVec4 gradientSwatches[] = {{1, .25f, .1f, 1}, {1, .85f, .2f, 1}, {.2f, .4f, 1, 1}, {.3f, 1, .6f, 1}};
            go.swatches = gradientSwatches;
            const GradientRequest r = GradientEditor("gradient", gv, &s.selectedStop, go, o);
            Record(host, "led-gradient");
            if (r.added)
                s.last = "stop added";
            if (r.removed)
                s.last = "stop removed";
            ImGui::Spacing();
            PixelPreviewView strip;
            strip.colors = std::span<const ImVec4>(s.pixels.data(), 64);
            PixelPreviewOptions po;
            po.size = {0, u * 2.2f};
            PixelPreview("strip", strip, &s.selectedPixel, po, o);
            Record(host, "led-strip");
            ImGui::Spacing();
            PixelPreviewView grid;
            grid.colors = std::span<const ImVec4>(s.pixels.data(), 192);
            grid.columns = 16;
            grid.serpentine = true;
            PixelPreviewOptions pg;
            pg.size = {0, u * 9.f};
            pg.showWiring = true;
            const PixelPreviewRequest pr = PixelPreview("grid", grid, &s.selectedPixel, pg, o);
            Record(host, "led-grid");
            if (pr.clicked >= 0)
                s.last = "pixel " + std::to_string(pr.clicked) + " selected";
        }
        EndSection(section3);

        const bool section4 = BeginSection(host, "Addressing", "Blocks per universe. PAR 1 and Bar 2 overlap on purpose: hatching, a mark and a count show it. Drag a block to move it.");
        if (section4) {
            AddressMapView av;
            av.blocks = s.blocks;
            av.universes = 3;
            const AddressMapRequest r = AddressMap("addresses", av, s.address, {}, o);
            Record(host, "led-address");
            if (r.moved != 0 && (r.changed || r.finished)) {
                for (AddressBlock& b : s.blocks)
                    if (b.id == r.moved) {
                        b.universe = r.universe;
                        b.start = r.start;
                    }
                if (r.finished)
                    s.last = "block moved";
            }
            ImGui::Spacing();
            ImGui::TextDisabled("RGB pixels per universe: %d   RGBW: %d", PixelsPerUniverse(3), PixelsPerUniverse(4));
            ChannelMonitorView cv;
            cv.values = std::span<const unsigned char>(s.channels.data(), 128);
            cv.blocks = std::span<const AddressBlock>(s.blocks.data(), 4);
            ChannelMonitorOptions mo;
            mo.columns = 32;
            const ChannelMonitorRequest cr = ChannelMonitor("channels", cv, s.channelState, mo, o);
            Record(host, "led-channels");
            if (cr.index >= 0 && (cr.changed || cr.finished))
                s.channels[static_cast<std::size_t>(cr.index)] = static_cast<unsigned char>(cr.value);
            if (cr.allZero)
                s.channels.fill(0);
            if (cr.allFull)
                s.channels.fill(255);
        }
        EndSection(section4);

        const bool section5 = BeginSection(host, "Power and ports", "Current against the supply limit, and the state of every output port.");
        if (section5) {
            ImGui::SetNextItemWidth(u * 10.f);
            ImGui::SliderFloat("Brightness", &s.brightness, 0.f, 1.f, "%.2f");
            const float a = EstimateLedCurrent(150, .06f, s.brightness, .8f);
            const float b = EstimateLedCurrent(150, .06f, s.brightness, .8f);
            const float c = EstimateLedCurrent(480, .06f, s.brightness, .8f);
            const BudgetSegment segments[3] = {{"Strip A", a, {}}, {"Strip B", b, {}}, {"Wall 1", c, {}}};
            BudgetBarView bv;
            bv.segments = segments;
            bv.limit = 20.f;
            bv.title = "5 V supply";
            const BudgetStatus status = BudgetBar("budget", bv, {}, o);
            Record(host, "led-budget");
            ImGui::TextDisabled("Status: %s", status == BudgetStatus::Over ? "over limit" : status == BudgetStatus::Warning ? "near limit" : "within limit");
            ImGui::Spacing();
            const int clicked = PortStrip("ports", s.ports, &s.selectedPort, {}, o);
            Record(host, "led-ports");
            if (clicked >= 0)
                s.last = "port " + std::to_string(clicked + 1) + " selected";
        }
        EndSection(section5);
        EndGrid();
    }
    ImGui::TextDisabled("Last event: %s", s.last.c_str());
}
} // namespace imkit::gallery
