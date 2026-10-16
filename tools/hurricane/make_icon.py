"""Draws the Hurricane build's program icon: a golden dragon coiled around a sword (original art, no game data).

  python tools/hurricane/make_icon.py <out.ico> [preview.png]

Drawn at 1024x1024 and scaled down to the icon's sizes (256, 64, 48, 32, 16).
"""
import math
import sys

from PIL import Image, ImageDraw, ImageFilter

S = 1024
C = S / 2


def gold(size):
    """A vertical metal gradient: light gold at the top, deep gold at the bottom."""
    g = Image.new('RGBA', (size, size))
    px = g.load()
    stops = [(0.0, (255, 240, 170)), (0.35, (240, 196, 72)), (0.7, (196, 140, 30)), (1.0, (130, 82, 12))]
    for y in range(size):
        t = y / (size - 1)
        for (a, ca), (b, cb) in zip(stops, stops[1:]):
            if a <= t <= b:
                k = (t - a) / (b - a)
                col = tuple(int(ca[i] + (cb[i] - ca[i]) * k) for i in range(3))
                break
        for x in range(size):
            px[x, y] = col + (255,)
    return g


def body_path():
    """Centre line of the dragon: from the tail (bottom) winding up around the sword to the head (top right)."""
    pts = []
    n = 400
    for i in range(n + 1):
        t = i / n
        y = 900 - 600 * t                       # climbs the blade
        x = C + 230 * math.sin(t * 2.6 * math.pi + 0.3) * (0.55 + 0.45 * t)
        pts.append((x, y))
    return pts


def width_at(t):
    return 18 + 62 * math.sin(min(1.0, t * 1.15) * math.pi * 0.5) ** 1.2


def outline(pts, wfun):
    left, right, normals = [], [], []
    for i, (x, y) in enumerate(pts):
        a = pts[max(0, i - 1)]
        b = pts[min(len(pts) - 1, i + 1)]
        dx, dy = b[0] - a[0], b[1] - a[1]
        L = math.hypot(dx, dy) or 1
        nx, ny = -dy / L, dx / L
        w = wfun(i / (len(pts) - 1))
        left.append((x + nx * w, y + ny * w))
        right.append((x - nx * w, y - ny * w))
        normals.append((nx, ny, w))
    return left, right, normals


def draw():
    mask = Image.new('L', (S, S), 0)        # the dragon in front of the sword (gold)
    back = Image.new('L', (S, S), 0)        # the coils behind it
    d = ImageDraw.Draw(mask)
    db = ImageDraw.Draw(back)
    pts = body_path()
    left, right, normals = outline(pts, width_at)
    for i in range(len(pts) - 1):
        quad = [left[i], left[i + 1], right[i + 1], right[i]]
        # a helix: moving right = in front, moving left = behind
        (db if pts[i + 1][0] < pts[i][0] - 0.2 and abs(pts[i][0] - C) < 120 else d).polygon(quad, fill=255)
        (db if pts[i + 1][0] < pts[i][0] - 0.2 and abs(pts[i][0] - C) < 120 else d).ellipse(
            [pts[i][0] - 2, pts[i][1] - 2, pts[i][0] + 2, pts[i][1] + 2], fill=255)

    # dorsal spikes on the left edge
    for i in range(30, len(pts) - 30, 18):
        x, y = pts[i]
        nx, ny, w = normals[i]
        tx, ty = pts[i + 6][0] - x, pts[i + 6][1] - y
        L = math.hypot(tx, ty) or 1
        tx, ty = tx / L, ty / L
        base = 0.6 * w + 6
        p1 = (x + nx * w * 0.8 - tx * base, y + ny * w * 0.8 - ty * base)
        p2 = (x + nx * w * 0.8 + tx * base, y + ny * w * 0.8 + ty * base)
        tip = (x + nx * (w + 26 + 0.45 * w) + tx * base * 1.4, y + ny * (w + 26 + 0.45 * w) + ty * base * 1.4)
        d.polygon([p1, p2, tip], fill=255)

    # tail fin
    tx, ty = pts[0]
    d.polygon([(tx - 10, ty + 10), (tx - 80, ty + 70), (tx + 10, ty + 40), (tx + 60, ty + 95), (tx + 20, ty - 10)], fill=255)

    # head at the top end, facing right
    hx, hy = pts[-1]
    head = [(hx - 70, hy + 40), (hx - 60, hy - 50), (hx + 20, hy - 72), (hx + 120, hy - 50),
            (hx + 175, hy - 20), (hx + 180, hy + 5), (hx + 110, hy + 10), (hx + 165, hy + 45),
            (hx + 150, hy + 70), (hx + 40, hy + 70)]
    d.polygon(head, fill=255)
    # horns sweeping back
    d.polygon([(hx - 30, hy - 55), (hx + 20, hy - 70), (hx - 140, hy - 190), (hx - 70, hy - 90)], fill=255)
    d.polygon([(hx + 10, hy - 65), (hx + 60, hy - 60), (hx - 40, hy - 175)], fill=255)
    # mane
    d.polygon([(hx - 70, hy + 40), (hx - 60, hy - 30), (hx - 160, hy - 40), (hx - 110, hy + 10), (hx - 175, hy + 40), (hx - 100, hy + 60)], fill=255)
    # sword: blade behind the dragon, gold guard and dark hilt
    sword = Image.new('L', (S, S), 0)
    sd = ImageDraw.Draw(sword)
    sd.polygon([(C - 34, 200), (C + 34, 200), (C + 34, 900), (C, 985), (C - 34, 900)], fill=255)
    hilt = Image.new('L', (S, S), 0)
    hd = ImageDraw.Draw(hilt)
    hd.rectangle([C - 28, 40, C + 28, 175], fill=255)
    hd.ellipse([C - 40, 24, C + 40, 70], fill=255)
    guard = Image.new('L', (S, S), 0)
    gd = ImageDraw.Draw(guard)
    gd.ellipse([C - 100, 160, C + 100, 215], fill=255)

    img = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    bg = Image.new('L', (S, S), 0)
    ImageDraw.Draw(bg).rounded_rectangle([16, 16, S - 16, S - 16], radius=150, fill=255)
    img.paste((20, 18, 16, 255), (0, 0), bg)

    def layer(m, fill, edge=None, grow=0):
        if edge is not None:
            e = m.filter(ImageFilter.MaxFilter(grow * 2 + 1))
            img.paste(edge, (0, 0), e)
        img.paste(fill, (0, 0), m)

    steel = Image.new('RGBA', (S, S))
    sp = steel.load()
    for x in range(S):
        k = 1 - abs(x - C) / 40
        v = int(150 + 90 * max(0, k))
        for y in range(S):
            sp[x, y] = (v, v + 6, v + 14, 255)
    layer(back, gold(S).point(lambda v: int(v * 0.72)), (40, 24, 4, 255), 9)
    layer(sword, steel, (10, 10, 12, 255), 7)
    layer(hilt, (60, 20, 18, 255), (10, 10, 12, 255), 7)
    layer(guard, gold(S), (40, 24, 4, 255), 7)

    g = gold(S)
    layer(mask, g, (40, 24, 4, 255), 9)
    # scales: dark arcs along the body
    scl = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    sc = ImageDraw.Draw(scl)
    for i in range(20, len(pts) - 20, 12):
        x, y = pts[i]
        nx, ny, w = normals[i]
        r = w * 0.55
        sc.arc([x - r, y - r, x + r, y + r], 200, 340, fill=(150, 96, 14, 255), width=6)
    vis = Image.new('L', (S, S), 0)        # scales only where the body is seen: front coils, or back coils off the sword
    vis.paste(255, (0, 0), mask)
    vis.paste(255, (0, 0), Image.composite(back, Image.new('L', (S, S), 0), sword.point(lambda v: 255 - v)))
    img.paste(scl, (0, 0), Image.composite(scl.getchannel('A'), Image.new('L', (S, S), 0), vis))
    sc = ImageDraw.Draw(img)
    # eye
    sc.ellipse([hx + 50, hy - 35, hx + 78, hy - 12], fill=(200, 20, 16, 255), outline=(40, 10, 4, 255), width=4)
    return img


def main():
    img = draw()
    out = sys.argv[1]
    sizes = [(256, 256), (64, 64), (48, 48), (32, 32), (16, 16)]
    img.resize((256, 256), Image.LANCZOS).save(out, sizes=sizes)
    if len(sys.argv) > 2:
        img.resize((256, 256), Image.LANCZOS).save(sys.argv[2])
        sm = Image.new('RGBA', (256 + 64 + 48 + 32 + 16 + 50, 64), (40, 40, 40, 255))
        x = 0
        for s in (256, 64, 48, 32, 16):
            if s == 256:
                continue
            sm.paste(img.resize((s, s), Image.LANCZOS), (x, 0))
            x += s + 10
        sm.save(sys.argv[2].replace('.png', '_small.png'))


if __name__ == '__main__':
    main()
