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
    parser.add_argument("--expected-frames", type=int, default=120)
    parser.add_argument("--width", type=int, default=960)
    parser.add_argument("--height", type=int, default=540)
    parser.add_argument("--fps", type=int, default=10)
    parser.add_argument("--max-bytes", type=int, default=8 * 1024 * 1024)
    args = parser.parse_args()
    if args.expected_frames < 1 or args.width < 1 or args.height < 1 or args.fps < 1 or args.max_bytes < 1:
        raise SystemExit("frame count, dimensions, fps and max bytes must be positive")
    paths = sorted(args.frames.glob("frame-*.png"))
    if len(paths) != args.expected_frames:
        raise SystemExit(f"expected {args.expected_frames} native frames, found {len(paths)}")
    images = [Image.open(path).convert("RGB") for path in paths]
    if any(image.size != (args.width, args.height) for image in images):
        raise SystemExit(f"frames must be {args.width}x{args.height}")
    samples = Image.new("RGB", (240 * 12, 135 * 10))
    for index, image in enumerate(images):
        samples.paste(image.resize((240, 135), Image.Resampling.LANCZOS), ((index % 12) * 240, (index // 12) * 135))
    palette = samples.quantize(colors=args.colors, method=Image.Quantize.MEDIANCUT)
    encoded = [image.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG) for image in images]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    encoded[0].save(args.output, save_all=True, append_images=encoded[1:], duration=round(1000 / args.fps), loop=0,
                    optimize=True, disposal=2)
    size = args.output.stat().st_size
    if size > args.max_bytes:
        raise SystemExit(f"GIF exceeds {args.max_bytes} bytes: {size} bytes")
    print(f"wrote {args.output} ({size} bytes, {len(images) / args.fps:g} seconds at {args.fps} fps)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
