#!/usr/bin/env python3
"""Replace the body of a lifted function kept verbatim in a hand-written file with the x2.1 lift.

    python tools/hurricane/merge_static_body.py <file.c> <c-name> <retail-lift.c> <x21-lift.c>

ngbrecomp keeps some lifted functions unchanged behind a wrapper of its own (src/menu_mouse.c:
`void sub_001E86A0(void) { ...; menu_text(); }` with `static void menu_text(void)` the lifted body).
That is a three-way merge whose 'theirs' side made no change: this asserts exactly that -- the body in
the file is the retail lift, line for line -- and only then puts the x2.1 lift's body in its place.
If ngbrecomp ever edits such a body the assertion fails, and the merge has to be done by hand.
The bodies come from tools/hurricane/lift_funcs.sh.
"""
import re
import sys

path, cname, base_path, ours_path = sys.argv[1:5]


def lifted_body(p):
    t = open(p, encoding='utf-8').read()
    m = re.search(r'\nvoid sub_[0-9A-F]{8}\(void\)\n\{\n(.*?)\n\}\n', t, re.S)
    assert m, p
    return m.group(1)


text = open(path, encoding='utf-8').read()
m = re.search(r'\n(static )?void %s\(void\)\n\{\n(.*?)\n\}\n' % re.escape(cname), text, re.S)
assert m, cname
base, ours = lifted_body(base_path), lifted_body(ours_path)
assert m.group(2) == base, '%s: the body in %s is not the retail lift -- merge by hand' % (cname, path)
if base == ours:
    sys.exit('%s: the x2.1 lift is the same as retail; nothing to do' % cname)
text = text[:m.start(2)] + ours + text[m.end(2):]
open(path, 'w', encoding='utf-8', newline='\n').write(text)
print('%s: %d -> %d lines' % (cname, base.count('\n') + 1, ours.count('\n') + 1))
