#!/usr/bin/env python3
"""Extract selected still frames from checked-in native Gallery GIF captures."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "docs/images"
DESTINATION = ROOT / "website/src/assets/captures"
CAPTURES = (
    "v4-start", "v4-themes", "v4-compare", "v4-studio", "v4-workspace", "v4-timeline", "v4-show",
    "v4-surface", "v4-charts", "v4-toasts", "v4-icons", "v4-progress", "v4-node-editor",
)
# Frame numbers (20 fps, about 160 frames) that show the capture at its most telling moment.
POSTER_FRAMES = {
    "v4-start": 120,  # The ImKit side after the flip, slider moved.
    "v4-themes": 70,
    "v4-compare": 100,
    "v4-studio": 70,
    "v4-workspace": 90,
    "v4-timeline": 70,
    "v4-show": 100,
    "v4-surface": 80,
    "v4-charts": 80,
    "v4-toasts": 80,
    "v4-icons": 80,
    "v4-progress": 60,
    "v4-node-editor": 70,  # Actual socket drag and connection.
}


def main() -> None:
    DESTINATION.mkdir(parents=True, exist_ok=True)
    for name in CAPTURES:
        source = SOURCE / f"{name}.gif"
        output = DESTINATION / f"{name}-poster.png"
        with Image.open(source) as image:
            frame_count = getattr(image, "n_frames", 1)
            requested = POSTER_FRAMES.get(name, 0)
            frame_index = requested if requested >= 0 else frame_count + requested
            if not 0 <= frame_index < frame_count:
                raise ValueError(f"{source} has {frame_count} frames; selected frame {frame_index} is out of range")
            image.seek(frame_index)
            image.convert("RGB").save(output, format="PNG")
        print(f"{source.relative_to(ROOT)}[{frame_index}/{frame_count}] -> {output.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
