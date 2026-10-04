#!/usr/bin/env python3
"""Extract selected still frames from checked-in native Gallery GIF captures."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "docs/images"
DESTINATION = ROOT / "website/src/assets/captures"
CAPTURES = (
    "v3-overview", "v3-workspace", "v3-comparison", "v3-toasts", "v3-icons",
    "v3-node-editor",
    "v3-workflow-progress",
    "v3-timeline",
    "v3-theme-comparison",
    "gallery-icons",
)
POSTER_FRAMES = {
    "v3-overview": 65,  # Enter the live 3.2 workspace from Start.
    "v3-workspace": 85,  # Inspector, shared values and live preview.
    "v3-comparison": 80,
    "v3-icons": 65,
    "v3-node-editor": 65,  # Actual socket drag and connection.
    "v3-timeline": 65,
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
