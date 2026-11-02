"""Convert Pixellab 1-bit sprites to white-on-transparent for Pebble."""
from __future__ import annotations

import os
import urllib.request

from PIL import Image

OUT = os.path.join(os.path.dirname(__file__), "..", "resources", "images")
OUT = os.path.abspath(OUT)


def to_pebble_white(im: Image.Image) -> Image.Image:
    im = im.convert("RGBA")
    w, h = im.size
    out_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = im.load()
    opx = out_im.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 32:
                continue
            lum = (r + g + b) / 3
            if lum >= 100:
                opx[x, y] = (255, 255, 255, 255)
    return out_im


def main() -> None:
    os.makedirs(OUT, exist_ok=True)

    peg_job = "38657b82-0059-426f-9f4c-ea07e917a192"
    base = f"https://api.pixellab.ai/mcp/images/{peg_job}/download?index="
    for i in [0, 5, 10, 15, 20, 25, 30, 40, 50]:
        path = os.path.join(OUT, f"peg_cand_{i}.png")
        urllib.request.urlretrieve(base + str(i), path)

    # Process menu / ball / title from existing downloads
    mapping = {
        "menu_icon.png": "menu_icon.png",
        "ball.png": "ball.png",
        "title_3.png": "title.png",
    }
    # title_3 may already have been renamed; fall back
    if not os.path.exists(os.path.join(OUT, "title_3.png")):
        if os.path.exists(os.path.join(OUT, "title.png")):
            mapping = {
                "menu_icon.png": "menu_icon.png",
                "ball.png": "ball.png",
                "title.png": "title.png",
            }

    for src_name, dst_name in mapping.items():
        src = os.path.join(OUT, src_name)
        if not os.path.exists(src):
            print("missing", src)
            continue
        processed = to_pebble_white(Image.open(src))
        processed.save(os.path.join(OUT, dst_name))
        opaque = sum(1 for c in processed.getdata() if c[3] > 0)
        print(dst_name, processed.size, "opaque", opaque)

    # Pick best compact bright peg candidate near center
    best = None
    best_score = -1.0
    for i in [0, 5, 10, 15, 20, 25, 30, 40, 50]:
        p = os.path.join(OUT, f"peg_cand_{i}.png")
        im = Image.open(p).convert("RGBA")
        w, h = im.size
        score = 0.0
        bright = 0
        for y in range(h):
            for x in range(w):
                r, g, b, a = im.getpixel((x, y))
                if a > 32 and (r + g + b) / 3 > 160:
                    bright += 1
                    dist = ((x - w / 2) ** 2 + (y - h / 2) ** 2) ** 0.5
                    score += max(0.0, 8.0 - dist)
        # Prefer compact (~8-40 bright px)
        if bright < 4 or bright > 80:
            score *= 0.2
        print("peg cand", i, "score", round(score, 1), "bright", bright)
        if score > best_score:
            best_score = score
            best = p

    peg_path = os.path.join(OUT, "peg.png")
    if best and best_score > 5:
        processed = to_pebble_white(Image.open(best))
        bbox = processed.getbbox()
        if bbox:
            cropped = processed.crop(bbox)
            canvas = Image.new("RGBA", (12, 12), (0, 0, 0, 0))
            cw, ch = cropped.size
            # Scale down if huge
            if cw > 10 or ch > 10:
                cropped = cropped.resize((min(cw, 8), min(ch, 8)), Image.NEAREST)
                cw, ch = cropped.size
            ox = (12 - cw) // 2
            oy = (12 - ch) // 2
            canvas.paste(cropped, (ox, oy), cropped)
            canvas.save(peg_path)
            print("peg from", os.path.basename(best), "opaque",
                  sum(1 for c in canvas.getdata() if c[3] > 0))
        else:
            best = None

    if not best or best_score <= 5:
        peg = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
        pp = peg.load()
        cx, cy, rad = 3.5, 3.5, 2.6
        for y in range(8):
            for x in range(8):
                if (x - cx) ** 2 + (y - cy) ** 2 <= rad * rad:
                    pp[x, y] = (255, 255, 255, 255)
        peg.save(peg_path)
        print("peg fallback circle")

    # Cleanup candidates / alternates
    keep = {"menu_icon.png", "ball.png", "peg.png", "title.png"}
    for name in os.listdir(OUT):
        if name not in keep:
            os.remove(os.path.join(OUT, name))
            print("removed", name)

    print("final:", sorted(os.listdir(OUT)))
    for f in sorted(os.listdir(OUT)):
        im = Image.open(os.path.join(OUT, f)).convert("RGBA")
        opaque = [c for c in im.getdata() if c[3] > 0]
        print(f, im.size, opaque[:2], "n=", len(opaque))


if __name__ == "__main__":
    main()
