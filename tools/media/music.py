#!/usr/bin/env python3
"""Original upbeat house score for the ImKit film, synthesized with numpy: no samples, no external music.

The arrangement is written in bars, so cuts in film.json can be placed on bar lines. Swapping in a licensed track
later only needs its BPM and first-downbeat offset (see `beat_grid`). Render a score alone:

    python tools/media/music.py out/media/score.wav --bpm 120 --bars 90
"""
from pathlib import Path
import argparse, math, wave
import numpy as np

SR = 48000

# Am - F - C - G, one chord per bar. Midi notes.
BASS = [45, 41, 48, 43]
CHORDS = [[57, 60, 64], [53, 57, 60], [60, 64, 67], [55, 59, 62]]
LEAD = [  # eight eighth-notes per bar, None = rest
    [76, None, 76, 79, None, 81, None, 79],
    [77, None, 77, 76, None, 72, None, 76],
    [76, None, 79, None, 84, None, 79, 76],
    [74, None, 74, 76, None, 79, None, 83],
]

# (first bar, last bar, layer gains). Bars are 1-based; later rows override earlier ones.
SECTIONS = [
    (1, 4, dict(pad=.9, arp=.5, kick=0.0)),
    (3, 4, dict(kick=.5)),
    (5, 8, dict(kick=.9, hat=.5, bass=.6, pad=.8, arp=.55)),
    (9, 24, dict(kick=1, hat=.8, clap=.8, bass=.9, stab=.55, pad=.6, arp=.4)),
    (25, 26, dict(kick=0, hat=.6, clap=0, bass=0, stab=0, pad=.8, arp=.6, roll=1, riser=1)),
    (27, 42, dict(kick=1, hat=1, clap=1, bass=1, stab=.85, pad=.6, arp=.0, lead=.8, open=.8)),
    (43, 44, dict(kick=0, hat=0, clap=0, bass=0, stab=0, pad=1, arp=.5, lead=0, riser=1)),
    (45, 58, dict(kick=1, hat=1, clap=1, bass=1, stab=.8, pad=.6, arp=.35, lead=.85, open=.8, perc=.6)),
    (59, 72, dict(kick=1, hat=1, clap=1, bass=1, stab=.7, pad=.6, arp=.45, lead=.8, open=1, perc=.9)),
    (73, 80, dict(kick=.55, hat=.45, clap=0, bass=.55, stab=.0, pad=.9, arp=.8, lead=0, open=0, perc=0)),
    (81, 82, dict(kick=0, hat=.7, bass=0, pad=.9, arp=.8, roll=1, riser=1)),
    (83, 88, dict(kick=1, hat=1, clap=1, bass=1, stab=.9, pad=.7, arp=.4, lead=.95, open=1, perc=1, crash=1)),
    (89, 90, dict(kick=0, hat=0, clap=0, bass=0, stab=0, pad=1, arp=.3, lead=0, open=0, perc=0)),
]
LAYERS = ["kick", "hat", "open", "clap", "bass", "stab", "pad", "arp", "lead", "perc", "roll", "riser", "crash"]


def beat_grid(bpm: float, offset: float = 0.0):
    """(seconds per beat, seconds per bar, time of bar n) for a score or a licensed track."""
    beat = 60.0 / bpm
    return beat, 4 * beat, lambda bar: offset + (bar - 1) * 4 * beat


def midi_hz(note: float) -> float:
    return 440.0 * 2 ** ((note - 69) / 12)


def _shape(x, response):
    """Zero-phase filter in the frequency domain; `response(f)` is the magnitude at each frequency (Hz)."""
    spectrum = np.fft.rfft(x, axis=0)
    freq = np.fft.rfftfreq(len(x), 1 / SR)
    gain = response(freq)
    return np.fft.irfft(spectrum * (gain[:, None] if x.ndim > 1 else gain), n=len(x), axis=0)


def lowpass(x, cutoff, order=2):
    return _shape(x, lambda f: 1 / np.sqrt(1 + (f / cutoff) ** (2 * order)))


def highpass(x, cutoff, order=2):
    return _shape(x, lambda f: 1 / np.sqrt(1 + (cutoff / np.maximum(f, 1e-3)) ** (2 * order)))


def convolve(x, ir):
    size = 1 << int(math.ceil(math.log2(len(x) + len(ir))))
    return np.fft.irfft(np.fft.rfft(x, size) * np.fft.rfft(ir, size), size)[:len(x)]


def saw(freq, t, detune=0.0):
    phase = (freq * (1 + detune) * t) % 1.0
    return 2 * phase - 1


class Mix:
    """Stereo buffer with sample-accurate note placement."""

    def __init__(self, seconds):
        self.n = int(seconds * SR)
        self.bus = {name: np.zeros((self.n, 2), np.float32) for name in ("drums", "bass", "music", "fx")}

    def add(self, bus, start, sound, gain=1.0, pan=0.0):
        i = int(round(start * SR))
        if i >= self.n or i + len(sound) <= 0:
            return
        a, b = max(0, -i), min(len(sound), self.n - i)
        left, right = math.sqrt((1 - pan) / 2), math.sqrt((1 + pan) / 2)
        self.bus[bus][i + a:i + b, 0] += sound[a:b] * gain * left
        self.bus[bus][i + a:i + b, 1] += sound[a:b] * gain * right


def kick(length=.32):
    t = np.arange(int(SR * length)) / SR
    return np.sin(2 * np.pi * (46 * t + 5.2 * (1 - np.exp(-t * 38)))) * np.exp(-t * 14) + .35 * np.sin(2 * np.pi * 120 * t) * np.exp(-t * 90)


def clap(rng):
    t = np.arange(int(SR * .22)) / SR
    noise = highpass(rng.standard_normal(len(t)), 900)
    env = sum(np.exp(-np.maximum(t - d, 0) * 38) * (t >= d) for d in (0, .011, .022)) * .5
    return noise * env * np.exp(-t * 9)


def hat(rng, open_=False):
    t = np.arange(int(SR * (.28 if open_ else .06))) / SR
    return highpass(rng.standard_normal(len(t)), 7000) * np.exp(-t * (14 if open_ else 80))


def bass_note(midi, length):
    t = np.arange(int(SR * length)) / SR
    f = midi_hz(midi)
    body = np.sin(2 * np.pi * f * t) + .35 * lowpass(saw(f, t), 420) + .2 * np.sin(4 * np.pi * f * t)
    return body * (1 - np.exp(-t * 400)) * np.exp(-t * 3.2)


def stab(notes, length=.22):
    t = np.arange(int(SR * length)) / SR
    wave_ = sum(saw(midi_hz(n), t, d) for n in notes for d in (-.004, .004))
    return lowpass(wave_, 3200) * np.exp(-t * 11) * (1 - np.exp(-t * 600)) / (len(notes) * 1.6)


def pad(notes, length):
    t = np.arange(int(SR * length)) / SR
    wave_ = sum(saw(midi_hz(n), t, d) for n in notes for d in (-.006, 0, .006))
    env = np.minimum(1, t / .35) * np.minimum(1, (length - t) / .4)
    return lowpass(wave_, 1900) * env / (len(notes) * 3.2)


def pluck(midi, length=.26):
    t = np.arange(int(SR * length)) / SR
    f = midi_hz(midi)
    return lowpass(saw(f, t) + .5 * saw(f * 2, t), 3800) * np.exp(-t * 14) * (1 - np.exp(-t * 800)) * .5


def lead(midi, length):
    t = np.arange(int(SR * length)) / SR
    f = midi_hz(midi) * (1 + .003 * np.sin(2 * np.pi * 5.5 * t))
    phase = np.cumsum(f) / SR
    tone = np.sign(np.sin(2 * np.pi * phase)) * .5 + saw(1, phase, 0) * .35
    return lowpass(tone, 4200) * np.minimum(1, t / .01) * np.exp(-t * 3.5) * np.minimum(1, (length - t) / .03) * .6


def noise_riser(seconds, rng):
    """Noise plus a rising tone: the build-up before a drop."""
    t = np.arange(int(SR * seconds)) / SR
    sweep = np.sin(2 * np.pi * np.cumsum(180 * (36 ** (t / seconds))) / SR)
    hiss = highpass(rng.standard_normal(len(t)), 3500)
    return (hiss * .8 + sweep * .35) * (t / seconds) ** 2.2 * .5


def reverb(x, rng, seconds=1.8, mix=.28):
    n = int(SR * seconds)
    ir = rng.standard_normal((n, 2)) * np.exp(-np.arange(n) / (SR * seconds * .28))[:, None]
    ir = lowpass(ir, 5500, 1) * .02
    wet = np.stack([convolve(x[:, c].astype(np.float64), ir[:, c]) for c in range(2)], axis=1)
    return x * (1 - mix * .5) + wet * mix


def gains_for(bar):
    g = {name: 0.0 for name in LAYERS}
    for first, last, layer in SECTIONS:
        if first <= bar <= last:
            g.update(layer)
    return g


def render_score(path: Path, bpm=120.0, bars=90, cuts=(), clicks=()):
    """Write a stereo 16-bit WAV. `cuts` are picture-cut times (soft whoosh), `clicks` are recorded mouse presses."""
    rng = np.random.default_rng(404)
    beat, bar_len, bar_time = beat_grid(bpm)
    seconds = bars * bar_len
    mix = Mix(seconds + 3)
    kick_hit, clap_hit = kick(), clap(rng)
    for bar in range(1, bars + 1):
        g, start, chord = gains_for(bar), bar_time(bar), (bar - 1) % 4
        for b in range(4):
            at = start + b * beat
            if g["kick"]:
                mix.add("drums", at, kick_hit, .95 * g["kick"])
            if g["clap"] and b in (1, 3):
                mix.add("drums", at, clap_hit, .34 * g["clap"], .1)
            if g["perc"] and b in (0, 2):
                mix.add("drums", at + beat * .75, hat(rng, False), .16 * g["perc"], -.45 if b == 0 else .45)
            for s in range(2):
                if g["hat"]:
                    mix.add("drums", at + s * beat / 2 + beat / 4, hat(rng), .13 * g["hat"] * (1 if s else .6), (-.3, .3)[s])
            if g["open"]:
                mix.add("drums", at + beat / 2, hat(rng, True), .11 * g["open"], .25)
            if g["bass"]:
                for s in range(2):
                    mix.add("bass", at + s * beat / 2, bass_note(BASS[chord] + (12 if (b == 3 and s) else 0), beat / 2 * .92), .5 * g["bass"])
            if g["stab"] and (b % 2 == 1 or s):
                mix.add("music", at + beat / 2, stab(CHORDS[chord] + [CHORDS[chord][0] + 12]), .22 * g["stab"], -.2)
            if g["roll"]:
                for s in range(4):
                    mix.add("drums", at + s * beat / 4, clap_hit[:int(SR * .1)], .12 * (1 + (bar - 25 if bar < 40 else bar - 81) * .35 + b * .1 + s * .05) * g["roll"], 0)
        if g["pad"]:
            mix.add("music", start, pad(CHORDS[chord] + [CHORDS[chord][0] - 12], bar_len + .3), .55 * g["pad"])
        if g["arp"]:
            notes = CHORDS[chord] + [CHORDS[chord][0] + 12]
            for s in range(16):
                mix.add("music", start + s * beat / 4, pluck(notes[(s * 3) % 4] + (12 if s % 8 >= 4 else 0)), .32 * g["arp"], math.sin(s) * .5)
        if g["lead"]:
            octave = 12 if bar >= 59 and (bar // 4) % 2 else 0
            for i, note in enumerate(LEAD[chord]):
                if note is not None:
                    mix.add("music", start + i * beat / 2, lead(note + octave, beat * .5), .3 * g["lead"], .15)
        if g["riser"]:
            mix.add("fx", start, noise_riser(bar_len, rng), .45)
        if g["crash"] and bar in (83,):
            mix.add("fx", start, highpass(rng.standard_normal(int(SR * 2.2)), 3000) * np.exp(-np.arange(int(SR * 2.2)) / SR * 2.2), .35)
    # Picture cuts and recorded mouse presses.
    for at in cuts:
        t = np.arange(int(SR * .22)) / SR
        mix.add("fx", at - .1, highpass(rng.standard_normal(len(t)), 2500) * np.sin(np.pi * t / .22) ** 2, .1, -.2)
    for at in clicks:
        t = np.arange(int(SR * .04)) / SR
        mix.add("fx", at, np.sin(2 * np.pi * 1700 * t) * np.exp(-t * 110), .12)
    # Side-chain: music and bass duck on every kick.
    pump = np.ones(mix.n, np.float32)
    for bar in range(1, bars + 1):
        if gains_for(bar)["kick"]:
            for b in range(4):
                i = int((bar_time(bar) + b * beat) * SR)
                length = int(SR * beat * .8)
                curve = .35 + .65 * (1 - np.exp(-np.arange(min(length, mix.n - i)) / SR * 14))
                pump[i:i + len(curve)] = np.minimum(pump[i:i + len(curve)], curve)
    music = reverb(mix.bus["music"], rng) * pump[:, None]
    out = mix.bus["drums"] + mix.bus["bass"] * pump[:, None] * .9 + music + mix.bus["fx"]
    t = np.arange(mix.n) / SR
    out *= (np.minimum(1, t / .05) * np.minimum(1, np.maximum(0, seconds + 2.2 - t) / 2.0))[:, None]
    out = np.tanh(out * 1.15)
    out *= .9 / max(.01, float(np.abs(out).max()))
    with wave.open(str(path), "wb") as f:
        f.setnchannels(2)
        f.setsampwidth(2)
        f.setframerate(SR)
        f.writeframes((out[:int((seconds + 2.2) * SR)] * 32767).astype("<i2").tobytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("output", type=Path)
    parser.add_argument("--bpm", type=float, default=120.0)
    parser.add_argument("--bars", type=int, default=90)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    render_score(args.output, args.bpm, args.bars)
    print(f"{args.output}: {args.bars} bars at {args.bpm:g} BPM")


if __name__ == "__main__":
    main()


# ---------------------------------------------------------------------------------------------------- licensed track
def load_track(path: Path) -> np.ndarray:
    """Decode any audio file to stereo float32 at SR."""
    import subprocess
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(path), "-f", "f32le", "-ac", "2", "-ar", str(SR), "pipe:1"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.float32).reshape(-1, 2).copy()


def track_grid(audio: np.ndarray, bpm: float):
    """First downbeat (seconds) and per-bar RMS of a track whose tempo is known.

    The kick shows up as low-frequency energy, so the beat phase is where low-passed energy peaks most often; the downbeat
    is the strongest of the first four beats. Returns (offset_seconds, beat_scores, bar_rms)."""
    mono = audio.mean(axis=1)
    low = lowpass(mono[: SR * 40].astype(np.float64), 150, 2)
    hop = SR // 200
    energy = np.abs(low[: len(low) // hop * hop]).reshape(-1, hop).mean(axis=1)
    flux = np.maximum(0, np.diff(energy, prepend=0))
    beat = 60.0 / bpm
    steps = int(beat * 200)
    scores = np.array([flux[p::steps][:60].sum() for p in range(steps)])
    phase = int(np.argmax(scores)) / 200
    beats = [phase + k * beat for k in range(4)]
    strength = [float(flux[int(t * 200): int(t * 200) + 3].sum()) for t in beats]
    downbeat = beats[int(np.argmax(strength))]
    bar = 4 * beat
    rms = [float(np.sqrt((mono[int((downbeat + i * bar) * SR): int((downbeat + (i + 1) * bar) * SR)] ** 2).mean()))
           for i in range(int((len(mono) / SR - downbeat) // bar))]
    return downbeat, strength, rms


def render_track_score(path: Path, track: Path, bpm: float, bars: int, offset: float, cuts=(), clicks=()):
    """Write the film's score: the licensed track from its first downbeat, with the picture's cut whooshes and mouse clicks
    mixed in softly. The track is trimmed to the film length and fades out; it is never time-stretched."""
    beat, bar_len, _ = beat_grid(bpm)
    seconds = bars * bar_len
    audio = load_track(track)[int(offset * SR):]
    n = int((seconds + 2.0) * SR)
    audio = audio[:n] if len(audio) >= n else np.vstack([audio, np.zeros((n - len(audio), 2), np.float32)])
    t = np.arange(n) / SR
    fade_out = np.clip((seconds + 1.6 - t) / 3.0, 0, 1)
    audio = audio * (np.minimum(1, t / .08) * fade_out)[:, None]
    rng = np.random.default_rng(404)
    fx = Mix(seconds + 3)
    for at in cuts:
        w = np.arange(int(SR * .22)) / SR
        fx.add("fx", at - .1, highpass(rng.standard_normal(len(w)), 2500) * np.sin(np.pi * w / .22) ** 2, .05, -.2)
    for at in clicks:
        w = np.arange(int(SR * .04)) / SR
        fx.add("fx", at, np.sin(2 * np.pi * 1700 * w) * np.exp(-w * 110), .07)
    out = audio + fx.bus["fx"][:n]
    out *= .9 / max(.01, float(np.abs(out).max()))
    with wave.open(str(path), "wb") as f:
        f.setnchannels(2)
        f.setsampwidth(2)
        f.setframerate(SR)
        f.writeframes((out * 32767).astype("<i2").tobytes())
