from pathlib import Path
import random

from PIL import Image, ImageDraw, ImageEnhance, ImageOps


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "ui图" / "生成素材" / "航海图鉴UI" / "encyclopedia_book_frame.png"
OUT = ROOT / "ui图" / "生成素材" / "航海图鉴UI" / "pixel_ui"
OUT.mkdir(parents=True, exist_ok=True)


def crop(src, box):
    return src.crop(box).convert("RGBA")


def fit(src, size, contrast=1.0, brightness=1.0):
    img = ImageOps.fit(src, size, method=Image.Resampling.NEAREST, centering=(0.5, 0.5))
    if contrast != 1.0:
        img = ImageEnhance.Contrast(img).enhance(contrast)
    if brightness != 1.0:
        img = ImageEnhance.Brightness(img).enhance(brightness)
    return img


def tint(img, color, alpha):
    overlay = Image.new("RGBA", img.size, color)
    return Image.blend(img, overlay, alpha)


def rgba(hex_color, alpha=255):
    hex_color = hex_color.lstrip("#")
    return tuple(int(hex_color[i:i + 2], 16) for i in (0, 2, 4)) + (alpha,)


def paste_masked(dst, src, mask, pos=(0, 0)):
    dst.alpha_composite(src, pos)
    if mask is not None:
        clipped = Image.new("RGBA", dst.size, (0, 0, 0, 0))
        clipped.alpha_composite(src, pos)
        dst.paste(clipped, (0, 0), mask)


def polygon_mask(size, inset=0, notch=8):
    w, h = size
    pts = [
        (inset + notch, inset),
        (w - 1 - inset - notch, inset),
        (w - 1 - inset, inset + notch),
        (w - 1 - inset, h - 1 - inset - notch),
        (w - 1 - inset - notch, h - 1 - inset),
        (inset + notch, h - 1 - inset),
        (inset, h - 1 - inset - notch),
        (inset, inset + notch),
    ]
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    return mask, pts


def draw_shadow(img, mask, offset=(4, 5), alpha=82):
    shadow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    shifted = Image.new("L", img.size, 0)
    shifted.paste(mask, offset)
    shadow.paste((31, 16, 4, alpha), (0, 0), shifted)
    img.alpha_composite(shadow)


def add_texture_noise(img, box, amount=0.10, seed=1):
    rng = random.Random(seed)
    px = img.load()
    x1, y1, x2, y2 = box
    for y in range(y1, y2 + 1):
        for x in range(x1, x2 + 1):
            if rng.random() > amount:
                continue
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            d = rng.randint(-8, 8)
            px[x, y] = (max(0, min(255, r + d)),
                        max(0, min(255, g + d)),
                        max(0, min(255, b + d)),
                        a)


def draw_material_panel(name, size, fill_tex, wood_tex, brass_tex,
                        blue_tex=None, style="parchment", seed=1):
    w, h = size
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    mask, pts = polygon_mask(size, inset=0, notch=8)
    draw_shadow(img, mask)

    outer = Image.new("RGBA", size, (0, 0, 0, 0))
    outer.paste(fit(wood_tex, size, contrast=1.12), (0, 0), mask)
    img.alpha_composite(outer)

    brass_mask, brass_pts = polygon_mask(size, inset=3, notch=7)
    brass_layer = Image.new("RGBA", size, (0, 0, 0, 0))
    brass_layer.paste(fit(brass_tex, size, contrast=1.08, brightness=1.03), (0, 0), brass_mask)
    img.alpha_composite(brass_layer)

    inner_mask, inner_pts = polygon_mask(size, inset=9, notch=5)
    inner = Image.new("RGBA", size, (0, 0, 0, 0))
    tex = blue_tex if style == "blue" and blue_tex is not None else fill_tex
    inner_fill = fit(tex, size, contrast=1.04 if style == "parchment" else 1.18,
                     brightness=1.02 if style == "parchment" else 0.92)
    inner.paste(inner_fill, (0, 0), inner_mask)
    img.alpha_composite(inner)

    d = ImageDraw.Draw(img)
    d.line((18, 10, w - 20, 10), fill=rgba("#fff1bd", 90))
    d.line((18, h - 11, w - 20, h - 11), fill=rgba("#2c1605", 90))
    d.line((10, 18, 10, h - 20), fill=rgba("#fff1bd", 52))
    d.line((w - 11, 18, w - 11, h - 20), fill=rgba("#251305", 90))
    for x, y in [(6, 6), (w - 15, 6), (6, h - 15), (w - 15, h - 15)]:
        draw_real_rivet(img, brass_tex, x, y, 9)
    add_texture_noise(img, (10, 10, w - 11, h - 11), 0.04, seed)
    img.save(OUT / name)


def draw_real_rivet(img, brass_tex, x, y, size):
    rivet = fit(brass_tex, (size, size), contrast=1.2, brightness=1.18)
    d = ImageDraw.Draw(rivet)
    d.rectangle((0, 0, size - 1, size - 1), outline=rgba("#3a1f07", 180))
    d.point((2, 2), fill=rgba("#fff1a9", 210))
    d.point((3, 2), fill=rgba("#fff1a9", 180))
    d.point((size - 3, size - 3), fill=rgba("#3a1f07", 145))
    img.alpha_composite(rivet, (x, y))


def draw_tab(name, tab_tex, blue_tex, brass_tex, selected=False, hover=False):
    size = (150, 50)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    mask, pts = polygon_mask(size, inset=1, notch=9)
    draw_shadow(img, mask, offset=(3, 4), alpha=90)

    base = fit(tab_tex, size, contrast=1.08, brightness=1.0)
    if selected:
        center = fit(blue_tex, size, contrast=1.14, brightness=0.96)
        base = Image.blend(base, center, 0.62)
    elif hover:
        base = ImageEnhance.Brightness(base).enhance(1.08)

    clipped = Image.new("RGBA", size, (0, 0, 0, 0))
    clipped.paste(base, (0, 0), mask)
    img.alpha_composite(clipped)
    d = ImageDraw.Draw(img)
    d.line((20, 6, 130, 6), fill=rgba("#fff0b0", 130 if selected else 86))
    d.line((18, 43, 132, 43), fill=rgba("#2d1606", 120))
    d.line((28, 11, 120, 36), fill=rgba("#ffffff", 30))
    for x in (9, 132):
        draw_real_rivet(img, brass_tex, x, 9, 8)
    img.save(OUT / name)


def draw_row(name, page_tex, blue_tex, wood_tex, brass_tex, selected=False, hover=False):
    size = (358, 62)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    mask, pts = polygon_mask(size, inset=0, notch=7)
    draw_shadow(img, mask, offset=(4, 5), alpha=72)

    edge = fit(brass_tex if selected else wood_tex, size, contrast=1.08)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    layer.paste(edge, (0, 0), mask)
    img.alpha_composite(layer)

    inner_mask, _ = polygon_mask(size, inset=7, notch=4)
    inner = fit(page_tex, size, contrast=1.05, brightness=1.04 if selected else (1.08 if hover else 0.98))
    if hover:
        inner = tint(inner, rgba("#d5a052", 255), 0.10)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    layer.paste(inner, (0, 0), inner_mask)
    img.alpha_composite(layer)

    d = ImageDraw.Draw(img)
    if selected:
        strip_mask, _ = polygon_mask((24, 48), inset=0, notch=4)
        strip = fit(blue_tex, (24, 48), contrast=1.18, brightness=0.9)
        img.paste(strip, (10, 7), strip_mask)
        d.line((37, 11, 37, 51), fill=rgba("#e6ac3e", 190))
    elif hover:
        d.line((16, 10, 16, 52), fill=rgba("#aa6a24", 160), width=4)
    d.line((42, 50, 332, 50), fill=rgba("#80572b", 82))
    d.line((42, 12, 332, 12), fill=rgba("#fff0bf", 46))
    for x in (7, size[0] - 15):
        draw_real_rivet(img, brass_tex, x, 7, 7)
        draw_real_rivet(img, brass_tex, x, size[1] - 15, 7)
    add_texture_noise(img, (12, 10, size[0] - 12, size[1] - 11), 0.03, 80 + selected * 8 + hover * 4)
    img.save(OUT / name)


def draw_tag(page_tex, brass_tex):
    size = (86, 28)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    mask, _ = polygon_mask(size, inset=0, notch=6)
    draw_shadow(img, mask, offset=(2, 3), alpha=62)
    edge = fit(brass_tex, size, contrast=1.12, brightness=1.07)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    layer.paste(edge, (0, 0), mask)
    img.alpha_composite(layer)
    inner_mask, _ = polygon_mask(size, inset=4, notch=3)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    layer.paste(fit(page_tex, size, contrast=1.06, brightness=1.0), (0, 0), inner_mask)
    img.alpha_composite(layer)
    d = ImageDraw.Draw(img)
    d.line((12, 5, 74, 5), fill=rgba("#fff0b6", 95))
    img.save(OUT / "tag_badge.png")


def draw_scroll(rope_tex, brass_tex):
    track = Image.new("RGBA", (18, 112), (0, 0, 0, 0))
    rope = fit(rope_tex, (18, 112), contrast=1.1)
    mask, _ = polygon_mask((18, 112), inset=3, notch=3)
    track.paste(rope, (0, 0), mask)
    d = ImageDraw.Draw(track)
    d.line((9, 8, 9, 104), fill=rgba("#2b1607", 120), width=2)
    track.save(OUT / "scroll_track.png")

    thumb = Image.new("RGBA", (22, 72), (0, 0, 0, 0))
    mask, _ = polygon_mask((22, 72), inset=0, notch=5)
    thumb.paste(fit(brass_tex, (22, 72), contrast=1.18, brightness=1.06), (0, 0), mask)
    d = ImageDraw.Draw(thumb)
    d.line((5, 9, 15, 9), fill=rgba("#fff0a8", 150))
    for y in range(18, 58, 12):
        d.line((5, y, 15, y + 5), fill=rgba("#4f2b0c", 120))
    thumb.save(OUT / "scroll_thumb.png")


def draw_page(name, page_tex, side):
    size = (560, 590)
    w, h = size
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    if side == "left":
        pts = [(w - 16, 0), (22, 16), (14, h - 26), (w - 14, h - 2)]
    elif side == "right":
        pts = [(16, 0), (w - 22, 16), (w - 14, h - 26), (14, h - 2)]
    else:
        pts = [(18, 8), (w - 34, 18), (w - 18, h - 26), (20, h - 6)]

    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    draw_shadow(img, mask, offset=(5, 7), alpha=78)
    page = fit(page_tex, size, contrast=1.04, brightness=1.02)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    layer.paste(page, (0, 0), mask)
    img.alpha_composite(layer)
    d = ImageDraw.Draw(img)
    d.line((42, 34, w - 48, 28), fill=rgba("#fff0bc", 70))
    d.line((48, h - 34, w - 44, h - 42), fill=rgba("#5b3515", 70))
    for y in range(58, h - 70, 42):
        d.line((56, y, w - 58, y - 5 if side == "left" else y + 5), fill=rgba("#7e572b", 35))
    for x in range(78, w - 85, 70):
        d.line((x, 62, x + (12 if side == "right" else -12), h - 78), fill=rgba("#7e572b", 25))
    img.save(OUT / name)


def draw_cover(wood_tex, blue_tex, brass_tex, rope_tex):
    size = (388, 548)
    w, h = size
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    mask, _ = polygon_mask(size, inset=0, notch=10)
    draw_shadow(img, mask, offset=(9, 11), alpha=100)
    base = fit(wood_tex, size, contrast=1.18, brightness=0.88)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    layer.paste(base, (0, 0), mask)
    img.alpha_composite(layer)

    d = ImageDraw.Draw(img)
    for x in range(24, w - 34, 28):
        d.line((x, 25, x + 6, h - 34), fill=rgba("#180b03", 86))
        d.line((x + 12, 30, x + 15, h - 42), fill=rgba("#7b4421", 52))

    center_mask, _ = polygon_mask((278, 392), inset=0, notch=8)
    center = fit(blue_tex, (278, 392), contrast=1.2, brightness=0.86)
    img.paste(center, (45, 58), center_mask)

    rope = fit(rope_tex, (288, 38), contrast=1.08, brightness=1.0)
    img.alpha_composite(rope, (41, 96))
    d.rectangle((48, 109, w - 82, 113), fill=rgba("#2a1606", 95))

    for corner, pos in [
        (fit(brass_tex, (72, 72), contrast=1.2, brightness=1.08), (0, 0)),
        (fit(brass_tex.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (72, 72), contrast=1.2, brightness=1.08), (w - 72, 0)),
        (fit(brass_tex.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (72, 72), contrast=1.2, brightness=1.08), (0, h - 72)),
        (fit(brass_tex.transpose(Image.Transpose.ROTATE_180), (72, 72), contrast=1.2, brightness=1.08), (w - 72, h - 72)),
    ]:
        img.alpha_composite(corner, pos)

    for rx, ry in [(35, 32), (w - 48, 32), (35, h - 48), (w - 48, h - 48)]:
        draw_real_rivet(img, brass_tex, rx, ry, 12)

    d = ImageDraw.Draw(img)
    cx, cy = w // 2, h // 2 + 26
    d.rectangle((cx - 54, cy - 54, cx + 54, cy + 54), outline=rgba("#e5aa3f", 160), width=2)
    d.polygon([(cx, cy - 58), (cx - 11, cy), (cx + 11, cy)], fill=rgba("#e5aa3f", 165))
    d.polygon([(cx, cy + 58), (cx - 11, cy), (cx + 11, cy)], fill=rgba("#b77722", 120))
    d.line((cx - 58, cy, cx + 58, cy), fill=rgba("#e5aa3f", 120), width=2)
    d.line((cx, cy - 58, cx, cy + 58), fill=rgba("#e5aa3f", 120), width=2)
    img.save(OUT / "open_closed_cover.png")


def main():
    book = Image.open(SRC).convert("RGBA")
    page_left = crop(book, (285, 235, 735, 755))
    page_right = crop(book, (930, 235, 1415, 755))
    wood = crop(book, (1550, 190, 1635, 700))
    blue = crop(book, (720, 52, 1010, 80))
    blue_side = blue
    brass = crop(book, (20, 25, 140, 155))
    rope = crop(book, (475, 822, 1295, 872))
    tab = crop(book, (315, 122, 485, 188))

    draw_material_panel("panel_parchment.png", (128, 96), page_left, wood, brass, seed=3)
    draw_material_panel("panel_stat.png", (128, 96), page_right, wood, brass, seed=5)
    draw_material_panel("panel_sea_chart.png", (128, 96), page_left, wood, brass, blue_side, "blue", seed=4)
    draw_material_panel("detail_image_panel.png", (160, 112), page_left, wood, brass, blue_side, "blue", seed=7)
    draw_material_panel("icon_frame.png", (84, 54), page_left, wood, brass, blue_side, "blue", seed=9)

    draw_tab("tab_normal.png", tab, blue, brass)
    draw_tab("tab_hover.png", tab, blue, brass, hover=True)
    draw_tab("tab_selected.png", tab, blue, brass, selected=True)

    draw_row("row_normal.png", page_left, blue_side, wood, brass)
    draw_row("row_hover.png", page_left, blue_side, wood, brass, hover=True)
    draw_row("row_selected.png", page_left, blue_side, wood, brass, selected=True)
    draw_tag(page_left, brass)
    draw_scroll(rope, brass)

    draw_page("open_page_left.png", page_left, "left")
    draw_page("open_page_right.png", page_right, "right")
    draw_page("open_flip_page.png", page_left, "flip")
    draw_cover(wood, blue, brass, rope)


if __name__ == "__main__":
    main()
