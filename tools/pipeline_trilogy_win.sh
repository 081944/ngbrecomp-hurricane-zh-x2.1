#!/usr/bin/env bash
# tools/pipeline_trilogy.sh for a Windows host (Git Bash), as tools/pipeline_win.sh is for tools/pipeline.sh:
# the XBE is given directly and the Python is a given interpreter. The x2.1 disc's NinjaTrilogy.xbe and
# NinjaTomoe.xbe are the retail disc's, byte for byte.
#
#   NGB_PY=<python.exe with capstone> tools/pipeline_trilogy_win.sh path/to/NinjaTrilogy.xbe
#   NGB_PY=<python.exe with capstone> tools/pipeline_trilogy_win.sh path/to/NinjaTomoe.xbe tomoe
set -euo pipefail
export PYTHONIOENCODING=utf-8   # the XBE's debug path is printed; a GBK console cannot encode it

here=$(cd "$(dirname "$0")/.." && pwd)
toolkit=${XBOXRECOMP_DIR:-$here/xboxrecomp}
toolkit=$(cd "$toolkit" && pwd)
xbe=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
py=${NGB_PY:?set NGB_PY to a python with capstone}
[ -f "$xbe" ] || { echo "no XBE: $xbe" >&2; exit 1; }
if [ "${2:-}" = tomoe ]; then
    out=$here/build/tomoe gen=$here/src/recomp/gen_tomoe name="Ninja Gaiden (NES, Japanese)"
else
    out=$here/build/trilogy gen=$here/src/recomp/gen_trilogy name="Ninja Gaiden (NES)"
fi
mkdir -p "$out/disasm" "$out/func_id" "$out/abi" "$out/recomp"
cd "$toolkit"
echo "== parse";   "$py" -m tools.xbe_parser "$xbe" --json "$out/analysis.json" >/dev/null
seeds=()
[ -f "$here/src/icall_targets_trilogy.json" ] && seeds=(--seed-functions "$here/src/icall_targets_trilogy.json")
echo "== disasm";  "$py" -m tools.disasm "$xbe" --force -o "$out/disasm" --analysis-json "$out/analysis.json" "${seeds[@]}" >/dev/null
echo "== func_id"; "$py" -m tools.func_id "$xbe" --functions "$out/disasm/functions.json" \
                      --strings "$out/disasm/strings.json" --xrefs "$out/disasm/xrefs.json" -o "$out/func_id" >/dev/null
echo "== abi";     "$py" -m tools.abi_analysis "$xbe" --disasm-dir "$out/disasm" --func-id-dir "$out/func_id" \
                      --output-dir "$out/abi" >/dev/null
echo "== recomp"
mkdir -p "$gen" && find "$gen" -name "*.[ch]" -delete
# The DirectSound functions src/audio has by hand (tools/take_dsound.py): the generator leaves them out.
"$py" -m tools.recomp "$xbe" --all --split 250 --game-name "$name" \
    --manual-functions "$here/src/manual_functions_arcade.json" \
    --disasm-dir "$out/disasm" --func-id-dir "$out/func_id" --abi-dir "$out/abi" \
    -o "$out/recomp" --gen-dir "$gen" > "$out/recomp_log.txt" 2>&1 \
    || { tail -20 "$out/recomp_log.txt"; exit 1; }
tail -3 "$out/recomp_log.txt"
