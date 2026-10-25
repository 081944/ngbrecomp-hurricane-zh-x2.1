#!/usr/bin/env python3
"""Put a program's DirectSound functions behind hooks (src/audio/dsound.c).

    take_dsound.py [trilogy|tomoe]

Each function named in the program's symbol list (XbSymbolDatabase's names for its XDK 5849 DSOUND build, with
the bytes each pops off the stack: tools/dsound_symbols.txt for the game, tools/dsound_symbols_arcade.txt for
the arcade's two programs, which share their addresses) is moved from its generated code into
src/audio/dsound_orig[_<program>].c as dsound_orig_<address>, its address listed in the program's manual
functions (src/manual_functions.json, src/manual_functions_arcade.json), and its hooks are written:
sub_<address> calls the hook, and the recompiled body when the hook does not take the call, and the hooks'
file holds the program's table of the functions. src/audio/dsound_funcs.h numbers every function any program
has. Running it again only rewrites the hooks and the tables; functions already moved stay where they are.
"""
import glob
import json
import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
out = os.path.join(root, 'src', 'audio')

PROGRAMS = {
    'game': ('dsound_symbols.txt', 'gen', 'manual_functions.json', 'dsound_orig.c', 'dsound_hooks.c'),
    'trilogy': ('dsound_symbols_arcade.txt', 'gen_trilogy', 'manual_functions_arcade.json',
                'dsound_orig_trilogy.c', 'dsound_hooks_arcade.c'),
    'tomoe': ('dsound_symbols_arcade.txt', 'gen_tomoe', 'manual_functions_arcade.json',
              'dsound_orig_tomoe.c', 'dsound_hooks_arcade.c'),
}


def symbols(name):
    funcs = []
    for line in open(os.path.join(here, name)):
        n, rest = [x.strip() for x in line.split('=')]
        addr, stack = rest.split()
        funcs.append((int(addr, 16), n.split('__', 1)[1], int(stack)))
    return sorted(funcs)


def take(gen, va, orig_name):
    name = 'sub_%08X' % va
    for path in sorted(glob.glob(os.path.join(gen, 'recomp_*.c'))):
        text = open(path).read()
        m = re.search(r'/\*\*\n \* %s\n.*?\n\}\n' % name, text, re.S)
        if not m:
            continue
        open(path, 'w').write(text[:m.start()] +
                              '/* %s: hand-written in src/audio/%s */\n' % (name, orig_name) +
                              text[m.end():])
        return m.group(0).replace('void %s(void)' % name, 'void dsound_orig_%08X(void)' % va, 1)
    return None


def main():
    prog = sys.argv[1] if len(sys.argv) > 1 else 'game'
    sym, gen_dir, manual_name, orig_name, hooks_name = PROGRAMS[prog]
    gen = os.path.join(root, 'src', 'recomp', gen_dir)
    funcs = symbols(sym)
    orig_path = os.path.join(out, orig_name)
    have = open(orig_path).read() if os.path.exists(orig_path) else (
        '/* The recompiled DirectSound functions, moved here by tools/take_dsound.py:\n'
        ' * src/audio/%s calls them when the hook does not take the call. */\n'
        '#define RECOMP_GENERATED_CODE\n#include "recomp_funcs.h"\n\n' % hooks_name)
    manual = os.path.join(root, 'src', manual_name)
    listed = json.load(open(manual)) if os.path.exists(manual) else []
    for va, base, stack in funcs:
        if 'dsound_orig_%08X' % va in have:
            continue
        body = take(gen, va, orig_name)
        if body is None:
            raise SystemExit('sub_%08X (%s) not found' % (va, base))
        have += body + '\n'
        if '0x%08X' % va not in listed:
            listed.append('0x%08X' % va)
    open(orig_path, 'w').write(have)
    with open(manual, 'w') as f:
        json.dump(listed, f, indent=2)
        f.write('\n')

    names = sorted({base for p in set(v[0] for v in PROGRAMS.values()) for _, base, _ in symbols(p)})
    with open(os.path.join(out, 'dsound_funcs.h'), 'w') as f:
        f.write('/* Every DirectSound function a program hooks (tools/take_dsound.py, from tools/dsound_symbols*.txt);\n'
                ' * each program\'s hooks file holds its table (ds_funcs), the functions it does not have zero. */\n')
        f.write('#pragma once\n#include <stdint.h>\n\n')
        f.write('typedef struct { uint32_t va; const char *name; int stack; } DsFunc;\n\n')
        f.write('enum {\n')
        for i, base in enumerate(names):
            f.write('    DSF_%s = %d,\n' % (base, i))
        f.write('    DSF_COUNT = %d\n};\n\n' % len(names))
        f.write('extern const DsFunc ds_funcs[DSF_COUNT];\n')

    with open(os.path.join(out, hooks_name), 'w') as f:
        f.write('/* Every DirectSound entry point of the %s (tools/take_dsound.py writes this):\n'
                ' * the hook first (src/audio/dsound.c), the recompiled body unless it took the call. */\n'
                % ('game' if prog == 'game' else 'arcade\'s programs (NinjaTrilogy and NinjaTomoe share them)'))
        f.write('#define RECOMP_GENERATED_CODE\n#include "recomp_funcs.h"\n#include "dsound_funcs.h"\n\n')
        f.write('const DsFunc ds_funcs[DSF_COUNT] = {\n')
        for va, base, stack in funcs:
            f.write('    [DSF_%s] = { 0x%08X, "%s", %d },\n' % (base, va, base, stack))
        f.write('};\n\nint dsound_hook(int fn);\n\n')
        for va, base, stack in funcs:
            f.write('void dsound_orig_%08X(void);\n' % va)
            f.write('void sub_%08X(void)  /* %s */\n{\n    if (!dsound_hook(DSF_%s))\n'
                    '        dsound_orig_%08X();\n}\n\n' % (va, base, base, va))
    print('%s: %d functions' % (prog, len(funcs)))


if __name__ == '__main__':
    main()
