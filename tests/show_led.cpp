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

    // ---- Colour maths ------------------------------------------------------------------------------------------
    {
        const ImVec4 daylight = KelvinToRgb(6500), warm = KelvinToRgb(2000), cool = KelvinToRgb(10000);
        Check(daylight.x > .9f && daylight.y > .9f && daylight.z > .9f, "6500 K is close to white");
        Check(warm.x >= warm.y && warm.y > warm.z && warm.x > .99f, "2000 K is orange: red above green above blue");
        Check(cool.z >= cool.x && cool.z > .99f, "10000 K is bluish");
        Check(Near(KelvinToRgb(std::numeric_limits<float>::quiet_NaN()).x, KelvinToRgb(6500).x), "non-finite kelvin falls back to 6500 K");
        Check(Near(KelvinToRgb(100).x, KelvinToRgb(1667).x) && Near(KelvinToRgb(90000).z, KelvinToRgb(25000).z), "kelvin is clamped");
        const LightColor w = RgbToRgbw(1.f, .5f, .25f);
        Check(Near(w.w, .25) && Near(w.r, .75) && Near(w.g, .25) && Near(w.b, 0), "RGB to RGBW moves the shared part to white");
        LightColor l;
        l.r = 1.f;
        l.g = .5f;
        l.intensity = .5f;
        const ImVec4 mixed = MixLightColor(l);
        Check(Near(mixed.x, .5) && Near(mixed.y, .25), "dimmer scales the mixed colour");
        l.intensity = 1.f;
        l.w = 1.f;
        Check(MixLightColor(l).z > .85f, "white adds to every channel");
    }

    // ---- Gradient ----------------------------------------------------------------------------------------------
    {
        const GradientStop stops[] = {{0.f, {0, 0, 0, 1}}, {1.f, {1, 1, 1, 1}}};
        Check(Near(SampleGradient(stops, 0.f).x, 0) && Near(SampleGradient(stops, 1.f).x, 1) && Near(SampleGradient(stops, .5f).x, .5), "linear gradient");
        Check(Near(SampleGradient(stops, .25f, GradientInterpolation::Smooth).x, .15625, 1e-3), "smooth gradient eases");
        Check(Near(SampleGradient(stops, .9f, GradientInterpolation::Step).x, 0), "step gradient holds the previous colour");
        const GradientStop unsorted[] = {{1.f, {1, 0, 0, 1}}, {0.f, {0, 0, 1, 1}}};
        Check(Near(SampleGradient(unsorted, .5f).x, .5) && Near(SampleGradient(unsorted, .5f).z, .5), "stops need no sorting");
        const GradientStop single[] = {{.5f, {.2f, .4f, .6f, 1}}};
        Check(Near(SampleGradient(single, 0.f).y, .4) && Near(SampleGradient(single, 1.f).y, .4), "a single stop fills the bar");
        Check(SampleGradient({}, .5f).w == 0, "an empty gradient is transparent");
    }

    // ---- Addressing and power ----------------------------------------------------------------------------------
    {
        AddressBlock a, b, c, d;
        a.id = 1; a.start = 1; a.count = 10;
        b.id = 2; b.start = 8; b.count = 10;
        c.id = 3; c.start = 8; c.count = 10; c.universe = 1;
        d.id = 4; d.start = 20; d.count = 5;
        const AddressBlock blocks[] = {a, b, c, d};
        AddressOverlap out[4];
        Check(FindOverlaps(blocks, out) == 1 && out[0].a == 0 && out[0].b == 1 && out[0].first == 8 && out[0].last == 10, "overlap is found with its shared slots");
        Check(FindOverlaps(std::span<const AddressBlock>(blocks, 1), out) == 0, "a single block cannot overlap");
        AddressBlock many[3] = {a, a, a};
        many[1].id = 5;
        many[2].id = 6;
        AddressOverlap small[1];
        Check(FindOverlaps(many, small) == 3, "overlaps are counted past the capacity of the output");
        Check(FirstFreeRange(blocks, 0, 512, 5) == 25 && FirstFreeRange(blocks, 0, 512, 2) == 18, "free runs are found after and between blocks");
        Check(FirstFreeRange(blocks, 1, 512, 5) == 1 && FirstFreeRange(blocks, 1, 512, 10) == 18, "other universes are separate");
        Check(FirstFreeRange(blocks, 0, 512, 600) == 0 && FirstFreeRange(blocks, 0, 512, 0) == 0, "impossible requests find nothing");
        Check(PixelsPerUniverse(3) == 170 && PixelsPerUniverse(4) == 128 && PixelsPerUniverse(0) == 0, "pixels per universe");
        Check(Near(EstimateLedCurrent(60, .06f, 1.f, 1.f), 3.6) && EstimateLedCurrent(60, .06f, 0.f, 1.f) == 0.f && EstimateLedCurrent(-5, .06f, 1.f) == 0.f,
              "LED current estimate");
        Check(Near(EstimateLedCurrent(100, .06f, 2.f, 1.f), 6.0), "brightness is clamped to 1");
    }

    // ---- Colour order ------------------------------------------------------------------------------------------
    {
        std::array<std::array<char, 5>, 24> list{};
        Check(ColorOrderChoices(false, list) == 6 && std::string_view(list[0].data()) == "RGB", "six RGB orders, RGB first");
        bool hasGrb = false;
        for (std::size_t i = 0; i < 6; ++i)
            hasGrb |= std::string_view(list[i].data()) == "GRB";
        Check(hasGrb, "GRB is offered");
        Check(ColorOrderChoices(true, list) == 24 && std::string_view(list[23].data()).size() == 4, "twenty-four RGBW orders");
        std::array<std::array<char, 5>, 2> tiny{};
        Check(ColorOrderChoices(false, tiny) == 2, "output capacity is respected");
        Check(std::string_view(TestPatternName(TestPattern::Grid)) == "Grid" && std::string_view(TestPatternName(TestPattern::Off)) == "Off", "pattern names");
    }

    // ---- Components ---------------------------------------------------------------------------------------------
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // Like every real renderer: lets a draw list exceed 64k vertices.
    io.DisplaySize = {1400, 6000};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int fw, fh;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &fw, &fh);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 2048> nodes{};
    std::array<accessibility::ActionRequest, 8> storage{};
    accessibility::ActionQueue actions{storage};
    accessibility::AccessibilityFrame semantics{nodes, &actions};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    LightColor color{.2f, .4f, .6f, 0, 0, 0, 1.f};
    float kelvin = 3200.f;
    std::array<GradientStop, 6> stops{};
    stops[0] = {0.f, {1, 0, 0, 1}};
    stops[1] = {1.f, {0, 0, 1, 1}};
    int stopCount = 2, selectedStop = 0;
    std::vector<ImVec4> pixelsA(600, ImVec4{.5f, .2f, .9f, 1.f});
    int pixelColumns = 30;
    std::array<unsigned char, 512> channels{};
    ChannelMonitorState channelState;
    AddressMapState addressState;
    std::vector<AddressBlock> blocks(2);
    blocks[0].id = 1; blocks[0].start = 1; blocks[0].count = 30; blocks[0].name = "Strip";
    blocks[1].id = 2; blocks[1].start = 20; blocks[1].count = 30; blocks[1].name = "Bar";
    char order[5] = "GRB";
    TestPattern pattern = TestPattern::Off;
    std::array<PortView, 3> ports{};
    ports[0].label = "Out 1";
    ports[1].label = "Out 2";
    ports[1].state = PortState::Error;
    int selectedPort = -1, selectedPixel = -1;
    LedColorRequest colorRequest;
    GradientRequest gradientRequest;
    TestPatternRequest patternRequest;
    ColorOrderRequest orderRequest;
    BudgetStatus budgetStatus = BudgetStatus::Ok;
    PixelPreviewRequest pixelRequest;
    ChannelMonitorRequest channelRequest;
    AddressMapRequest addressRequest;
    int portClick = -1;
    float budgetAmount = 6.f;

    auto frame = [&](bool reduced = false) {
        theme.motion.reducedMotion = reduced;
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        LedColorOptions co;
        co.white = true;
        co.cct = true;
        static const ImVec4 sw[] = {{1, 0, 0, 1}, {0, 1, 0, 1}};
        co.swatches = sw;
        colorRequest = LedColorPicker("light", &color, &kelvin, co, options);
        GradientEditorView gv;
        gv.stops = stops.data();
        gv.count = &stopCount;
        gv.capacity = static_cast<int>(stops.size());
        gradientRequest = GradientEditor("gradient", gv, &selectedStop, {}, options);
        PixelPreviewView pv;
        pv.colors = pixelsA;
        pv.columns = pixelColumns;
        pv.serpentine = true;
        pixelRequest = PixelPreview("pixels", pv, &selectedPixel, {}, options);
        ChannelMonitorView cv;
        cv.values = channels;
        cv.blocks = blocks;
        channelRequest = ChannelMonitor("channels", cv, channelState, {}, options);
        AddressMapView av;
        av.blocks = blocks;
        av.universes = 2;
        addressRequest = AddressMap("addresses", av, addressState, {}, options);
        const BudgetSegment seg[2] = {{"A", budgetAmount, {}}, {"B", 4.f, {}}};
        BudgetBarView bv;
        bv.segments = seg;
        bv.limit = 12.f;
        bv.title = "Supply";
        budgetStatus = BudgetBar("budget", bv, {}, options);
        patternRequest = TestPatternBar("patterns", pattern, {}, options);
        orderRequest = ColorOrderPicker("order", order, false, options);
        portClick = PortStrip("ports", ports, &selectedPort, {}, options);
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
    auto click = [&](const accessibility::SemanticNode& n) {
        const ImVec2 c = centre(n);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
    };

    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid with every LED component on screen");
    Check(budgetStatus == BudgetStatus::Warning, "10 of 12 A is near the limit");
    budgetAmount = 9.f;
    frame();
    Check(budgetStatus == BudgetStatus::Over, "13 of 12 A is over the limit");
    budgetAmount = 7.5f;
    frame();
    Check(budgetStatus == BudgetStatus::Warning, "11.5 of 12 A is near the limit");
    Check(find("Light colour") && find("Pixel preview") && find("Supply"), "preview, pixel preview and budget are announced");

    // Test pattern chip toggles.
    if (const auto* grid = find("Grid")) {
        click(*grid);
        Check(patternRequest.changed && patternRequest.pattern == TestPattern::Grid, "pressing a pattern selects it");
        release();
        pattern = TestPattern::Grid;
        frame();
        click(*find("Grid"));
        Check(patternRequest.changed && patternRequest.pattern == TestPattern::Off, "pressing the active pattern turns it off");
        release();
        pattern = TestPattern::Off;
    } else
        Check(false, "grid pattern node");

    // Colour order chip.
    if (const auto* rgb = find("RGB")) {
        click(*rgb);
        release();
        Check(std::string_view(order) == "RGB" && orderRequest.changed == false, "chip sets the order (reported on the click frame)");
    } else
        Check(false, "RGB order node");

    // Swatch sets the colour with a full edit.
    if (const auto* swatch = find("Swatch #FF0000")) {
        click(*swatch);
        Check(colorRequest.field == LedColorField::Swatch && colorRequest.edit.started && colorRequest.edit.finished, "swatch is an instant edit");
        release();
        Check(Near(color.r, 1.0) && Near(color.g, 0.0), "swatch colour is applied");
    } else
        Check(false, "swatch node");

    // Gradient: double-click the bar adds a stop.
    if (const auto* stopNode = find("Gradient stop 1")) {
        Check(stopNode->role == accessibility::SemanticRole::Slider && Near(stopNode->numericValue, 0), "gradient stop is a slider with its position");
    } else
        Check(false, "gradient stop node");

    // Channel monitor: dragging in a cell sets its value from the pointer height.
    if (const auto* cell = find("Channel 3")) {
        const ImVec2 c = centre(*cell);
        io.AddMousePosEvent(c.x, cell->minimum.y + 1.f);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(channelRequest.index == 2 && channelRequest.value >= 250 && channelRequest.started, "dragging near the top of a cell requests about 255");
        release();
    } else
        Check(false, "channel cell node");

    // Address map: blocks are announced and an overlap is reported through the count text path (no crash with 10,000 pixels either).
    Check(find("Strip, universe 1, slots 1 to 30") && find("Bar, universe 1, slots 20 to 49"), "address blocks are named");
    Check(pixelRequest.hovered == -1, "no pixel is hovered without the pointer");
    // Ten thousand pixels still draw (as plain squares) and keep the tree valid.
    pixelsA.assign(10000, ImVec4{.2f, .8f, .4f, 1.f});
    pixelColumns = 100;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate() && find("Pixel preview") != nullptr, "ten thousand pixels are drawn without trouble");

    // Port strip.
    if (const auto* port = find("Out 2, error")) {
        click(*port);
        Check(portClick == 1 && selectedPort == 1, "clicking a port selects it");
        release();
    } else
        Check(false, "port node");
    ImGui::DestroyContext();
    std::puts(failures ? "show LED tests failed" : "show LED tests passed");
    return failures ? 1 : 0;
}
