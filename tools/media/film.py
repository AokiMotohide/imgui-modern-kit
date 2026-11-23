#!/usr/bin/env python3
"""Compose the ImKit 4.2 promotional film (1080p60, JA & EN):
- 100% genuine native Gallery 1080p60 recordings for every UI screen
- 3D perspective camera rig, laser wipe transform, 4-layer exploded diagram, 3-panel mosaic & 3D card wall
- Studio backdrops (generated art or animated 3D parallax), 96-bar 128 BPM score + voice ducking,
  and Gemini Flash-Lite TTS bilingual narration.
"""
from __future__ import annotations

from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
import argparse
import hashlib
import json
import math
import re
import subprocess
import sys

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
import music

ROOT = Path(__file__).resolve().parents[2]
NATIVE = ROOT / "out/media/native"
GEMINI_DIR = ROOT / "out/media/gemini"
WORK = ROOT / "out/media/film"
CACHE = WORK / "cache"
SPEC = json.loads((Path(__file__).parent / "film.json").read_text(encoding="utf-8"))
ENGINE_REV = 3
CHUNK_FRAMES = 360
FILM_NAME = "imkit-film-{lang}.mp4"
POSTER_NAME = "imkit-poster.jpg"
W, H = SPEC["size"]
FPS = SPEC["fps"]
BEAT = 60.0 / SPEC["bpm"]
BAR = 4 * BEAT
TOTAL = SPEC["bars"] * BAR
AUDIO_BITRATE = "256k"
BG = (10, 13, 19)
WHITE = (244, 247, 250)
MUTED = (162, 174, 188)
CARD = (1520, 855)
CARD_XY = ((W - CARD[0]) // 2, 148)


def clamp01(x: float) -> float:
    return max(0.0, min(1.0, x))


def ease(x: float) -> float:
    u = clamp01(x)
    return u * u * (3.0 - 2.0 * u)


def ease_out(x: float) -> float:
    u = clamp01(x)
    return 1.0 - (1.0 - u) ** 3


def ease_out_back(x: float) -> float:
    u = clamp01(x)
    c1 = 1.70158
    c3 = c1 + 1.0
    return 1.0 + c3 * (u - 1.0) ** 3 + c1 * (u - 1.0) ** 2


def start_of(shot: dict) -> float:
    return (shot["at"] - 1) * BAR


def duration_of(shot: dict) -> float:
    return shot["bars"] * BAR


def perspective_coeffs(src_pts, dst_pts):
    """Solve 8-coefficient perspective transform mapping dst (output) -> src (input) for PIL.Image.transform."""
    matrix = []
    rhs = []
    for (x, y), (u, v) in zip(dst_pts, src_pts):
        matrix.append([x, y, 1, 0, 0, 0, -u * x, -u * y])
        rhs.append(u)
        matrix.append([0, 0, 0, x, y, 1, -v * x, -v * y])
        rhs.append(v)
    a = np.asarray(matrix, dtype=np.float64)
    b = np.asarray(rhs, dtype=np.float64)
    return tuple(np.linalg.solve(a, b).tolist())


# --------------------------------------------------------------------------------------------------- footage
class VideoReader:
    """Sequential 1080p frame reader for MKV/MP4 sources; reopens only when seeking backwards or far ahead."""

    def __init__(self, path: Path, clamp_loop: bool = False):
        self.path = path
        self.clamp_loop = clamp_loop
        info = json.loads(
            subprocess.check_output(["ffprobe", "-v", "error", "-show_format", "-of", "json", str(self.path)])
        )
        self.duration = max(0.1, float(info["format"]["duration"]))
        self.proc, self.index, self.image = None, -1, None

    def close(self):
        if self.proc:
            self.proc.stdout.close()
            self.proc.terminate()
            self.proc.wait()
            self.proc = None

    def frame(self, t: float) -> Image.Image:
        if self.clamp_loop and self.duration > 0.2:
            # Ping-pong loop smoothly for short atmospheric background clips.
            cycle = 2.0 * (self.duration - 0.05)
            pos = t % max(0.1, cycle)
            t = pos if pos <= (self.duration - 0.05) else cycle - pos
        max_frame = max(0, int(self.duration * FPS) - 1)
        target = min(max(0, int(t * FPS + 1e-6)), max_frame)
        if self.image is not None and target == self.index:
            return self.image
        if self.proc is None or target < self.index or target - self.index > 90:
            self.close()
            self.proc = subprocess.Popen(
                [
                    "ffmpeg",
                    "-v",
                    "error",
                    "-threads",
                    "1",
                    "-ss",
                    f"{target / FPS:.6f}",
                    "-i",
                    str(self.path),
                    "-f",
                    "rawvideo",
                    "-pix_fmt",
                    "rgb24",
                    "pipe:1",
                ],
                stdout=subprocess.PIPE,
            )
            self.index = target - 1
        while self.index < target:
            raw = self.proc.stdout.read(W * H * 3)
            if len(raw) != W * H * 3:
                if self.image is not None:
                    return self.image
                raise RuntimeError(f"short read from {self.path.name} at frame {target}")
            self.image = Image.frombytes("RGB", (W, H), raw)
            self.index += 1
        return self.image


# --------------------------------------------------------------------------------------------------- drawing
FONTS: dict[tuple, ImageFont.FreeTypeFont] = {}


def font(lang: str, size: int, bold: bool) -> ImageFont.FreeTypeFont:
    key = (lang, int(size), bold)
    if key not in FONTS:
        if lang == "ja":
            FONTS[key] = ImageFont.truetype(str(ROOT / "assets/fonts/NotoSansJP-Regular.otf"), int(size))
        else:
            name = "Inter-SemiBold.ttf" if bold else "Inter-Regular.ttf"
            FONTS[key] = ImageFont.truetype(str(ROOT / "assets/fonts" / name), int(size))
    return FONTS[key]


def blit_text(im, xy, s, lang, size, color=WHITE, bold=False, alpha=1.0, max_width=None, anchor="la"):
    if not s or alpha <= 0.01:
        return 0.0
    f = font(lang, size, bold)
    stroke = 1 if (bold and lang == "ja") else 0
    if max_width:
        while f.getlength(s) > max_width and size > 12:
            size -= 2
            f = font(lang, size, bold)
    box = f.getbbox(s, stroke_width=stroke, anchor=anchor)
    mask = Image.new("L", (box[2] - box[0] + 4, box[3] - box[1] + 4), 0)
    ImageDraw.Draw(mask).text(
        (2 - box[0], 2 - box[1]),
        s,
        font=f,
        fill=int(255 * clamp01(alpha)),
        stroke_width=stroke,
        stroke_fill=int(255 * clamp01(alpha)),
        anchor=anchor,
    )
    im.paste(Image.new("RGB", mask.size, color), (int(xy[0] + box[0] - 2), int(xy[1] + box[1] - 2)), mask)
    return f.getlength(s)


class Layers:
    """Pre-rendered studio backdrops, glows, and masks."""

    def __init__(self):
        yy = np.linspace(0, 1, H)[:, None, None]
        base = np.array(BG, np.float32)[None, None, :] + (np.array((12, 18, 28), np.float32) * (1 - yy))
        self.background = Image.fromarray(np.broadcast_to(base, (H, W, 3)).clip(0, 255).astype(np.uint8))

        self.art_bgs: dict[str, Image.Image] = {}
        self.art_overscan: dict[str, Image.Image] = {}
        for name in ("bg_opening", "bg_layers", "bg_show", "bg_architecture", "bg_poster"):
            p = GEMINI_DIR / "images" / f"{name}.png"
            if p.is_file():
                art = Image.open(p).convert("RGB").resize((W, H), Image.Resampling.LANCZOS)
                # Blend subtly with dark studio base so foreground UI always dominates contrast.
                blended = Image.blend(self.background, art, 0.42)
                self.art_bgs[name] = blended
                # Pre-scale +8% overscan canvas for smooth subtle camera drift without per-frame high-cost LANCZOS.
                self.art_overscan[name] = blended.resize((int(W * 1.08), int(H * 1.08)), Image.Resampling.BILINEAR)

        # Pure radial Gaussian falloff that reaches exact zero before canvas edges (zero box-clipping seams).
        gsize = 900
        g_grid = np.linspace(-1.0, 1.0, gsize, dtype=np.float32)
        gx, gy = np.meshgrid(g_grid, g_grid)
        r2 = gx * gx + gy * gy
        radial = np.exp(-r2 * 5.8) * np.clip(1.0 - r2, 0.0, 1.0)
        self.glow = Image.fromarray(np.clip(radial * 255.0, 0, 255).astype(np.uint8), mode="L")
        self.masks: dict[tuple[int, int, int], Image.Image] = {}
        shadow = Image.new("L", (CARD[0] + 160, CARD[1] + 160), 0)
        ImageDraw.Draw(shadow).rounded_rectangle((80, 96, 80 + CARD[0], 96 + CARD[1]), 26, fill=205)
        self.shadow = shadow.filter(ImageFilter.GaussianBlur(28))

    def rounded_mask(self, size: tuple[int, int], radius: int = 24) -> Image.Image:
        key = (size[0], size[1], radius)
        if key not in self.masks:
            m = Image.new("L", size, 0)
            ImageDraw.Draw(m).rounded_rectangle((0, 0, size[0] - 1, size[1] - 1), radius, fill=255)
            self.masks[key] = m
        return self.masks[key]


class Film:
    def __init__(self, lang: str):
        self.lang = lang
        self.caps = json.loads((Path(__file__).parent / f"captions.{lang}.json").read_text(encoding="utf-8"))
        self.layers = Layers()
        self.readers: dict[str, VideoReader] = {}

    def take(self, name: str) -> VideoReader:
        key = f"take:{name}"
        if key not in self.readers:
            self.readers[key] = VideoReader(NATIVE / f"{name}.mkv", clamp_loop=False)
        return self.readers[key]

    def clip(self, name: str) -> VideoReader | None:
        key = f"clip:{name}"
        if key not in self.readers:
            path = GEMINI_DIR / "videos" / f"{name}.mp4"
            if not path.is_file():
                return None
            self.readers[key] = VideoReader(path, clamp_loop=True)
        return self.readers[key]

    def close(self):
        for r in self.readers.values():
            r.close()

    def chapter_color(self, bar: int) -> tuple[tuple[int, int, int], int]:
        color, index = SPEC["chapters"][0]["color"], 0
        for i, c in enumerate(SPEC["chapters"]):
            if bar >= c["first"]:
                color, index = c["color"], i
        return tuple(color), index

    def shot_at(self, t: float) -> dict:
        for shot in SPEC["shots"]:
            if start_of(shot) <= t < start_of(shot) + duration_of(shot) - 1e-9:
                return shot
        return SPEC["shots"][-1]

    def _backdrop_key(self, chapter: int) -> str | None:
        if chapter == 0:
            return "bg_opening"
        if chapter == 1:
            return "bg_layers" if "bg_layers" in self.layers.art_overscan else "bg_opening"
        if chapter == 2:
            return "bg_opening"
        if chapter == 3:
            return "bg_show"
        if chapter == 4:
            return "bg_architecture" if "bg_architecture" in self.layers.art_overscan else "bg_opening"
        return "bg_poster" if "bg_poster" in self.layers.art_overscan else "bg_opening"

    # ------------------------------------------------------------------ background & cards
    def background(self, t: float, color: tuple[int, int, int], chapter: int) -> Image.Image:
        key = self._backdrop_key(chapter)
        if key and key in self.layers.art_overscan:
            canvas = self.layers.art_overscan[key]
            max_dx = canvas.width - W
            max_dy = canvas.height - H
            dx = int((0.5 + 0.45 * math.sin(t * 0.14 + chapter)) * max_dx)
            dy = int((0.5 + 0.45 * math.cos(t * 0.11 + chapter * 0.7)) * max_dy)
            im = canvas.crop((dx, dy, dx + W, dy + H))
        else:
            im = self.layers.background.copy()

        beat_phase = (t % BEAT) / BEAT
        pulse = 1.0 - ease_out(beat_phase)
        for i, (cx, cy, scale) in enumerate([(1520, 240, 1.15), (300, 860, 0.92)]):
            x = int(cx + math.sin(t * 0.22 + i * 2.1) * 140 - 450 * scale)
            y = int(cy + math.cos(t * 0.16 + i) * 95 - 450 * scale)
            glow = self.layers.glow.resize((int(900 * scale),) * 2)
            alpha = glow.point(lambda v, p=pulse, idx=i: int(v * (0.28 + 0.12 * p) * (1.0 if idx == 0 else 0.7)))
            im.paste(Image.new("RGB", glow.size, tuple(int(c * 0.85) for c in color)), (x, y), alpha)
        return im

    def card_flat(
        self,
        im: Image.Image,
        source: Image.Image,
        rect,
        size: tuple[int, int],
        xy: tuple[int, int],
        color: tuple[int, int, int],
        radius: int = 24,
        border_width: int = 2,
        bloom: float = 0.0,
    ):
        x0, y0, x1, y1 = rect
        crop = source.crop((int(x0), int(y0), int(x1), int(y1)))
        resamp = Image.Resampling.BILINEAR if (x1 - x0) > 1500 else Image.Resampling.LANCZOS
        frame = crop.resize(size, resamp)
        mask = self.layers.rounded_mask(size, radius)
        if size == CARD:
            im.paste((4, 7, 11), (xy[0] - 80, xy[1] - 58), self.layers.shadow)
        if bloom > 0.01:
            halo = Image.new("RGBA", (size[0] + 60, size[1] + 60), (0, 0, 0, 0))
            ImageDraw.Draw(halo).rounded_rectangle(
                (30, 30, 30 + size[0], 30 + size[1]),
                radius + 6,
                outline=color + (int(190 * clamp01(bloom)),),
                width=10,
            )
            halo = halo.filter(ImageFilter.GaussianBlur(16))
            im.paste(halo, (xy[0] - 30, xy[1] - 30), halo)
        im.paste(frame, xy, mask)
        if border_width > 0:
            ImageDraw.Draw(im).rounded_rectangle(
                (xy[0], xy[1], xy[0] + size[0], xy[1] + size[1]),
                radius,
                outline=tuple(int(min(255, c * 0.65)) for c in color),
                width=border_width,
            )

    def card_3d(
        self,
        im: Image.Image,
        source: Image.Image,
        rect,
        size: tuple[int, int],
        center_xy: tuple[int, int],
        tilt: float,
        color: tuple[int, int, int],
        bloom: float = 0.0,
    ):
        """Render a UI card with genuine 3D perspective yaw/tilt and subtle border sheen."""
        if abs(tilt) < 0.004:
            self.card_flat(
                im,
                source,
                rect,
                size,
                (center_xy[0] - size[0] // 2, center_xy[1] - size[1] // 2),
                color,
                bloom=bloom,
            )
            return

        x0, y0, x1, y1 = rect
        crop = source.crop((int(x0), int(y0), int(x1), int(y1))).resize(size, Image.Resampling.BILINEAR)
        sw, sh = size
        card_rgba = crop.convert("RGBA")
        card_rgba.putalpha(self.layers.rounded_mask(size, 22))
        ImageDraw.Draw(card_rgba).rounded_rectangle(
            (1, 1, sw - 2, sh - 2), 22, outline=color + (185,), width=3
        )

        # Perspective distortion: positive tilt brings right edge closer and pushes left edge back.
        dx = int(abs(tilt) * sw * 0.14)
        dy_l = int(max(0.0, tilt) * sh * 0.085)
        dy_r = int(max(0.0, -tilt) * sh * 0.085)
        src_pts = [(0, 0), (sw, 0), (sw, sh), (0, sh)]
        dst_pts = [
            (dx, dy_l),
            (sw - dx, dy_r),
            (sw - dx, sh - dy_r),
            (dx, sh - dy_l),
        ]
        coeffs = perspective_coeffs(src_pts, dst_pts)
        warped = card_rgba.transform((sw, sh), Image.Transform.PERSPECTIVE, coeffs, Image.Resampling.BICUBIC)
        px, py = center_xy[0] - sw // 2, center_xy[1] - sh // 2

        # Soft ground shadow under tilted card
        sh_alpha = warped.getchannel("A").point(lambda v: int(v * 0.45)).filter(ImageFilter.GaussianBlur(22))
        im.paste((3, 5, 9), (px + 12, py + 26), sh_alpha)
        im.paste(warped, (px, py), warped)

    # ------------------------------------------------------------------ captions & overlays
    def caption(self, im: Image.Image, key: str, local: float, dur: float, style: str, color: tuple[int, int, int]):
        cap = self.caps[key]
        fade_in, fade_out = ease(local / 0.32), ease((dur - local) / 0.28)
        a = min(fade_in, fade_out)
        if a <= 0.01:
            return
        offset = int(18 * (1.0 - ease_out(local / 0.42)))

        if style == "title":
            veil = Image.new("L", (W, 340), int(185 * a))
            im.paste(Image.new("RGB", (W, 340), (6, 9, 14)), (0, 370), veil)
            blit_text(im, (W // 2, 435 + offset), cap["title"], self.lang, 114, WHITE, True, a, W - 200, "ma")
            blit_text(im, (W // 2, 588 + offset), cap["sub"], self.lang, 40, color, False, a, W - 240, "ma")
            return

        if style == "hero_tag":
            veil = Image.new("L", (W, 220), int(150 * a))
            im.paste(Image.new("RGB", (W, 220), (6, 9, 14)), (0, 430), veil)
            blit_text(im, (W // 2, 470 + offset), cap["title"], "en", 76, WHITE, True, a, W - 240, "ma")
            blit_text(im, (W // 2, 566 + offset), cap["sub"], self.lang, 34, color, False, a, W - 280, "ma")
            return

        # Standard top glass header bar so caption never overlaps the UI card at y=148
        blit_text(im, (CARD_XY[0], 24 + offset), cap["title"], self.lang, 46, WHITE, True, a, W - 2 * CARD_XY[0])
        blit_text(im, (CARD_XY[0] + 2, 86 + offset), cap["sub"], self.lang, 25, MUTED, False, a, W - 2 * CARD_XY[0])
        ImageDraw.Draw(im).rounded_rectangle(
            (CARD_XY[0] - 22, 32 + offset, CARD_XY[0] - 14, 114 + offset),
            4,
            fill=tuple(int(c * a + 12 * (1 - a)) for c in color),
        )

    def cut_wipe(self, im: Image.Image, local: float, color: tuple[int, int, int]):
        if local > 0.28:
            return
        u = ease(local / 0.28)
        x = int((W + 400) * u) - 200
        band = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        ImageDraw.Draw(band).polygon(
            [(x - 90, -10), (x + 40, -10), (x - 40, H + 10), (x - 170, H + 10)],
            fill=color + (int(175 * (1 - u * 0.65)),),
        )
        im.paste(band, (0, 0), band)

    def progress(self, im: Image.Image, t: float, color: tuple[int, int, int], chapter: int):
        total = len(SPEC["chapters"])
        draw = ImageDraw.Draw(im)
        for i in range(total):
            x = W // 2 - (total * 34) // 2 + i * 34
            draw.rounded_rectangle((x, 1048, x + (24 if i == chapter else 10), 1056), 4, fill=color if i == chapter else (52, 60, 72))
        draw.rectangle((0, H - 3, int(W * t / TOTAL), H), fill=color)

    # ------------------------------------------------------------------ shot renderers
    def draw_veo_intro(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        reader = self.clip(shot["clip"])
        if reader is not None:
            bg_clip = reader.frame(local)
            im.paste(Image.blend(im, bg_clip, 0.72))
        # Multi-ring animated precision architectural reticle & subtle horizon light sweep
        cx, cy = W // 2, H // 2
        dur = max(1.0, duration_of(shot))
        u = ease_out(local / 2.0)
        prog = clamp01(local / dur)
        draw = ImageDraw.Draw(im, "RGBA")
        for ring_i, (base_r, speed, w_px, a_mul) in enumerate([(440, 0.35, 2, 75), (320, -0.5, 2, 105), (195, 0.8, 1, 60)]):
            r = int(base_r - 70 * prog + 55 * (1.0 - u))
            a = int(a_mul * u)
            draw.ellipse((cx - r, cy - r, cx + r, cy + r), outline=color + (a // 2,), width=1)
            ang = math.degrees(t * speed + ring_i * 1.1)
            draw.arc((cx - r, cy - r, cx + r, cy + r), start=ang, end=ang + 78, fill=color + (a,), width=w_px)
        draw.line((100, cy, W - 100, cy), fill=color + (int(42 * u),), width=1)
        draw.line((cx, 90, cx, H - 90), fill=color + (int(42 * u),), width=1)

    def draw_laser_transform(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        src_t = shot["in"] + local * shot["speed"]
        live_src = self.take(shot["take"]).frame(src_t)
        # During local 0..3.6s, show plain Dear ImGui vs laser-swept ImKit 4.2 preview!
        if 2.2 <= local <= 4.4:
            plain_src = self.take(shot["take"]).frame(0.5)
            modern_src = self.take(shot["take"]).frame(5.4)
            u = ease((local - 2.2) / 1.8)
            cut_x = int(W * u)
            merged = plain_src.copy()
            if cut_x > 0:
                merged.paste(modern_src.crop((0, 0, cut_x, H)), (0, 0))
            # Glowing laser scan beam across the transform edge
            if 0.02 < u < 0.98:
                beam = Image.new("RGBA", (W, H), (0, 0, 0, 0))
                bdraw = ImageDraw.Draw(beam)
                bdraw.rectangle((cut_x - 28, 0, cut_x + 28, H), fill=color + (80,))
                bdraw.rectangle((cut_x - 4, 0, cut_x + 4, H), fill=(240, 252, 255, 240))
                beam = beam.filter(ImageFilter.GaussianBlur(8))
                merged = Image.alpha_composite(merged.convert("RGBA"), beam).convert("RGB")
            source = merged
        else:
            source = live_src

        rect = (0, 0, 1920, 1080)
        tilt = 0.0
        keys = shot.get("camera")
        if keys:
            for a, b in zip(keys, keys[1:]):
                if a["t"] <= local <= b["t"]:
                    u = ease((local - a["t"]) / max(1e-6, b["t"] - a["t"]))
                    rect = [p + (q - p) * u for p, q in zip(a["rect"], b["rect"])]
                    tilt = a.get("tilt", 0.0) + (b.get("tilt", 0.0) - a.get("tilt", 0.0)) * u
                    break
            else:
                last = keys[0] if local < keys[0]["t"] else keys[-1]
                rect, tilt = last["rect"], last.get("tilt", 0.0)

        pop = ease_out(local / 0.45)
        scale = 0.94 + 0.06 * pop
        size = (int(CARD[0] * scale), int(CARD[1] * scale))
        self.card_3d(im, source, rect, size, (W // 2, CARD_XY[1] + CARD[1] // 2), tilt, color, bloom=0.35 if local > 3.5 else 0.0)

    def draw_take_3d(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        src_t = shot["in"] + local * shot["speed"]
        source = self.take(shot["take"]).frame(src_t)
        pop = ease_out(local / 0.42)
        base_tilt = shot.get("tilt", 0.0)
        dur = duration_of(shot)
        # Gentle cinematic drift from initial 3D tilt toward near-flat readability
        tilt = base_tilt * (1.0 - 0.65 * ease(local / max(0.5, dur)))
        scale = 0.93 + 0.07 * pop
        size = (int(CARD[0] * scale), int(CARD[1] * scale))
        bloom = 0.55 if shot.get("glow_bloom") else 0.0
        if shot.get("beat_flash"):
            beat_u = 1.0 - ease_out((t % BEAT) / BEAT)
            bloom = max(bloom, 0.45 * beat_u)
        self.card_3d(im, source, (0, 0, 1920, 1080), size, (W // 2, CARD_XY[1] + CARD[1] // 2), tilt, color, bloom=bloom)

    def draw_layers_3d(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        """3D exploded glass stack showing ImKit 4.2's 4 customization layers alongside live Theme Studio."""
        layers_meta = [
            ("L1", "Theme Text", "button.hover.fill = accent", (72, 209, 214)),
            ("L2", "Typed Style & StyleScope", "StyleScope(\"toolbar\");", (167, 139, 250)),
            ("L3", "Skin", "theme.skin = &mySkin;", (246, 192, 108)),
            ("L4", "Interact & Draw", "Interact() / DrawSmoothRect()", (52, 211, 153)),
        ]
        dur = duration_of(shot)
        active_idx = min(3, int(local / max(0.5, dur / 4.0)))
        draw = ImageDraw.Draw(im, "RGBA")

        base_x, base_y = 140, 220
        box_w, box_h = 700, 132
        for i, (code, title, snippet, lc) in enumerate(layers_meta):
            enter = ease_out_back((local - i * 0.25) / 0.55)
            is_on = i == active_idx
            lift = -14 if is_on else 0
            x0 = base_x + i * 18 + int((1.0 - enter) * -80)
            y0 = base_y + i * 176 + lift
            fill_a = 215 if is_on else 145
            border_w = 3 if is_on else 1
            draw.rounded_rectangle((x0 + 8, y0 + 12, x0 + box_w + 8, y0 + box_h + 12), 20, fill=(4, 7, 12, 130))
            draw.rounded_rectangle(
                (x0, y0, x0 + box_w, y0 + box_h),
                20,
                fill=(18, 25, 38, fill_a),
                outline=lc + (245 if is_on else 110,),
                width=border_w,
            )
            draw.rounded_rectangle((x0 + 22, y0 + 24, x0 + 82, y0 + 72), 12, fill=lc + (235,))
            blit_text(im, (x0 + 52, y0 + 32), code, "en", 28, (10, 14, 20), True, enter, anchor="ma")
            blit_text(im, (x0 + 104, y0 + 24), title, "en", 28, WHITE, True, enter)
            blit_text(im, (x0 + 104, y0 + 68), snippet, "en", 21, lc if is_on else MUTED, False, enter)

        # Live Gallery Theme Studio card on the right
        src_t = shot["in"] + local * shot["speed"]
        source = self.take(shot["take"]).frame(src_t)
        self.card_3d(im, source, (0, 0, 1920, 1080), (880, 495), (1380, 560), 0.04, color, bloom=0.25)

    def draw_split_dual(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        left = shot["left"]
        right = shot["right"]
        src_l = self.take(left["take"]).frame(left["in"] + local * left["speed"])
        src_r = self.take(right["take"]).frame(right["in"] + local * right["speed"])
        pop = ease_out(local / 0.35)
        w_card = int(830 * (0.94 + 0.06 * pop))
        h_card = int(467 * (0.94 + 0.06 * pop))
        self.card_3d(im, src_l, (0, 0, 1920, 1080), (w_card, h_card), (515, 560), -0.05, color)
        self.card_3d(im, src_r, (0, 0, 1920, 1080), (w_card, h_card), (1405, 560), 0.05, color)
        blit_text(im, (515, 830), left["label"], "en", 26, color, True, pop, anchor="ma")
        blit_text(im, (1405, 830), right["label"], "en", 26, color, True, pop, anchor="ma")

    def draw_veo_bridge(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        reader = self.clip(shot["clip"])
        if reader is not None:
            im.paste(Image.blend(im, reader.frame(local), 0.78))
        if shot.get("inset_take"):
            u = ease((local - duration_of(shot) * 0.45) / 1.5)
            if u > 0.02:
                src = self.take(shot["inset_take"]).frame(local * 1.1)
                size = (int(960 * (0.9 + 0.1 * u)), int(540 * (0.9 + 0.1 * u)))
                self.card_3d(im, src, (0, 0, 1920, 1080), size, (W // 2, H // 2 + 40), 0.0, color, bloom=0.35 * u)

    def draw_mosaic_3(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        t0, t1, t2 = shot["takes"]
        s0 = self.take(t0["take"]).frame(t0["in"] + local * t0["speed"])
        s1 = self.take(t1["take"]).frame(t1["in"] + local * t1["speed"])
        s2 = self.take(t2["take"]).frame(t2["in"] + local * t2["speed"])
        pop = ease_out(local / 0.4)
        self.card_flat(im, s0, (0, 0, 1920, 1080), (1040, 585), (90, 210), color, radius=20, bloom=0.22 * pop)
        self.card_flat(im, s1, (0, 0, 1920, 1080), (670, 377), (1160, 175), color, radius=18)
        self.card_flat(im, s2, (0, 0, 1920, 1080), (670, 377), (1160, 590), color, radius=18)
        blit_text(im, (108, 816), t0["label"], "en", 24, color, True, pop)
        blit_text(im, (1176, 560), t1["label"], "en", 20, MUTED, True, pop)
        blit_text(im, (1176, 976), t2["label"], "en", 20, MUTED, True, pop)

    def draw_code(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        panel = (110, 185, 980, 745)
        draw = ImageDraw.Draw(im, "RGBA")
        draw.rounded_rectangle((panel[0] + 10, panel[1] + 16, panel[2] + 10, panel[3] + 16), 24, fill=(4, 7, 12, 160))
        draw.rounded_rectangle(panel, 22, fill=(15, 20, 29, 235), outline=color + (165,), width=2)
        for j, dc in enumerate([(244, 114, 182), (246, 192, 108), (52, 211, 153)]):
            draw.ellipse((panel[0] + 28 + j * 24, panel[1] + 24, panel[0] + 40 + j * 24, panel[1] + 36), fill=dc)
        tabs = ["CMakeLists.txt", "main.cpp"]
        for i, label in enumerate(tabs):
            on = (local < 5.6) == (i == 0)
            blit_text(im, (panel[0] + 130 + i * 210, panel[1] + 20), label, "en", 22, color if on else (112, 124, 138), True)

        blocks = [
            [
                "set(IMKIT_IMGUI_TARGET host_imgui)",
                "add_subdirectory(external/imgui-modern-kit)",
                "target_link_libraries(your_app PRIVATE imkit::imkit)",
            ],
            [
                "auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);",
                "imkit::ApplyTheme(theme);  // before ImGui::NewFrame()",
                "imkit::Toggle(\"Enabled\", &enabled);  // same frame loop",
            ],
        ]
        index = 0 if local < 5.6 else 1
        lines = blocks[index]
        started = 0.35 if index == 0 else 5.8
        shown = int(max(0.0, (local - started)) * 92)
        y = panel[1] + 115
        cur_x, cur_y = panel[0] + 38, y
        for line in lines:
            count = min(len(line), max(0, shown))
            shown -= len(line)
            vis = line[:count]
            code, sep, comment = vis.partition("//")
            w_px = blit_text(im, (panel[0] + 38, y), code, "en", 22, WHITE)
            total_w = w_px
            if sep or comment:
                c_w = blit_text(im, (panel[0] + 38 + w_px, y), "//" + comment, "en", 22, (124, 152, 168))
                total_w += c_w
            if count > 0 or line == lines[0]:
                cur_x = panel[0] + 42 + int(total_w)
                cur_y = y
            y += 96
        if int(local * 2.5) % 2 == 0:
            draw.rectangle((cur_x, cur_y, cur_x + 10, cur_y + 28), fill=color)

        inset = shot["inset"]
        src = self.take(inset["take"]).frame(inset["in"] + (local % 6.5) * inset["speed"])
        self.card_3d(im, src, (1010, 24, 1890, 520), (760, 428), (1440, 445), 0.04, color, bloom=0.28)
        label = "テーマを足すだけで即反映" if self.lang == "ja" else "Live theme result"
        blit_text(im, (1440, 695), label, self.lang, 28, color, True, anchor="ma")

    def draw_arch_kinetic(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        nodes = [
            ("Your ImGui Context", (220, 230)),
            ("Your GPU Renderer", (220, 390)),
            ("Your Font Atlas", (220, 550)),
            ("Your Data & Undo", (220, 710)),
        ]
        hub = (790, 470)
        draw = ImageDraw.Draw(im, "RGBA")
        for idx, (label, (nx, ny)) in enumerate(nodes):
            u = ease_out((local - idx * 0.25) / 0.5)
            draw.line((nx + 180, ny + 42, hub[0] - 130, hub[1]), fill=color + (int(120 * u),), width=3)
            # Animated data packet dot traveling from app -> hub
            p_u = ((t * 0.7 + idx * 0.25) % 1.0)
            px = nx + 180 + (hub[0] - 130 - (nx + 180)) * p_u
            py = (ny + 42) + (hub[1] - (ny + 42)) * p_u
            draw.ellipse((px - 6, py - 6, px + 6, py + 6), fill=color + (235,))

            draw.rounded_rectangle((nx - 100, ny, nx + 200, ny + 84), 16, fill=(18, 25, 38, 220), outline=color + (150,), width=2)
            blit_text(im, (nx + 50, ny + 26), label, "en", 22, WHITE, True, u, anchor="ma")

        draw.rounded_rectangle((hub[0] - 140, hub[1] - 90, hub[0] + 150, hub[1] + 90), 26, fill=(22, 34, 52, 235), outline=color + (235,), width=3)
        blit_text(im, (hub[0] + 5, hub[1] - 32), "ImKit 4.2", "en", 36, WHITE, True, 1.0, anchor="ma")
        blit_text(im, (hub[0] + 5, hub[1] + 20), "C++20 Static Lib", "en", 21, color, False, 1.0, anchor="ma")

        inset = shot["inset"]
        src = self.take(inset["take"]).frame(inset["in"] + (local % 5.5) * inset["speed"])
        self.card_3d(im, src, (1010, 24, 1890, 520), (760, 428), (1440, 470), 0.04, color, bloom=0.25)

    def draw_card_wall_3d(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        reader = self.clip(shot["clip"])
        if reader is not None:
            im.paste(Image.blend(im, reader.frame(local), 0.65))
        takes = shot["takes"]
        cols, rows = 3, 3
        cw, ch = 500, 281
        drift_x = int(-110 + local * 18)
        for idx, name in enumerate(takes[:9]):
            c = idx % cols
            r = idx // cols
            src = self.take(name).frame((local * 1.4 + idx * 1.1) % 8.0)
            cx = 360 + c * 590 + drift_x + (r - 1) * 45
            cy = 250 + r * 315
            pop = ease_out((local - idx * 0.08) / 0.45)
            if pop > 0.02:
                self.card_3d(im, src, (0, 0, 1920, 1080), (int(cw * pop), int(ch * pop)), (cx, cy), -0.05, color)

    def draw_outro(self, im: Image.Image, shot: dict, t: float, local: float, color: tuple[int, int, int]):
        reader = self.clip(shot["clip"])
        if reader is not None:
            im.paste(Image.blend(im, reader.frame(local + 4.0), 0.62))
        veil = Image.new("L", (W, H), 120)
        im.paste(Image.new("RGB", (W, H), (7, 10, 16)), (0, 0), veil)

        u = ease_out(local / 0.65)
        for i, key in enumerate(["outro-1", "outro-2", "outro-3"]):
            cap = self.caps[key]
            a = ease((local - i * 0.55) / 0.55)
            if i == 0:
                blit_text(im, (180 - int(56 * (1 - u)), 195), cap["title"], "en", 210, WHITE, True, a)
                blit_text(im, (186, 465), cap["sub"], self.lang, 50, color, True, a, W - 440)
            elif i == 1:
                blit_text(im, (186, 625), cap["title"], self.lang, 60, WHITE, True, a, W - 420)
                blit_text(im, (188, 720), cap["sub"], "en", 36, MUTED, False, a)
            else:
                blit_text(im, (188, 845), cap["title"], self.lang, 30, color, True, a, W - 420)
                blit_text(im, (188, 902), cap["sub"], self.lang, 22, (132, 146, 164), False, a, W - 420)

        cx, cy = 1560, 410
        draw = ImageDraw.Draw(im, "RGBA")
        draw.rounded_rectangle((cx - 190, cy - 190, cx + 190, cy + 190), 84, fill=(18, 26, 38, 235), outline=color + (235,), width=4)
        for i, c in enumerate([color, (167, 139, 250), (246, 192, 108)]):
            x = cx - 100 + i * 100
            y = cy + 20 + math.sin(t * 3.0 + i) * 24 * (1 - ease((local - 1.8) / 1.0))
            if i:
                draw.line((x - 100, cy + 20, x, y), fill=color + (235,), width=6)
            draw.ellipse((x - 24, y - 24, x + 24, y + 24), fill=c + (255,))

    # ------------------------------------------------------------------ frame router
    def frame(self, i: int) -> Image.Image:
        t = i / FPS
        shot = self.shot_at(t)
        local = t - start_of(shot)
        bar = int(t // BAR) + 1
        color, chapter = self.chapter_color(bar)
        im = self.background(t, color, chapter)
        kind = shot["kind"]
        if kind == "veo_intro":
            self.draw_veo_intro(im, shot, t, local, color)
        elif kind == "laser_transform":
            self.draw_laser_transform(im, shot, t, local, color)
        elif kind == "chapter":
            cap = self.caps[shot["key"]]
            u = ease_out(local / 0.45)
            a = min(1.0, ease(local / 0.22), ease((duration_of(shot) - local) / 0.22))
            blit_text(im, (160 - int(70 * (1 - u)), 230), shot["num"], "en", 320, tuple(int(c * 0.55) for c in color), True, a)
            blit_text(im, (172 - int(40 * (1 - u)), 615), cap["title"], self.lang, 104, WHITE, True, a, W - 340)
            blit_text(im, (176 - int(40 * (1 - u)), 765), cap["sub"], self.lang, 36, MUTED, False, a, W - 360)
            ImageDraw.Draw(im).rounded_rectangle((172, 588, 172 + int(520 * u), 596), 4, fill=color)
        elif kind == "take_3d":
            self.draw_take_3d(im, shot, t, local, color)
        elif kind == "layers_3d":
            self.draw_layers_3d(im, shot, t, local, color)
        elif kind == "split_dual":
            self.draw_split_dual(im, shot, t, local, color)
        elif kind == "veo_bridge":
            self.draw_veo_bridge(im, shot, t, local, color)
        elif kind == "mosaic_3":
            self.draw_mosaic_3(im, shot, t, local, color)
        elif kind == "code":
            self.draw_code(im, shot, t, local, color)
        elif kind == "arch_kinetic":
            self.draw_arch_kinetic(im, shot, t, local, color)
        elif kind == "card_wall_3d":
            self.draw_card_wall_3d(im, shot, t, local, color)
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
    part = path.with_suffix(".part.mkv")
    proc = subprocess.Popen(
        [
            "ffmpeg",
            "-y",
            "-v",
            "error",
            "-f",
            "rawvideo",
            "-pix_fmt",
            "rgb24",
            "-s",
            f"{W}x{H}",
            "-r",
            str(FPS),
            "-i",
            "pipe:0",
            "-c:v",
            "libx264",
            "-threads",
            "2",
            "-preset",
            "veryfast",
            "-crf",
            "12",
            "-pix_fmt",
            "yuv420p",
            "-g",
            str(FPS),
            str(part),
        ],
        stdin=subprocess.PIPE,
    )
    try:
        for i in range(first, last):
            proc.stdin.write(film.frame(i).tobytes())
    finally:
        proc.stdin.close()
        film.close()
    if proc.wait():
        raise RuntimeError(f"chunk {first}-{last} failed")
    part.replace(path)
    return str(path)


_DIGESTS: dict[str, dict] = {}


def file_digest(path: Path) -> str:
    key = str(path.relative_to(ROOT))
    stat = path.stat()
    memo = NATIVE / "digests.json"
    if not _DIGESTS and memo.is_file():
        _DIGESTS.update(json.loads(memo.read_text(encoding="utf-8")))
    stamp = f"{stat.st_size}:{stat.st_mtime_ns}"
    entry = _DIGESTS.get(key)
    if not entry or entry["stamp"] != stamp:
        h = hashlib.sha256()
        with path.open("rb") as f:
            for block in iter(lambda: f.read(1 << 24), b""):
                h.update(block)
        _DIGESTS[key] = {"stamp": stamp, "sha256": h.hexdigest()}
        memo.write_text(json.dumps(_DIGESTS, indent=1), encoding="utf-8")
    return _DIGESTS[key]["sha256"]


def chunk_key(lang: str, first: int, last: int, captions: dict) -> str:
    t0, t1 = first / FPS, last / FPS
    shots = [s for s in SPEC["shots"] if start_of(s) < t1 and start_of(s) + duration_of(s) > t0 - 1e-9]
    keys = {cap["key"] for s in shots for cap in s.get("caps", [])} | {s["key"] for s in shots if "key" in s}
    if any(s["kind"] == "outro" for s in shots):
        keys |= {"outro-1", "outro-2", "outro-3"}

    takes = set()
    clips = set()
    for s in shots:
        for tname in [s.get("take"), s.get("inset_take"), s.get("inset", {}).get("take"), s.get("left", {}).get("take"), s.get("right", {}).get("take")]:
            if tname:
                takes.add(tname)
        for item in s.get("takes", []):
            takes.add(item["take"] if isinstance(item, dict) else item)
        if s.get("clip"):
            clips.add(s["clip"])

    asset_hashes = {}
    for name in sorted(takes):
        asset_hashes[f"take:{name}"] = file_digest(NATIVE / f"{name}.mkv")
    for cname in sorted(clips):
        cp = GEMINI_DIR / "videos" / f"{cname}.mp4"
        if cp.is_file():
            asset_hashes[f"clip:{cname}"] = file_digest(cp)
    for iname in ("bg_opening", "bg_layers", "bg_show", "bg_architecture", "bg_poster"):
        ip = GEMINI_DIR / "images" / f"{iname}.png"
        if ip.is_file():
            asset_hashes[f"img:{iname}"] = file_digest(ip)

    payload = {
        "engine": ENGINE_REV,
        "lang": lang,
        "frames": [first, last],
        "format": [W, H, FPS, SPEC["bpm"], SPEC["bars"]],
        "chapters": SPEC["chapters"],
        "shots": shots,
        "captions": {k: captions[k] for k in sorted(keys)},
        "assets": asset_hashes,
    }
    return hashlib.sha256(json.dumps(payload, sort_keys=True).encode()).hexdigest()[:16]


def chunk_plan(lang: str, frames: int, force: bool = False):
    captions = json.loads((Path(__file__).parent / f"captions.{lang}.json").read_text(encoding="utf-8"))
    folder = CACHE / lang
    folder.mkdir(parents=True, exist_ok=True)
    plan = []
    for first in range(0, frames, CHUNK_FRAMES):
        last = min(frames, first + CHUNK_FRAMES)
        path = folder / f"{first:06d}-{chunk_key(lang, first, last, captions)}.mkv"
        plan.append((first, last, path, path.is_file() and not force))
    return plan


def print_status(lang: str, plan):
    stale = [(f / FPS, l / FPS) for f, l, _, cached in plan if not cached]
    print(f"[{lang}] {len(plan) - len(stale)} of {len(plan)} chunks cached", end="")
    print(": " + ", ".join(f"{a:.0f}-{b:.0f}s" for a, b in stale) + " stale" if stale else ": nothing to render")


def click_times():
    times = []
    for shot in SPEC["shots"]:
        entries = []
        if shot.get("take") and shot.get("in") is not None:
            entries.append((shot["take"], shot["in"], shot.get("speed", 1.0), start_of(shot), duration_of(shot)))
        for name, begin, speed, at, length in entries:
            jpath = NATIVE / f"{name}.mkv.jsonl"
            if not jpath.is_file():
                continue
            down = False
            for line in jpath.read_text().splitlines():
                if '"time"' not in line:
                    continue
                e = json.loads(line)
                if e["down"] and not down:
                    local = (e["time"] - begin) / speed
                    if 0 <= local < length:
                        times.append(at + local)
                down = e["down"]
    return sorted(times)


def loudnorm(wav: Path) -> str:
    measured = subprocess.run(
        ["ffmpeg", "-hide_banner", "-i", str(wav), "-af", "loudnorm=I=-14:TP=-2.5:LRA=11:print_format=json", "-f", "null", "-"],
        capture_output=True,
        text=True,
        check=True,
    ).stderr
    stats = json.loads(re.search(r"\{\s*\"input_i\"[\s\S]*?\}", measured).group())
    return (
        "loudnorm=I=-14:TP=-2.5:LRA=11:linear=true:"
        + ":".join(
            f"{a}={stats[b]}"
            for a, b in [
                ("measured_I", "input_i"),
                ("measured_TP", "input_tp"),
                ("measured_LRA", "input_lra"),
                ("measured_thresh", "input_thresh"),
                ("offset", "target_offset"),
            ]
        )
        + ",alimiter=limit=0.66:level=0"
    )


def render_audio_for_lang(lang: str, synth: bool) -> Path:
    wav = WORK / f"score-{lang}.wav"
    cuts = [start_of(s) for s in SPEC["shots"]]
    clicks = click_times()
    laser_times = [5.625, 7.825]
    lyria_wav = GEMINI_DIR / "music/lyria_score.wav"
    narr_manifest_path = GEMINI_DIR / f"tts/{lang}/manifest.json"
    narr_manifest = json.loads(narr_manifest_path.read_text(encoding="utf-8")) if narr_manifest_path.is_file() else None

    if lyria_wav.is_file() and not synth:
        bgm_source = lyria_wav
        source_label = "Lyria 3.5"
    else:
        bgm_source = WORK / "synth-bgm-96bars.wav"
        if not bgm_source.is_file() or synth:
            bgm_source.parent.mkdir(parents=True, exist_ok=True)
            music.render_score(bgm_source, SPEC["bpm"], SPEC["bars"], (), ())
        source_label = "96-bar synthesized 128 BPM score"

    music.render_gemini_score(wav, bgm_source, SPEC["bpm"], SPEC["bars"], narr_manifest, cuts, clicks, laser_times)
    print(f"[{lang}] score mixed from {source_label} + TTS ({len(narr_manifest or {})} lines, ducking + SFX)")
    return wav


def build(lang: str, seconds: float, out_path: Path, wav: Path | None, workers: int, force: bool = False):
    frames = int(round(seconds * FPS))
    WORK.mkdir(parents=True, exist_ok=True)
    plan = chunk_plan(lang, frames, force)
    print_status(lang, plan)
    jobs = [(lang, first, last, path) for first, last, path, cached in plan if not cached]
    if jobs:
        with ProcessPoolExecutor(workers) as pool:
            for done, _ in enumerate(pool.map(render_chunk, jobs), 1):
                print(f"  [{lang}] drew chunk {done}/{len(jobs)}", flush=True)
    listing = WORK / f"{lang}-chunks.txt"
    listing.write_text("".join(f"file '{path.as_posix()}'\n" for _, _, path, _ in plan), encoding="utf-8")
    if seconds >= TOTAL:
        for stale in (CACHE / lang).glob("*.mkv"):
            if stale not in {path for _, _, path, _ in plan}:
                stale.unlink()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    command = ["ffmpeg", "-y", "-v", "error", "-f", "concat", "-safe", "0", "-i", str(listing)]
    if wav:
        command += ["-i", str(wav), "-af", loudnorm(wav), "-c:a", "aac", "-b:a", AUDIO_BITRATE, "-ar", "48000", "-shortest"]
    command += [
        "-c:v",
        "libx264",
        "-preset",
        "slow",
        "-crf",
        "21",
        "-pix_fmt",
        "yuv420p",
        "-r",
        str(FPS),
        "-movflags",
        "+faststart",
        str(out_path),
    ]
    subprocess.run(command, check=True)


def write_provenance(output_dir: Path, films: dict[str, Path]):
    path = output_dir / "provenance.json"
    has_lyria = (GEMINI_DIR / "music/lyria_score.wav").is_file()
    has_omni = any((GEMINI_DIR / "videos" / f"{n}.mp4").is_file() for n in ("clip_intro_bg", "clip_stage_bridge", "clip_finale_bg"))
    art_meta_path = GEMINI_DIR / "images/images_meta.json"
    art_model = "antigravity-image-generator"
    if art_meta_path.is_file():
        art_model = json.loads(art_meta_path.read_text(encoding="utf-8")).get("model", art_model)
    tts_meta_path = GEMINI_DIR / "tts/tts_meta.json"
    tts_model = "gemini-3.8-flash-tts / gemini-3.8-flash-lite-tts / gemini-3.1-flash-tts-preview"
    if tts_meta_path.is_file():
        tts_model = json.loads(tts_meta_path.read_text(encoding="utf-8")).get("model", tts_model)
    record = {
        "edition": SPEC["edition"],
        "license": "MIT; repository font and icon notices retained",
        "duration": round(TOTAL),
        "fps": FPS,
        "resolution": [W, H],
        "languages": ["ja", "en"],
        "shots": len(SPEC["shots"]),
        "music": {
            "title": "ImKit 4.2 Original Score",
            "model": "lyria-3.5" if has_lyria else "procedural-numpy-96bar",
            "bpm": SPEC["bpm"],
            "bars": SPEC["bars"],
            "license": "Original repository instrumental score; MIT",
            "use": (
                "Synthesized 96-bar 128 BPM electronic score mixed with automated voice-ducking (-9 dB) and recorded click/cut/laser SFX."
                if not has_lyria
                else "Generated from native Gallery 4.2 screenshots at 128 BPM, mixed with automated voice-ducking and UI SFX."
            ),
        },
        "narration": {
            "model": tts_model,
            "voices": {"ja": "Aoede", "en": "Charon"},
            "lines_per_language": 19,
        },
        "cinematic_backdrops": {
            "video_model": "gemini-omni-1.2-flash" if has_omni else "procedural-3d-parallax",
            "image_model": art_model,
            "review_model": "antigravity-multimodal-director",
        },
        "fonts": "Inter and Noto Sans JP (SIL OFL), rasterized only; no font binaries are redistributed",
        "visuals": "Every UI screen is recorded from the native Gallery and Node Editor OpenGL backbuffers through public Dear ImGui IO, composed with 3D perspective motion graphics and layered studio backdrops.",
        "scripts": [
            "tools/media/capture.py",
            "tools/media/generate_gemini_assets.py",
            "tools/media/film.py",
            "tools/media/music.py",
            "examples/gallery/main.cpp (CaptureMotion)",
        ],
        "validate": "tools/media/verify.py",
        "sha256": {lang: hashlib.sha256(file.read_bytes()).hexdigest() for lang, file in films.items()},
    }
    path.write_text(json.dumps(record, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--lang", choices=["ja", "en", "both"], default="both")
    parser.add_argument("--seconds", type=float, default=TOTAL)
    parser.add_argument("--review", action="store_true")
    parser.add_argument("--no-audio", action="store_true")
    parser.add_argument("--synth", action="store_true")
    parser.add_argument("--audio-only", action="store_true")
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--status", action="store_true")
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "website/public/media")
    args = parser.parse_args()
    langs = ["ja", "en"] if args.lang == "both" else [args.lang]
    WORK.mkdir(parents=True, exist_ok=True)

    if args.review:
        film = Film(langs[0])
        times = [1.5] + [start_of(s) + duration_of(s) * 0.5 for s in SPEC["shots"] if s["kind"] != "chapter"]
        sheet = Image.new("RGB", (1920, ((len(times) + 3) // 4) * 300), BG)
        for k, t in enumerate(times):
            frame = film.frame(int(t * FPS))
            frame.save(WORK / f"review-{t:05.1f}.jpg", quality=92)
            sheet.paste(frame.resize((480, 270), Image.Resampling.LANCZOS), ((k % 4) * 480, (k // 4) * 300))
            blit_text(sheet, ((k % 4) * 480 + 8, (k // 4) * 300 + 274), f"{t:.1f}s", "en", 18, (120, 214, 198))
        film.close()
        sheet.save(WORK / f"contact-sheet-{langs[0]}.jpg", quality=92)
        print("wrote", WORK / f"contact-sheet-{langs[0]}.jpg")
        return

    if args.status:
        for lang in langs:
            print_status(lang, chunk_plan(lang, int(round(min(args.seconds, TOTAL) * FPS)), args.force))
        return

    for lang in langs:
        wav = None if args.no_audio else render_audio_for_lang(lang, args.synth)
        target = args.output_dir / FILM_NAME.format(lang=lang)
        if args.audio_only:
            tmp = target.with_suffix(".tmp.mp4")
            subprocess.run(
                [
                    "ffmpeg",
                    "-y",
                    "-v",
                    "error",
                    "-i",
                    str(target),
                    "-i",
                    str(wav),
                    "-map",
                    "0:v",
                    "-map",
                    "1:a",
                    "-c:v",
                    "copy",
                    "-af",
                    loudnorm(wav),
                    "-c:a",
                    "aac",
                    "-b:a",
                    AUDIO_BITRATE,
                    "-ar",
                    "48000",
                    "-shortest",
                    "-movflags",
                    "+faststart",
                    str(tmp),
                ],
                check=True,
            )
            tmp.replace(target)
            print("remuxed", target)
        else:
            build(lang, min(args.seconds, TOTAL), target, wav, args.workers, args.force)
            print("wrote", target, f"{target.stat().st_size / 1024 ** 2:.1f} MiB")

    if args.seconds >= TOTAL and not args.audio_only:
        poster = Film("en")
        poster.frame(int(SPEC.get("posterSeconds", 11.5) * FPS)).save(args.output_dir / POSTER_NAME, quality=94)
        poster.close()
        if len(langs) == 2:
            write_provenance(args.output_dir, {lang: args.output_dir / FILM_NAME.format(lang=lang) for lang in langs})


if __name__ == "__main__":
    main()
