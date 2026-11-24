#!/usr/bin/env python3
"""Render 5 ultra-clean 1920x1080 studio backdrop artworks via 4K (3840x2160) supersampling.

Used when external image-generation endpoints report 503 capacity exhaustion so Wave 3 never stalls.
Saves into out/media/gemini/images/ alongside images_meta.json recording honest provenance.
"""
from __future__ import annotations

import json
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "out/media/gemini/images"
SW, SH = 3840, 2160
W, H = 1920, 1080


def _base_radial(top_rgb, bottom_rgb, glow_centers) -> np.ndarray:
    y = np.linspace(0.0, 1.0, SH, dtype=np.float32)[:, None, None]
    x = np.linspace(0.0, 1.0, SW, dtype=np.float32)[None, :, None]
    c0 = np.array(top_rgb, dtype=np.float32)[None, None, :]
    c1 = np.array(bottom_rgb, dtype=np.float32)[None, None, :]
    canvas = np.broadcast_to(c0 * (1.0 - y * 0.85) + c1 * (y * 0.85), (SH, SW, 3)).copy()

    for cx, cy, rx, ry, rgb, strength in glow_centers:
        d2 = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
        falloff = np.exp(-d2 * 1.8) * strength
        canvas += falloff * np.array(rgb, dtype=np.float32)[None, None, :]
    return canvas


def _add_bokeh(im: Image.Image, seed: int, count: int, palette) -> Image.Image:
    rng = np.random.default_rng(seed)
    layer = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    for _ in range(count):
        px = float(rng.uniform(0.0, SW))
        py = float(rng.uniform(0.0, SH))
        # Keep center calmer so overlaid UI pops
        center_dist = math.hypot((px - SW * 0.5) / SW, (py - SH * 0.5) / SH)
        if center_dist < 0.25 and rng.random() < 0.7:
            continue
        r = int(rng.uniform(4, 24))
        col = palette[int(rng.integers(0, len(palette)))]
        alpha = int(rng.uniform(18, 75))
        draw.ellipse((px - r, py - r, px + r, py + r), fill=col + (alpha,))
    layer = layer.filter(ImageFilter.GaussianBlur(8))
    return Image.alpha_composite(im.convert("RGBA"), layer)


def _save_supersampled(rgba: Image.Image, name: str) -> Path:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = OUT_DIR / name
    final = rgba.convert("RGB").resize((W, H), Image.Resampling.LANCZOS)
    final.save(out_path, optimize=True)
    print(f"[backdrop] wrote {name} ({out_path.stat().st_size} bytes)")
    return out_path


def make_bg_opening() -> Path:
    arr = _base_radial(
        (8, 12, 20),
        (5, 8, 14),
        [
            (0.16, 0.20, 0.36, 0.42, (56, 189, 248), 0.28),
            (0.84, 0.78, 0.38, 0.44, (167, 139, 250), 0.24),
            (0.50, 0.50, 0.55, 0.50, (24, 52, 74), 0.25),
        ],
    )
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)).convert("RGBA")

    # Perspective architectural micro-grid anchored near horizon
    grid = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
    gdraw = ImageDraw.Draw(grid)
    vx, vy = SW * 0.5, SH * 0.44
    for k in range(-18, 19):
        bx = vx + k * 240
        gdraw.line((vx + k * 14, vy, bx, SH), fill=(72, 209, 214, 26), width=2)
    for row in range(1, 14):
        u = (row / 13.0) ** 1.85
        ry = int(vy + u * (SH - vy))
        alpha = int(14 + 28 * u)
        gdraw.line((0, ry, SW, ry), fill=(72, 209, 214, alpha), width=2)

    # Diagonal translucent glass edge refraction planes at outer corners
    glass = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
    gl = ImageDraw.Draw(glass)
    gl.polygon([(0, 0), (1450, 0), (480, SH), (0, SH)], fill=(56, 189, 248, 22), outline=(120, 224, 240, 75))
    gl.polygon([(SW, 220), (SW, SH), (2550, SH), (3280, 220)], fill=(167, 139, 250, 20), outline=(185, 162, 255, 70))
    glass_blur = glass.filter(ImageFilter.GaussianBlur(14))

    im = Image.alpha_composite(im, grid)
    im = Image.alpha_composite(im, glass_blur)
    im = Image.alpha_composite(im, glass)
    im = _add_bokeh(im, 101, 55, [(72, 209, 214), (167, 139, 250), (186, 230, 253)])
    return _save_supersampled(im, "bg_opening.png")


def make_bg_layers() -> Path:
    arr = _base_radial(
        (9, 14, 23),
        (6, 10, 16),
        [
            (0.22, 0.30, 0.35, 0.42, (45, 212, 191), 0.24),
            (0.80, 0.36, 0.32, 0.42, (99, 102, 241), 0.22),
            (0.52, 0.85, 0.42, 0.30, (56, 189, 248), 0.16),
        ],
    )
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)).convert("RGBA")

    # Subtle 4-plane layered prism glass silhouettes near sides
    planes = [
        ([(80, 240), (980, 120), (840, 1980), (40, 2060)], (120, 214, 198)),
        ([(240, 380), (1120, 260), (960, 2080), (140, 2120)], (72, 209, 214)),
        ([(2860, 140), (3760, 260), (3720, 2040), (2720, 1920)], (129, 140, 248)),
        ([(3020, 320), (3820, 440), (3780, 2140), (2880, 2040)], (56, 189, 248)),
    ]
    for poly, col in planes:
        layer = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
        d = ImageDraw.Draw(layer)
        d.polygon(poly, fill=col + (18,), outline=col + (90,))
        im = Image.alpha_composite(im, layer.filter(ImageFilter.GaussianBlur(6)))
        im = Image.alpha_composite(im, layer)

    im = _add_bokeh(im, 202, 42, [(120, 214, 198), (99, 102, 241)])
    return _save_supersampled(im, "bg_layers.png")


def make_bg_show() -> Path:
    arr = _base_radial(
        (9, 10, 15),
        (5, 6, 10),
        [
            (0.20, 0.12, 0.34, 0.38, (246, 192, 108), 0.28),
            (0.82, 0.14, 0.34, 0.38, (56, 189, 248), 0.25),
            (0.50, 0.92, 0.45, 0.22, (246, 192, 108), 0.16),
        ],
    )
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)).convert("RGBA")

    beams = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
    bd = ImageDraw.Draw(beams)
    # Left amber laser fan
    for k in range(6):
        tx = 680 + k * 270
        bd.polygon([(460, -40), (500, -40), (tx + 140, SH + 40), (tx - 140, SH + 40)], fill=(246, 192, 108, 22 - k * 2))
    # Right cyan laser fan
    for k in range(6):
        tx = SW - 680 - k * 270
        bd.polygon([(SW - 500, -40), (SW - 460, -40), (tx + 140, SH + 40), (tx - 140, SH + 40)], fill=(56, 189, 248, 22 - k * 2))
    beams = beams.filter(ImageFilter.GaussianBlur(34))

    # Crisp thin core rays
    rays = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
    rd = ImageDraw.Draw(rays)
    for k in range(4):
        rd.line((480, 0, 760 + k * 340, SH), fill=(252, 220, 150, 32), width=3)
        rd.line((SW - 480, 0, SW - 760 - k * 340, SH), fill=(125, 225, 252, 32), width=3)
    rays = rays.filter(ImageFilter.GaussianBlur(4))

    im = Image.alpha_composite(im, beams)
    im = Image.alpha_composite(im, rays)
    im = _add_bokeh(im, 303, 60, [(246, 192, 108), (56, 189, 248)])
    return _save_supersampled(im, "bg_show.png")


def make_bg_architecture() -> Path:
    arr = _base_radial(
        (8, 14, 19),
        (5, 9, 14),
        [
            (0.14, 0.80, 0.34, 0.38, (52, 211, 153), 0.25),
            (0.86, 0.20, 0.36, 0.40, (96, 165, 250), 0.24),
        ],
    )
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)).convert("RGBA")

    fibers = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
    fd = ImageDraw.Draw(fibers)
    for lane in range(18):
        col = (52, 211, 153) if lane % 2 == 0 else (96, 165, 250)
        pts = []
        y0 = 120 + lane * 112
        for step in range(48):
            u = step / 47.0
            px = u * SW
            py = y0 + math.sin(u * math.pi * 2.2 + lane * 0.45) * 165.0 + (u - 0.5) * 320.0
            pts.append((px, py))
        fd.line(pts, fill=col + (42,), width=4)
        # Subtle fiber node
        nx, ny = pts[8 + (lane * 3) % 30]
        fd.ellipse((nx - 8, ny - 8, nx + 8, ny + 8), fill=col + (115,))

    glow = fibers.filter(ImageFilter.GaussianBlur(12))
    im = Image.alpha_composite(im, glow)
    im = Image.alpha_composite(im, fibers)
    im = _add_bokeh(im, 404, 48, [(52, 211, 153), (96, 165, 250)])
    return _save_supersampled(im, "bg_architecture.png")


def make_bg_poster() -> Path:
    arr = _base_radial(
        (7, 10, 17),
        (4, 6, 11),
        [
            (0.18, 0.24, 0.38, 0.38, (72, 209, 214), 0.30),
            (0.84, 0.28, 0.36, 0.38, (167, 139, 250), 0.28),
            (0.68, 0.82, 0.36, 0.34, (244, 114, 182), 0.22),
            (0.26, 0.84, 0.32, 0.30, (246, 192, 108), 0.16),
        ],
    )
    im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)).convert("RGBA")

    ribbons = Image.new("RGBA", (SW, SH), (0, 0, 0, 0))
    rd = ImageDraw.Draw(ribbons)
    specs = [
        (-120, 420, (72, 209, 214), 1.2),
        (180, 620, (167, 139, 250), 0.9),
        (1520, 180, (244, 114, 182), 1.1),
        (1780, 340, (246, 192, 108), 0.8),
    ]
    for y_off, amp, col, freq in specs:
        upper = []
        lower = []
        for s in range(60):
            u = s / 59.0
            px = u * SW
            cy = SH * 0.92 - u * SH * 0.78 + y_off + math.sin(u * math.pi * 2.0 * freq) * amp * 0.35
            w = 55 + 45 * math.sin(u * math.pi)
            upper.append((px, cy - w))
            lower.append((px, cy + w))
        poly = upper + list(reversed(lower))
        rd.polygon(poly, fill=col + (34,))
        rd.line(upper, fill=col + (120,), width=4)

    im = Image.alpha_composite(im, ribbons.filter(ImageFilter.GaussianBlur(26)))
    im = Image.alpha_composite(im, ribbons.filter(ImageFilter.GaussianBlur(4)))
    im = _add_bokeh(im, 505, 70, [(72, 209, 214), (167, 139, 250), (244, 114, 182), (246, 192, 108)])
    return _save_supersampled(im, "bg_poster.png")


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    files = [
        make_bg_opening(),
        make_bg_layers(),
        make_bg_show(),
        make_bg_architecture(),
        make_bg_poster(),
    ]
    meta = {
        "model": "procedural-4k-supersampled-studio-art",
        "reason": "Gemini image backend returned 503 MODEL_CAPACITY_EXHAUSTED during batch generation",
        "resolution": [W, H],
        "files": [p.name for p in files],
    }
    (OUT_DIR / "images_meta.json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
