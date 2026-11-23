#!/usr/bin/env python3
"""Generate cached Gemini API assets (Lyria 3.5 music, Omni 1.2 Flash 1080p clips, Nano Banana 2.1 art, 3.8 Flash TTS)
for the ImKit 4.2 promotional film. All outputs stay in out/media/gemini (git-ignored).
"""
from __future__ import annotations

import argparse
import base64
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
import json
import os
from pathlib import Path
import subprocess
import sys
import time

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(line_buffering=True)
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(line_buffering=True)

import httpx
from google import genai
from google.genai import types


ROOT = Path(__file__).resolve().parents[2]
MEDIA_DIR = Path(__file__).resolve().parent
OUT = ROOT / "out/media/gemini"
REFS = OUT / "refs"
OMNI_SCRIPT = Path(
    r"C:\Users\aokim\.gemini\config\plugins\gemini-api\skills\gemini-omni-flash-api\scripts\video\generate_video.py"
)


def log(msg: str) -> None:
    ts = datetime.now().strftime("%H:%M:%S")
    print(f"[{ts}] {msg}")


def ensure_api_key() -> str:
    key = os.environ.get("GEMINI_API_KEY", "").strip()
    if not key and sys.platform == "win32":
        import winreg

        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as reg:
            try:
                val, _ = winreg.QueryValueEx(reg, "GEMINI_API_KEY")
                key = str(val).strip()
            except FileNotFoundError:
                key = ""
        if key:
            os.environ["GEMINI_API_KEY"] = key
    if not key:
        raise RuntimeError("GEMINI_API_KEY is not set in environment or Windows User profile.")
    # Do NOT force GOOGLE_GENAI_API_VERSION globally; pick v1 / v1beta per model.
    os.environ.pop("GOOGLE_GENAI_API_VERSION", None)
    return key


def make_client(api_version: str, timeout_ms: int) -> genai.Client:
    return genai.Client(http_options=types.HttpOptions(api_version=api_version, timeout=timeout_ms))


def run_worker(payload: dict, hard_timeout_s: int) -> dict:
    """Run a single Gemini API call in a fresh subprocess bounded by an OS timeout."""
    import uuid

    res_file = OUT / "_tmp" / f"worker_{os.getpid()}_{uuid.uuid4().hex[:10]}.json"
    res_file.parent.mkdir(parents=True, exist_ok=True)
    req_file = res_file.with_suffix(".req.json")
    req_file.write_text(json.dumps(payload, ensure_ascii=False), encoding="utf-8")
    try:
        proc = subprocess.run(
            [sys.executable, "-u", str(Path(__file__).resolve()), "--_worker", str(req_file), str(res_file)],
            cwd=ROOT,
            timeout=hard_timeout_s,
            capture_output=True,
            text=True,
        )
        if proc.stdout:
            for line in proc.stdout.strip().splitlines():
                print(f"    {line}")
        if proc.returncode != 0:
            err_detail = (proc.stderr or proc.stdout or "").strip()
            raise RuntimeError(f"Worker exited {proc.returncode}: {err_detail[-400:]}")
        if not res_file.is_file():
            raise RuntimeError("Worker finished without result payload.")
        return json.loads(res_file.read_text(encoding="utf-8"))
    finally:
        req_file.unlink(missing_ok=True)
        res_file.unlink(missing_ok=True)


def ffprobe_duration(path: Path) -> float:
    out = subprocess.check_output(
        ["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "default=noprint_wrappers=1:nokey=1", str(path)],
        text=True,
    ).strip()
    return float(out)


def image_part(path: Path) -> dict:
    return {
        "type": "image",
        "mime_type": "image/png",
        "data": base64.b64encode(path.read_bytes()).decode("ascii"),
    }


def extract_audio_bytes(interaction) -> tuple[bytes, str]:
    audio = getattr(interaction, "output_audio", None)
    if audio and getattr(audio, "data", None):
        return base64.b64decode(audio.data), getattr(audio, "mime_type", "") or "audio/mpeg"
    for step in getattr(interaction, "steps", None) or []:
        if getattr(step, "type", "") == "model_output":
            for item in getattr(step, "content", None) or []:
                if getattr(item, "type", "") == "audio" and getattr(item, "data", None):
                    return base64.b64decode(item.data), getattr(item, "mime_type", "") or "audio/mpeg"
    raise RuntimeError("No audio payload found in Gemini interaction response.")


def _worker_exec(req_path: Path, res_path: Path) -> int:
    ensure_api_key()
    req = json.loads(req_path.read_text(encoding="utf-8"))
    kind = req["kind"]
    client = make_client(req["api_version"], req["timeout_ms"])

    if kind == "tts":
        interaction = client.interactions.create(
            model=req["model"],
            input=[
                {
                    "type": "user_input",
                    "content": [
                        {
                            "type": "text",
                            "text": req["text"],
                            "annotations": [{"type": "speech_metadata", "style": req["style"]}],
                        }
                    ],
                }
            ],
            response_format={"type": "audio"},
            generation_config={"speech_config": [{"voice": req["voice"]}]},
            timeout=req["timeout_ms"] / 1000.0,
        )
        audio_bytes, mime = extract_audio_bytes(interaction)
        Path(req["raw_out"]).write_bytes(audio_bytes)
        res_path.write_text(json.dumps({"ok": True, "mime": mime, "bytes": len(audio_bytes)}), encoding="utf-8")
        return 0

    if kind == "music":
        interaction = client.interactions.create(model=req["model"], input=req["inputs"], timeout=req["timeout_ms"] / 1000.0)
        raw_bytes, mime = extract_audio_bytes(interaction)
        ext = ".wav" if "wav" in mime else ".mp3"
        raw_path = Path(req["raw_base"] + ext)
        raw_path.write_bytes(raw_bytes)
        res_path.write_text(json.dumps({"ok": True, "raw_path": str(raw_path), "mime": mime, "bytes": len(raw_bytes)}), encoding="utf-8")
        return 0

    if kind == "image":
        interaction = client.interactions.create(
            model=req["model"],
            input=req["prompt"],
            response_format={"type": "image", "aspect_ratio": "16:9", "image_size": "2K"},
            timeout=req["timeout_ms"] / 1000.0,
        )
        img = interaction.output_image
        if not img or not img.data:
            raise RuntimeError(f"{req['model']} returned no image payload.")
        raw_bytes = base64.b64decode(img.data)
        Path(req["raw_out"]).write_bytes(raw_bytes)
        res_path.write_text(json.dumps({"ok": True, "bytes": len(raw_bytes)}), encoding="utf-8")
        return 0

    raise ValueError(f"Unknown worker kind: {kind}")


def generate_music(force: bool = False) -> Path:
    dest_dir = OUT / "music"
    dest_dir.mkdir(parents=True, exist_ok=True)
    wav_out = dest_dir / "lyria_score.wav"
    if wav_out.is_file() and not force:
        log(f"[music] cached: {wav_out.name} ({ffprobe_duration(wav_out):.2f}s)")
        return wav_out

    ref_names = ["hook_modern.png", "studio.png", "workspace.png", "node_editor.png", "show_live.png", "glow.png"]
    inputs: list[dict] = [
        {
            "type": "text",
            "text": (
                "Compose an instrumental modern electronic product-launch soundtrack inspired by these sleek creative-tool UI screenshots. "
                "STRICTLY INSTRUMENTAL, NO VOCALS, NO SINGING, NO SPOKEN WORDS. "
                "Tempo: 128 BPM, crisp modern studio production like an Apple or Figma keynote trailer. "
                "Duration target: 3 minutes (around 180 seconds). "
                "Musical arc with timestamps: "
                "0:00-0:06 airy crystalline synth arpeggio and warm ambient pad building tension; "
                "0:06-0:36 tight punchy four-on-the-floor electronic kick, clean sub-bass, and bright future-synth plucks; "
                "0:36-1:30 energetic uplifting tech-house groove with melodic bell hooks and crisp hi-hats; "
                "1:30-1:36 brief atmospheric half-time synth breakdown; "
                "1:36-2:38 soaring anthemic electronic climax with layered pads and sparkling arpeggios; "
                "2:38-3:00 triumphant finale resolving cleanly into a warm lingering reverb tail."
            ),
        }
    ]
    for name in ref_names:
        p = REFS / name
        if p.is_file():
            inputs.append(image_part(p))

    attempts = [
        ("lyria-3.5", "v1", 150_000, 175),
        ("lyria-3.5", "v1beta", 150_000, 175),
        ("lyria-3-pro-preview", "v1beta", 120_000, 145),
        ("lyria-3-clip-preview", "v1beta", 90_000, 110),
    ]
    last_err: Exception | None = None
    res: dict | None = None
    used_model = ""
    for model_id, api_ver, timeout_ms, hard_s in attempts:
        log(f"[music] calling {model_id} ({api_ver}, timeout={hard_s}s) with {len(inputs) - 1} UI refs...")
        try:
            res = run_worker(
                {
                    "kind": "music",
                    "model": model_id,
                    "api_version": api_ver,
                    "timeout_ms": timeout_ms,
                    "inputs": inputs,
                    "raw_base": str(dest_dir / "lyria_raw"),
                },
                hard_timeout_s=hard_s,
            )
            used_model = model_id
            break
        except Exception as e:
            last_err = e
            log(f"[music] {model_id} ({api_ver}) failed: {str(e)[:220]}")

    if res is None:
        raise RuntimeError(f"All Lyria music generation attempts failed: {last_err}")

    raw_path = Path(res["raw_path"])
    raw_dur = ffprobe_duration(raw_path)
    log(f"[music] {used_model} returned {res['mime']}, {res['bytes']} bytes, duration {raw_dur:.2f}s")
    (dest_dir / "music_meta.json").write_text(
        json.dumps({"model": used_model, "duration": round(raw_dur, 3), "mime": res["mime"]}, indent=2) + "\n",
        encoding="utf-8",
    )

    subprocess.run(
        ["ffmpeg", "-y", "-v", "error", "-i", str(raw_path), "-ar", "48000", "-ac", "2", "-c:a", "pcm_s16le", str(wav_out)],
        check=True,
    )
    return wav_out


def generate_images(force: bool = False) -> list[Path]:
    dest_dir = OUT / "images"
    dest_dir.mkdir(parents=True, exist_ok=True)
    specs = [
        (
            "bg_opening.png",
            "Minimalist dark architectural studio void, subtle deep obsidian-navy gradient, fine precision blueprint micro-grid lines, "
            "soft volumetric cyan and violet glass light refraction edges, ultra-clean modern tech product launch backdrop, no text, no letters, no UI widgets.",
        ),
        (
            "bg_show.png",
            "Dark modern live production stage lighting atmosphere, subtle volumetric amber and cyan laser beams through soft haze, "
            "matte obsidian background, sleek hardware concert atmosphere, ultra-clean, no text, no people, no letters.",
        ),
        (
            "bg_poster.png",
            "Sleek high-contrast dark studio backdrop with glowing cyan, violet, and warm amber glass light ribbons sweeping diagonally across obsidian space, "
            "clean center breathing room for overlay, 4K luxury software announcement aesthetic, no text, no logos.",
        ),
    ]
    outputs = []
    for filename, prompt in specs:
        out_path = dest_dir / filename
        if out_path.is_file() and not force:
            log(f"[image] cached: {filename}")
            outputs.append(out_path)
            continue
        tmp = dest_dir / f"_raw_{filename}"
        attempts = [
            ("gemini-nano-banana-2.1", "v1", 35_000, 45),
            ("gemini-3.1-flash-lite-image", "v1", 30_000, 40),
        ]
        ok = False
        last_err: Exception | None = None
        for model_id, api_ver, timeout_ms, hard_s in attempts:
            log(f"[image] generating {filename} via {model_id} ({api_ver}, timeout={hard_s}s)...")
            try:
                res = run_worker(
                    {
                        "kind": "image",
                        "model": model_id,
                        "api_version": api_ver,
                        "timeout_ms": timeout_ms,
                        "prompt": prompt,
                        "raw_out": str(tmp),
                    },
                    hard_timeout_s=hard_s,
                )
                ok = True
                log(f"[image] {filename} generated via {model_id} ({res['bytes']} raw bytes)")
                break
            except Exception as e:
                last_err = e
                log(f"[image] {filename} via {model_id} failed: {str(e)[:220]}")
        if not ok:
            raise RuntimeError(f"Image generation failed for {filename}: {last_err}")

        subprocess.run(
            ["ffmpeg", "-y", "-v", "error", "-i", str(tmp), "-vf", "scale=1920:1080:flags=lanczos", str(out_path)],
            check=True,
        )
        tmp.unlink(missing_ok=True)
        outputs.append(out_path)
        log(f"[image] saved {filename} ({out_path.stat().st_size} bytes)")
    return outputs


def call_tts_httpx(api_key: str, model: str, text: str, style: str, voice: str, timeout_s: float = 25.0) -> tuple[bytes, str]:
    """Call Gemini v1beta Interactions TTS directly via httpx, honoring 3-RPM rate limits cleanly."""
    import re

    url = "https://generativelanguage.googleapis.com/v1beta/interactions"
    payload = {
        "model": model,
        "input": [
            {
                "type": "user_input",
                "content": [
                    {
                        "type": "text",
                        "text": text,
                        "annotations": [{"type": "speech_metadata", "style": style}],
                    }
                ],
            }
        ],
        "response_format": {"type": "audio"},
        "generation_config": {"speech_config": [{"voice": voice}]},
    }
    headers = {"x-goog-api-key": api_key, "Content-Type": "application/json"}

    last_err: Exception | None = None
    for attempt in range(1, 5):
        try:
            resp = httpx.post(url, headers=headers, json=payload, timeout=timeout_s)
        except Exception as e:
            last_err = e
            if attempt < 4:
                time.sleep(3.0)
                continue
            raise
        if resp.status_code == 200:
            data = resp.json()
            audio = data.get("output_audio") or {}
            if audio.get("data"):
                return base64.b64decode(audio["data"]), audio.get("mime_type") or "audio/wav"
            for step in data.get("steps") or []:
                if step.get("type") == "model_output":
                    for item in step.get("content") or []:
                        if item.get("type") == "audio" and item.get("data"):
                            return base64.b64decode(item["data"]), item.get("mime_type") or "audio/wav"
            raise RuntimeError(f"200 OK but no audio payload in response: {str(data)[:220]}")
        if resp.status_code == 429:
            body_text = resp.text
            if "requests per day" in body_text.lower() or "limit: 0" in body_text.lower():
                raise RuntimeError(f"Daily/Zero Quota 429 on {model}: {body_text[:240]}")
            m = re.search(r"retry in (\d+(?:\.\d+)?)s", body_text, re.IGNORECASE)
            wait_s = min(65.0, float(m.group(1)) + 2.0) if m else 25.0
            if attempt < 4:
                log(f"  [tts] 3-RPM rate limit on {model}, waiting {wait_s:.1f}s ({attempt}/4)...")
                time.sleep(wait_s)
                last_err = RuntimeError(f"HTTP 429: {body_text[:180]}")
                continue
            raise RuntimeError(f"HTTP 429 Rate Limit on {model}: {body_text[:240]}")
        if resp.status_code == 503 and attempt < 4:
            log(f"  [tts] HTTP 503 on {model}, retrying in 4s ({attempt}/4)...")
            time.sleep(4.0)
            last_err = RuntimeError(f"HTTP 503: {resp.text[:180]}")
            continue
        raise RuntimeError(f"HTTP {resp.status_code} on {model}: {resp.text[:240]}")
    raise RuntimeError(f"TTS failed after retries: {last_err}")


_LAST_TTS_CALL_AT = 0.0


def generate_tts_lang(lang: str, force: bool = False) -> dict[str, dict]:
    global _LAST_TTS_CALL_AT
    api_key = ensure_api_key()
    spec = json.loads((MEDIA_DIR / f"narration.{lang}.json").read_text(encoding="utf-8"))
    dest_dir = OUT / "tts" / lang
    dest_dir.mkdir(parents=True, exist_ok=True)
    voice = spec["voice"]
    style = spec["style"]
    tts_model = "gemini-3.8-flash-lite-tts"
    manifest: dict[str, dict] = {}

    log(f"[tts:{lang}] synthesizing {len(spec['lines'])} lines via {tts_model} (voice={voice}, sequential 21s pacing for 3 RPM)...")
    for line in spec["lines"]:
        lid = line["id"]
        wav_path = dest_dir / f"{lid}.wav"
        if not wav_path.is_file() or force:
            since = time.monotonic() - _LAST_TTS_CALL_AT
            if _LAST_TTS_CALL_AT > 0 and since < 21.0:
                time.sleep(21.0 - since)
            raw_path = dest_dir / f"_{lid}_raw.wav"
            _LAST_TTS_CALL_AT = time.monotonic()
            audio_bytes, _ = call_tts_httpx(api_key, tts_model, line["text"], style, voice, timeout_s=25.0)
            raw_path.write_bytes(audio_bytes)
            subprocess.run(
                [
                    "ffmpeg",
                    "-y",
                    "-v",
                    "error",
                    "-i",
                    str(raw_path),
                    "-af",
                    "silenceremove=start_periods=1:start_silence=0.05:start_threshold=-45dB,areverse,silenceremove=start_periods=1:start_silence=0.05:start_threshold=-45dB,areverse",
                    "-ar",
                    "48000",
                    "-ac",
                    "2",
                    "-c:a",
                    "pcm_s16le",
                    str(wav_path),
                ],
                check=True,
            )
            raw_path.unlink(missing_ok=True)
        dur = ffprobe_duration(wav_path)
        if dur > line["max_dur"]:
            ratio = min(1.26, dur / (line["max_dur"] - 0.12))
            if ratio > 1.01:
                sped = dest_dir / f"_{lid}_fit.wav"
                subprocess.run(
                    ["ffmpeg", "-y", "-v", "error", "-i", str(wav_path), "-af", f"atempo={ratio:.4f}", str(sped)],
                    check=True,
                )
                sped.replace(wav_path)
                dur = ffprobe_duration(wav_path)
        info = {"file": str(wav_path.relative_to(ROOT)), "at": line["at"], "max_dur": line["max_dur"], "duration": round(dur, 3)}
        manifest[lid] = info
        log(f"  [tts:{lang}] {lid}: {info['duration']:.2f}s / max {info['max_dur']:.2f}s")

    ordered = {line["id"]: manifest[line["id"]] for line in spec["lines"]}
    (dest_dir / "manifest.json").write_text(json.dumps(ordered, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return ordered


def generate_videos(force: bool = False, only: list[str] | None = None) -> list[Path]:
    dest_dir = OUT / "videos"
    dest_dir.mkdir(parents=True, exist_ok=True)
    jobs = [
        {
            "name": "clip_intro_bg.mp4",
            "args": [
                "A sleek dark minimalist software studio void with subtle floating translucent glass planes, cyan and violet volumetric edge lighting slowly drifting toward camera, ultra-clean architectural motion design backdrop, no text, no UI elements",
                "--duration",
                "8",
                "--resolution",
                "720p",
                "--aspect-ratio",
                "16:9",
                "--timeout",
                "180",
            ],
        },
        {
            "name": "clip_stage_bridge.mp4",
            "args": [
                "Cinematic slow camera glide past a dark sleek broadcast lighting and media control console with glowing amber and teal light beams cutting softly through atmospheric haze, ultra-clean modern production studio, no text, no people",
                "--duration",
                "6",
                "--resolution",
                "720p",
                "--aspect-ratio",
                "16:9",
                "--timeout",
                "180",
            ],
        },
        {
            "name": "clip_finale_bg.mp4",
            "args": [
                "Deep obsidian cosmic studio space with luminous cyan, violet, and warm amber light particles and subtle glass refraction waves gracefully converging toward the center, premium software keynote finale backdrop, no text",
                "--duration",
                "8",
                "--resolution",
                "720p",
                "--aspect-ratio",
                "16:9",
                "--timeout",
                "180",
            ],
        },
    ]
    if only:
        jobs = [j for j in jobs if j["name"] in only]

    def run_one(job: dict) -> Path:
        out_path = dest_dir / job["name"]
        if out_path.is_file() and not force:
            log(f"[video] cached: {job['name']} ({ffprobe_duration(out_path):.2f}s)")
            return out_path
        raw_out = dest_dir / f"_raw_{job['name']}"
        cmd = [sys.executable, "-u", str(OMNI_SCRIPT), *job["args"], "--output", str(raw_out)]
        log(f"[video] generating {job['name']} via gemini-omni-1.2-flash (720p->1080p, OS timeout=210s)...")
        proc = subprocess.run(cmd, cwd=ROOT, timeout=210, check=False)
        if proc.returncode != 0 or not raw_out.is_file():
            raise RuntimeError(f"gemini-omni-1.2-flash failed for {job['name']} (rc={proc.returncode})")
        # Upscale 720p -> 1920x1080 60fps silent intermediate for film.py compositing.
        subprocess.run(
            [
                "ffmpeg",
                "-y",
                "-v",
                "error",
                "-i",
                str(raw_out),
                "-an",
                "-vf",
                "fps=60,scale=1920:1080:flags=lanczos",
                "-c:v",
                "libx264",
                "-preset",
                "fast",
                "-crf",
                "14",
                "-pix_fmt",
                "yuv420p",
                str(out_path),
            ],
            check=True,
        )
        raw_out.unlink(missing_ok=True)
        log(f"[video] ready: {job['name']} ({ffprobe_duration(out_path):.2f}s)")
        return out_path

    results = []
    for job in jobs:
        results.append(run_one(job))
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--music", action="store_true")
    parser.add_argument("--images", action="store_true")
    parser.add_argument("--tts", action="store_true")
    parser.add_argument("--videos", action="store_true")
    parser.add_argument("--video-name", action="append", help="Specific video filename(s) when using --videos")
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--_worker", nargs=2, metavar=("REQ", "RES"), help=argparse.SUPPRESS)
    args = parser.parse_args()

    if args._worker:
        return _worker_exec(Path(args._worker[0]), Path(args._worker[1]))

    ensure_api_key()
    run_all = args.all or not (args.music or args.images or args.tts or args.videos)

    # Order: TTS -> Music -> Images -> Videos (fastest / most reliable first)
    if run_all or args.tts:
        generate_tts_lang("ja", args.force)
        generate_tts_lang("en", args.force)
    if run_all or args.music:
        generate_music(args.force)
    if run_all or args.images:
        generate_images(args.force)
    if run_all or args.videos:
        generate_videos(args.force, args.video_name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
