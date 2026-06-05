from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "ui图" / "界面概念图" / "背包.png"
OUT = ROOT / "ui图" / "生成素材" / "背包UI"
CANVAS_SIZE = (1280, 720)


def rgba(hex_color: str, alpha: int = 255) -> tuple[int, int, int, int]:
    hex_color = hex_color.lstrip("#")
    return tuple(int(hex_color[i:i + 2], 16) for i in (0, 2, 4)) + (alpha,)


def base_image() -> Image.Image:
    return Image.open(SRC).convert("RGBA").resize(CANVAS_SIZE, Image.Resampling.LANCZOS)


def crop(base: Image.Image, x: int, y: int, w: int, h: int) -> Image.Image:
    return base.crop((x, y, x + w, y + h))


def soften_region(img: Image.Image, box: tuple[int, int, int, int], radius: int = 9) -> None:
    blurred = img.filter(ImageFilter.GaussianBlur(radius))
    img.paste(blurred.crop(box), box)


def cover_region(img: Image.Image, box: tuple[int, int, int, int],
                 fill: tuple[int, int, int, int], radius: int = 4) -> None:
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    draw.rounded_rectangle(box, radius=radius, fill=fill)
    img.alpha_composite(layer)


def tile_region(img: Image.Image, box: tuple[int, int, int, int],
                source_box: tuple[int, int, int, int]) -> None:
    source = img.crop(source_box)
    if source.width <= 0 or source.height <= 0:
        return
    patch = Image.new("RGBA", (box[2] - box[0], box[3] - box[1]), (0, 0, 0, 0))
    for y in range(0, patch.height, source.height):
        for x in range(0, patch.width, source.width):
            patch.alpha_composite(source, (x, y))
    img.paste(patch.crop((0, 0, patch.width, patch.height)), box)


def clean_text_regions(img: Image.Image, regions: list[tuple[int, int, int, int]],
                       fill: tuple[int, int, int, int] | None = None,
                       source_box: tuple[int, int, int, int] | None = None) -> Image.Image:
    out = img.copy()
    for box in regions:
        soften_region(out, box, 12)
        if source_box:
            tile_region(out, box, source_box)
        elif fill:
            cover_region(out, box, fill, 4)
    return out


def add_panel_shadow(img: Image.Image, alpha: int = 90) -> Image.Image:
    shadow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    mask = img.getchannel("A").filter(ImageFilter.GaussianBlur(4))
    shadow.paste((0, 0, 0, alpha), (5, 6), mask)
    shadow.alpha_composite(img)
    return shadow


def make_top_status(base: Image.Image) -> Image.Image:
    return crop(base, 80, 13, 1160, 54)


def make_window_panel(base: Image.Image) -> Image.Image:
    return crop(base, 150, 88, 980, 568)


def make_title_plaque(base: Image.Image) -> Image.Image:
    return crop(base, 430, 76, 420, 72)


def make_info_strip(base: Image.Image) -> Image.Image:
    img = crop(base, 276, 149, 730, 36)
    return clean_text_regions(img, [(52, 5, 698, 31)], rgba("#e5c98f", 182))


def make_tab(base: Image.Image, selected: bool) -> Image.Image:
    box = (236, 194, 190, 44) if selected else (438, 194, 190, 44)
    img = crop(base, *box)
    fill = rgba("#9d681b", 155) if selected else rgba("#2d180d", 160)
    return clean_text_regions(img, [(36, 9, 154, 34)], fill)


def make_panel(base: Image.Image, x: int, y: int, w: int, h: int,
               content_box: tuple[int, int, int, int]) -> Image.Image:
    img = crop(base, x, y, w, h)
    soften_region(img, content_box, 18)
    cover_region(img, content_box, rgba("#e4c890", 184), 4)
    return img


def make_row(base: Image.Image, y: int, selected: bool = False, disabled: bool = False) -> Image.Image:
    img = crop(base, 224, y, 340, 62)
    fill = rgba("#e7cc92", 150)
    if selected:
        fill = rgba("#f0d091", 130)
    if disabled:
        fill = rgba("#82694c", 160)
        img = img.convert("LA").convert("RGBA")
    dynamic_boxes = [
        (10, 9, 82, 53),
        (92, 10, 248, 52),
        (260, 14, 330, 44),
    ]
    img = clean_text_regions(img, dynamic_boxes, fill)
    return img


def make_slot(base: Image.Image) -> Image.Image:
    img = crop(base, 238, 236, 80, 72)
    img = clean_text_regions(img, [(8, 8, 72, 64)], rgba("#e3c78f", 170))
    return img.resize((96, 80), Image.Resampling.LANCZOS)


def make_detail_card(base: Image.Image) -> Image.Image:
    img = crop(base, 602, 236, 204, 294)
    return clean_text_regions(img, [(18, 18, 186, 276)], rgba("#e6cd98", 160))


def make_stats_sheet(base: Image.Image) -> Image.Image:
    img = crop(base, 812, 236, 262, 294)
    return clean_text_regions(img, [(14, 14, 250, 280)], rgba("#e6cd98", 165))


def make_button(base: Image.Image, x: int, color: tuple[int, int, int, int]) -> Image.Image:
    img = crop(base, x, 578, 210, 54)
    return clean_text_regions(img, [(30, 11, 184, 43)], color)


def make_footer(base: Image.Image) -> Image.Image:
    img = crop(base, 366, 641, 520, 34)
    return clean_text_regions(img, [(72, 4, 450, 30)], rgba("#0c0a08", 196))


def save_decorations(base: Image.Image) -> None:
    # These are intentionally rectangular crops. They are drawn back at their
    # original locations over the panel, so the surrounding pixels match.
    crop(base, 162, 90, 116, 70).save(OUT / "backpack_deco_lifebuoy.png")
    crop(base, 226, 86, 250, 52).save(OUT / "backpack_deco_rope.png")
    crop(base, 1060, 88, 72, 92).save(OUT / "backpack_deco_anchor.png")
    crop(base, 148, 154, 70, 106).save(OUT / "backpack_deco_lantern.png")


def make_assets() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    base = base_image()

    make_top_status(base).save(OUT / "backpack_top_status_bar.png")
    make_window_panel(base).save(OUT / "backpack_window_panel.png")
    make_title_plaque(base).save(OUT / "backpack_title_plaque.png")
    make_info_strip(base).save(OUT / "backpack_info_strip.png")
    make_tab(base, True).save(OUT / "backpack_tab_selected.png")
    make_tab(base, False).save(OUT / "backpack_tab_normal.png")
    make_panel(base, 202, 224, 378, 350, (18, 14, 360, 332)).save(OUT / "backpack_list_panel.png")
    make_panel(base, 580, 224, 512, 350, (18, 14, 494, 332)).save(OUT / "backpack_detail_panel.png")
    make_detail_card(base).save(OUT / "backpack_detail_card.png")
    make_stats_sheet(base).save(OUT / "backpack_stats_sheet.png")
    make_row(base, 236, selected=False).save(OUT / "backpack_row_normal.png")
    make_row(base, 236, selected=True).save(OUT / "backpack_row_selected.png")
    make_row(base, 482, disabled=True).save(OUT / "backpack_row_disabled.png")
    make_slot(base).save(OUT / "backpack_slot_frame.png")
    make_button(base, 270, rgba("#57701d", 160)).save(OUT / "backpack_button_green.png")
    make_button(base, 532, rgba("#285a73", 160)).save(OUT / "backpack_button_blue.png")
    make_button(base, 790, rgba("#813819", 160)).save(OUT / "backpack_button_red.png")
    make_footer(base).save(OUT / "backpack_footer_hint.png")
    save_decorations(base)


if __name__ == "__main__":
    make_assets()
