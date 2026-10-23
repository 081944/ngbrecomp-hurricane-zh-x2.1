#!/usr/bin/env python3
"""Put the game's DirectSound functions behind hooks (src/audio/dsound.c).

    take_dsound.py

Each function named in tools/dsound_symbols.txt (XbSymbolDatabase's names
for the title's XDK 5849 DSOUND build, with the bytes each pops off the
stack) is moved from src/recomp/gen into src/audio/dsound_orig.c as
dsound_orig_<address>, its address listed in src/manual_functions.json, and
src/audio/dsound_hooks.c and dsound_funcs.h are written: sub_<address> calls
the hook, and the recompiled body when the hook does not take the call.
Running it again only rewrites the hooks and the table; functions already
moved stay where they are.
"""
import glob
import json
import os
import re

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
gen = os.path.join(root, 'src', 'recomp', 'gen')
out = os.path.join(root, 'src', 'audio')
manual = os.path.join(root, 'src', 'manual_functions.json')


def symbols():
    funcs = []
    for line in open(os.path.join(here, 'dsound_symbols.txt')):
        name, rest = [x.strip() for x in line.split('=')]
        addr, stack = rest.split()
        funcs.append((int(addr, 16), name.split('__', 1)[1], int(stack)))
    return sorted(funcs)


def take(va):
    name = 'sub_%08X' % va
    for path in sorted(glob.glob(os.path.join(gen, 'recomp_*.c'))):
        text = open(path).read()
        m = re.search(r'/\*\*\n \* %s\n.*?\n\}\n' % name, text, re.S)
        if not m:
            continue
        open(path, 'w').write(text[:m.start()] +
                              '/* %s: hand-written in src/audio/dsound_orig.c */\n' % name +
                              text[m.end():])
        return m.group(0).replace('void %s(void)' % name, 'void dsound_orig_%08X(void)' % va, 1)
    return None


def main():
    os.makedirs(out, exist_ok=True)
    funcs = symbols()
    orig_path = os.path.join(out, 'dsound_orig.c')
    have = open(orig_path).read() if os.path.exists(orig_path) else (
        '/* The recompiled DirectSound functions, moved here by tools/take_dsound.py:\n'
        ' * src/audio/dsound_hooks.c calls them when the hook does not take the call. */\n'
        '#define RECOMP_GENERATED_CODE\n#include "recomp_funcs.h"\n\n')
    listed = json.load(open(manual))
    for va, base, stack in funcs:
        if 'dsound_orig_%08X' % va in have:
            continue
        body = take(va)
        if body is None:
            raise SystemExit('sub_%08X (%s) not found' % (va, base))
        have += body + '\n'
        if '0x%08X' % va not in listed:
            listed.append('0x%08X' % va)
    open(orig_path, 'w').write(have)
    with open(manual, 'w') as f:
        json.dump(listed, f, indent=2)
        f.write('\n')

    with open(os.path.join(out, 'dsound_funcs.h'), 'w') as f:
        f.write('/* The title\'s DirectSound functions (tools/take_dsound.py, from tools/dsound_symbols.txt). */\n')
        f.write('#pragma once\n#include <stdint.h>\n\n')
        f.write('typedef struct { uint32_t va; const char *name; int stack; } DsFunc;\n\n')
        f.write('enum {\n')
        for i, (va, base, stack) in enumerate(funcs):
            f.write('    DSF_%s = %d,\n' % (base, i))
        f.write('    DSF_COUNT = %d\n};\n\n' % len(funcs))
        f.write('#ifdef DS_FUNCS_TABLE\nstatic const DsFunc ds_funcs[DSF_COUNT] = {\n')
        for va, base, stack in funcs:
            f.write('    { 0x%08X, "%s", %d },\n' % (va, base, stack))
        f.write('};\n#endif\n')

    with open(os.path.join(out, 'dsound_hooks.c'), 'w') as f:
        f.write('/* Every DirectSound entry point of the title (tools/take_dsound.py writes this):\n'
                ' * the hook first (src/audio/dsound.c), the recompiled body unless it took the call. */\n')
        f.write('#define RECOMP_GENERATED_CODE\n#include "recomp_funcs.h"\n#include "dsound_funcs.h"\n\n')
        f.write('int dsound_hook(int fn);\n\n')
        for va, base, stack in funcs:
            f.write('void dsound_orig_%08X(void);\n' % va)
            f.write('void sub_%08X(void)  /* %s */\n{\n    if (!dsound_hook(DSF_%s))\n'
                    '        dsound_orig_%08X();\n}\n\n' % (va, base, base, va))
    print('%d functions' % len(funcs))


if __name__ == '__main__':
    main()
