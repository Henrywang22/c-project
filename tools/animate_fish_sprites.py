from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageEnhance


ROOT = Path(__file__).resolve().parents[1]
FISH_DIR = ROOT / "assets" / "fish_new"

FISH_FILES = [
    "anchovy",
    "clownfish",
    "mackerel",
    "sea_bream",
    "lanternfish",
    "grouper",
    "koi",
    "crystal_fish",
]


def clean_chroma_fringe(img: Image.Image) -> Image.Image:
    """Remove green-key remnants left from generated source sheets."""
    rgba = img.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            hard_key = g > 215 and r < 80 and b < 110
            soft_key = a < 245 and g > max(r, b) * 1.35 and g - max(r, b) > 35
            if hard_key or soft_key:
                pixels[x, y] = (r, g, b, 0)
            elif a < 255 and g > max(r, b) + 30:
                # Despill semitransparent edge pixels without touching opaque fish color.
                pixels[x, y] = (r, max(r, b), b, a)
    return rgba


def trim_alpha(img: Image.Image, padding: int = 8) -> Image.Image:
    alpha = img.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        return img
    left, top, right, bottom = bbox
    left = max(0, left - padding)
    top = max(0, top - padding)
    right = min(img.width, right + padding)
    bottom = min(img.height, bottom + padding)
    return img.crop((left, top, right, bottom))


def warp_frame(base: Image.Image, phase: float, amp: float) -> Image.Image:
    width, height = base.size
    pad_x = max(18, int(width * 0.08))
    pad_y = max(18, int(height * 0.24))
    frame = Image.new("RGBA", (width + pad_x * 2, height + pad_y * 2), (0, 0, 0, 0))

    for x in range(width):
        tail_weight = max(0.0, 1.0 - x / max(1, width - 1))
        tail_weight = tail_weight ** 1.75
        body_weight = 0.10 * math.sin((x / max(1, width - 1)) * math.pi)
        y_offset = int(round(amp * (tail_weight + body_weight) * math.sin(phase + x / width * 1.35)))
        column = base.crop((x, 0, x + 1, height))
        frame.alpha_composite(column, (pad_x + x, pad_y + y_offset))

    return frame


def animate_one(name: str) -> Image.Image:
    cutout_path = FISH_DIR / f"{name}_cutout.png"
    swim_path = FISH_DIR / f"{name}_swim.png"

    base = clean_chroma_fringe(Image.open(cutout_path))
    base = trim_alpha(base, padding=6)
    base.save(cutout_path)

    amp = max(4.0, min(13.0, base.height * 0.16))
    phases = [math.pi * 0.15, math.pi * 0.68, math.pi * 1.20, math.pi * 1.72]
    frames = []
    for i, phase in enumerate(phases):
        frame = warp_frame(base, phase, amp)
        if i % 2 == 1:
            frame = ImageEnhance.Brightness(frame).enhance(1.035)
        frames.append(frame)

    frame_w = max(frame.width for frame in frames)
    frame_h = max(frame.height for frame in frames)
    sheet = Image.new("RGBA", (frame_w * len(frames), frame_h), (0, 0, 0, 0))
    for i, frame in enumerate(frames):
        x = i * frame_w + (frame_w - frame.width) // 2
        y = (frame_h - frame.height) // 2
        sheet.alpha_composite(frame, (x, y))

    sheet.save(swim_path)
    return sheet


def make_preview(sheets: list[Image.Image]) -> None:
    cell_w = max(sheet.width // 4 for sheet in sheets)
    cell_h = max(sheet.height for sheet in sheets)
    gap_x = 44
    gap_y = 28
    preview = Image.new(
        "RGBA",
        (cell_w * 4 + gap_x * 5, cell_h * len(sheets) + gap_y * (len(sheets) + 1)),
        (0, 255, 0, 255),
    )

    for row, sheet in enumerate(sheets):
        frame_w = sheet.width // 4
        y = gap_y + row * (cell_h + gap_y)
        for col in range(4):
            frame = sheet.crop((col * frame_w, 0, (col + 1) * frame_w, sheet.height))
            x = gap_x + col * (cell_w + gap_x) + (cell_w - frame.width) // 2
            preview.alpha_composite(frame, (x, y + (cell_h - frame.height) // 2))

    preview.convert("RGB").save(FISH_DIR / "_animated_preview.png")


def main() -> None:
    sheets = [animate_one(name) for name in FISH_FILES]
    make_preview(sheets)


if __name__ == "__main__":
    main()
