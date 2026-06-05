from __future__ import annotations

from collections import deque
from pathlib import Path

from PIL import Image, ImageEnhance, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SHEET = Path(r"C:\Users\lenovo\.codex\generated_images\019e86c4-668b-7a03-a1a3-b2d42566aff9\ig_0ef8292bdcac4105016a1e8618f2f4819992b70861a6d6b9a1.png")
OUT_DIR = ROOT / "ui图" / "生成素材" / "航海图鉴UI" / "opening_sequence_v3"
PREVIEW = OUT_DIR / "sequence_v3_preview_latest.png"
BOOK_FRAME = ROOT / "ui图" / "生成素材" / "航海图鉴UI" / "encyclopedia_book_frame_v3.png"


def is_green(r: int, g: int, b: int) -> bool:
    return g > 135 and g - max(r, b) > 34


def chroma_to_alpha(image: Image.Image) -> Image.Image:
    image = image.convert("RGBA")
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = pixels[x, y]
            if is_green(r, g, b):
                strength = g - max(r, b)
                a = min(a, max(0, int(255 * (1.0 - min(1.0, (strength - 34) / 110.0)))))
            if a > 0 and g > max(r, b) + 8:
                g = min(g, max(r, b) + 8)
            pixels[x, y] = (r, g, b, a)
    alpha = image.getchannel("A").filter(ImageFilter.MinFilter(3)).filter(ImageFilter.GaussianBlur(0.35))
    image.putalpha(alpha)
    return image


def non_green_mask(image: Image.Image) -> list[list[bool]]:
    rgb = image.convert("RGB")
    px = rgb.load()
    return [[not is_green(*px[x, y]) for x in range(rgb.width)] for y in range(rgb.height)]


def components(mask: list[list[bool]], min_area: int = 900) -> list[tuple[int, int, int, int, int]]:
    height = len(mask)
    width = len(mask[0])
    seen = [[False] * width for _ in range(height)]
    boxes: list[tuple[int, int, int, int, int]] = []
    for sy in range(height):
        for sx in range(width):
            if seen[sy][sx] or not mask[sy][sx]:
                continue
            queue = deque([(sx, sy)])
            seen[sy][sx] = True
            min_x = max_x = sx
            min_y = max_y = sy
            area = 0
            while queue:
                x, y = queue.popleft()
                area += 1
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if nx < 0 or ny < 0 or nx >= width or ny >= height:
                        continue
                    if seen[ny][nx] or not mask[ny][nx]:
                        continue
                    seen[ny][nx] = True
                    queue.append((nx, ny))
            if area >= min_area:
                boxes.append((min_x, min_y, max_x + 1, max_y + 1, area))
    return sorted(boxes, key=lambda b: b[0])


def fit_on_canvas(image: Image.Image, size: tuple[int, int], pad: int = 6) -> Image.Image:
    max_w = size[0] - pad * 2
    max_h = size[1] - pad * 2
    scale = min(max_w / image.width, max_h / image.height)
    resized = image.resize((int(image.width * scale), int(image.height * scale)), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.alpha_composite(resized, ((size[0] - resized.width) // 2, (size[1] - resized.height) // 2))
    return canvas


def canvas_frame(image: Image.Image, index: int) -> Image.Image:
    if index < 2:
        frame = fit_on_canvas(image, (1240, 704), 46)
    else:
        frame = image.resize((1240, 704), Image.Resampling.LANCZOS)
    frame = ImageEnhance.Sharpness(frame).enhance(1.35)
    return frame


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    sheet = Image.open(SHEET)
    boxes = components(non_green_mask(sheet))
    if len(boxes) < 5:
        raise RuntimeError(f"Expected at least 5 frame components, found {len(boxes)}")
    boxes = boxes[:5]

    frames = []
    canvas_frames = []
    rgba_sheet = chroma_to_alpha(sheet)
    for i, (left, top, right, bottom, _) in enumerate(boxes):
        pad = 18
        crop = rgba_sheet.crop((
            max(0, left - pad),
            max(0, top - pad),
            min(rgba_sheet.width, right + pad),
            min(rgba_sheet.height, bottom + pad),
        ))
        crop = ImageEnhance.Contrast(crop).enhance(1.05)
        out = OUT_DIR / f"open_sequence_{i}_v3.png"
        crop.save(out)
        canvas = canvas_frame(crop, i)
        canvas_out = OUT_DIR / f"open_sequence_canvas_{i}_v3.png"
        canvas.save(canvas_out)
        canvas_frames.append(canvas)
        frames.append(fit_on_canvas(crop, (620, 352), 12))
        print(i, out, crop.size, canvas_out)

    preview = Image.new("RGBA", (620 * 5, 352), (22, 29, 34, 255))
    for i, frame in enumerate(frames):
        preview.alpha_composite(frame, (620 * i, 0))
    preview.convert("RGB").save(PREVIEW)
    canvas_frames[-1].save(BOOK_FRAME)
    print(PREVIEW)
    print(BOOK_FRAME)


if __name__ == "__main__":
    main()
