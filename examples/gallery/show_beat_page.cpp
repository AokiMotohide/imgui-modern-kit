#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::gallery {
using namespace show;

void ShowPages::Beat(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    BeatSim& s = beat;
    const float dt = ImGui::GetIO().DeltaTime;
    const double now = ImGui::GetTime();
    const float u = ImGui::GetFrameHeight();

    if (!s.initialized) {
        s.initialized = true;
        for (std::size_t d = 0; d < s.decks.size(); ++d) {
            BeatSim::DeckSim& deck = s.decks[d];
            deck.position = d == 0 ? 20 : 64;
            deck.bpm = d == 0 ? 124 : 126;
            deck.playing = d == 0;
            deck.peaks.resize(static_cast<std::size_t>(deck.length * 50));
            for (std::size_t i = 0; i < deck.peaks.size(); ++i) {
                const float beat = static_cast<float>(i) / 50.f * static_cast<float>(deck.bpm) / 60.f;
                const float kick = std::exp(-std::fmod(beat, 1.f) * 5.f);
                const float section = .55f + .35f * std::sin(static_cast<float>(i) * .0016f + static_cast<float>(d));
                deck.peaks[i] = std::clamp(section * (.35f + .65f * kick), 0.f, 1.f);
            }
            deck.hot[0] = {true, 12.0, "Intro", {}};
            deck.hot[1] = {true, 44.5, "Drop", {}};
            deck.hot[2] = {true, 96.0, "Break", {}};
        }
        s.decks[0].loopIn = 24;
        s.decks[0].loopOut = 32;
        s.knobs[0] = s.knobs[1] = {0, 0, 0, 0, 0, 0};
        for (std::size_t lane = 0; lane < 4; ++lane)
            for (std::size_t step = 0; step < 16; ++step) {
                const bool on = lane == 0 ? step % 4 == 0 : lane == 1 ? step % 8 == 4 : lane == 2 ? step % 2 == 1 : (step == 6 || step == 14);
                s.steps[lane * 16 + step] = on ? 100 : 0;
            }
        s.bands.fill(-60.f);
    }

    // ---- Simulation -----------------------------------------------------------------------------------------------
    if (s.running)
        s.clockSeconds += dt;
    const BeatInfo clockBeat = BeatAt(s.clockSeconds, s.bpm, s.firstBeat);
    for (std::size_t d = 0; d < s.decks.size(); ++d) {
        BeatSim::DeckSim& deck = s.decks[d];
        const double speed = 1.0 + deck.pitch / 100.0;
        if (deck.playing) {
            deck.position += dt * speed;
            if (deck.loopOut > deck.loopIn && deck.loopIn >= 0 && deck.position >= deck.loopOut)
                deck.position = deck.loopIn;
            if (deck.position >= deck.length)
                deck.position = 0;
        }
        deck.angle += static_cast<float>((deck.playing ? 1.0 : 0.0) * speed * dt * 3.5);
        const float in[2] = {-14.f + 10.f * std::sin(static_cast<float>(now) * (3.f + static_cast<float>(d))) + (deck.playing ? 0.f : -60.f),
                             -15.f + 9.f * std::sin(static_cast<float>(now) * (2.4f + static_cast<float>(d)) + 1.f) + (deck.playing ? 0.f : -60.f)};
        s.hold[d].Update(in, dt);
    }
    for (std::size_t i = 0; i < s.bands.size(); ++i) {
        const float shape = -18.f - static_cast<float>(i) * 1.1f;
        const float pulse = 14.f * std::exp(-clockBeat.phase * 4.f) * (i < 6 ? 1.f : .35f);
        s.bands[i] = std::max(-60.f, shape + pulse + 4.f * std::sin(static_cast<float>(now) * 5.f + static_cast<float>(i) * .7f));
        s.peakBands[i] = std::max(s.bands[i], s.peakBands[i] - 30.f * dt);
    }
    const int stepIndex = static_cast<int>(std::floor(clockBeat.beat * 4.0)) % 16;
    const int playhead = s.running ? (stepIndex + 16) % 16 : -1;

    if (BeginGrid(host, "show-beat-grid", 2)) {
        const bool s1 = BeginSection(host, "Beat clock", "Tap four times to set the tempo. The ring shows the phase inside the current beat.");
        if (s1) {
            const char* sources[] = {"Internal", "Link", "MIDI clock"};
            BeatClockView view;
            view.bpm = s.bpm;
            view.beat = clockBeat;
            view.running = s.running;
            view.sources = sources;
            view.source = s.source;
            view.synced = s.source == 0;
            const BeatClockRequest r = BeatClock("clock", view, o);
            Record(host, "show-beatclock");
            if (r.tap) {
                const double estimate = s.tap.Tap(now);
                if (estimate > 40 && estimate < 240)
                    s.bpm = std::round(estimate * 10.0) / 10.0;
            }
            if (r.bpmStep != 0)
                s.bpm = std::clamp(s.bpm + r.bpmStep, 40.0, 240.0);
            if (r.resync)
                s.firstBeat = s.clockSeconds - std::floor((s.clockSeconds - s.firstBeat) * s.bpm / 60.0) * 60.0 / s.bpm;
            if (r.toggleRunning)
                s.running = !s.running;
            if (r.sourceSelected >= 0)
                s.source = r.sourceSelected;
        }
        EndSection(s1);

        const bool s2 = BeginSection(host, "Spectrum", "Drag the band handles to choose the range that drives an audio-reactive parameter.");
        if (s2) {
            SpectrumOptions so;
            so.size = {0, u * 6.f};
            Spectrum("spectrum", s.bands, s.peakBands, &s.bandLow, &s.bandHigh, so, o);
            Record(host, "show-spectrum");
            ImGui::TextDisabled("Band %.0f%% - %.0f%%", static_cast<double>(s.bandLow) * 100.0, static_cast<double>(s.bandHigh) * 100.0);
        }
        EndSection(s2);

        const bool s3 = BeginSection(host, "Decks and waveforms", "Drag the platter to scratch, drag the waveform to scrub, click the overview to seek.");
        if (s3) {
            const char* titles[2] = {"Night Drive", "Afterglow"};
            const char* keys[2] = {"8A", "9A"};
            for (std::size_t d = 0; d < s.decks.size(); ++d) {
                BeatSim::DeckSim& deck = s.decks[d];
                ImGui::PushID(static_cast<int>(d));
                WaveformSpan track;
                track.peaks = deck.peaks;
                track.position = deck.position;
                track.length = deck.length;
                track.bpm = deck.bpm;
                track.cues = {};
                std::array<CueMarker, 3> cues{};
                std::size_t cueCount = 0;
                for (const HotCue& cue : deck.hot)
                    if (cue.set && cueCount < cues.size())
                        cues[cueCount++] = {cue.seconds, cue.label, {}};
                track.cues = std::span<const CueMarker>(cues.data(), cueCount);
                track.loopIn = deck.loopIn;
                track.loopOut = deck.loopOut;
                ScrollingWaveformOptions so;
                so.size = {0, u * 3.6f};
                const WaveformRequest scroll = ScrollingWaveform(d == 0 ? "scrollA" : "scrollB", track, so, o);
                if (scroll.scrub.changed)
                    deck.position = scroll.scrub.value;
                if (scroll.cueClicked >= 0)
                    deck.position = cues[static_cast<std::size_t>(scroll.cueClicked)].seconds;
                OverviewWaveformOptions oo;
                oo.size = {0, u * 1.6f};
                const WaveformRequest overview = OverviewWaveform(d == 0 ? "overviewA" : "overviewB", track, oo, o);
                if (overview.scrub.changed)
                    deck.position = overview.scrub.value;
                ImGui::PopID();
            }
            Record(host, "show-scrolling");
            // Decks side by side with hot cues.
            for (std::size_t d = 0; d < s.decks.size(); ++d) {
                BeatSim::DeckSim& deck = s.decks[d];
                ImGui::PushID(static_cast<int>(d) + 10);
                ImGui::BeginGroup();
                DeckView view;
                view.title = titles[d];
                view.artist = d == 0 ? "Aoki Sound" : "Studio Nine";
                view.key = keys[d];
                view.bpm = PitchedBpm(deck.bpm, deck.pitch);
                view.position = deck.position;
                view.length = deck.length;
                view.playing = deck.playing;
                view.cueHeld = deck.cueHeld;
                view.sync = deck.sync;
                view.platterAngle = deck.angle;
                view.secondDeck = d == 1;
                const DeckRequest r = Deck(d == 0 ? "deckA" : "deckB", view, u * 6.5f, o);
                Record(host, d == 0 ? "show-deck-a" : "show-deck-b");
                if (r.play)
                    deck.playing = !deck.playing;
                if (r.cuePressed) {
                    deck.cueHeld = true;
                    deck.position = deck.hot[0].seconds;
                }
                if (r.cueReleased)
                    deck.cueHeld = false;
                if (r.syncToggled)
                    deck.sync = !deck.sync;
                if (r.scratching) {
                    deck.position = std::clamp(deck.position + static_cast<double>(r.jogRadians) * 0.35, 0.0, deck.length);
                    deck.angle += r.jogRadians;
                }
                const HotCueEvent e = HotCuePads(d == 0 ? "hotA" : "hotB", deck.hot, o);
                if (e.index >= 0) {
                    HotCue& cue = deck.hot[static_cast<std::size_t>(e.index)];
                    if (e.jump)
                        deck.position = cue.seconds;
                    if (e.store) {
                        cue.set = true;
                        cue.seconds = deck.position;
                        cue.label = "";
                    }
                    if (e.clear)
                        cue = {};
                }
                ImGui::EndGroup();
                if (d == 0)
                    ImGui::SameLine(0, u);
                ImGui::PopID();
            }
        }
        EndSection(s3);

        const bool s4 = BeginSection(host, "Mixer and pitch", "Trim, three-band EQ with kill, filter, channel fader with meter, headphone cue and a pitch fader per deck.");
        if (s4) {
            for (std::size_t d = 0; d < 2; ++d) {
                ImGui::PushID(static_cast<int>(d) + 20);
                DjChannel ch;
                ch.name = d == 0 ? "Deck A" : "Deck B";
                ch.trimDb = &s.knobs[d][0];
                ch.high = &s.knobs[d][1];
                ch.mid = &s.knobs[d][2];
                ch.low = &s.knobs[d][3];
                ch.filter = &s.knobs[d][4];
                ch.killHigh = &s.killHigh[d];
                ch.killMid = &s.killMid[d];
                ch.killLow = &s.killLow[d];
                ch.gainDb = &s.gain[d];
                ch.cue = &s.cue[d];
                ch.levelsDb = std::span<const float>(s.hold[d].level.data(), 2);
                ch.peaksDb = std::span<const float>(s.hold[d].peak.data(), 2);
                DjChannelStrip(d == 0 ? "channelA" : "channelB", ch, u * 6.f, o);
                Record(host, d == 0 ? "show-channel-a" : "show-channel-b");
                ImGui::SameLine(0, u * .5f);
                PitchFaderOptions po;
                po.baseBpm = s.decks[d].bpm;
                po.height = u * 6.f;
                PitchFader(d == 0 ? "pitchA" : "pitchB", &s.decks[d].pitch, po, o, &s.decks[d].keyLock);
                Record(host, d == 0 ? "show-pitch-a" : "show-pitch-b");
                ImGui::PopID();
                if (d == 0)
                    ImGui::SameLine(0, u);
            }
            FaderOptions xf;
            xf.vertical = false;
            xf.bipolar = true;
            xf.defaultValue = .5f;
            xf.format = "A/B %.2f";
            xf.size = {ImGui::GetContentRegionAvail().x, u * 1.6f};
            Fader("Crossfader", &s.crossfade, xf, o);
            Record(host, "show-xfade");
        }
        EndSection(s4);

        const bool s5 = BeginSection(host, "Step sequencer", "Click or drag to paint steps. Ctrl+wheel changes velocity, right-click clears. The playhead follows the beat clock.");
        if (s5) {
            const char* lanes[] = {"Kick", "Snare", "Hat", "Clap"};
            SequencerView view;
            view.lanes = 4;
            view.steps = 16;
            view.laneNames = lanes;
            view.velocity = s.steps;
            view.playhead = playhead;
            std::array<StepEdit, 32> storage{};
            StepEditBuffer edits(storage);
            StepSequencer("sequencer", view, edits, {}, o);
            Record(host, "show-sequencer");
            for (const StepEdit& e : edits.Edits())
                s.steps[static_cast<std::size_t>(e.lane) * 16 + static_cast<std::size_t>(e.step)] = e.velocity;
        }
        EndSection(s5);
        EndGrid();
    }
}
} // namespace imkit::gallery
