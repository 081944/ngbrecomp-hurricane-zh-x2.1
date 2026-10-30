#!/usr/bin/env python3
"""Run the existing game-code generation pipeline on Windows or POSIX hosts."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("iso", type=Path, help="Player's Ninja Gaiden Black disc image")
    parser.add_argument("--toolkit", type=Path,
                        default=Path(os.environ.get("XBOXRECOMP_DIR", root / "xboxrecomp")))
    args = parser.parse_args()
    toolkit = args.toolkit.resolve()
    iso = args.iso.resolve()
    if not iso.is_file():
        parser.error(f"Disc image not found: {iso}")
    try:
        import capstone  # noqa: F401; fail before extracting a large disc
    except ImportError:
        parser.error("Install capstone in this Python environment before running the pipeline")
    files = root / "game_files"
    xbe = files / "default.xbe"
    analysis = root / "build" / "analysis" / "default_analysis.json"
    analysis.parent.mkdir(parents=True, exist_ok=True)

    def run(module, *arguments, log=None):
        print(f"== {module}", flush=True)
        subprocess.run([sys.executable, "-m", module, *map(str, arguments)],
                       cwd=toolkit, check=True, stdout=log, stderr=subprocess.STDOUT)

    if not xbe.is_file():
        run("tools.xiso", "unpack", iso, "-o", files)
    run("tools.xbe_parser", xbe, "--json", analysis)
    seeds = root / "src" / "icall_targets.json"
    seed_args = ["--seed-functions", seeds] if seeds.is_file() else []
    run("tools.disasm", xbe, "--force", "--analysis-json", analysis, *seed_args)
    run("tools.func_id", xbe)
    run("tools.abi_analysis", xbe)
    log_path = root / "recomp_log.txt"
    with log_path.open("w", encoding="utf-8") as log:
        run("tools.recomp", xbe, "--all", "--split", "250",
            "--game-name", "Ninja Gaiden Black",
            "--exclude-manual", root / "src" / "recomp_manual.c",
            "--manual-functions", root / "src" / "manual_functions.json",
            "--gen-dir", root / "src" / "recomp" / "gen", log=log)
    print(f"Generated code: {root / 'src' / 'recomp' / 'gen'}", flush=True)
    print(f"Recompiler log: {log_path}", flush=True)


if __name__ == "__main__":
    main()
