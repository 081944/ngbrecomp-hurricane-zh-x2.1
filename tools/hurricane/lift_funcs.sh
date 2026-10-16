#!/usr/bin/env bash
# Lift single functions of an XBE as the pipeline would, into a separate folder, hand-written ones
# included: the inputs of a three-way merge (retail lift = base, ngbrecomp's hand-written = theirs,
# the Hurricane x2.1 lift = ours).
#
#   NGB_PY=<python> tools/hurricane/lift_funcs.sh <default.xbe> <out-dir> 0x001E6D30 [0x... ...]
#
# The analysis (parse, disasm with the same seeds, func_id, abi) goes to <out-dir>/analysis, so the
# pipeline's own outputs in xboxrecomp/tools/*/output are not touched (and reused when present). Each
# function is written to <out-dir>/sub_XXXXXXXX.c (the lifter prints it), its messages to .log. The pipeline's --flags-after-call table is applied (it only matters at the
# x2.1 call sites; the retail XBE has no call there).
set -euo pipefail
here=$(cd "$(dirname "$0")/../.." && pwd)
toolkit=$here/xboxrecomp
xbe=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
out=$(mkdir -p "$2" && cd "$2" && pwd)
shift 2
py=${NGB_PY:?set NGB_PY to a python with capstone}
a=$out/analysis
mkdir -p "$a/disasm" "$a/func_id" "$a/abi"
cd "$toolkit"
if [ ! -f "$a/abi/abi_functions.json" ]; then
"$py" -m tools.xbe_parser "$xbe" --json "$a/default_analysis.json" >/dev/null
"$py" -m tools.disasm "$xbe" --force -o "$a/disasm" --analysis-json "$a/default_analysis.json" \
    --seed-functions "$here/src/icall_targets.json" >/dev/null
"$py" -m tools.func_id "$xbe" --functions "$a/disasm/functions.json" --strings "$a/disasm/strings.json" \
    --xrefs "$a/disasm/xrefs.json" -o "$a/func_id" >/dev/null
"$py" -m tools.abi_analysis --disasm-dir "$a/disasm" --func-id-dir "$a/func_id" --output-dir "$a/abi" "$xbe" >/dev/null
fi
for f in "$@"; do
    name=$(printf 'sub_%08X' "$((f))")
    "$py" -m tools.recomp "$xbe" -f "$f" --disasm-dir "$a/disasm" --func-id-dir "$a/func_id" --abi-dir "$a/abi" \
        --flags-after-call "$here/src/flags_after_call.json" --flags-out "$here/src/flags_out.json" \
        > "$out/$name.c" 2> "$out/$name.log" \
        || { tail -20 "$out/$name.log"; exit 1; }
    echo "$name: $(grep -c '' "$out/$name.c") lines"
done
