#!/usr/bin/env python3
"""Render Rebble App Store screenshots at exact platform resolutions.

Mirrors Plinko watch UI (no watch bezel — store requirement).
"""
from __future__ import annotations

import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "store" / "screenshots"
ASSETS = ROOT / "resources" / "images"

PLATFORMS = {
    "basalt": (144, 168, True),
    "diorite": (144, 168, False),
    "flint": (144, 168, False),
    "emery": (200, 228, True),
}

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
DARK_GRAY = (85, 85, 85)
LIGHT_GRAY = (170, 170, 170)
BG_COLOR = (0, 0, 85)       # DukeBlue-ish
PEG_COLOR = (170, 170, 170)
BALL_COLOR = (255, 170, 0)  # ChromeYellow
BIN_HI = (255, 255, 0)
MULTIS = [1, 3, 5, 10, 5, 3, 1]


def font(size: int) -> ImageFont.ImageFont:
    for path in (
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ):
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def font_bold(size: int) -> ImageFont.ImageFont:
    for path in (
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ):
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return font(size)


def text_center(draw, text, y, w, fnt, fill):
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    draw.text(((w - tw) // 2, y), text, font=fnt, fill=fill)


def text_left(draw, text, x, y, fnt, fill):
    draw.text((x, y), text, font=fnt, fill=fill)


def text_right(draw, text, x_right, y, fnt, fill):
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    draw.text((x_right - tw, y), text, font=fnt, fill=fill)


def load_sprite(name: str) -> Image.Image | None:
    path = ASSETS / name
    if not path.is_file():
        return None
    return Image.open(path).convert("RGBA")


def paste_sprite(base: Image.Image, sprite: Image.Image | None, cx: int, cy: int, tint=None):
    if sprite is None:
        return
    img = sprite
    if tint is not None:
        img = Image.new("RGBA", sprite.size, (0, 0, 0, 0))
        sp = sprite.load()
        ip = img.load()
        for y in range(sprite.size[1]):
            for x in range(sprite.size[0]):
                r, g, b, a = sp[x, y]
                if a > 32 and (r + g + b) / 3 > 100:
                    ip[x, y] = (*tint, 255)
    ox = cx - img.size[0] // 2
    oy = cy - img.size[1] // 2
    base.paste(img, (ox, oy), img)


def board_layout(w: int, h: int):
    left, right = 6, w - 7
    drop_y, top, bottom = 22, 34, h - 28
    bin_label_y = h - 22
    play_w = right - left
    rows = 9 if h >= 200 else 7
    pegs_per_even = 6
    row_gap = (bottom - top) // (rows + 1)
    col_gap = play_w // (pegs_per_even + 1)

    pegs = []
    for r in range(rows):
        odd = (r % 2) != 0
        cols = pegs_per_even - 1 if odd else pegs_per_even
        y = top + (r + 1) * row_gap
        x_off = col_gap // 2 if odd else 0
        for c in range(cols):
            pegs.append((left + x_off + (c + 1) * col_gap, y))

    bins = []
    bin_w = play_w // 7
    for i in range(7):
        x0 = left + i * bin_w
        x1 = right if i == 6 else left + (i + 1) * bin_w
        bins.append((x0, x1, MULTIS[i]))

    return {
        "left": left,
        "right": right,
        "drop_y": drop_y,
        "top": top,
        "bottom": bottom,
        "bin_label_y": bin_label_y,
        "pegs": pegs,
        "bins": bins,
    }


def draw_board(img: Image.Image, draw, board, color: bool, highlight_bin: int | None = None):
    wall = DARK_GRAY if color else WHITE
    peg_col = PEG_COLOR if color else WHITE
    dim = LIGHT_GRAY if color else WHITE
    hi = BIN_HI if color else WHITE

    draw.line((board["left"] - 1, board["drop_y"], board["left"] - 1, board["bottom"]), fill=wall)
    draw.line((board["right"] + 1, board["drop_y"], board["right"] + 1, board["bottom"]), fill=wall)

    peg_spr = load_sprite("peg.png")
    for x, y in board["pegs"]:
        if peg_spr is not None:
            paste_sprite(img, peg_spr, x, y, tint=peg_col if color else WHITE)
        else:
            draw.ellipse((x - 2, y - 2, x + 2, y + 2), fill=peg_col)

    draw.line((board["left"], board["bottom"], board["right"], board["bottom"]), fill=wall)
    f14 = font(max(10, img.size[0] // 14))
    for i, (x0, x1, mult) in enumerate(board["bins"]):
        draw.line((x0, board["bottom"], x0, board["bottom"] + 18), fill=wall)
        mid = (x0 + x1) // 2
        fill = hi if highlight_bin == i else dim
        label = str(mult)
        bbox = draw.textbbox((0, 0), label, font=f14)
        tw = bbox[2] - bbox[0]
        draw.text((mid - tw // 2, board["bin_label_y"]), label, font=f14, fill=fill)
    draw.line((board["right"], board["bottom"], board["right"], board["bottom"] + 18), fill=wall)


def draw_ball(img: Image.Image, draw, x: int, y: int, color: bool):
    ball_spr = load_sprite("ball.png")
    tint = BALL_COLOR if color else WHITE
    if ball_spr is not None:
        paste_sprite(img, ball_spr, x, y, tint=tint)
    else:
        draw.ellipse((x - 4, y - 4, x + 4, y + 4), fill=tint)


def draw_hud(draw, w, score: int, high: int, balls: int | None, color: bool):
    f14 = font_bold(max(11, w // 13))
    text_left(draw, f"S:{score}  H:{high}", 2, 0, f14, WHITE)
    if balls is not None:
        text_right(draw, f"B:{balls}", w - 2, 0, f14, WHITE)


def bg(color: bool):
    return BG_COLOR if color else BLACK


def render_title(w, h, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), bg(color))
    draw = ImageDraw.Draw(img)
    title = load_sprite("title.png")
    if title is not None:
        # Tint white title onto background
        tinted = Image.new("RGBA", title.size, (0, 0, 0, 0))
        sp, tp = title.load(), tinted.load()
        for y in range(title.size[1]):
            for x in range(title.size[0]):
                r, g, b, a = sp[x, y]
                if a > 32 and (r + g + b) / 3 > 100:
                    tp[x, y] = (255, 255, 255, 255)
        ox = (w - tinted.size[0]) // 2
        img.paste(tinted, (ox, 36), tinted)
    else:
        text_center(draw, "PLINKO", 50, w, font_bold(max(22, w // 6)), WHITE)

    dim = LIGHT_GRAY if color else WHITE
    text_center(draw, "SELECT to play", h - 40, w, font(max(14, w // 10)), dim)
    text_center(draw, "Best 128", h - 22, w, font(max(11, w // 13)), dim)
    return img


def render_aim(w, h, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), bg(color))
    draw = ImageDraw.Draw(img)
    board = board_layout(w, h)
    draw_hud(draw, w, 24, 128, 7, color)
    draw_board(img, draw, board, color)
    aim_x = w // 2 - 10
    # dashed aim guide
    dim = LIGHT_GRAY if color else WHITE
    y = board["drop_y"] + 6
    while y < board["top"]:
        draw.line((aim_x, y, aim_x, y + 2), fill=dim)
        y += 4
    draw_ball(img, draw, aim_x, board["drop_y"], color)
    text_center(draw, "AIM  SELECT: drop", 14, w, font_bold(max(12, w // 12)), WHITE)
    return img


def render_falling(w, h, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), bg(color))
    draw = ImageDraw.Draw(img)
    board = board_layout(w, h)
    draw_hud(draw, w, 37, 128, 6, color)
    draw_board(img, draw, board, color)
    # ball mid-board bouncing
    pegs = board["pegs"]
    mid = pegs[len(pegs) // 2 + 2]
    draw_ball(img, draw, mid[0] + 6, mid[1] + 8, color)
    return img


def render_scored(w, h, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), bg(color))
    draw = ImageDraw.Draw(img)
    board = board_layout(w, h)
    draw_hud(draw, w, 47, 128, 5, color)
    draw_board(img, draw, board, color, highlight_bin=3)
    # ball in center 10x bin
    x0, x1, _ = board["bins"][3]
    bx = (x0 + x1) // 2
    draw_ball(img, draw, bx, board["bottom"] - 6, color)
    text_center(draw, "+10", 14, w, font_bold(max(14, w // 10)), BIN_HI if color else WHITE)
    return img


def render_round_end(w, h, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), bg(color))
    draw = ImageDraw.Draw(img)
    board = board_layout(w, h)
    draw_hud(draw, w, 86, 128, 0, color)
    draw_board(img, draw, board, color)
    text_center(draw, "ROUND OVER", 14, w, font_bold(max(14, w // 10)), WHITE)
    text_center(draw, "Score 86", h // 2 - 20, w, font_bold(max(18, w // 8)), WHITE)
    dim = LIGHT_GRAY if color else WHITE
    text_center(draw, "SELECT: again  UP: menu", h - 22, w, font(max(10, w // 14)), dim)
    return img


SCENES = [
    ("01-title.png", render_title),
    ("02-aim.png", render_aim),
    ("03-falling.png", render_falling),
    ("04-scored.png", render_scored),
    ("05-round-end.png", render_round_end),
]


def main() -> None:
    for plat, (w, h, color) in PLATFORMS.items():
        dest = OUT / plat
        dest.mkdir(parents=True, exist_ok=True)
        for filename, fn in SCENES:
            img = fn(w, h, color)
            path = dest / filename
            img.save(path, "PNG", optimize=True)
            print(f"wrote {path.relative_to(ROOT)} ({w}x{h})")
    print("Done:", OUT)


if __name__ == "__main__":
    main()
