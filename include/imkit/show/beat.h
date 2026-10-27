#pragma once
#include <imkit/show/controls.h>
#include <array>
#include <cstdint>
#include <span>

// Beat and DJ-style components: tempo, decks, waveforms, hot cues, channel strips, pitch, step sequencing and a
// spectrum with a band selector. Audio analysis, playback and MIDI stay with the host.
namespace imkit::show {
// ---- Tempo --------------------------------------------------------------------------------------------------
// Tap-tempo estimator: call Tap with a monotonic clock in seconds. Gaps over two seconds start a new count; the
// estimate averages the last intervals after dropping ones far from the median. Returns BPM, or 0 until two taps.
struct TapTempo {
    std::array<double, 8> taps{};
    int count = 0;
    double Tap(double seconds);
    void Reset() { count = 0; }
};
struct BeatInfo {
    double beat = 0;                // Continuous beat number since the first beat (negative before it).
    int index = 0;                  // Whole beats since the first beat.
    float phase = 0;                // 0..1 inside the beat.
    int beatInBar = 1;              // 1..beatsPerBar.
};
BeatInfo BeatAt(double seconds, double bpm, double firstBeatSeconds, int beatsPerBar = 4);
// BPM after moving a fader by `percent` (for example +3.5).
double PitchedBpm(double baseBpm, double percent);

struct BeatClockView {
    double bpm = 120;
    BeatInfo beat{};
    int beatsPerBar = 4;
    bool running = true;
    bool synced = true;             // False shows the source in the warning colour.
    std::span<const char* const> sources{}; // For example {"Internal", "Link", "MIDI clock"}.
    int source = 0;
};
struct BeatClockRequest {
    bool tap = false;
    bool resync = false;            // Snap the phase to the next beat.
    bool toggleRunning = false;
    float nudge = 0;                // -1 or +1 while a nudge button is held, in beats per second of correction.
    double bpmStep = 0;             // +/-0.1 from the fine buttons, +/-1 with Shift.
    int sourceSelected = -1;
};
BeatClockRequest BeatClock(const char* id, const BeatClockView& view, ComponentOptions options = {});

// ---- Waveforms ----------------------------------------------------------------------------------------------
struct CueMarker {
    double seconds = 0;
    const char* label = "";
    ImVec4 color{};                 // Alpha <= 0 picks a layer colour from the index.
};
struct WaveformSpan {
    std::span<const float> peaks{}; // 0..1 per bucket.
    double bucketsPerSecond = 50;
    double position = 0;            // Playhead, seconds.
    double length = 0;              // Track length; zero derives it from the peaks.
    double bpm = 0;                 // Zero hides the beat grid.
    double firstBeat = 0;
    int beatsPerBar = 4;
    std::span<const CueMarker> cues{};
    double loopIn = -1, loopOut = -1; // Negative hides the loop.
};
struct ScrollingWaveformOptions {
    ImVec2 size{0, 0};              // Zero fills the width and uses about 4 frame heights.
    double windowSeconds = 8;       // Visible span, centred on the playhead.
};
struct WaveformRequest {
    ValueEdit scrub{};              // Drag to move the track; `value` is the new position in seconds.
    int cueClicked = -1;            // A cue flag was clicked.
};
WaveformRequest ScrollingWaveform(const char* id, const WaveformSpan& track, ScrollingWaveformOptions layout = {}, ComponentOptions options = {});
struct OverviewWaveformOptions {
    ImVec2 size{0, 0};              // Zero fills the width and uses about 2 frame heights.
};
// Whole-track view. Click or drag to seek.
WaveformRequest OverviewWaveform(const char* id, const WaveformSpan& track, OverviewWaveformOptions layout = {}, ComponentOptions options = {});

// ---- Deck ---------------------------------------------------------------------------------------------------
struct DeckView {
    const char* title = "";
    const char* artist = "";
    const char* key = "";
    double bpm = 0;
    double position = 0, length = 0; // Seconds.
    bool playing = false, cueHeld = false, sync = false, loaded = true;
    float platterAngle = 0;         // Radians, integrated by the host from the platter speed.
    ImVec4 color{};                 // Deck accent; alpha <= 0 uses the A or B bus colour.
    bool secondDeck = false;
};
struct DeckRequest {
    bool play = false;              // Play/pause pressed.
    bool cuePressed = false, cueReleased = false;
    bool syncToggled = false;
    bool load = false;
    bool scratching = false;        // The platter is being dragged.
    float jogRadians = 0;           // Angle dragged this frame; the host converts it to a position change.
};
DeckRequest Deck(const char* id, const DeckView& view, float diameter = 0, ComponentOptions options = {});

// ---- Hot cues -----------------------------------------------------------------------------------------------
struct HotCue {
    bool set = false;
    double seconds = 0;
    const char* label = "";
    ImVec4 color{};
};
struct HotCueEvent {
    int index = -1;
    bool jump = false;              // A set cue was pressed.
    bool store = false;             // An empty cue was pressed: store the playhead here.
    bool clear = false;             // Right-click on a set cue.
};
HotCueEvent HotCuePads(const char* id, std::span<const HotCue> cues, ComponentOptions options = {});

// ---- Channel strip and pitch --------------------------------------------------------------------------------
struct DjChannel {
    const char* name = "";
    ImVec4 color{};
    float* trimDb = nullptr;        // -12..+12.
    float* high = nullptr, *mid = nullptr, *low = nullptr; // -24..+6 dB, with kill switches below.
    bool* killHigh = nullptr, *killMid = nullptr, *killLow = nullptr;
    float* filter = nullptr;        // -1 (low-pass) .. +1 (high-pass).
    float* gainDb = nullptr;        // Channel fader, -60..+6.
    bool* cue = nullptr;            // Headphone pre-fader listen.
    std::span<const float> levelsDb{};
    std::span<const float> peaksDb{};
};
struct DjChannelRequest {
    ValueEdit edit{};
    int control = -1;               // 0 trim, 1 high, 2 mid, 3 low, 4 filter, 5 fader.
    bool toggled = false;           // A kill or cue switch changed.
};
DjChannelRequest DjChannelStrip(const char* id, const DjChannel& channel, float faderHeight = 0, ComponentOptions options = {});

struct PitchFaderOptions {
    float range = 8;                // Percent either way.
    double baseBpm = 0;             // Shows the pitched BPM when positive.
    float height = 0;
};
// Vertical pitch fader with a centre detent. `value` is percent; positive speeds up.
ValueEdit PitchFader(const char* id, float* percent, PitchFaderOptions layout = {}, ComponentOptions options = {}, bool* keyLock = nullptr);

// ---- Step sequencer -----------------------------------------------------------------------------------------
struct StepEdit {
    int lane = 0, step = 0;
    std::uint8_t velocity = 0;      // 0 turns the step off.
};
class StepEditBuffer {
  public:
    explicit StepEditBuffer(std::span<StepEdit> storage) : storage_(storage) {}
    bool Push(StepEdit e) {
        if (count_ == storage_.size()) {
            overflow_ = true;
            return false;
        }
        storage_[count_++] = e;
        return true;
    }
    std::span<const StepEdit> Edits() const { return storage_.first(count_); }
    bool Overflow() const { return overflow_; }
    void Clear() {
        count_ = 0;
        overflow_ = false;
    }

  private:
    std::span<StepEdit> storage_;
    std::size_t count_ = 0;
    bool overflow_ = false;
};
struct SequencerView {
    int lanes = 0, steps = 16;
    std::span<const char* const> laneNames{};
    std::span<const std::uint8_t> velocity{}; // lanes * steps, row-major by lane; 0 = off.
    int playhead = -1;              // Current step, or -1.
    int stepsPerBeat = 4;
    int swingSteps = 0;             // Reserved for hosts that visualise swing.
};
struct SequencerOptions {
    ImVec2 size{0, 0};              // Zero fills the width; height follows the lane count.
    std::uint8_t defaultVelocity = 100;
};
// Click toggles a step, drag paints (the first cell decides on or off), right-click clears, Ctrl+wheel changes velocity.
void StepSequencer(const char* id, const SequencerView& view, StepEditBuffer& edits, SequencerOptions layout = {}, ComponentOptions options = {});

// ---- Spectrum -----------------------------------------------------------------------------------------------
struct SpectrumOptions {
    ImVec2 size{0, 0};              // Zero fills the width and uses about 5 frame heights.
    float minimumDb = -72, maximumDb = 0;
};
// Bars in dB with peak marks. When `bandLow` and `bandHigh` are set (0..1 across the bars), a range handle below the
// bars selects the band that drives audio-reactive parameters; bars inside it use the accent colour.
ValueEdit Spectrum(const char* id, std::span<const float> bandsDb, std::span<const float> peaksDb, float* bandLow, float* bandHigh,
                   SpectrumOptions layout = {}, ComponentOptions options = {});
} // namespace imkit::show
