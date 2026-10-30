#!/usr/bin/env python3
"""Record the Gallery's scripted takes as lossless 1080p60 video, and cut the README GIFs from them.

Every take drives the real Gallery through public Dear ImGui input (see CaptureMotion in examples/gallery/main.cpp),
so what the film and the GIFs show is what the library does. Videos and event logs go to out/media/native, which is
regenerable and never committed. Requires the Debug targets and FFmpeg on PATH:

    python tools/media/capture.py                  # every take
    python tools/media/capture.py hook worlds      # some takes
    python tools/media/capture.py --gif-only       # only rebuild the GIFs from the recorded takes
"""
from pathlib import Path
import argparse, json, shutil, subprocess

ROOT = Path(__file__).resolve().parents[2]
NATIVE = ROOT / "out/media/native"
BINARIES = ROOT / "build/windows-debug/catalog/Debug"
GIF_BYTES = 2 * 1024 ** 2
GIF_SECONDS = 8.0

# take -> (executable, README/Pages GIF stem or None). The GIF plays the whole take, sped up to GIF_SECONDS if longer.
TAKES = {
    "hook": ("imkit_gallery", "v4-start"),
    "worlds": ("imkit_gallery", "v4-themes"),
    "comparison": ("imkit_gallery", "v4-compare"),
    "studio": ("imkit_gallery", "v4-studio"),
    "workspace": ("imkit_gallery", "v4-workspace"),
    "timeline": ("imkit_gallery", "v4-timeline"),
    "show-live": ("imkit_gallery", "v4-show"),
    "show-surface": ("imkit_gallery", "v4-surface"),
    "charts": ("imkit_gallery", "v4-charts"),
    "ptz": ("imkit_gallery", None),
    "toasts": ("imkit_gallery", "v4-toasts"),
    "icons": ("imkit_gallery", "v4-icons"),
    "workflow": ("imkit_gallery", "v4-progress"),
    "node-editor-final": ("imkit_node_editor_gallery", "v4-node-editor"),
}


def probe(path: Path) -> float:
    info = json.loads(subprocess.check_output(["ffprobe", "-v", "error", "-show_streams", "-show_format", "-of", "json", str(path)]))
    video = next(s for s in info["streams"] if s["codec_type"] == "video")
    assert (video["width"], video["height"], video["r_frame_rate"]) == (1920, 1080, "60/1"), (path.name, video)
    return float(info["format"]["duration"])


def make_gif(movie: Path, dest: Path, seconds: float) -> None:
    """960x540 at 20 fps, at most GIF_BYTES; trades colours, then frame rate, before it gives up."""
    duration = min(seconds, GIF_SECONDS)
    speed = duration / seconds  # <1 speeds a long take up so the whole take fits
    for colors, fps in [(128, 20), (96, 20), (64, 20), (48, 20), (64, 12)]:
        base = f"setpts={speed:.9f}*PTS,fps={fps},scale=960:540:flags=lanczos"
        graph = f"[0:v]{base},split[a][b];[a]palettegen=max_colors={colors}:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle"
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", str(movie), "-filter_complex", graph, "-t", f"{duration:.3f}", "-loop", "0", str(dest)], check=True)
        if dest.stat().st_size <= GIF_BYTES:
            return
    raise RuntimeError(f"GIF exceeds 2 MiB even at 12 fps: {dest}")


def validate(movie: Path) -> tuple[float, list, int]:
    seconds = probe(movie)
    events = [json.loads(line) for line in Path(str(movie) + ".jsonl").read_text().splitlines()]
    frames = [e for e in events if "time" in e]
    checks = [e for e in events if "check" in e]
    assert frames and abs(seconds - len(frames) / 60) < 0.05, (movie.name, seconds, len(frames))
    assert all(e["passed"] for e in checks), movie.name
    assert any(e["down"] or e["middle"] or e["wheel"] for e in frames), f"{movie.name}: no real input"
    return seconds, checks, len(frames)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("takes", nargs="*", metavar="take", help="one of: " + ", ".join(TAKES))
    parser.add_argument("--gif-only", action="store_true")
    parser.add_argument("--no-gif", action="store_true")
    args = parser.parse_args()
    unknown = [take for take in args.takes if take not in TAKES]
    if unknown:
        parser.error(f"unknown take: {', '.join(unknown)}")
    free = shutil.disk_usage(ROOT).free
    if free < 30 * 1024 ** 3:
        raise RuntimeError(f"Need 30 GiB free for lossless takes, have {free / 1024 ** 3:.0f} GiB")
    NATIVE.mkdir(parents=True, exist_ok=True)
    report = []
    for take in args.takes or list(TAKES):
        exe, gif = TAKES[take]
        movie = NATIVE / f"{take}.mkv"
        if not args.gif_only:
            command = [str(BINARIES / f"{exe}.exe"), "--capture-motion"]
            command += [str(NATIVE)] if take == "node-editor-final" else [take, "--output", str(NATIVE)]
            command += ["--width", "1920", "--height", "1080"]
            print(f"Record {take}", flush=True)
            subprocess.run(command, cwd=ROOT, check=True)
        seconds, checks, frames = validate(movie)
        entry = {"take": take, "seconds": round(seconds, 3), "frames": frames, "checks": len(checks)}
        if gif and not args.no_gif:
            dest = ROOT / "docs/images" / f"{gif}.gif"
            make_gif(movie, dest, seconds)
            entry["gif"] = {"file": dest.name, "bytes": dest.stat().st_size}
        report.append(entry)
        print(f"  {take}: {seconds:.2f}s, {frames} frames, {len(checks)} checks passed", flush=True)
    (NATIVE / "takes.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"free before={free // 1024 ** 2} MiB, after={shutil.disk_usage(ROOT).free // 1024 ** 2} MiB")


if __name__ == "__main__":
    main()
