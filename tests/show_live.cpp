#include <imkit/imkit.h>
#include <imkit/show.h>
#include <array>
#include <cmath>
#include <cstdio>
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

    // Pure functions.
    Check(Near(ModulatorValue(ModShape::Sine, .25f), 1) && Near(ModulatorValue(ModShape::Sine, .75f), -1), "sine peaks");
    Check(Near(ModulatorValue(ModShape::Triangle, .25f), 1) && Near(ModulatorValue(ModShape::Triangle, .75f), -1) &&
              Near(ModulatorValue(ModShape::Triangle, 0), 0),
          "triangle shape");
    Check(Near(ModulatorValue(ModShape::Saw, 0), -1) && ModulatorValue(ModShape::Saw, .99f) > .9f, "saw rises");
    Check(ModulatorValue(ModShape::Square, .1f) == 1 && ModulatorValue(ModShape::Square, .9f) == -1, "square halves");
    Check(ModulatorValue(ModShape::Sine, 1.25f) == ModulatorValue(ModShape::Sine, .25f) && ModulatorValue(ModShape::Sine, std::nanf("")) == 0,
          "phase wraps and ignores NaN");
    {
        bool inRange = true;
        for (int i = 0; i < 64; ++i) {
            const float v = ModulatorValue(ModShape::Random, static_cast<float>(i) / 64.f, 7);
            inRange = inRange && v >= -1.f && v <= 1.f;
        }
        Check(inRange && ModulatorValue(ModShape::Random, .3f, 7) == ModulatorValue(ModShape::Random, .3f, 7), "random is bounded and repeatable");
    }
    Check(Near(CrossfadeLaw(0, CrossfadeCurve::Linear).a, 1) && Near(CrossfadeLaw(1, CrossfadeCurve::Linear).b, 1), "linear crossfade ends");
    Check(Near(CrossfadeLaw(.5f, CrossfadeCurve::EqualPower).a, std::sqrt(.5f)) && Near(CrossfadeLaw(.5f, CrossfadeCurve::EqualPower).b, std::sqrt(.5f)),
          "equal power keeps the sum of squares");
    Check(CrossfadeLaw(.3f, CrossfadeCurve::Cut).a == 1 && CrossfadeLaw(.7f, CrossfadeCurve::Cut).b == 1, "cut crossfade");

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1800, 3600};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 512> nodes{};
    std::array<accessibility::ActionRequest, 8> storage{};
    accessibility::ActionQueue actions{storage};
    accessibility::AccessibilityFrame semantics{nodes, &actions};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    // Cue list with a collapsed group; 100k extra rows are used only for the virtualisation check.
    std::vector<CueView> cues(5);
    cues[0] = {1, "10", "Intro", "Video", CueState::Done};
    cues[1] = {2, "20", "Walk-in", "Video", CueState::Running, CueContinue::AutoContinue, 0, 42, 12};
    cues[2] = {3, "30", "Lights", "Group", CueState::Standby, CueContinue::None, 2, 0, -1, 0, true, false};
    cues[3] = {4, "30.1", "Hidden child", "Light", CueState::Idle, CueContinue::None, 0, 0, -1, 1};
    cues[4] = {5, "50", "Outro", "Video", CueState::Broken, CueContinue::None, 0, 10, -1, 0, false, true, "Missing file"};
    CueListState cueState;
    cueState.standby = 3;
    std::vector<CueView> many(100000);
    for (std::size_t i = 0; i < many.size(); ++i) {
        many[i].id = i + 1;
        many[i].number = "1";
        many[i].name = "Cue";
    }
    bool useMany = false;
    CueRequest cueRequest;
    CueListState manyState;

    GoBarView goView;
    goView.next = "Lights";
    bool armed = false;
    GoBarRequest goRequest;

    std::array<LayerView, 2> layers{};
    float opacity0 = 1, opacity1 = .5f;
    layers[0].name = "Layer 1";
    layers[0].bus = 1;
    layers[0].opacity = &opacity0;
    layers[1].name = "Layer 2";
    layers[1].bus = 2;
    layers[1].opacity = &opacity1;
    const char* columnNames[3] = {"Scene A", "Scene B", "Scene C"};
    std::array<PadView, 6> cells{};
    cells[0].label = "Intro";
    cells[0].state = PadState::Playing;
    cells[0].progress = .3f;
    cells[1].label = "Loop";
    cells[1].state = PadState::Loaded;
    cells[2].state = PadState::Empty;
    cells[3].label = "Title";
    cells[4].label = "Logo";
    cells[4].state = PadState::Queued;
    cells[5].label = "Bad";
    cells[5].state = PadState::Error;
    ClipLauncherRequest launcherRequest;

    float gainA = -6, gainB = 0;
    bool muteA = false;
    int busA = 0;
    std::array<MixerStrip, 2> strips{};
    const float lv[2] = {-12.f, -9.f};
    strips[0].name = "Deck A";
    strips[0].gainDb = &gainA;
    strips[0].mute = &muteA;
    strips[0].bus = &busA;
    strips[0].levelsDb = lv;
    strips[0].peaksDb = lv;
    strips[1].name = "Deck B";
    strips[1].gainDb = &gainB;
    MixerState mixerState;
    MixerRequest mixerRequest;

    ModShape shape = ModShape::Sine;
    float rate = 1, phase = 0, amount = 1, offset = 0;
    bool sync = false;
    ModulatorRequest modRequest;

    bool bypass = false;
    float mix = 1, param = .3f;
    EffectParam params[1];
    params[0].name = "Amount";
    params[0].value = &param;
    EffectSlot slots[1];
    slots[0].name = "Blur";
    slots[0].bypass = &bypass;
    slots[0].mix = &mix;
    slots[0].params = params;
    EffectRackState rackState;
    EffectRackRequest rackRequest;

    std::array<MappingRow, 2> mappings{};
    mappings[0] = {10, BindingKind::Midi, "CC 21", "Layer 1 / Opacity", "0-100%", true, false};
    mappings[1] = {11, BindingKind::Osc, "/l1/opacity", "Layer 1 / Opacity", "", true, true};
    MappingState mappingState;
    MappingRequest mappingRequest;
    std::array<MessageView, 3> messages{};
    messages[0] = {0.0, BindingKind::Midi, "CC 21 ch1 = 64"};
    messages[1] = {0.5, BindingKind::Osc, "/layer1/opacity 0.5"};
    messages[2] = {1.0, BindingKind::Key, "Space"};
    MessageMonitorState monitorState;
    std::array<DeviceView, 2> devices{};
    const float history[4] = {59, 60, 60, 58};
    devices[0].name = "Output 1";
    devices[0].status = "Connected";
    devices[0].fps = 60.f;
    devices[0].gpuPercent = 42.f;
    devices[0].temperatureC = 63.f;
    devices[0].droppedFrames = 0;
    devices[0].fpsHistory = history;
    devices[1].name = "Output 2";
    devices[1].status = "Lost signal";
    devices[1].led = LedState::Error;
    int deviceClicked = -1;

    auto frame = [&](bool reduced = false) {
        theme.motion.reducedMotion = reduced;
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        CueListOptions co;
        co.size = {600, 380};
        if (useMany)
            cueRequest = CueList("many", many, manyState, co, options);
        else
            cueRequest = CueList("cues", cues, cueState, co, options);
        goRequest = GoBar("go", goView, &armed, options);
        ClipLauncherView launcher;
        launcher.layers = layers;
        launcher.columns = columnNames;
        launcher.cells = cells;
        launcherRequest = ClipLauncher("launcher", launcher, {}, options);
        mixerRequest = Mixer("mixer", strips, mixerState, {}, options);
        ModulatorView mv;
        mv.shape = &shape;
        mv.rate = &rate;
        mv.phase = &phase;
        mv.amount = &amount;
        mv.offset = &offset;
        mv.beatSync = &sync;
        mv.playPhase = .3f;
        modRequest = ModulatorEditor("mod", mv, options);
        rackRequest = EffectRack("rack", slots, rackState, options);
        mappingRequest = MappingTable("map", mappings, mappingState, options);
        MessageMonitor("mon", messages, monitorState, {600, 120}, options);
        deviceClicked = DeviceHealth("dev", devices, options);
        ImGui::End();
        ImGui::Render();
    };
    auto find = [&](std::string_view name) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name)
                return &n;
        return nullptr;
    };
    auto centre = [](const accessibility::SemanticNode& n) {
        return ImVec2{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f};
    };
    auto click = [&](const accessibility::SemanticNode& n, float dx = 0) {
        const ImVec2 c = centre(n);
        io.AddMousePosEvent(c.x + dx, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
    };
    auto release = [&] {
        io.AddMouseButtonEvent(0, false);
        for (int i = 0; i < 32; ++i)
            frame();
    };

    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid");
    Check(find("10 Intro") && find("20 Walk-in") && find("30 Lights") && !find("30.1 Hidden child"), "collapsed group hides its children");

    // Cue list.
    if (const auto* row = find("20 Walk-in")) {
        click(*row, 40);
        Check(cueRequest.select == 2, "clicking a row selects it");
        release();
    } else
        Check(false, "cue row");
    useMany = true;
    for (int i = 0; i < 3; ++i)
        frame();
    int rowNodes = 0;
    for (const auto& n : semantics.Tree().nodes)
        rowNodes += n.role == accessibility::SemanticRole::Row ? 1 : 0;
    Check(rowNodes > 0 && rowNodes < 60, "a hundred thousand cues only create the visible rows");
    useMany = false;
    for (int i = 0; i < 3; ++i)
        frame();

    // GO bar: disabled until armed.
    if (const auto* go = find("GO")) {
        click(*go);
        Check(!goRequest.go, "GO does nothing while disarmed");
        release();
        armed = true;
        frame();
        click(*go);
        Check(goRequest.go, "GO fires once armed");
        release();
    } else
        Check(false, "go node");

    // Clip launcher.
    if (const auto* cell = find("Loop, Layer 1, Scene B")) {
        click(*cell);
        Check(launcherRequest.pressed && launcherRequest.layer == 0 && launcherRequest.column == 1, "cell press reports layer and column");
        release();
    } else
        Check(false, "launcher cell");
    if (const auto* col = find("Launch column Scene C")) {
        click(*col);
        Check(launcherRequest.triggerColumn == 2, "column header launches the column");
        release();
    } else
        Check(false, "column header");
    if (const auto* mute = find("Mute Layer 2")) {
        click(*mute);
        Check(launcherRequest.toggle == LayerToggle::Mute && launcherRequest.toggledLayer == 1, "layer mute toggles");
        release();
    } else
        Check(false, "mute toggle");

    // Mixer.
    if (const auto* mute = find("Mute Deck A")) {
        click(*mute);
        Check(muteA && mixerRequest.toggled && mixerRequest.strip == 0, "mixer mute toggles");
        release();
    } else
        Check(false, "mixer mute");
    if (const auto* fader = find("Deck B")) {
        const float before = gainB;
        const ImVec2 c = centre(*fader);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMousePosEvent(c.x, c.y + 30);
        frame();
        Check(gainB != before && mixerRequest.strip == 1 && mixerRequest.edit.changed, "mixer fader reports its strip");
        release();
    } else
        Check(false, "mixer fader");

    // Modulator shape buttons.
    if (const auto* square = find("Square")) {
        click(*square);
        Check(shape == ModShape::Square && modRequest.shapeChanged, "choosing a shape changes the modulator");
        release();
    } else
        Check(false, "shape button");

    // Effect rack.
    if (const auto* enable = find("Enable Blur")) {
        click(*enable);
        Check(bypass, "the enable toggle bypasses the effect");
        release();
    } else
        Check(false, "enable toggle");

    // Mapping table and devices.
    if (const auto* learn = find("Learn Layer 1 / Opacity")) {
        click(*learn);
        Check(mappingRequest.learn == 10, "learn asks the host to wait for a message");
        release();
    } else
        Check(false, "learn toggle");
    if (const auto* device = find("Output 2, Lost signal")) {
        click(*device);
        Check(deviceClicked == 1, "device row reports a click");
        release();
    } else
        Check(false, "device row");

    frame(true);
    Check(semantics.Tree().Validate(), "semantic tree stays valid with reduced motion");
    ImGui::DestroyContext();
    std::printf("Show live failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
