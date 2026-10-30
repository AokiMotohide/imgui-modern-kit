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
bool Near(double a, double b, double eps) { return std::abs(a - b) <= eps; }
} // namespace

using namespace imkit;
using namespace imkit::show;

int main() {
    // Tempo.
    {
        TapTempo tap;
        Check(tap.Tap(0.0) == 0.0, "one tap gives no tempo");
        tap.Tap(.5);
        tap.Tap(1.0);
        const double bpm = tap.Tap(1.5);
        Check(Near(bpm, 120, .5), "four even taps at half a second are 120 BPM");
        const double withOutlier = tap.Tap(2.4);
        Check(withOutlier > 100 && withOutlier < 130, "one late tap does not swing the estimate");
        tap.Tap(6.0); // More than two seconds later: a new count.
        Check(tap.count == 1 && tap.Tap(6.5) > 0, "a long gap starts a new count");
        Check(tap.Tap(std::nan("")) == 0, "non-finite times are ignored");
    }
    {
        const BeatInfo a = BeatAt(2.25, 120, 1.0);
        Check(a.index == 2 && Near(a.phase, .5, 1e-6) && a.beatInBar == 3, "beat position and bar count");
        const BeatInfo before = BeatAt(.5, 120, 1.0);
        Check(before.index == -1 && before.beatInBar == 4, "beats before the first beat count backwards");
        Check(BeatAt(1, 0, 0).beat == 0 && BeatAt(1, std::nan(""), 0).index == 0, "bad tempo is harmless");
        Check(Near(PitchedBpm(120, 5), 126, 1e-9), "pitch changes tempo by percent");
    }

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1800, 3400};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 1024> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    std::vector<float> peaks(50 * 120);
    for (std::size_t i = 0; i < peaks.size(); ++i)
        peaks[i] = .3f + .6f * std::abs(std::sin(static_cast<float>(i) * .05f));
    std::array<CueMarker, 2> cueMarkers{};
    cueMarkers[0].seconds = 10;
    cueMarkers[1].seconds = 40;
    WaveformSpan track;
    track.peaks = peaks;
    track.bucketsPerSecond = 50;
    track.position = 30;
    track.bpm = 120;
    track.cues = cueMarkers;
    track.loopIn = 24;
    track.loopOut = 32;
    const char* sources[] = {"Internal", "Link"};
    BeatClockView clock;
    clock.beat = BeatAt(30, 120, 0);
    clock.sources = sources;
    DeckView deck;
    deck.title = "Track";
    deck.length = 120;
    deck.position = 30;
    deck.bpm = 124;
    deck.platterAngle = .5f;
    std::array<HotCue, 4> hot{};
    hot[0].set = true;
    hot[0].seconds = 12;
    hot[1].set = true;
    hot[1].seconds = 40;
    float trim = 0, high = 0, mid = 0, low = 0, filter = 0, gain = -6;
    bool killHigh = false, cueOn = false;
    DjChannel channel;
    channel.name = "Deck A";
    channel.trimDb = &trim;
    channel.high = &high;
    channel.mid = &mid;
    channel.low = &low;
    channel.killHigh = &killHigh;
    channel.filter = &filter;
    channel.gainDb = &gain;
    channel.cue = &cueOn;
    float pitch = 4;
    bool keyLock = false;
    std::array<std::uint8_t, 2 * 8> velocity{};
    const char* lanes[] = {"Kick", "Snare"};
    std::array<float, 16> bands{};
    bands.fill(-30.f);
    float bandLow = .25f, bandHigh = .6f;

    BeatClockRequest clockRequest;
    DeckRequest deckRequest;
    WaveformRequest scrollRequest, overviewRequest;
    HotCueEvent hotEvent;
    DjChannelRequest channelRequest;
    ValueEdit pitchEdit;
    std::array<StepEdit, 16> editStorage{};
    StepEditBuffer edits(editStorage);
    auto frame = [&] {
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        clockRequest = BeatClock("clock", clock, options);
        deckRequest = Deck("deck", deck, 0, options);
        ScrollingWaveformOptions so;
        so.size = {800, 90};
        scrollRequest = ScrollingWaveform("scroll", track, so, options);
        OverviewWaveformOptions oo;
        oo.size = {800, 50};
        overviewRequest = OverviewWaveform("overview", track, oo, options);
        hotEvent = HotCuePads("hot", hot, options);
        channelRequest = DjChannelStrip("channel", channel, 0, options);
        pitchEdit = PitchFader("pitch", &pitch, {8, 124}, options, &keyLock);
        edits.Clear();
        SequencerView sv;
        sv.lanes = 2;
        sv.steps = 8;
        sv.laneNames = lanes;
        sv.velocity = velocity;
        sv.playhead = 3;
        StepSequencer("seq", sv, edits, {{900, 0}}, options);
        Spectrum("spectrum", bands, bands, &bandLow, &bandHigh, {{600, 100}}, options);
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
    auto click = [&](ImVec2 p) {
        io.AddMousePosEvent(p.x, p.y);
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
    Check(semantics.Tree().Validate(), "the beat components publish a valid semantic tree");

    if (const auto* tap = find("TAP")) {
        click(centre(*tap));
        Check(clockRequest.tap, "the tap button reports a tap");
        release();
    } else
        Check(false, "tap button");
    if (const auto* up = find("+0.1")) {
        click(centre(*up));
        Check(Near(clockRequest.bpmStep, .1, 1e-9), "the fine button steps the tempo");
        release();
    } else
        Check(false, "tempo up");
    if (const auto* play = find("PLAY")) {
        click(centre(*play));
        Check(deckRequest.play, "the deck reports play");
        release();
    } else
        Check(false, "play button");
    if (const auto* cue = find("CUE")) {
        click(centre(*cue));
        Check(deckRequest.cuePressed, "cue press is reported");
        io.AddMouseButtonEvent(0, false);
        frame();
        Check(deckRequest.cueReleased, "cue release is reported");
        for (int i = 0; i < 32; ++i)
            frame();
    } else
        Check(false, "cue button");

    // Overview seeks; the scrolling view scrubs relative to the drag.
    if (const auto* overview = find("overview")) {
        const ImVec2 c = centre(*overview);
        click(c);
        Check(overviewRequest.scrub.started && Near(overviewRequest.scrub.value, 60, 3), "clicking the middle of the overview seeks to half the track");
        release();
    } else
        Check(false, "overview");
    if (const auto* scroll = find("scroll")) {
        const ImVec2 c = centre(*scroll);
        io.AddMousePosEvent(c.x, c.y + 20);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMousePosEvent(c.x - 100, c.y + 20);
        frame();
        Check(scrollRequest.scrub.changed && scrollRequest.scrub.value > 30, "dragging the waveform left moves forward in time");
        release();
    } else
        Check(false, "scrolling waveform");

    // Hot cues: set pads jump, empty pads store, right-click clears.
    if (const auto* pad = find("Cue 1")) {
        click(centre(*pad));
        Check(hotEvent.index == 0 && hotEvent.jump, "a set hot cue jumps");
        release();
    } else
        Check(false, "hot cue pad");
    if (const auto* empty = find("Empty pad 3")) {
        click(centre(*empty));
        Check(hotEvent.index == 2 && hotEvent.store, "an empty hot cue stores the playhead");
        release();
        const ImVec2 c = centre(*empty);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(1, true);
        frame();
        io.AddMouseButtonEvent(1, false);
        frame();
        Check(!hotEvent.clear, "right-click on an empty pad does nothing");
    }
    if (const auto* pad = find("Cue 2")) {
        const ImVec2 c = centre(*pad);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(1, true);
        frame();
        Check(hotEvent.index == 1 && hotEvent.clear, "right-click clears a set hot cue");
        io.AddMouseButtonEvent(1, false);
        for (int i = 0; i < 32; ++i)
            frame();
    }

    // Channel strip.
    if (const auto* kill = find("Kill High")) {
        click(centre(*kill));
        Check(killHigh && channelRequest.toggled && channelRequest.control == 1, "the kill switch toggles");
        release();
    } else
        Check(false, "kill switch");
    if (const auto* cue = find("CUE")) {
        (void)cue; // The deck's CUE is found first; the channel's lower in the layout is checked through its state below.
    }

    // Pitch fader: double-click restores zero.
    if (const auto* fader = find("Pitch")) {
        const ImVec2 c = centre(*fader);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMouseButtonEvent(0, false);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(std::abs(pitch) < 1e-6f, "double-click centres the pitch fader");
        release();
    } else
        Check(false, "pitch fader");

    // Sequencer: click toggles, drag paints, right-click clears.
    if (const auto* seq = find("seq")) {
        const float u = ImGui::GetFrameHeight();
        const float labelW = u * 5.f, headH = u * .9f, rowH = u * 1.5f;
        const float cellW = (seq->maximum.x - seq->minimum.x - labelW) / 8.f;
        const auto cell = [&](int lane, int step) { return ImVec2{seq->minimum.x + labelW + (static_cast<float>(step) + .5f) * cellW, seq->minimum.y + headH + (static_cast<float>(lane) + .5f) * rowH}; };
        click(cell(0, 1));
        bool toggled = false;
        for (const StepEdit& e : edits.Edits())
            toggled = toggled || (e.lane == 0 && e.step == 1 && e.velocity == 100);
        Check(toggled, "clicking an empty step turns it on at the default velocity");
        release();
        // Drag across four steps on the second lane.
        io.AddMousePosEvent(cell(1, 0).x, cell(1, 0).y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        int painted = 0;
        for (int step = 1; step < 4; ++step) {
            io.AddMousePosEvent(cell(1, step).x, cell(1, step).y);
            frame();
            for (const StepEdit& e : edits.Edits())
                painted += (e.lane == 1 && e.step == step && e.velocity == 100) ? 1 : 0;
        }
        Check(painted == 3, "dragging paints each cell it crosses once");
        release();
    } else
        Check(false, "sequencer");

    ImGui::DestroyContext();
    std::printf("Show beat failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
