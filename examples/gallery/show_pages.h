#pragma once
#include <imkit/imkit.h>
#include <imkit/show.h>
#include <array>
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

    void Show(int page, GalleryState& host);

  private:
    void Controls(GalleryState& host);
    void Live(GalleryState& host);
    void Rules(GalleryState& host);
    void Init();
};
} // namespace imkit::gallery
