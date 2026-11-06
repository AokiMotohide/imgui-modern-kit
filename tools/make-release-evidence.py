#!/usr/bin/env python3
"""Write the two evidence files that tools/package-release.py requires. Maintainers only; it publishes nothing.

    # CI: the push run of the supported-platforms workflow at exactly HEAD (wait for it to finish first).
    python tools/make-release-evidence.py ci

    # Native: run the Debug Gallery's public-IO verification on the committed tree.
    python tools/make-release-evidence.py native

Outputs go to out/release (git-ignored): ci-evidence.json and native-evidence.json. The version comes from CMakeLists.txt.
Run it from the committed release commit, after the Debug Gallery was built through tools/Invoke-SharedBuild.ps1.
"""
from pathlib import Path
import argparse, hashlib, json, re, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "out/release"
GALLERY = ROOT / "build/windows-debug/catalog/Debug/imkit_gallery.exe"
VERIFY_DIR = OUT / "verify"
WORKFLOW = "windows.yml"  # The workflow named supported-platforms.


def version() -> str:
    match = re.search(r"project\(imgui-modern-kit VERSION ([\d.]+)", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
    if not match:
        raise RuntimeError("project version was not found in CMakeLists.txt")
    return match.group(1)


def gh(*args: str) -> str:
    return subprocess.check_output(["gh", *args], cwd=ROOT, text=True, encoding="utf-8")


def ci(args) -> None:
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    run = args.run
    if run is None:
        runs = json.loads(gh("run", "list", "--workflow", WORKFLOW, "--commit", head, "--event", "push", "--json", "databaseId,status"))
        if len(runs) != 1:
            raise RuntimeError(f"expected one push run at {head[:7]}, found {len(runs)}; pass --run")
        run = runs[0]["databaseId"]
    record = json.loads(gh("run", "view", str(run), "--json", "databaseId,headSha,conclusion,event,status,url,workflowName,createdAt,updatedAt,jobs"))
    record["jobs"] = [{key: job.get(key) for key in ("name", "conclusion", "startedAt", "completedAt", "url")} for job in record["jobs"]]
    if record["headSha"] != head:
        raise RuntimeError(f"run {run} is for {record['headSha'][:7]}, HEAD is {head[:7]}")
    OUT.mkdir(parents=True, exist_ok=True)
    target = OUT / "ci-evidence.json"
    target.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(f"{target}: {record['workflowName']} {record['conclusion']} at {head[:7]}, {len(record['jobs'])} jobs")


def native(args) -> None:
    if not GALLERY.is_file():
        raise RuntimeError(f"build the Debug Gallery first: {GALLERY}")
    VERIFY_DIR.mkdir(parents=True, exist_ok=True)
    for flag in ("--verify", "--verify-comparison"):
        subprocess.run([str(GALLERY), flag, "--output", str(VERIFY_DIR)], cwd=ROOT, check=True)
    files = [VERIFY_DIR / name for name in ("interaction.txt", "capture-info.txt") if (VERIFY_DIR / name).is_file()]
    log = (VERIFY_DIR / "interaction.txt").read_text(encoding="utf-8")
    passed, failed = log.count("PASS "), log.count("FAIL ")
    if failed or not passed:
        raise RuntimeError(f"interaction log: {passed} passed, {failed} failed")
    record = {
        "version": version(),
        "result": "pass",
        "scope": f"Gallery public-IO interaction verification ({passed} checks) and comparison verification. "
                 "Native OS input, IME, screen readers and physical DPI are not covered.",
        "files": [{"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in files],
    }
    OUT.mkdir(parents=True, exist_ok=True)
    target = OUT / "native-evidence.json"
    target.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(f"{target}: {passed} checks passed, {len(files)} files")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="kind", required=True)
    ci_parser = sub.add_parser("ci", help="record the push CI run at HEAD")
    ci_parser.add_argument("--run", type=int, help="run id; found from HEAD when omitted")
    ci_parser.set_defaults(func=ci)
    native_parser = sub.add_parser("native", help="run the Gallery verification and record its logs")
    native_parser.set_defaults(func=native)
    args = parser.parse_args()
    args.func(args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
