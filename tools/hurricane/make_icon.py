"""Draws the Hurricane build's program icon from the game's own Dragon Sword picture (the weapon page's art,
spr.afs #18 katana, 256x256 with alpha, exported by the localization project's tools/xpr.py).

  python tools/hurricane/make_icon.py <katana png> <out.ico> [preview.png]

The sword on a dark rounded tile with a crimson glow and a thin gold rim; drawn at 256x256 (the picture's
own size) and scaled down to the icon's sizes (256, 64, 48, 32, 16).
"""
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

S = 256


def rounded(size, r):
    m = Image.new('L', (size, size), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, size - 1, size - 1), r, fill=255)
    return m


def radial(size, inner, outer, k=1.0):
    """A radial gradient from inner (centre) to outer (corners)."""
    g = Image.new('RGB', (size, size))
    px = g.load()
    c = (size - 1) / 2
    rmax = (2 * c * c) ** 0.5
    for y in range(size):
        for x in range(size):
            t = min(1.0, (((x - c) ** 2 + (y - c) ** 2) ** 0.5 / rmax) ** k)
            px[x, y] = tuple(int(inner[i] + (outer[i] - inner[i]) * t) for i in range(3))
    return g


def draw(sword, fill, rim_w):
    """The icon at 256x256: the sword's longer side fill pixels, a gold rim rim_w wide."""
    tile = rounded(S, 44)
    bg = radial(S, (92, 18, 24), (10, 8, 12), 0.8).convert('RGBA')
    icon = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    icon.paste(bg, (0, 0), tile)

    # the sword centred on its own bounding box
    art = sword.crop(sword.getbbox())
    scale = fill / max(art.size)
    art = art.resize((round(art.width * scale), round(art.height * scale)), Image.LANCZOS)
    layer = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    layer.paste(art, ((S - art.width) // 2, (S - art.height) // 2), art)

    # a pale glow behind the blade (as on the weapon page), kept inside the tile
    a = layer.getchannel('A').filter(ImageFilter.GaussianBlur(7)).point(lambda v: min(255, v * 2))
    glow = Image.new('RGBA', (S, S), (255, 236, 220, 0))
    glow.putalpha(ImageChops.multiply(a, tile).point(lambda v: int(v * 0.55)))
    icon = Image.alpha_composite(icon, glow)
    icon = Image.alpha_composite(icon, layer)

    # gold rim
    rim = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(rim).rounded_rectangle((2, 2, S - 3, S - 3), 42, outline=(214, 172, 84, 255), width=rim_w)
    icon = Image.alpha_composite(icon, rim)
    icon.putalpha(ImageChops.multiply(icon.getchannel('A'), tile))
    return icon


def main():
    if len(sys.argv) < 3 or not sys.argv[2].lower().endswith('.ico'):
        sys.exit(__doc__)
    sword = Image.open(sys.argv[1]).convert('RGBA')
    assert sword.size == (S, S), sword.size

    big = draw(sword, 216, 5)
    # the small sizes: the sword larger and the rim thicker, so both still read at 32 and 16 pixels
    small = draw(sword, 246, 14)
    sizes = [256, 64, 48, 32, 16]
    imgs = {w: (big if w >= 48 else small).resize((w, w), Image.LANCZOS) for w in sizes}
    imgs[256].save(sys.argv[2], sizes=[(w, w) for w in sizes], append_images=[imgs[w] for w in sizes[1:]])
    if len(sys.argv) > 3:
        sheet = Image.new('RGBA', (sum(sizes) + 10 * len(sizes), 256), (40, 40, 48, 255))
        x = 0
        for w in sizes:
            sheet.alpha_composite(imgs[w], (x, 256 - w))
            x += w + 10
        sheet.save(sys.argv[3])


if __name__ == '__main__':
    main()
