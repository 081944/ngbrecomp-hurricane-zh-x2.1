#!/usr/bin/env python3
"""Merge the Screen Settings page's three functions in src/pc_menu.c for the Hurricane x2.1 build.

    python tools/hurricane/merge_screen.py <merge-dir>

<merge-dir>/<VA>/{base,ours,theirs}.c are the bodies of sub_0017F4E0, sub_0021F080, sub_00220870 lifted
from the retail XBE (base) and the x2.1 XBE (ours, tools/hurricane/lift_funcs.sh with the flags tables)
and as src/pc_menu.c has them (theirs). The pages decide which side runs: ngbrecomp's own pages
(is_ours(page): RECOMP SETTINGS' 10..14) keep ngbrecomp's code, the game's pages -- Screen Settings 4 / 6
with x2.1's crosshair row -- run x2.1's. Every replaced text must be found exactly once (asserted), and
each function's untouched lines are checked against base / theirs / ours as noted. Also drops
pc_menu_init's rewrite of pages 4 / 6 (it rebuilt them as retail's, removing x2.1's crosshair row).
"""
import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(os.path.dirname(here))
pc_menu = os.path.join(root, 'src', 'pc_menu.c')
mdir = sys.argv[1]


def read(va, side):
    return open(os.path.join(mdir, va, side + '.c'), encoding='utf-8').read()


def once(text, old, new, what):
    n = text.count(old)
    assert n == 1, '%s: found %d times' % (what, n)
    return text.replace(old, new)


# ---------------- sub_0017F4E0: the page's input ----------------
# ours + theirs merged cleanly (git merge-file); the x2.1 hook is gated off on ngbrecomp's pages.
import subprocess
d = os.path.join(mdir, '0017F4E0')
r = subprocess.run(['git', 'merge-file', '-p', os.path.join(d, 'ours.c'), os.path.join(d, 'base.c'),
                    os.path.join(d, 'theirs.c')], capture_output=True, text=True, encoding='utf-8')
assert r.returncode == 0, 'sub_0017F4E0 no longer merges cleanly'
f17 = r.stdout
f17 = once(f17,
           'loc_0017F567: ;\n'
           '    PUSH32(esp, 0x0017F56Cu); RECOMP_ABI_CALL(0x00DC8A40u, sub_00DC8A40); /* call 0x00DC8A40 */\n'
           '    /* flags as 0x0017F567\'s callee left them: cmp dl, 0xff */\n',
           'loc_0017F567: ;\n'
           '    if (is_ours(MEM8(edi))) {\n'
           '        SET_LO8(edx, MEM8(edi + 0xC));   /* Hurricane: PC pages, the original instruction */\n'
           '    } else {\n'
           '    PUSH32(esp, 0x0017F56Cu); RECOMP_ABI_CALL(0x00DC8A40u, sub_00DC8A40); /* call 0x00DC8A40 */\n'
           '    }\n'
           '    /* flags as 0x0017F567\'s callee left them: cmp dl, 0xff (and the original\'s own on PC pages) */\n',
           '17F567 gate')

# ---------------- sub_0021F080: the page's drawing ----------------
f21 = read('0021F080', 'ours')
th21 = read('0021F080', 'theirs')
# 1. rows and their names: x2.1's counts on pages 4 / 6, then ngbrecomp's own pages over them
# Both paths (widescreen 0x21F0F8 jumps there, 4:3 0x21F10D falls through) meet at 0x21F11D, where
# ngbrecomp's own pages take their rows -- as both of ngbrecomp's paths met at its loc_0021F11C.
f21 = once(f21,
           '    MEM32(esp + 0x1C) = 3;\n'
           '    MEM32(esp + 0x4C) = edx;\n'
           '\n'
           'loc_0021F11D: ;\n',
           '    MEM32(esp + 0x1C) = 3;\n'
           '    MEM32(esp + 0x4C) = edx;\n'
           '    pc_menu_init();\n'
           '\n'
           'loc_0021F11D: ;\n'
           '    /* PC: the page\'s rows and their text (ngbrecomp\'s pages; 4 and 6 keep x2.1\'s 5 / 3 rows). */\n'
           '    if (is_ours(page_now())) {\n'
           '        MEM32(esp + 0x1C) = (uint32_t)s_ours[page_now() - PAGE_GFX].n;\n'
           '        MEM32(esp + 0x4C) = s_labels_ours[page_now() - PAGE_GFX];\n'
           '    }\n',
           '21F0F8 rows')
assert f21.count('goto loc_0021F11D;') == 1 and f21.index('goto loc_0021F11D;') < f21.index('loc_0021F11D: ;')
assert ('loc_0021F11C: ;\n    if (is_ours(page_now())) {\n        MEM32(esp + 0x1C) = (uint32_t)s_ours[page_now() - PAGE_GFX].n;\n'
        '        MEM32(esp + 0x4C) = s_labels_ours[page_now() - PAGE_GFX];\n    }\n' in th21)
f21 = once(f21, 'loc_0021F0F8: ;\n    ecx = esp + 0x80;\n    MEM32(esp + 0x1C) = 5;\n',
           'loc_0021F0F8: ;\n    pc_menu_init();\n    ecx = esp + 0x80;\n    MEM32(esp + 0x1C) = 5;\n', '21F0F8 init')
# 5. ngbrecomp's layout for pages of more than seven rows (theirs, verbatim)
raise1 = ('    ebx = 0x70;\n'
          '    if ((int)MEM32(esp + 0x1C) > 7)\n'
          '        ebx = 0x70 - PC_RAISE;                   /* PC: more than seven rows, a little higher */\n')
assert raise1 in th21
f21 = once(f21, '    ebx = 0x70;\n', raise1, 'raise')
raise2 = ('    esi = MEM32(0x43B478);\n'
          '    if ((int)MEM32(esp + 0x1C) > 7)\n'
          '        esi -= PC_CLOSER;                        /* PC: and a little closer */\n')
assert raise2 in th21
f21 = once(f21, '    esi = MEM32(0x43B478);\n', raise2, 'closer')
# 2. the hint flag (0x21F200, rewritten in place by x2.1): retail's computation on ngbrecomp's pages
hint_x21 = f21[f21.index('loc_0021F200: ;\n'):f21.index('    edx = MEM32(esp + 0x10);\n    edi = esp + 0x14;\n')]
base21 = read('0021F080', 'base')
hint_base = base21[base21.index('loc_0021F200: ;\n'):base21.index('    edx = MEM32(esp + 0x10);\n    edi = esp + 0x14;\n')]
assert hint_base in th21, 'ngbrecomp changed the hint flag'
hint_retail = (hint_base.replace('loc_0021F200: ;\n', '')
               .replace('loc_0021F209: ;\n', '')
               .replace('goto loc_0021F21E;', 'goto hz_hint_21E;').replace('loc_0021F21E: ;', 'hz_hint_21E: ;')
               .replace('goto loc_0021F223;', 'goto hz_hint_223;').replace('loc_0021F223: ;', 'hz_hint_223: ;'))
assert 'loc_0021F2' not in hint_retail, hint_retail
f21 = once(f21, hint_x21,
           'loc_0021F200: ;\n'
           '    if (is_ours(page_now())) {   /* Hurricane: PC pages, the retail hint flag */\n'
           + hint_retail +
           '    goto hz_hint_done;\n'
           '    }\n'
           + hint_x21.replace('loc_0021F200: ;\n', '') +
           'hz_hint_done: ;\n',
           '21F200 hint')
# 3. the row names' table (0x21F3A9): ngbrecomp's pages take theirs from [esp+0x4C] as retail did
f21 = once(f21,
           'loc_0021F3A9: ;\n'
           '    g_ebp = ebp; /* frame stays current across calls */\n'
           '    g_seh_ebp = ebp;\n'
           '    PUSH32(esp, 0x0021F3AEu); RECOMP_ABI_CALL(0x00DC8980u, sub_00DC8980); /* call 0x00DC8980 */\n',
           'loc_0021F3A9: ;\n'
           '    if (is_ours(page_now())) {   /* Hurricane: PC pages, the original instructions */\n'
           '    edx = MEM32(esp + 0x4C);\n'
           '    eax = SX16(LO16(ebx));\n'
           '    } else {\n'
           '    g_ebp = ebp; /* frame stays current across calls */\n'
           '    g_seh_ebp = ebp;\n'
           '    PUSH32(esp, 0x0021F3AEu); RECOMP_ABI_CALL(0x00DC8980u, sub_00DC8980); /* call 0x00DC8980 */\n'
           '    }\n',
           '21F3A9 names')
assert '    edx = MEM32(esp + 0x4C);\n    eax = SX16(LO16(ebx));\n' in base21
# 6. the values (0x21F542 / 0x21F557)
old_val = (
    'loc_0021F542: ;\n'
    '    g_ebp = ebp; /* frame stays current across calls */\n'
    '    g_seh_ebp = ebp;\n'
    '    PUSH32(esp, 0x0021F547u); RECOMP_ABI_CALL(0x00DC89A0u, sub_00DC89A0); /* call 0x00DC89A0 */\n'
    '\n'
    'loc_0021F547: ;\n'
    '    /* nop */\n'
    '    /* nop */\n'
    '    if (_flags /* je: equal / zero */) goto loc_0021F5BF;\n')
new_val = (
    'loc_0021F542: ;\n'
    '    if (is_ours(page_now())) {   /* PC pages: ngbrecomp\'s values (a text for each kind of row) */\n'
    '        eax = MEM32(0x43B474);\n'
    '        ecx = MEM32(esp + 0x18);\n'
    '        edi = eax + ecx;\n'
    '        eax = pc_value_id((int)MEM32(esp + 0x10),\n'
    '                          MEM32(esp + ZX8(MEM8(0xDA42EF)) * 4 + 0x78));\n'
    '        if (eax == 0) goto loc_0021F5BF;\n'
    '        goto hz_value_text;\n'
    '    }\n'
    '    g_ebp = ebp; /* frame stays current across calls */\n'
    '    g_seh_ebp = ebp;\n'
    '    PUSH32(esp, 0x0021F547u); RECOMP_ABI_CALL(0x00DC89A0u, sub_00DC89A0); /* call 0x00DC89A0 */\n'
    '\n'
    'loc_0021F547: ;\n'
    '    /* Hurricane: the hook\'s ZF, from its two returns -- row 1 (the crosshair it just drew): `cmp eax,eax`,\n'
    '     * ZF set; any other row: the replaced `test byte [0xD21748],1`, ZF when not widescreen. Its [esp+0x14]\n'
    '     * is this frame\'s [esp+0x10] (the call pushed the return address). */\n'
    '    if (MEM32(esp + 0x10) == 1 || !(MEM8(0xD21748) & 1)) goto loc_0021F5BF;\n')
f21 = once(f21, old_val, new_val, '21F542 value')
assert ('eax = pc_value_id((int)MEM32(esp + 0x10),\n                      MEM32(esp + ZX8(MEM8(0xDA42EF)) * 4 + 0x78));\n'
        '    if (eax == 0) goto loc_0021F5BF;\n' in th21)
f21 = once(f21,
           'loc_0021F55E: ;\n'
           '    eax = ZX8(MEM8(0xDA42EF));\n'
           '    eax = MEM32(esp + eax * 4 + 0x78);\n',
           'loc_0021F55E: ;\n'
           '    eax = ZX8(MEM8(0xDA42EF));\n'
           '    eax = MEM32(esp + eax * 4 + 0x78);\n'
           '\n'
           'hz_value_text: ;\n',
           '21F55E label')

# ---------------- sub_00220870: the information line ----------------
f22 = read('00220870', 'theirs')
f22 = once(f22,
           '        else if (tab == 2 && row < 6) id = control[row];\n'
           '        else if (tab == 3) {\n'
           '            int kind = row_kind((int)row);\n'
           '            if (kind >= 0)\n'
           '                id = video_kind[kind];\n'
           '        }\n',
           '        /* Hurricane x2.1 (its hook at 0x2209FA / 0x220A22, 0xDC8790): Controller Settings has 7 rows,\n'
           '         * Screen Settings 5 (widescreen) or 3, their descriptions in tables of the XBE\'s HANZI data. */\n'
           '        else if (tab == 2 && row < 7) id = MEM16(0xDC8DC8 + 2 * row);\n'
           '        else if (tab == 2 && row < 6) id = control[row];\n'
           '        else if (tab == 3 && (MEM8(0xD21748) & 1) && row < 5) id = MEM16(0xDC8EE8 + 2 * row);\n'
           '        else if (tab == 3 && !(MEM8(0xD21748) & 1) && row < 3) id = MEM16(0xDC8EF4 + 2 * row);\n'
           '        else if (tab == 3) {\n'
           '            int kind = row_kind((int)row);\n'
           '            if (kind >= 0)\n'
           '                id = video_kind[kind];\n'
           '        }\n',
           '220870 tables')

# ---------------- into pc_menu.c ----------------
text = open(pc_menu, encoding='utf-8').read()
for name, body in (('sub_0017F4E0', f17), ('sub_0021F080', f21), ('sub_00220870', f22)):
    m = re.search(r'\nvoid %s\(void\)\n\{\n(.*?)\n\}\n' % name, text, re.S)
    assert m and m.group(1) + '\n' == read({'sub_0017F4E0': '0017F4E0', 'sub_0021F080': '0021F080',
                                            'sub_00220870': '00220870'}[name], 'theirs'), name + ' changed in pc_menu.c'
    text = text[:m.start(1)] + body.rstrip('\n') + text[m.end(1):]
text = once(text,
            '    /* Pages 4 (0x429D18) and 6 (0x429D48): last row, row table. */\n'
            '    MEM8(0x429D18 + 2) = N_WS - 1;\n'
            '    MEM32(0x429D18 + 0xC) = make_rows(s_rows_ws, N_WS);\n'
            '    MEM8(0x429D48 + 2) = N_STD - 1;\n'
            '    MEM32(0x429D48 + 0xC) = make_rows(s_rows_std, N_STD);\n',
            '    /* Pages 4 (0x429D18) and 6 (0x429D48) are left as the Hurricane x2.1 XBE has them (its crosshair row:\n'
            '     * 5 / 3 rows); rebuilt from s_rows_ws / s_rows_std they would be retail\'s. Those tables still name\n'
            '     * retail\'s rows for row_kind(), which the x2.1 hooks feed retail\'s row numbers. */\n',
            'page rebuild')
open(pc_menu, 'w', encoding='utf-8', newline='\n').write(text)
print('merged: sub_0017F4E0, sub_0021F080, sub_00220870; pages 4 / 6 left to the XBE')
