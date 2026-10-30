#!/usr/bin/env python3
"""Regenerate the launcher icons from one square piece of art.

    python tools/android/make-icon.py path/to/art.png

Writes res/mipmap-*/ic_launcher.png (legacy plate) and
res/mipmap-*/ic_launcher_foreground.png (adaptive), then the APK needs a
rebuild.  The adaptive foreground is drawn at SAFE of the 108dp canvas: the
art is edge-to-edge, so 0.80 fills a round mask while still keeping the face
and the blade inside it.
"""
import argparse
import os

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, "..", "..", "android", "app", "src", "main", "res")
DENSITIES = {"mdpi": 1.0, "hdpi": 1.5, "xhdpi": 2.0, "xxhdpi": 3.0, "xxxhdpi": 4.0}
LEGACY_DP = 48          # 48dp legacy icon
FOREGROUND_DP = 108     # adaptive icon canvas
PLATE = 0.90            # art size on the legacy plate
SAFE = 0.80             # art size on the adaptive canvas
WHITE = (255, 255, 255, 255)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("art")
    args = ap.parse_args()

    src = Image.open(args.art).convert("RGBA")
    if src.width != src.height:
        side = min(src.size)
        left = (src.width - side) // 2
        top = (src.height - side) // 2
        src = src.crop((left, top, left + side, top + side))

    for density, scale in DENSITIES.items():
        out = os.path.join(RES, f"mipmap-{density}")
        os.makedirs(out, exist_ok=True)

        n = int(round(LEGACY_DP * scale))
        plate = Image.new("RGBA", (n, n), WHITE)
        inner = max(1, int(round(n * PLATE)))
        plate.paste(src.resize((inner, inner), Image.LANCZOS),
                    ((n - inner) // 2, (n - inner) // 2))
        plate.convert("RGB").save(os.path.join(out, "ic_launcher.png"), optimize=True)

        n = int(round(FOREGROUND_DP * scale))
        fg = Image.new("RGBA", (n, n), (0, 0, 0, 0))
        inner = max(1, int(round(n * SAFE)))
        fg.paste(src.resize((inner, inner), Image.LANCZOS),
                 ((n - inner) // 2, (n - inner) // 2))
        fg.save(os.path.join(out, "ic_launcher_foreground.png"), optimize=True)

        print(f"{density}: legacy {int(round(LEGACY_DP * scale))}px, "
              f"foreground {n}px (art {inner}px)")


if __name__ == "__main__":
    main()
