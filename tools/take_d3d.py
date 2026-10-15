#!/usr/bin/env python3
"""Put the game's Direct3D functions behind hooks (docs/renderer.md).

    take_d3d.py

Each function named in tools/d3d_symbols.txt (XbSymbolDatabase's names for
the title's XDK 5849 D3D8LTCG build) is moved from src/recomp/gen into
src/d3d/d3d_orig.c as d3d_orig_<address>, its address listed in
src/manual_functions.json, and src/d3d/d3d_hooks.c and d3d_funcs.h are
written: sub_<address> calls the hook (src/d3d/d3d.c), and the recompiled
body when the hook does not take the call. Running it again only rewrites
the hooks and the table; functions already moved stay where they are.
"""
import glob
import json
import os
import re

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
gen = os.path.join(root, 'src', 'recomp', 'gen')
out = os.path.join(root, 'src', 'd3d')
manual = os.path.join(root, 'src', 'manual_functions.json')


def symbols():
    funcs = []
    for line in open(os.path.join(here, 'd3d_symbols.txt')):
        name, addr = [x.strip() for x in line.split('=')]
        name = name.split('__', 1)[1]
        m = re.match(r'(.*?)(?:_(\d+))?(?:__LTCG_(.*))?$', name)
        base, stack, regs = m.group(1), m.group(2), m.group(3)
        argregs = {}
        for r in re.findall(r'(e[a-z]{2})(\d)', regs or ''):
            argregs[int(r[1])] = r[0]
        funcs.append((int(addr, 16), base, int(stack) if stack else -1, argregs))
    return sorted(funcs)


def take(va):
    name = 'sub_%08X' % va
    for path in sorted(glob.glob(os.path.join(gen, 'recomp_*.c'))):
        text = open(path).read()
        m = re.search(r'/\*\*\n \* %s\n.*?\n\}\n' % name, text, re.S)
        if not m:
            continue
        open(path, 'w').write(text[:m.start()] +
                              '/* %s: hand-written in src/d3d/d3d_orig.c */\n' % name +
                              text[m.end():])
        return m.group(0).replace('void %s(void)' % name, 'void d3d_orig_%08X(void)' % va, 1)
    return None


def main():
    os.makedirs(out, exist_ok=True)
    funcs = symbols()
    orig_path = os.path.join(out, 'd3d_orig.c')
    have = open(orig_path).read() if os.path.exists(orig_path) else (
        '/* The recompiled Direct3D functions, moved here by tools/take_d3d.py:\n'
        ' * src/d3d/d3d_hooks.c calls them when the hook does not take the call. */\n'
        '#define RECOMP_GENERATED_CODE\n#include "recomp_funcs.h"\n\n')
    listed = json.load(open(manual))
    for va, base, stack, regs in funcs:
        if 'd3d_orig_%08X' % va in have:
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

    with open(os.path.join(out, 'd3d_funcs.h'), 'w') as f:
        f.write('/* The title\'s Direct3D functions (tools/take_d3d.py, from tools/d3d_symbols.txt). */\n')
        f.write('#pragma once\n#include <stdint.h>\n\n')
        f.write('typedef struct { uint32_t va; const char *name; int stack; const char *regs[4]; } D3dFunc;\n\n')
        f.write('enum {\n')
        for i, (va, base, stack, regs) in enumerate(funcs):
            f.write('    D3DF_%s = %d,\n' % (base, i))
        f.write('    D3DF_COUNT = %d\n};\n\n' % len(funcs))
        f.write('#ifdef D3D_FUNCS_TABLE\nstatic const D3dFunc d3d_funcs[D3DF_COUNT] = {\n')
        for va, base, stack, regs in funcs:
            rs = ', '.join('"%s"' % regs[i] if i in regs else '0' for i in range(1, 5))
            f.write('    { 0x%08X, "%s", %d, { %s } },\n' % (va, base, stack, rs))
        f.write('};\n#endif\n')

    with open(os.path.join(out, 'd3d_hooks.c'), 'w') as f:
        f.write('/* Every Direct3D entry point of the title (tools/take_d3d.py writes this):\n'
                ' * the hook first (src/d3d/d3d.c), the recompiled body unless it took the call. */\n')
        f.write('#define RECOMP_GENERATED_CODE\n#include "recomp_funcs.h"\n#include "d3d_funcs.h"\n\n')
        f.write('int d3d_hook(int fn);\nvoid d3d_after(int fn);\n\n')
        for va, base, stack, regs in funcs:
            f.write('void d3d_orig_%08X(void);\n' % va)
            f.write('void sub_%08X(void)  /* %s */\n{\n    if (!d3d_hook(D3DF_%s)) {\n'
                    '        d3d_orig_%08X();\n        d3d_after(D3DF_%s);\n    }\n}\n\n'
                    % (va, base, base, va, base))
    print('%d functions' % len(funcs))


if __name__ == '__main__':
    main()
