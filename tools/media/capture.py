#!/usr/bin/env python3
"""Record the Gallery's scripted takes as lossless 1080p60 video, and cut the README GIFs from them.

Every take drives the real Gallery through public Dear ImGui input (see CaptureMotion in examples/gallery/main.cpp),
so what the film and the GIFs show is what the library does. Videos and event logs go to out/media/native, which is
regenerable and never committed. Requires the Debug targets and FFmpeg on PATH:

    python tools/media/capture.py                  # every take
    python tools/media/capture.py hook worlds      # some takes
    python tools/media/capture.py --gif-only       # only rebuild the GIFs from the recorded takes
    python tools/media/capture.py --check          # re-record everything, keep only the takes whose pixels changed

A take is a scripted scene (kScenes in examples/gallery/main.cpp), so recording is cheap and repeatable. --check records
each take into out/media/native/_new and compares a hash of the decoded frames with the one stored in takes.json: an
unchanged take keeps its file (and its film chunks stay cached), a changed take replaces it and gets a new GIF.
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
    "tilemap": ("imkit_gallery", "v4-tilemap"),
    "sprite": ("imkit_gallery", "v4-sprites"),
    "glow": ("imkit_gallery", None),
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


def pixel_hash(movie: Path) -> str:
    """SHA-256 of the decoded video frames, so two recordings of the same scene compare equal."""
    out = subprocess.run(["ffmpeg", "-v", "error", "-i", str(movie), "-map", "0:v:0", "-f", "hash", "-hash", "sha256", "-"],
                         capture_output=True, text=True, check=True).stdout
    return out.strip().split("=", 1)[1]


def record(take: str, folder: Path) -> None:
    exe, _ = TAKES[take]
    command = [str(BINARIES / f"{exe}.exe"), "--capture-motion"]
    command += [str(folder)] if take == "node-editor-final" else [take, "--output", str(folder)]
    command += ["--width", "1920", "--height", "1080"]
    print(f"Record {take}", flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("takes", nargs="*", metavar="take", help="one of: " + ", ".join(TAKES))
    parser.add_argument("--gif-only", action="store_true")
    parser.add_argument("--no-gif", action="store_true")
    parser.add_argument("--check", action="store_true", help="keep a new recording only if its pixels differ from the stored take")
    args = parser.parse_args()
    unknown = [take for take in args.takes if take not in TAKES]
    if unknown:
        parser.error(f"unknown take: {', '.join(unknown)}")
    free = shutil.disk_usage(ROOT).free
    if free < 30 * 1024 ** 3:
        raise RuntimeError(f"Need 30 GiB free for lossless takes, have {free / 1024 ** 3:.0f} GiB")
    NATIVE.mkdir(parents=True, exist_ok=True)
    known = {entry["take"]: entry for entry in json.loads((NATIVE / "takes.json").read_text())} if (NATIVE / "takes.json").is_file() else {}
    changed, unchanged = [], []
    for take in args.takes or list(TAKES):
        _, gif = TAKES[take]
        movie = NATIVE / f"{take}.mkv"
        adopted = True
        if not args.gif_only:
            folder = NATIVE / "_new" if args.check else NATIVE
            if args.check:
                shutil.rmtree(folder, ignore_errors=True)
                folder.mkdir()
            record(take, folder)
            if args.check:
                fresh = pixel_hash(folder / f"{take}.mkv")
                stored = known.get(take, {}).get("pixels") or (pixel_hash(movie) if movie.is_file() else None)
                adopted = fresh != stored
                if adopted:
                    for path in folder.iterdir():
                        if path.name.startswith(take + "."):
                            path.replace(NATIVE / path.name)
                shutil.rmtree(folder, ignore_errors=True)
        seconds, checks, frames = validate(movie)
        entry = {"take": take, "seconds": round(seconds, 3), "frames": frames, "checks": len(checks),
                 "pixels": known.get(take, {}).get("pixels") if args.gif_only else pixel_hash(movie) if adopted else stored}
        if gif and not args.no_gif:
            dest = ROOT / "docs/images" / f"{gif}.gif"
            if adopted or not dest.is_file():
                make_gif(movie, dest, seconds)
            entry["gif"] = {"file": dest.name, "bytes": dest.stat().st_size}
        known[take] = entry
        (changed if adopted else unchanged).append(take)
        print(f"  {take}: {seconds:.2f}s, {frames} frames, {len(checks)} checks passed" + ("" if adopted else " (pixels unchanged)"), flush=True)
    (NATIVE / "takes.json").write_text(json.dumps([known[take] for take in TAKES if take in known], indent=2) + "\n")
    if args.check:
        print(f"changed: {', '.join(changed) or 'none'}")
        print(f"unchanged: {', '.join(unchanged) or 'none'}")
    print(f"free before={free // 1024 ** 2} MiB, after={shutil.disk_usage(ROOT).free // 1024 ** 2} MiB")


if __name__ == "__main__":
    main()
