"""Full test pipeline: configure + build with CMake, run the C++ test suite,
then cross-check perft counts against python-chess.

Every step's command line and output are written to results/. Expects CMake,
Ninja and a C++17 compiler on PATH (run_tests.bat sets this up for MSVC).
Exits non-zero if any step fails.
"""

import platform
import subprocess
import sys
from datetime import datetime
from pathlib import Path

import chess

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
RESULTS = ROOT / "results"
EXE = ".exe" if platform.system() == "Windows" else ""


def run(name, command):
    """Run a command, echo its output, and save command + output to results/<name>.txt."""
    print(f"\n### {name}: {' '.join(command)}", flush=True)
    proc = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    output = proc.stdout + proc.stderr
    print(output, end="", flush=True)
    (RESULTS / f"{name}.txt").write_text(
        f"$ {' '.join(command)}\n{output}\n[exit code {proc.returncode}]\n", encoding="utf-8")
    return proc.returncode


def main():
    RESULTS.mkdir(exist_ok=True)
    steps = [
        ("01_configure", ["cmake", "-S", ".", "-B", "build", "-G", "Ninja",
                          "-DCMAKE_BUILD_TYPE=Release"]),
        ("02_build", ["cmake", "--build", "build"]),
        ("03_engine_tests", [str(BUILD / f"chess_tests{EXE}")]),
        ("04_python_chess_crosscheck", [sys.executable, "tools/crosscheck_python_chess.py",
                                        str(BUILD / f"chess{EXE}"), str(BUILD)]),
    ]

    failures = []
    for name, command in steps:
        code = run(name, command)
        if code != 0:
            failures.append(name)
            if name in ("01_configure", "02_build"):
                break

    (RESULTS / "00_environment.txt").write_text(
        f"date: {datetime.now().isoformat(timespec='seconds')}\n"
        f"platform: {platform.platform()}\n"
        f"processor: {platform.processor()}\n"
        f"python: {sys.version.split()[0]}\n"
        f"python-chess: {chess.__version__}\n"
        f"failed steps: {', '.join(failures) if failures else 'none'}\n",
        encoding="utf-8")

    print(f"\n### overall: {'FAILED: ' + ', '.join(failures) if failures else 'all steps passed'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
