#!/usr/bin/env bash
# tools/pipeline.sh for a Windows host (Git Bash): the same five steps with the same arguments, but
#   - the XBE is given directly (an already extracted, verified copy; nothing is unpacked here),
#   - the Python is a given interpreter (a Windows venv has Scripts/python.exe, not .venv/bin/python).
#
#   NGB_PY=<python.exe with capstone> tools/pipeline_win.sh path/to/default.xbe
set -euo pipefail

here=$(cd "$(dirname "$0")/.." && pwd)
toolkit=${XBOXRECOMP_DIR:-$here/xboxrecomp}
toolkit=$(cd "$toolkit" && pwd)
xbe=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
py=${NGB_PY:?set NGB_PY to a python with capstone}
[ -f "$xbe" ] || { echo "no XBE: $xbe" >&2; exit 1; }
cd "$toolkit"

analysis=$here/build/analysis/default_analysis.json
mkdir -p "$(dirname "$analysis")"
echo "== parse";    "$py" -m tools.xbe_parser "$xbe" --json "$analysis" >/dev/null
seeds=()
db=$here/src/icall_targets.json
[ -f "$db" ] && seeds=(--seed-functions "$db")
echo "== disasm";   "$py" -m tools.disasm "$xbe" --force --analysis-json "$analysis" "${seeds[@]}" >/dev/null
echo "== func_id";  "$py" -m tools.func_id "$xbe" >/dev/null
echo "== abi";      "$py" -m tools.abi_analysis "$xbe" >/dev/null
echo "== recomp"
"$py" -m tools.recomp "$xbe" --all --split 250 \
    --game-name "Ninja Gaiden Black" \
    --exclude-manual "$here/src/recomp_manual.c" \
    --manual-functions "$here/src/manual_functions.json" \
    --gen-dir "$here/src/recomp/gen" > "$here/recomp_log.txt" 2>&1 \
    || { tail -20 "$here/recomp_log.txt"; exit 1; }
tail -3 "$here/recomp_log.txt"
