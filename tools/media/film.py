#!/usr/bin/env python3
"""Compose the ImKit film from the recorded takes: footage on a beat-synced timeline, captions in two languages, a score.

The timeline lives in film.json (shots placed on bar lines) and the words in captions.<lang>.json, so a language or a
music swap never needs a new recording. Only the final MP4, poster and provenance are meant for git; everything else
stays in out/media/film.

    python tools/media/film.py --lang both            # ja and en films, then the poster and provenance
    python tools/media/film.py --lang ja --review     # contact sheet of key frames, no encoding
    python tools/media/film.py --lang en --seconds 20 # a quick partial render
"""
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
import argparse, hashlib, json, math, re, subprocess, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
import music

ROOT = Path(__file__).resolve().parents[2]
NATIVE = ROOT / "out/media/native"
WORK = ROOT / "out/media/film"
SPEC = json.loads((Path(__file__).parent / "film.json").read_text(encoding="utf-8"))
W, H = SPEC["size"]
FPS = SPEC["fps"]
BEAT = 60.0 / SPEC["bpm"]
BAR = 4 * BEAT
TOTAL = SPEC["bars"] * BAR
AUDIO_BITRATE = "256k"
BG =(11, 14, 18)
WHITE = (240, 244, 247)
MUTED = (150, 162, 172)
CARD = (1520, 855)
CARD_XY = ((W - CARD[0]) // 2, 140)


def ease(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def ease_out(x):
    x = max(0.0, min(1.0, x))
    return 1 - (1 - x) ** 3


def mix(a, b, u):
    return tuple(x + (y - x) * u for x, y in zip(a, b))


def start_of(shot):
    return (shot["at"] - 1) * BAR


def duration_of(shot):
    return shot["bars"] * BAR


# --------------------------------------------------------------------------------------------------- footage
class Take:
    """Sequential frame reader for one recorded take; reopens when asked to jump."""

    def __init__(self, name):
        self.path = NATIVE / f"{name}.mkv"
        info = json.loads(subprocess.check_output(["ffprobe", "-v", "error", "-show_format", "-of", "json", str(self.path)]))
        self.duration = float(info["format"]["duration"])
        self.proc, self.index, self.image = None, -1, None

    def close(self):
        if self.proc:
            self.proc.stdout.close()
            self.proc.terminate()
            self.proc.wait()
            self.proc = None

    def frame(self, t):
        target = min(int(max(t, 0) * 60 + 1e-6), int(self.duration * 60) - 1)
        if self.image is not None and target == self.index:
            return self.image
        if self.proc is None or target < self.index or target - self.index > 90:
            self.close()
            self.proc = subprocess.Popen(["ffmpeg", "-v", "error", "-threads", "1", "-ss", f"{target / 60:.6f}", "-i", str(self.path),
                                          "-f", "rawvideo", "-pix_fmt", "rgb24", "pipe:1"], stdout=subprocess.PIPE)
            self.index = target - 1
        while self.index < target:
            raw = self.proc.stdout.read(1920 * 1080 * 3)
            if len(raw) != 1920 * 1080 * 3:
                raise RuntimeError(f"short read from {self.path.name} at frame {target}")
            self.image = Image.frombytes("RGB", (1920, 1080), raw)
            self.index += 1
        return self.image


# --------------------------------------------------------------------------------------------------- drawing
FONTS = {}


def font(lang, size, bold):
    key = (lang, int(size), bold)
    if key not in FONTS:
        if lang == "ja":
            FONTS[key] = ImageFont.truetype(str(ROOT / "assets/fonts/NotoSansJP-Regular.otf"), int(size))
        else:
            FONTS[key] = ImageFont.truetype(str(ROOT / "assets/fonts" / ("Inter-SemiBold.ttf" if bold else "Inter-Regular.ttf")), int(size))
    return FONTS[key]


def blit_text(im, xy, s, lang, size, color=WHITE, bold=False, alpha=1.0, max_width=None, anchor="la"):
    """Draws text through a mask, so fades and the Japanese pseudo-bold (a 1 px stroke) stay cheap."""
    f = font(lang, size, bold)
    stroke = 1 if (bold and lang == "ja") else 0
    if max_width:
        while f.getlength(s) > max_width and size > 12:
            size -= 2
            f = font(lang, size, bold)
    box = f.getbbox(s, stroke_width=stroke, anchor=anchor)
    mask = Image.new("L", (box[2] - box[0] + 4, box[3] - box[1] + 4), 0)
    ImageDraw.Draw(mask).text((2 - box[0], 2 - box[1]), s, font=f, fill=int(255 * alpha), stroke_width=stroke, stroke_fill=int(255 * alpha), anchor=anchor)
    im.paste(Image.new("RGB", mask.size, color), (int(xy[0] + box[0] - 2), int(xy[1] + box[1] - 2)), mask)
    return f.getlength(s)


class Layers:
    """Pre-rendered pieces that never change between frames."""

    def __init__(self):
        yy = np.linspace(0, 1, H)[:, None, None]
        base = np.array(BG, np.float32)[None, None, :] + (np.array((10, 16, 22), np.float32) * (1 - yy))
        self.background = Image.fromarray(np.broadcast_to(base, (H, W, 3)).clip(0, 255).astype(np.uint8))
        glow = Image.new("L", (900, 900), 0)
        ImageDraw.Draw(glow).ellipse((100, 100, 800, 800), fill=255)
        self.glow = glow.filter(ImageFilter.GaussianBlur(170))
        self.mask = {}
        shadow = Image.new("L", (CARD[0] + 160, CARD[1] + 160), 0)
        ImageDraw.Draw(shadow).rounded_rectangle((80, 92, 80 + CARD[0], 92 + CARD[1]), 26, fill=190)
        self.shadow = shadow.filter(ImageFilter.GaussianBlur(26))

    def card_mask(self, size):
        if size not in self.mask:
            m = Image.new("L", size, 0)
            ImageDraw.Draw(m).rounded_rectangle((0, 0, size[0] - 1, size[1] - 1), 24, fill=255)
            self.mask[size] = m
        return self.mask[size]


class Film:
    def __init__(self, lang):
        self.lang = lang
        self.caps = json.loads((Path(__file__).parent / f"captions.{lang}.json").read_text(encoding="utf-8"))
        self.layers = Layers()
        self.takes = {}

    def take(self, name):
        if name not in self.takes:
            self.takes[name] = Take(name)
        return self.takes[name]

    def close(self):
        for t in self.takes.values():
            t.close()

    def chapter_color(self, bar):
        color, index = SPEC["chapters"][0]["color"], 0
        for i, c in enumerate(SPEC["chapters"]):
            if bar >= c["first"]:
                color, index = c["color"], i
        return tuple(color), index

    def shot_at(self, t):
        for shot in SPEC["shots"]:
            if start_of(shot) <= t < start_of(shot) + duration_of(shot) - 1e-9:
                return shot
        return SPEC["shots"][-1]

    # ------------------------------------------------------------------ pieces
    def background(self, t, color):
        im = self.layers.background.copy()
        beat_phase = (t % BEAT) / BEAT
        for i, (cx, cy, scale) in enumerate([(1500, 260, 1.15), (320, 860, .9)]):
            x = int(cx + math.sin(t * .18 + i * 2.1) * 140 - 450 * scale)
            y = int(cy + math.cos(t * .14 + i) * 90 - 450 * scale)
            glow = self.layers.glow.resize((int(900 * scale),) * 2)
            alpha = glow.point(lambda v: int(v * (.34 + .1 * (1 - beat_phase)) * (1 if i == 0 else .7)))
            im.paste(Image.new("RGB", glow.size, tuple(int(c * .85) for c in color)), (x, y), alpha)
        return im

    def card(self, im, source, rect, size, xy, local, color):
        x0, y0, x1, y1 = rect
        frame = source.crop((int(x0), int(y0), int(x1), int(y1))).resize(size, Image.Resampling.BILINEAR if (x1 - x0) > 1500 else Image.Resampling.LANCZOS)
        mask = self.layers.card_mask(size)
        shadow = self.layers.shadow
        if size == CARD:
            im.paste((4, 7, 10), (xy[0] - 80, xy[1] - 56), shadow)
        im.paste(frame, xy, mask)
        draw = ImageDraw.Draw(im)
        draw.rounded_rectangle((xy[0], xy[1], xy[0] + size[0], xy[1] + size[1]), 24, outline=tuple(int(c * .55) for c in color), width=2)

    def caption(self, im, key, local, dur, style, color):
        cap = self.caps[key]
        fade_in, fade_out = ease(local / .35), ease((dur - local) / .3)
        a = min(fade_in, fade_out)
        if a <= 0:
            return
        offset = int(18 * (1 - ease_out(local / .45)))
        if style == "title":
            veil = Image.new("L", (W, 330), int(170 * a))
            im.paste(Image.new("RGB", (W, 330), (6, 9, 12)), (0, 380), veil)
            blit_text(im, (W // 2, 440 + offset), cap["title"], self.lang, 108, WHITE, True, a, W - 200, "ma")
            blit_text(im, (W // 2, 590 + offset), cap["sub"], self.lang, 46, color, False, a, W - 240, "ma")
            return
        blit_text(im, (CARD_XY[0], 22 + offset), cap["title"], self.lang, 50, WHITE, True, a, W - 2 * CARD_XY[0])
        blit_text(im, (CARD_XY[0] + 2, 92 + offset), cap["sub"], self.lang, 28, (176, 188, 198), False, a, W - 2 * CARD_XY[0])
        # accent tick on the left of the title
        ImageDraw.Draw(im).rounded_rectangle((CARD_XY[0] - 22, 30 + offset, CARD_XY[0] - 14, 120 + offset), 4, fill=tuple(int(c * a + 12 * (1 - a)) for c in color))

    def cut_wipe(self, im, local, color):
        if local > .3:
            return
        u = ease(local / .3)
        x = int((W + 400) * u) - 200
        band = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        ImageDraw.Draw(band).polygon([(x - 90, -10), (x + 40, -10), (x - 40, H + 10), (x - 170, H + 10)], fill=color + (int(190 * (1 - u * .6)),))
        im.paste(band, (0, 0), band)

    def progress(self, im, t, color, chapter):
        total = len(SPEC["chapters"])
        draw = ImageDraw.Draw(im)
        for i in range(total):
            x = W // 2 - (total * 34) // 2 + i * 34
            draw.rounded_rectangle((x, 1048, x + (24 if i == chapter else 10), 1056), 4, fill=color if i == chapter else (54, 62, 70))
        draw.rectangle((0, H - 3, int(W * t / TOTAL), H), fill=color)

    # ------------------------------------------------------------------ shots
    def draw_take(self, im, shot, t, local, color):
        src_t = shot["in"] + local * shot["speed"]
        source = self.take(shot["take"]).frame(src_t)
        rect = (0, 0, 1920, 1080)
        keys = shot.get("camera")
        if keys:
            rect = keys[-1]["rect"]
            for a, b in zip(keys, keys[1:]):
                if a["t"] <= local <= b["t"]:
                    u = ease((local - a["t"]) / max(1e-6, b["t"] - a["t"]))
                    rect = [p + (q - p) * u for p, q in zip(a["rect"], b["rect"])]
                    break
            else:
                rect = keys[0]["rect"] if local < keys[0]["t"] else keys[-1]["rect"]
        pop = ease_out(local / .4)
        size = (int(CARD[0] * (.93 + .07 * pop)), int(CARD[1] * (.93 + .07 * pop)))
        xy = (CARD_XY[0] + (CARD[0] - size[0]) // 2, CARD_XY[1] + (CARD[1] - size[1]) // 2)
        self.card(im, source, rect, size, xy, local, color)

    def draw_chapter(self, im, shot, local, color):
        cap = self.caps[shot["key"]]
        u = ease_out(local / .5)
        a = min(1.0, ease(local / .25), ease((shot["bars"] * BAR - local) / .25))
        blit_text(im, (160 - int(70 * (1 - u)), 250), shot["num"], "en", 330, tuple(int(c * .55) for c in color), True, a)
        blit_text(im, (172 - int(40 * (1 - u)), 640), cap["title"], self.lang, 112, WHITE, True, a, W - 340)
        blit_text(im, (176 - int(40 * (1 - u)), 790), cap["sub"], self.lang, 40, MUTED, False, a, W - 360)
        ImageDraw.Draw(im).rounded_rectangle((172, 612, 172 + int(520 * u), 620), 4, fill=color)

    def draw_code(self, im, shot, t, local, color):
        panel = (CARD_XY[0], CARD_XY[1] + 20, CARD_XY[0] + 800, CARD_XY[1] + 20 + 470)
        draw = ImageDraw.Draw(im)
        draw.rounded_rectangle(panel, 22, fill=(16, 21, 27), outline=tuple(int(c * .5) for c in color), width=2)
        tabs = ["CMake", "C++"]
        for i, label in enumerate(tabs):
            on = (local < 5.0) == (i == 0)
            blit_text(im, (panel[0] + 34 + i * 140, panel[1] + 22), label, "en", 24, color if on else (110, 122, 132), True)
        blocks = [
            ["set(IMKIT_IMGUI_TARGET host_imgui)", "add_subdirectory(external/imgui-modern-kit)", "target_link_libraries(your_app PRIVATE imkit::imkit)"],
            ["auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);", "imkit::ApplyTheme(theme);  // before ImGui::NewFrame()", "imkit::Toggle(\"Enabled\", &enabled);  // inside any window"],
        ]
        index = 0 if local < 5.0 else 1
        lines = blocks[index]
        started = 0.6 if index == 0 else 5.4
        shown = int(max(0.0, (local - started)) * 48)
        y = panel[1] + 92
        for line in lines:
            count = min(len(line), max(0, shown))
            shown -= len(line)
            code, _, comment = line[:count].partition("//")
            width = blit_text(im, (panel[0] + 34, y), code, "en", 23, WHITE)
            if comment or (count == len(line) and "//" in line):
                blit_text(im, (panel[0] + 34 + width, y), "//" + comment, "en", 23, (110, 122, 132))
            y += 76
        if int(local * 2) % 2 == 0:
            draw.rectangle((panel[0] + 34, panel[1] + 340, panel[0] + 44, panel[1] + 368), fill=color)
        # the result: the real footage of the theme being applied
        inset = shot["inset"]
        size = (680, 383)
        xy = (CARD_XY[0] + 800 + 40, CARD_XY[1] + 70)
        source = self.take(inset["take"]).frame(inset["in"] + (local % 6.5) * inset["speed"])
        self.card(im, source, [1020, 30, 1880, 514], size, xy, local, color)
        blit_text(im, (xy[0], xy[1] + size[1] + 22), "Add a theme" if self.lang == "en" else "テーマを足すと", self.lang, 28, color, True)

    def draw_outro(self, im, shot, t, local, color):
        u = ease_out(local / .6)
        for i, key in enumerate(["outro-1", "outro-2", "outro-3"]):
            cap = self.caps[key]
            a = ease((local - i * .55) / .5)
            if i == 0:
                blit_text(im, (180 - int(60 * (1 - u)), 190), cap["title"], "en", 230, WHITE, True, a)
                blit_text(im, (186, 470), cap["sub"], self.lang, 54, color, True, a, W - 380)
            elif i == 1:
                blit_text(im, (186, 640), cap["title"], self.lang, 66, WHITE, True, a, W - 380)
                blit_text(im, (188, 740), cap["sub"], "en", 40, MUTED, False, a)
            else:
                blit_text(im, (188, 860), cap["title"], self.lang, 34, color, True, a, W - 380)
                blit_text(im, (188, 920), cap["sub"], self.lang, 26, (110, 122, 132), False, a, W - 380)
        # three connected dots, the ImKit mark
        cx, cy = 1560, 400
        draw = ImageDraw.Draw(im)
        draw.rounded_rectangle((cx - 190, cy - 190, cx + 190, cy + 190), 84, fill=(22, 32, 38), outline=color, width=4)
        for i, c in enumerate([color, (128, 170, 255), (246, 200, 121)]):
            x = cx - 100 + i * 100
            y = cy + 20 + math.sin(t * 3 + i) * 26 * (1 - ease((local - 1.5) / 1.0))
            if i:
                draw.line((x - 100, cy + 20, x, y), fill=color, width=6)
            draw.ellipse((x - 24, y - 24, x + 24, y + 24), fill=c)

    # ------------------------------------------------------------------ frame
    def frame(self, i):
        t = i / FPS
        shot = self.shot_at(t)
        local = t - start_of(shot)
        bar = int(t // BAR) + 1
        color, chapter = self.chapter_color(bar)
        im = self.background(t, color)
        kind = shot["kind"]
        if kind == "take":
            self.draw_take(im, shot, t, local, color)
        elif kind == "chapter":
            self.draw_chapter(im, shot, local, color)
        elif kind == "code":
            self.draw_code(im, shot, t, local, color)
        elif kind == "outro":
            self.draw_outro(im, shot, t, local, color)
        for cap in shot.get("caps", []):
            if cap["t"] <= local < cap["t"] + cap["d"]:
                self.caption(im, cap["key"], local - cap["t"], cap["d"], cap.get("style", "line"), color)
        if kind != "outro":
            self.progress(im, t, color, chapter)
        self.cut_wipe(im, local, color)
        return im


# --------------------------------------------------------------------------------------------------- rendering
def render_chunk(job):
    lang, first, last, path = job
    film = Film(lang)
    proc = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}", "-r", str(FPS), "-i", "pipe:0",
                             "-c:v", "libx264", "-preset", "veryfast", "-crf", "12", "-pix_fmt", "yuv420p", "-g", str(FPS), str(path)], stdin=subprocess.PIPE)
    try:
        for i in range(first, last):
            proc.stdin.write(film.frame(i).tobytes())
    finally:
        proc.stdin.close()
        film.close()
    if proc.wait():
        raise RuntimeError(f"chunk {first}-{last} failed")
    return str(path)


def click_times():
    """Film times of every recorded mouse press, for the score's click sounds."""
    times = []
    for shot in SPEC["shots"]:
        takes = [(shot.get("take"), shot.get("in", 0), shot.get("speed", 1), start_of(shot), duration_of(shot))]
        if shot["kind"] == "code":
            takes = []
        for name, begin, speed, at, length in takes:
            if not name:
                continue
            down = False
            for line in (NATIVE / f"{name}.mkv.jsonl").read_text().splitlines():
                if '"time"' not in line:
                    continue
                e = json.loads(line)
                if e["down"] and not down:
                    local = (e["time"] - begin) / speed
                    if 0 <= local < length:
                        times.append(at + local)
                down = e["down"]
    return sorted(times)


def loudnorm(wav):
    measured = subprocess.run(["ffmpeg", "-hide_banner", "-i", str(wav), "-af", "loudnorm=I=-14:TP=-2.5:LRA=11:print_format=json", "-f", "null", "-"],
                              capture_output=True, text=True, check=True).stderr
    stats = json.loads(re.search(r"\{\s*\"input_i\"[\s\S]*?\}", measured).group())
    # The limiter keeps about 3.6 dB of headroom for the AAC overshoot. At 192k the licensed track overshot by 2.6 dB
    # (-0.83 dBTP), at 256k by 1.3 dB (-2.19 dBTP), so the film uses AUDIO_BITRATE.
    return ("loudnorm=I=-14:TP=-2.5:LRA=11:linear=true:" +
            ":".join(f"{a}={stats[b]}" for a, b in [("measured_I", "input_i"), ("measured_TP", "input_tp"), ("measured_LRA", "input_lra"), ("measured_thresh", "input_thresh"), ("offset", "target_offset")]) +
            ",alimiter=limit=0.66:level=0")


def build(lang, seconds, out_path, wav, workers):
    frames = int(round(seconds * FPS))
    step = 360
    WORK.mkdir(parents=True, exist_ok=True)
    jobs = [(lang, a, min(frames, a + step), WORK / f"{lang}-{a:06d}.mkv") for a in range(0, frames, step)]
    with ProcessPoolExecutor(workers) as pool:
        for done, path in enumerate(pool.map(render_chunk, jobs), 1):
            print(f"  [{lang}] chunk {done}/{len(jobs)}", flush=True)
    listing = WORK / f"{lang}-chunks.txt"
    listing.write_text("".join(f"file '{j[3].as_posix()}'\n" for j in jobs))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    command = ["ffmpeg", "-y", "-v", "error", "-f", "concat", "-safe", "0", "-i", str(listing)]
    if wav:
        command += ["-i", str(wav), "-af", loudnorm(wav), "-c:a", "aac", "-b:a", AUDIO_BITRATE, "-ar", "48000", "-shortest"]
    command += ["-c:v", "libx264", "-preset", "slow", "-crf", "21", "-pix_fmt", "yuv420p", "-r", str(FPS), "-movflags", "+faststart", str(out_path)]
    subprocess.run(command, check=True)
    for j in jobs:
        j[3].unlink()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--lang", choices=["ja", "en", "both"], default="both")
    parser.add_argument("--seconds", type=float, default=TOTAL)
    parser.add_argument("--review", action="store_true", help="write a contact sheet of key frames and stop")
    parser.add_argument("--no-audio", action="store_true")
    parser.add_argument("--synth", action="store_true", help="use the synthesized score even when film.json names a track")
    parser.add_argument("--audio-only", action="store_true", help="re-mux a new score into the existing films without rendering video")
    parser.add_argument("--workers", type=int, default=16)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "website/public/media")
    args = parser.parse_args()
    langs = ["ja", "en"] if args.lang == "both" else [args.lang]
    WORK.mkdir(parents=True, exist_ok=True)
    if args.review:
        film = Film(langs[0])
        times = [0.6, 3, 7, 12.5, 18, 26, 33, 38, 41, 47, 55, 63, 72, 80, 90, 100, 112, 124, 138, 150, 160, 168, 172, 178]
        sheet = Image.new("RGB", (1920, 6 * 300), BG)
        for k, t in enumerate(times):
            frame = film.frame(int(t * FPS))
            frame.save(WORK / f"review-{t:05.1f}.jpg", quality=92)
            sheet.paste(frame.resize((480, 270), Image.Resampling.LANCZOS), ((k % 4) * 480, (k // 4) * 300))
            blit_text(sheet, ((k % 4) * 480 + 8, (k // 4) * 300 + 274), f"{t:.1f}s", "en", 18, (120, 214, 198))
        film.close()
        sheet.save(WORK / f"contact-sheet-{langs[0]}.jpg", quality=92)
        print("wrote", WORK / f"contact-sheet-{langs[0]}.jpg")
        return
    wav = None
    if not args.no_audio:
        wav = WORK / "score.wav"
        cuts = [start_of(s) for s in SPEC["shots"]]
        track = SPEC.get("track")
        source = ROOT / track["file"] if track and not args.synth else None
        if source and source.is_file():
            music.render_track_score(wav, source, SPEC["bpm"], SPEC["bars"], track["offset"], cuts, click_times())
            print("score from", source.name)
        else:
            music.render_score(wav, SPEC["bpm"], SPEC["bars"], cuts, click_times())
            print("score synthesized")
    if args.audio_only:
        for lang in langs:
            target = args.output_dir / f"imkit-4.0-film-{lang}.mp4"
            tmp = target.with_suffix(".tmp.mp4")
            subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", str(target), "-i", str(wav), "-map", "0:v", "-map", "1:a", "-c:v", "copy", "-af", loudnorm(wav),
                            "-c:a", "aac", "-b:a", AUDIO_BITRATE, "-ar", "48000", "-shortest", "-movflags", "+faststart", str(tmp)], check=True)
            tmp.replace(target)
            print("remuxed", target)
        return
    for lang in langs:
        name = f"imkit-4.0-film-{lang}.mp4"
        build(lang, min(args.seconds, TOTAL), args.output_dir / name, wav, args.workers)
        print("wrote", args.output_dir / name, f"{(args.output_dir / name).stat().st_size / 1024 ** 2:.1f} MiB")
    if args.seconds >= TOTAL:
        poster = Film("en")
        poster.frame(int(12.5 * FPS)).save(args.output_dir / "imkit-4.0-poster.jpg", quality=94)
        poster.close()


if __name__ == "__main__":
    main()
