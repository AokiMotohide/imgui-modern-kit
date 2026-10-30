#!/usr/bin/env python3
"""Check the published media against the contract: films, GIFs, poster and the recorded takes behind them.

    python tools/media/verify.py                      # website/public/media and docs/images
    python tools/media/verify.py --films out/media/film/v0 --skip-gifs
"""
from pathlib import Path
import argparse, hashlib, json, re, subprocess, sys
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import capture
import film

ROOT = Path(__file__).resolve().parents[2]
FILM_MIB = 40
LUFS, LUFS_TOLERANCE, TRUE_PEAK = -14.0, 1.0, -1.5  # streaming-safe true peak after AAC


def probe(path):
    info = json.loads(subprocess.check_output(["ffprobe", "-v", "error", "-count_frames", "-show_streams", "-show_format", "-of", "json", str(path)]))
    video = next(s for s in info["streams"] if s["codec_type"] == "video")
    audio = next((s for s in info["streams"] if s["codec_type"] == "audio"), None)
    return info, video, audio


def loudness(path):
    text = subprocess.run(["ffmpeg", "-hide_banner", "-i", str(path), "-af", "loudnorm=I=-14:TP=-1.5:LRA=11:print_format=json", "-f", "null", "-"],
                          capture_output=True, text=True, check=True).stderr
    return json.loads(re.search(r"\{\s*\"input_i\"[\s\S]*?\}", text).group())


def check_film(path):
    info, video, audio = probe(path)
    seconds = float(info["format"]["duration"])
    assert (video["width"], video["height"], video["r_frame_rate"]) == (1920, 1080, "60/1"), (path.name, video)
    assert abs(seconds - film.TOTAL) < 0.6, (path.name, seconds)
    assert audio and audio["channels"] == 2 and audio["sample_rate"] == "48000", path.name
    size = path.stat().st_size
    assert size <= FILM_MIB * 1024 ** 2, f"{path.name} is {size / 1024 ** 2:.1f} MiB, over {FILM_MIB}"
    stats = loudness(path)
    assert abs(float(stats["input_i"]) - LUFS) <= LUFS_TOLERANCE and float(stats["input_tp"]) <= TRUE_PEAK, (path.name, stats)
    return {"file": path.name, "seconds": round(seconds, 2), "mib": round(size / 1024 ** 2, 1), "lufs": float(stats["input_i"]),
            "true_peak_dbtp": float(stats["input_tp"]), "frames": int(video["nb_read_frames"]), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def check_gif(path):
    with Image.open(path) as image:
        assert image.size == (960, 540), (path.name, image.size)
        assert path.stat().st_size <= 2 * 1024 ** 2, path.name
        durations = []
        for i in range(image.n_frames):
            image.seek(i)
            durations.append(image.info.get("duration", 0))
    seconds = sum(durations) / 1000
    assert 6 <= seconds <= 8.1, (path.name, seconds)
    return {"file": path.name, "bytes": path.stat().st_size, "frames": len(durations), "seconds": round(seconds, 2)}


def check_captions():
    """Both languages must cover the same keys; report the longest line so a text overflow is caught early."""
    report = {}
    keys = None
    for lang in ("ja", "en"):
        caps = json.loads((Path(__file__).parent / f"captions.{lang}.json").read_text(encoding="utf-8"))
        keys = keys or set(caps)
        assert set(caps) == keys, f"captions differ between languages: {set(caps) ^ keys}"
        report[lang] = {"entries": len(caps), "longest_title": max(len(c["title"]) for c in caps.values()), "longest_sub": max(len(c["sub"]) for c in caps.values())}
    used = {cap["key"] for shot in film.SPEC["shots"] for cap in shot.get("caps", [])} | {s["key"] for s in film.SPEC["shots"] if "key" in s}
    missing = used - keys
    assert not missing, f"caption keys used but not defined: {sorted(missing)}"
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--films", type=Path, default=ROOT / "website/public/media")
    parser.add_argument("--skip-gifs", action="store_true")
    args = parser.parse_args()
    report = {"captions": check_captions(), "films": [], "gifs": []}
    for lang in ("ja", "en"):
        report["films"].append(check_film(args.films / f"imkit-4.0-film-{lang}.mp4"))
    if not args.skip_gifs:
        for take, (_, stem) in capture.TAKES.items():
            if stem:
                report["gifs"].append(check_gif(ROOT / "docs/images" / f"{stem}.gif"))
    poster = args.films / "imkit-4.0-poster.jpg"
    if poster.exists():
        assert Image.open(poster).size == (1920, 1080)
    out = ROOT / "out/media/media-validation.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))
    print("PASS")


if __name__ == "__main__":
    main()
