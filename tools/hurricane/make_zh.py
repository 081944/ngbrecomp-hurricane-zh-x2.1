"""The recomp's own text in Chinese for the Hurricane x2.1 build (tools/hurricane/zh_text.tsv).

  python tools/hurricane/make_zh.py <game dir: the x2.1 files> [<localization project dir>]

The localization project (default E:\\games\\emu\\ninja gaiden black local) is read only: its charmap
(work/translate/charmap.tsv), its glyph renderer (tools/hanzi_font.py) and fonts. Writes:

  src/hurricane_zh_data.c        the text table; for the game's text table (use E) each string in the game's
                                 encoding: x2.1's charmap, plus the characters it lacks put in the 9 free cells
                                 of the common glyph page B1 (demo.afs #3 fontjpn.spr texture 1, 256 x 2048 =
                                 2048 cells, 2039 used) -- their codes, appended to jpfont.map's block 1, and
                                 their cells drawn exactly as x2.1's build draws its own (checked here: cells of
                                 x2.1's characters drawn again equal the disc's); the words drawn as pictures
  assets/fonts/NGBHanzi-Bold.ttf Source Han Sans CN Bold (SIL OFL 1.1) cut to the characters used, renamed as the
                                 licence asks of a modified font, TrueType outlines (stb_truetype)

The pictures' words are drawn with 鼎猎宋刻体 as x2.1's own (its licence allows pictures made with it, not the
font in a program): the pause menu's tabs (tools/menu_labels.py of the localization project) and the Options
menu's category words (tools/option_tabs.py).
"""
import os
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FORK = os.path.normpath(os.path.join(HERE, '..', '..'))
GAME = sys.argv[1]
LOC = sys.argv[2] if len(sys.argv) > 2 else r'E:\games\emu\ninja gaiden black local'
sys.path.insert(0, os.path.join(LOC, 'tools'))
import hanzi_font as hf        # noqa: E402
import xpr                     # noqa: E402
import afs                     # noqa: E402
import menu_labels             # noqa: E402
import option_tabs             # noqa: E402

B1_CELLS = hf.B1_HEIGHT // 16 * 16


def load_rows():
    rows = []
    for l in open(os.path.join(HERE, 'zh_text.tsv'), encoding='utf-8').read().split('\n')[1:]:
        if not l or l.startswith('#'):
            continue
        ctx, en, zh, use = l.split('\t')
        assert use in ('E', 'P'), l
        rows.append((ctx, en, zh, use))
    keys = [(r[0], r[1]) for r in rows]
    assert len(set(keys)) == len(keys), 'duplicate (ctx, en)'
    return rows


def afs_file(name, want):
    d, e, n = afs.read_afs(os.path.join(GAME, name))
    i = n.index(want)
    o, s = e[i]
    return d[o:o + s]


def font_page():
    """x2.1's B1 page: (indices 2048 x 256, its palette as RGBA rows, raw palette bytes)"""
    spr = afs_file('demo.afs', 'fontjpn.spr')
    total, hdr, texs, pals = xpr.parse(spr)
    t1, p1 = texs[1], pals[1]
    assert (t1['w'], t1['h']) == (256, hf.B1_HEIGHT), (t1['w'], t1['h'])
    lin = np.frombuffer(spr, np.uint8, 256 * hf.B1_HEIGHT, t1['data'])[xpr.swizzle_map(256, hf.B1_HEIGHT)]
    pal_raw = spr[p1['data']:p1['data'] + 0x400]
    return lin.reshape(hf.B1_HEIGHT, 256), pal_raw


def cell_of(ch, pal_raw):
    """ch's 16 x 16 cell as x2.1's build makes it (hanzi_font.build_fontjpn)"""
    palf = np.frombuffer(pal_raw, np.uint8).reshape(256, 4)[:, [2, 1, 0, 3]].astype(np.int32)
    cell = np.array(hf.render_glyph(ch)).astype(np.int32).reshape(-1, 4)
    dist = ((cell[:, None, :3] - palf[None, :, :3]) ** 2).sum(2) * (cell[:, None, 3] / 255.0) \
        + 4 * (cell[:, None, 3] - palf[None, :, 3]) ** 2
    return dist.argmin(1).astype(np.uint8).reshape(16, 16)


def fnv1a(b):
    h = 0x811C9DC5
    for x in b:
        h = ((h ^ x) * 0x01000193) & 0xFFFFFFFF
    return h


def engine_glyphs(rows):
    cm = hf.load_charmap()
    jp = hf.parse_jpfont(afs_file('spr.afs', 'jpfont.map'))
    b1 = jp['blocks'][1]
    expect = list(hf.parse_jpfont(open(os.path.join(LOC, 'work/afs/spr/0364_jpfont.map'), 'rb').read())['blocks'][1])
    expect += [c for ch, (c, s) in sorted(cm.items(), key=lambda kv: kv[1][1])]
    assert b1 == expect, "the disc's jpfont.map block 1 is not x2.1's charmap"
    need = sorted({ch for c, en, zh, u in rows if u == 'E' for ch in zh if ord(ch) > 0x7F and ch not in cm})
    free = B1_CELLS - len(b1)
    assert len(need) <= free, 'new characters %d > free cells %d: %s' % (len(need), free, ''.join(need))
    used = hf.used_codes(jp['blocks']) | {v[0] for v in cm.values()}
    it = hf.free_code_iter(used)
    new = {ch: (next(it), len(b1) + i) for i, ch in enumerate(need)}
    page, pal_raw = font_page()
    # the renderer is x2.1's: its own characters drawn again equal the disc's cells
    for ch in ['龙', '设', '置', '·', '…'] + [k for k in list(cm)[::97] if not 0xE000 <= ord(k) < 0xF900]:
        r, c = divmod(cm[ch][1], 16)
        assert (cell_of(ch, pal_raw) == page[r * 16:r * 16 + 16, c * 16:c * 16 + 16]).all(), 'cell of ' + ch
    blank = page[(B1_CELLS - 1) // 16 * 16, (B1_CELLS - 1) % 16 * 16]
    for slot in range(len(b1), B1_CELLS):                  # the free cells: blank
        r, c = divmod(slot, 16)
        assert (page[r * 16:r * 16 + 16, c * 16:c * 16 + 16] == blank).all(), 'cell %d not blank' % slot
    cells = {ch: cell_of(ch, pal_raw) for ch in need}
    full = dict(cm)
    full.update(new)
    return full, new, cells, len(b1), b1[-1], fnv1a(pal_raw), page


def encode(zh, cmap):
    out = bytearray()
    for ch in zh:
        if ord(ch) < 0x80:
            out += ch.encode('ascii')
        else:
            assert ch in cmap, 'not in the font: %r in %s' % (ch, zh)
            out += cmap[ch][0]
    return bytes(out)


def c_str(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def c_bytes(b):
    return '{ ' + ', '.join('0x%02X' % x for x in b) + ', 0 }'


# ------------------------------------------------------------------ pictures
def art_plate():
    """The pause menu's tab word on its 128 x 64 plate, as x2.1's 武器 / 忍法 / ... (menu_labels._tab_mask)"""
    return menu_labels._tab_mask('设置', 128, 64, bold=1)


def art_category(text):
    """An Options-menu category word as x2.1's (option_tabs.render_label's letters): 32 rows, ink rows 0-19,
    cut to its ink's columns"""
    SS, INK_H, TOP = option_tabs.SS, option_tabs.INK_H, option_tabs.TOP
    width = 400
    font_px = INK_H * SS
    for _ in range(30):
        font = ImageFont.truetype(hf.ART_FONT, int(font_px))
        l, t, r, b = font.getbbox(text)
        if b - t <= INK_H * SS:
            break
        font_px *= 0.96
    big = Image.new('L', (width * SS, 32 * SS), 0)
    ImageDraw.Draw(big).text((SS - l, TOP * SS + (INK_H * SS - (b - t)) / 2 - t), text, font=font, fill=255)
    big = big.filter(ImageFilter.MaxFilter(option_tabs.BOLD))
    m = np.array(big.resize((width, 32), Image.LANCZOS), np.float32) / 255.0
    cols = np.nonzero(m.max(0) > 0.02)[0]
    return m[:, max(cols[0] - 1, 0):cols[-1] + 2]


ARTS = [('Settings', None), ('GRAPHICS SETTINGS', '图形设置'), ('DISPLAY SETTINGS', '显示设置'),
        ('STARTUP SETTINGS', '启动设置'), ('GAMEPLAY', '玩法'), ('CHEATS', '作弊'), ('CONTROLS', '按键设置'), ('EXIT', '返回')]


# ------------------------------------------------------------------ the font
def make_font(chars, out):
    from fontTools import subset
    from fontTools.ttLib import TTFont, newTable
    from fontTools.pens.cu2quPen import Cu2QuPen
    from fontTools.pens.ttGlyphPen import TTGlyphPen
    src = os.path.join(LOC, 'tools', 'fonts', 'candidates', 'sourcehan', 'SourceHanSansCN-Bold.otf')
    f = TTFont(src)
    opt = subset.Options()
    opt.name_IDs = ['*']
    opt.notdef_outline = True
    opt.desubroutinize = True
    opt.layout_features = []
    s = subset.Subsetter(opt)
    s.populate(unicodes=sorted(ord(c) for c in chars))
    s.subset(f)
    # CFF -> TrueType outlines
    gs = f.getGlyphSet()
    order = f.getGlyphOrder()
    glyf = newTable('glyf')
    glyf.glyphOrder = order
    glyf.glyphs = {}
    for name in order:
        pen = TTGlyphPen(gs)
        gs[name].draw(Cu2QuPen(pen, 1.0, reverse_direction=True))
        glyf.glyphs[name] = pen.glyph()
    f['glyf'] = glyf
    f['loca'] = newTable('loca')
    maxp = f['maxp']
    maxp.tableVersion = 0x00010000
    for k in ('maxZones', 'maxTwilightPoints', 'maxStorage', 'maxFunctionDefs', 'maxInstructionDefs',
              'maxStackElements', 'maxSizeOfInstructions', 'maxComponentElements'):
        setattr(maxp, k, 0)
    maxp.maxZones = 1
    for k in ('maxPoints', 'maxContours', 'maxCompositePoints', 'maxCompositeContours', 'maxComponentDepth'):
        setattr(maxp, k, 0)
    del f['CFF ']
    if 'VORG' in f:
        del f['VORG']
    f['head'].glyphDataFormat = 0
    f.sfntVersion = '\x00\x01\x00\x00'
    post = f['post']
    post.formatType = 3.0
    # renamed: the licence's Reserved Font Name "Source" is not used by a modified version
    for rec in f['name'].names:
        if rec.nameID in (1, 4, 16):
            rec.string = 'NGB Hanzi Bold' if rec.nameID == 4 else 'NGB Hanzi'
        elif rec.nameID == 6:
            rec.string = 'NGBHanzi-Bold'
        elif rec.nameID == 3:
            rec.string = 'NGBHanzi-Bold;subset'
    f.save(out)
    g = TTFont(out)
    cmap = g.getBestCmap()
    missing = [c for c in chars if ord(c) not in cmap]
    assert not missing, 'font lacks %s' % ''.join(missing)
    return os.path.getsize(out)


def main():
    rows = load_rows()
    cmap, new, cells, b1_count, b1_last, pal_fnv, page = engine_glyphs(rows)
    L = []
    L.append('/* Generated by tools/hurricane/make_zh.py from tools/hurricane/zh_text.tsv -- do not edit. */')
    L.append('#include "hurricane_zh.h"')
    L.append('')
    for i, (ctx, en, zh, use) in enumerate(rows):
        if use == 'E':
            L.append('static const char s_e%d[] = %s;   /* %s */' % (i, c_bytes(encode(zh, cmap)), zh))
    L.append('')
    L.append('const NgbZhText ngb_zh_texts[] = {')
    for i, (ctx, en, zh, use) in enumerate(rows):
        newc = use == 'E' and any(ch in new for ch in zh)
        L.append('    { %s, %s, %s, %s, %d },' % (c_str(ctx), c_str(en), c_str(zh),
                                                 's_e%d' % i if use == 'E' else 'NULL', newc))
    L.append('};')
    L.append('const int ngb_zh_text_count = %d;' % len(rows))
    L.append('')
    L.append('/* The glyph page B1 (texture 0xBC) as x2.1 has it, and what is added to it. */')
    L.append('const int ngb_zh_b1_count = %d;' % b1_count)
    L.append('const unsigned char ngb_zh_b1_last[2] = { 0x%02X, 0x%02X };' % tuple(b1_last))
    L.append('const uint32_t ngb_zh_pal_fnv = 0x%08X;' % pal_fnv)
    L.append('const int ngb_zh_new_count = %d;' % len(new))
    order = sorted(new, key=lambda ch: new[ch][1])
    L.append('const unsigned char ngb_zh_new_code[%d][2] = {%s};' % (
        max(len(new), 1), ', '.join('{ 0x%02X, 0x%02X }' % tuple(new[ch][0]) for ch in order) or '{ 0 }'))
    L.append('const unsigned char ngb_zh_new_cell[%d][256] = {' % max(len(new), 1))
    for ch in order:
        L.append('    { /* %s, cell %d */' % (ch, new[ch][1]))
        b = cells[ch].reshape(-1)
        for r in range(0, 256, 32):
            L.append('      ' + ', '.join('%d' % x for x in b[r:r + 32]) + ',')
        L.append('    },')
    L.append('};')
    blank = page[(B1_CELLS - 1) // 16 * 16, (B1_CELLS - 1) % 16 * 16]
    L.append('const unsigned char ngb_zh_blank = %d;   /* an unused cell\'s texel */' % blank)
    L.append('')
    L.append('/* The words drawn as pictures: coverage 0-255, w x h. */')
    arts = []
    for en, zh in ARTS:
        m = art_plate() if zh is None else art_category(zh)
        a = np.clip(np.round(m * 255), 0, 255).astype(np.uint8)
        h, w = a.shape
        L.append('static const unsigned char s_art%d[%d] = {' % (len(arts), w * h))
        flat = a.reshape(-1)
        for r in range(0, len(flat), 40):
            L.append('    ' + ','.join('%d' % x for x in flat[r:r + 40]) + ',')
        L.append('};')
        arts.append((en, zh or '设置', w, h))
    L.append('const NgbZhArt ngb_zh_arts[] = {')
    for i, (en, zh, w, h) in enumerate(arts):
        L.append('    { %s, %s, %d, %d, s_art%d },' % (c_str(en), c_str(zh), w, h, i))
    L.append('};')
    L.append('const int ngb_zh_art_count = %d;' % len(arts))
    out = os.path.join(FORK, 'src', 'hurricane_zh_data.c')
    open(out, 'w', encoding='utf-8', newline='\n').write('\n'.join(L) + '\n')
    chars = {ch for r in rows for ch in r[2]} | {chr(c) for c in range(0x20, 0x7F)} | set('，。、：；！？（）／')
    size = make_font(sorted(chars), os.path.join(FORK, 'assets', 'fonts', 'NGBHanzi-Bold.ttf'))
    print('%d texts (%d for the game\'s table), new glyphs %s at cells %s, font %d chars %d bytes, %d pictures' % (
        len(rows), sum(r[3] == 'E' for r in rows), ''.join(order), [new[c][1] for c in order], len(chars), size,
        len(arts)))


if __name__ == '__main__':
    main()
