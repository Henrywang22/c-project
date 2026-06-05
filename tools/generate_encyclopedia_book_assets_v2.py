from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = Path(r"C:\Users\lenovo\.codex\generated_images\019e86c4-668b-7a03-a1a3-b2d42566aff9")

OPEN_BOOK_SOURCE = SOURCE_DIR / "ig_0ef8292bdcac4105016a1e687b965481999446296b16c7b63b.png"
PAGE_SOURCE = SOURCE_DIR / "ig_0ef8292bdcac4105016a1e68b333ec8199b31c46822a4e046c.png"
CLOSED_COVER_SOURCE = SOURCE_DIR / "ig_0ef8292bdcac4105016a1e68ce7d148199960e032ffa472122.png"

BOOK_OUT = ROOT / "ui图" / "生成素材" / "航海图鉴UI" / "encyclopedia_book_frame_v2.png"
PIXEL_UI_DIR = ROOT / "ui图" / "生成素材" / "航海图鉴UI" / "pixel_ui"
COVER_OUT = PIXEL_UI_DIR / "open_closed_cover_v2.png"
PAGE_LEFT_OUT = PIXEL_UI_DIR / "open_page_left_v2.png"
PAGE_RIGHT_OUT = PIXEL_UI_DIR / "open_page_right_v2.png"
FLIP_PAGE_OUT = PIXEL_UI_DIR / "open_flip_page_v2.png"
PREVIEW_OUT = ROOT / "ui图" / "生成素材" / "航海图鉴UI" / "encyclopedia_book_assets_v2_preview.png"

FLIP_PAGE_SIZE = (482, 564)


def chroma_to_alpha(image: Image.Image) -> Image.Image:
    image = image.convert("RGBA")
    pixels = image.load()
    width, height = image.size

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            green_strength = g - max(r, b)
            if g > 115 and green_strength > 24:
                alpha = int(255 * (1.0 - min(1.0, (green_strength - 24) / 95.0)))
                a = min(a, max(0, alpha))
            if a > 0 and g > max(r, b) + 8:
                g = min(g, max(r, b) + 8)
            pixels[x, y] = (r, g, b, a)

    alpha = image.getchannel("A").filter(ImageFilter.MinFilter(3)).filter(ImageFilter.GaussianBlur(0.45))
    image.putalpha(alpha)
    return image


def crop_to_alpha(image: Image.Image, padding: int) -> Image.Image:
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        return image
    left, top, right, bottom = bbox
    left = max(0, left - padding)
    top = max(0, top - padding)
    right = min(image.width, right + padding)
    bottom = min(image.height, bottom + padding)
    return image.crop((left, top, right, bottom))


def fit_on_canvas(image: Image.Image, size: tuple[int, int], pad: int = 0) -> Image.Image:
    max_w = size[0] - pad * 2
    max_h = size[1] - pad * 2
    scale = min(max_w / image.width, max_h / image.height)
    resized = image.resize((int(image.width * scale), int(image.height * scale)), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.alpha_composite(resized, ((size[0] - resized.width) // 2, (size[1] - resized.height) // 2))
    return canvas


def fit_exact(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    return image.resize(size, Image.Resampling.LANCZOS)


def add_spine_shade(page: Image.Image, left_side: bool) -> Image.Image:
    page = page.convert("RGBA")
    shade = Image.new("RGBA", page.size, (0, 0, 0, 0))
    width, height = page.size
    shade_pixels = shade.load()
    for y in range(height):
        for x in range(width):
            side_distance = x if left_side else width - 1 - x
            t = max(0.0, 1.0 - side_distance / (width * 0.24))
            if t <= 0.0:
                continue
            alpha = int(52 * (t ** 1.7))
            shade_pixels[x, y] = (65, 35, 12, alpha)
    page.alpha_composite(shade)
    return page


def add_page_edge_depth(page: Image.Image) -> Image.Image:
    alpha = page.getchannel("A")
    edge = alpha.filter(ImageFilter.FIND_EDGES).filter(ImageFilter.GaussianBlur(0.7))
    edge_layer = Image.new("RGBA", page.size, (72, 39, 13, 0))
    edge_layer.putalpha(edge.point(lambda value: min(120, value)))
    page = page.convert("RGBA")
    page.alpha_composite(edge_layer)
    return page


def page_from_book(book: Image.Image, side: str, size: tuple[int, int] = (600, 660),
                   pad: int = 4, exact: bool = False) -> Image.Image:
    book = book.convert("RGBA")
    if side == "right":
        crop_box = (622, 48, 1102, 612)
        polygon = [
            (8, 56), (52, 20), (178, 4), (378, 4), (466, 42),
            (468, 508), (424, 548), (230, 558), (42, 536), (8, 494)
        ]
    else:
        crop_box = (138, 48, 618, 612)
        polygon = [
            (472, 56), (428, 20), (302, 4), (102, 4), (14, 42),
            (12, 508), (56, 548), (250, 558), (438, 536), (472, 494)
        ]

    crop = book.crop(crop_box)
    mask = Image.new("L", crop.size, 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.polygon(polygon, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(1.3))

    page = Image.new("RGBA", crop.size, (0, 0, 0, 0))
    page.alpha_composite(crop)
    page.putalpha(ImageChops.multiply(page.getchannel("A"), mask))
    page = crop_to_alpha(page, 0 if exact else 8)
    page = fit_exact(page, size) if exact else fit_on_canvas(page, size, pad)
    return add_page_edge_depth(polish(page, 1.03, 1.01))


def polish(image: Image.Image, contrast: float = 1.05, color: float = 1.04) -> Image.Image:
    image = ImageEnhance.Contrast(image).enhance(contrast)
    image = ImageEnhance.Color(image).enhance(color)
    return image


def make_preview(images: list[tuple[str, Image.Image]]) -> None:
    thumb_w, thumb_h = 360, 210
    gap = 18
    canvas = Image.new("RGBA", (thumb_w * 2 + gap * 3, thumb_h * 3 + gap * 4), (24, 29, 33, 255))
    for index, (_, image) in enumerate(images):
        thumb = fit_on_canvas(image, (thumb_w, thumb_h), 12)
        x = gap + (index % 2) * (thumb_w + gap)
        y = gap + (index // 2) * (thumb_h + gap)
        bg = Image.new("RGBA", (thumb_w, thumb_h), (42, 50, 55, 255))
        bg.alpha_composite(thumb)
        canvas.alpha_composite(bg, (x, y))
    canvas.convert("RGB").save(PREVIEW_OUT)


def main() -> None:
    for source in (OPEN_BOOK_SOURCE, PAGE_SOURCE, CLOSED_COVER_SOURCE):
        if not source.exists():
            raise FileNotFoundError(source)

    BOOK_OUT.parent.mkdir(parents=True, exist_ok=True)
    PIXEL_UI_DIR.mkdir(parents=True, exist_ok=True)

    open_book = chroma_to_alpha(Image.open(OPEN_BOOK_SOURCE))
    open_book = crop_to_alpha(open_book, 14)
    open_book = fit_on_canvas(polish(open_book, 1.08, 1.05), (1240, 704), 2)
    open_book.save(BOOK_OUT)

    closed_cover = chroma_to_alpha(Image.open(CLOSED_COVER_SOURCE))
    closed_cover = crop_to_alpha(closed_cover, 18)
    closed_cover = fit_on_canvas(polish(closed_cover, 1.08, 1.04), (520, 660), 2)
    closed_cover.save(COVER_OUT)

    page_left = add_spine_shade(page_from_book(open_book, "left"), left_side=False)
    page_right = add_spine_shade(page_from_book(open_book, "right"), left_side=True)
    flip_page = add_spine_shade(page_from_book(open_book, "right", FLIP_PAGE_SIZE, 0, True), left_side=True)

    page_left.save(PAGE_LEFT_OUT)
    page_right.save(PAGE_RIGHT_OUT)
    flip_page.save(FLIP_PAGE_OUT)

    make_preview([
        ("book", open_book),
        ("cover", closed_cover),
        ("left", page_left),
        ("right", page_right),
        ("flip", flip_page),
    ])

    print(BOOK_OUT)
    print(COVER_OUT)
    print(PAGE_LEFT_OUT)
    print(PAGE_RIGHT_OUT)
    print(FLIP_PAGE_OUT)
    print(PREVIEW_OUT)


if __name__ == "__main__":
    main()
