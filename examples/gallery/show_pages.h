#pragma once
#include <imkit/imkit.h>
#include <imkit/show.h>
#include <imkit/show/script.h>
#include <imkit/show/surface.h>
#include <imkit/show/led.h>
#include <imkit/show/address.h>
#include <array>
#include <bitset>
#include <string>
#include <vector>

namespace imkit::gallery {
struct GalleryState;
// Gallery pages for the optional Show module (pages 25-30). Every value here belongs to the page, exactly as it
// would belong to an application: the components only draw and report.
struct ShowPages {
    // Page 25: controls.
    std::array<float, 3> faders{-6.f, .64f, 0.f};
    float crossfade = .5f, knobs[5]{0.f, 0.f, 1.f, 210.f, .6f};
    float xy[2]{.5f, .5f};
    show::PeakHold hold;
    show::BindingInfo binding{show::BindingKind::Midi, "CC 21", "0-100%", false};
    bool bound = true, armed = false;
    std::array<show::PadView, 8> pads{};
    std::array<float, 8> padProgress{};
    int goCount = 0, panicCount = 0;
    show::ValueEdit lastEdit{};
    bool initialized = false;

    // Page 26: live.
    struct CueSim {
        std::vector<show::CueView> views;
        show::CueListState state;
        bool paused = false;
        int stopCount = 0, panicCount = 0, goCount = 0;
    } cue;
    struct LauncherSim {
        std::array<show::LayerView, 4> layers{};
        std::array<float, 4> opacity{1.f, .8f, 1.f, .6f};
        std::array<show::PadView, 24> cells{};
        std::array<float, 24> start{};
        int triggers = 0;
    } launcher;
    struct MixerSim {
        std::array<float, 4> gain{-6.f, -3.f, -12.f, 0.f};
        std::array<bool, 4> mute{}, solo{};
        std::array<int, 4> bus{1, 2, 1, 0};
        std::array<std::array<float, 2>, 4> level{}, peak{};
        std::array<show::PeakHold, 4> hold;
        show::PeakHold masterHold;
        std::array<float, 2> masterLevel{}, masterPeak{};
        show::MixerState state;
        float transition = 0, duration = 1.5f;
        bool running = false;
    } mixer;
    struct EffectSim {
        struct Fx {
            std::string name;
            bool bypass = false;
            float mix = 1;
            std::array<float, 3> values{};
        };
        std::vector<Fx> slots;
        show::EffectRackState state;
        show::ModShape shape = show::ModShape::Sine;
        float rate = 1.f, phase = 0, amount = .8f, offset = 0;
        bool sync = false;
    } effects;
    struct MappingSim {
        std::vector<show::MappingRow> rows;
        show::MappingState state;
        show::MessageMonitorState monitor;
        std::vector<std::array<char, 48>> text;
        std::vector<show::MessageView> messages;
        double nextMessage = 0;
        int counter = 0;
    } mapping;
    struct HealthSim {
        std::array<std::array<float, 32>, 3> fps{};
        double nextSample = 0;
    } health;

    // Page 27: rules.
    struct RulesSim {
        show::RuleDocument doc;
        show::RuleBuilderState state;
        bool initialized = false;
        float occupancy = 3, lux = 30, hour = 18.5f;
        bool screenOnline = true, emergency = false;
        int mode = 0, edits = 0;
    } rules;

    // Page 28: schedule.
    struct SchedSim {
        std::vector<show::CalendarEvent> events;
        std::vector<std::string> titles;
        show::CalendarState calendar;
        show::Recurrence rule;
        show::SolarTrigger solar;
        std::bitset<336> hours;
        char cron[64]{};
        std::int64_t dateTime = 0;
        std::int32_t pickedDay = 0;
        int timeOfDay = 0, edits = 0;
        show::StableId nextId = 1;
        bool initialized = false;
    } schedule;

    // Page 29: beat and DJ.
    struct BeatSim {
        show::TapTempo tap;
        double bpm = 124, clockSeconds = 0, firstBeat = 0;
        bool running = true;
        int source = 0;
        struct DeckSim {
            double position = 20, length = 210, bpm = 124;
            bool playing = true, sync = false, cueHeld = false;
            float angle = 0, pitch = 0;
            bool keyLock = false;
            std::array<show::HotCue, 8> hot{};
            std::vector<float> peaks;
            double loopIn = -1, loopOut = -1;
        };
        std::array<DeckSim, 2> decks;
        std::array<std::array<float, 6>, 2> knobs{};
        std::array<float, 2> gain{-6.f, -4.f};
        std::array<bool, 2> killHigh{}, killMid{}, killLow{}, cue{};
        std::array<show::PeakHold, 2> hold;
        float crossfade = .5f;
        std::array<std::uint8_t, 4 * 16> steps{};
        int lastStep = -1;
        std::array<float, 32> bands{}, peakBands{};
        float bandLow = .1f, bandHigh = .35f;
        bool initialized = false;
    } beat;

    // Page 30: script.
    struct ScriptSim {
        std::array<char, 8192> text{};
        show::ScriptPasteState state;
        std::vector<show::Diagnostic> diagnostics;
        std::vector<std::string> messages, suggestions;
        std::vector<show::ParamDecl> decls;
        std::vector<std::string> names;
        std::vector<show::ParamValue> values;
        std::string format, summary;
        bool analyzing = false, importable = true, initialized = false;
        double finishAt = 0;
        std::vector<show::LogEntry> log;
        std::vector<std::string> logText;
        show::LogViewState logState;
        show::ImportState import;
        int imported = 0, importedScripts = 0;
        int lastRevision = -1;
    } script;

    // Page 31: surface.
    struct SurfaceSim {
        std::vector<show::SurfaceItem> items;
        show::SurfaceState state;
        std::array<float, 40> values{};     // Fader, knob and meter values for the items (and room for added ones).
        std::array<bool, 16> keys{};
        std::array<char, 32> clock{};
        show::StableId nextId = 100;
        int spareValue = 12, spareKey = 4;
        int edits = 0, operated = 0;
        std::string last = "-";
        bool initialized = false;
    } surface;

    // Page 32: LED.
    struct LedSim {
        show::LightColor color{1.f, .42f, .1f, 0.f, 0.f, 0.f, 1.f};
        float kelvin = 3200.f;
        std::array<show::GradientStop, 8> stops{};
        int stopCount = 3, selectedStop = 0;
        std::array<ImVec4, 300> pixels{};
        int selectedPixel = -1;
        std::vector<show::AddressBlock> blocks;
        std::vector<std::string> blockNames;
        show::AddressMapState address;
        show::ChannelMonitorState channelState;
        std::array<unsigned char, 512> channels{};
        char order[5] = "GRB";
        bool orderWhite = false;
        show::TestPattern pattern = show::TestPattern::Off;
        float brightness = .35f;
        std::array<show::PortView, 8> ports{};
        int selectedPort = 0;
        std::string last = "-";
        bool initialized = false;
    } led;

    // Page 33: pictures as buttons.
    struct ImagesSim {
        std::array<show::ImageButtonView, 6> views{};
        std::array<bool, 6> on{};
        std::array<int, 6> presses{};
        std::array<bool, 6> assigned{true, true, true, true, true, true};
        int adjusting = -1;
        show::ImageCrop crop{};
        std::array<const char*, 6> names{"Sunset", "Waves", "Neon", "Stripes", "Radial", "Checker"};
        std::string last = "-";
        bool initialized = false;
    } images;

    void Show(int page, GalleryState& host);

  private:
    void Surface(GalleryState& host);
    void Led(GalleryState& host);
    void Images(GalleryState& host);
    void Script(GalleryState& host);
    void Beat(GalleryState& host);
    void Schedule(GalleryState& host);
    void Controls(GalleryState& host);
    void Live(GalleryState& host);
    void Rules(GalleryState& host);
    void Init();
};
} // namespace imkit::gallery
