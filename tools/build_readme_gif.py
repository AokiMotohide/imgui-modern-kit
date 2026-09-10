#!/usr/bin/env python3
"""Build the README animation from native Gallery PNG backbuffer frames."""
import argparse
from pathlib import Path

from PIL import Image


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("frames", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--colors", type=int, default=128)
    args = parser.parse_args()
    paths = sorted(args.frames.glob("frame-*.png"))
    if len(paths) != 120:
        raise SystemExit(f"expected 120 native frames, found {len(paths)}")
    images = [Image.open(path).convert("RGB") for path in paths]
    if any(image.size != (960, 540) for image in images):
        raise SystemExit("README frames must be 960x540")
    samples = Image.new("RGB", (240 * 12, 135 * 10))
    for index, image in enumerate(images):
        samples.paste(image.resize((240, 135), Image.Resampling.LANCZOS), ((index % 12) * 240, (index // 12) * 135))
    palette = samples.quantize(colors=args.colors, method=Image.Quantize.MEDIANCUT)
    encoded = [image.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG) for image in images]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    encoded[0].save(args.output, save_all=True, append_images=encoded[1:], duration=100, loop=0,
                    optimize=True, disposal=2)
    size = args.output.stat().st_size
    if size > 8 * 1024 * 1024:
        raise SystemExit(f"GIF exceeds 8 MiB: {size} bytes")
    print(f"wrote {args.output} ({size} bytes, 12 seconds at 10 fps)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
