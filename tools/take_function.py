#!/usr/bin/env python3
"""Move lifted functions out of the generated code, to be hand-edited.

    take_function.py 0x0020CDA0 [0x... ...] > copy.c

Each function (its doc comment through its closing brace) is printed and,
in src/recomp/gen, replaced with a note saying where it now lives; its
address goes into src/manual_functions.json so a regeneration leaves it out
too. The printed code is the starting point of the hand-written one.
"""
import glob
import json
import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
gen = os.path.join(root, 'src', 'recomp', 'gen')
manual = os.path.join(root, 'src', 'manual_functions.json')
where = os.environ.get('TAKE_INTO', 'src/pause_tab.c')


def take(va):
    name = 'sub_%08X' % va
    for path in sorted(glob.glob(os.path.join(gen, 'recomp_*.c'))):
        text = open(path).read()
        m = re.search(r'/\*\*\n \* %s\n.*?\n\}\n' % name, text, re.S)
        if not m:
            continue
        open(path, 'w').write(text[:m.start()] +
                              '/* %s: hand-written in %s */\n' % (name, where) +
                              text[m.end():])
        return m.group(0)
    sys.exit('%s not found in the generated code' % name)


def main():
    vas = [int(a, 16) for a in sys.argv[1:]]
    listed = json.load(open(manual))
    for va in vas:
        sys.stdout.write(take(va) + '\n')
        if '0x%08X' % va not in listed:
            listed.append('0x%08X' % va)
    with open(manual, 'w') as f:
        json.dump(listed, f, indent=2)
        f.write('\n')


if __name__ == '__main__':
    main()
