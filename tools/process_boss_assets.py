from __future__ import annotations

import hashlib
import math
import re
from collections import deque
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
GENERATED = ROOT / "assets" / "boss_generated"
EXISTING = ROOT / "ui图" / "boss"


GENERATED_SHEETS = {
    GENERATED / "five_head_shark" / "sources" / "summon_water_source.png":
        ("five_head_shark/summon_water", 4, 2, (0, 255, 0)),
    GENERATED / "five_head_shark" / "sources" / "hit_reaction_source.png":
        ("five_head_shark/hit_reaction", 3, 2, (0, 255, 0)),
    GENERATED / "five_head_shark" / "sources" / "death_source.png":
        ("five_head_shark/death", 4, 2, (0, 255, 0)),
    GENERATED / "siren" / "sources" / "phase_transition_source.png":
        ("siren/phase_transition", 4, 2, (0, 255, 0)),
    GENERATED / "siren" / "sources" / "resonance_pillar_source.png":
        ("siren/resonance_pillar", 3, 2, (0, 255, 0)),
    GENERATED / "siren" / "sources" / "seaweed_zone_source.png":
        ("siren/seaweed_zone", 4, 2, (255, 0, 255)),
    GENERATED / "siren" / "sources" / "reef_emerge_source.png":
        ("siren/reef_emerge", 4, 2, (0, 255, 0)),
    GENERATED / "siren" / "sources" / "phantom_stun_dissolve_source.png":
        ("siren/phantom_stun_dissolve", 4, 2, (255, 0, 255)),
    GENERATED / "siren" / "sources" / "immunity_feedback_source.png":
        ("siren/immunity_feedback", 3, 2, (0, 255, 0)),
    GENERATED / "siren" / "sources" / "elegy_pull_source.png":
        ("siren/elegy_pull", 4, 2, (0, 255, 0)),
    GENERATED / "siren" / "sources" / "focus_meter_source.png":
        ("siren/focus_meter", 1, 5, (0, 255, 0)),
    GENERATED / "siren" / "sources" / "death_source.png":
        ("siren/death", 4, 2, (0, 255, 0)),
}


EXISTING_GROUPS = [
    ("five_head_shark/idle", EXISTING / "夺命五头鲨", "待机"),
    ("five_head_shark/bite", EXISTING / "夺命五头鲨", "撕咬"),
    ("five_head_shark/cast", EXISTING / "夺命五头鲨", "召唤轰炸"),
    ("siren/idle", EXISTING / "cpp（boss设计——塞壬）", "待机"),
    ("siren/phantom_move", EXISTING / "cpp（boss设计——塞壬）", "倩影移动"),
    ("siren/soul_song", EXISTING / "cpp（boss设计——塞壬）", "噬魂迷音"),
    ("siren/elegy_windup", EXISTING / "cpp（boss设计——塞壬）", "塞壬哀歌蓄力"),
    ("siren/soul_song_windup", EXISTING / "cpp（boss设计——塞壬）", "塞壬迷音蓄力"),
    ("siren/elegy_wave", EXISTING / "cpp（boss设计——塞壬）", "飘渺哀歌"),
    ("siren/elegy_pull", EXISTING / "cpp（boss设计——塞壬）", "飘渺哀歌"),
    ("siren/seaweed_zone", EXISTING / "cpp（boss设计——塞壬）" / "cpp（boss设计——塞壬，补）", "草地（备选）"),
    ("siren/resonance_pillar", EXISTING / "cpp（boss设计——塞壬）" / "cpp（boss设计——塞壬，补）", "碎裂3-2"),
]


def numeric_key(path: Path) -> tuple[int, str]:
    numbers = re.findall(r"\d+", path.stem)
    return (int(numbers[-1]) if numbers else 0, path.name)


def chroma_to_alpha(image: Image.Image, key: tuple[int, int, int]) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            r, g, b, _ = pixels[x, y]
            distance = math.sqrt((r - key[0]) ** 2 + (g - key[1]) ** 2 + (b - key[2]) ** 2)
            if distance <= 28:
                alpha = 0
            elif distance >= 125:
                alpha = 255
            else:
                alpha = round((distance - 28) / 97 * 255)

            if key[1] > 200 and g > max(r, b):
                g = min(g, max(r, b) + 8)
            elif key[0] > 200 and key[2] > 200 and r > g and b > g:
                spill = min(r, b) - g
                r = max(g, r - spill)
                b = max(g, b - spill)
            pixels[x, y] = (r, g, b, alpha)
    return rgba


def remove_large_white_frame(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    w, h = rgba.size
    pixels = rgba.load()
    mask: set[tuple[int, int]] = set()
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a > 120 and r > 215 and g > 215 and b > 215:
                mask.add((x, y))

    visited: set[tuple[int, int]] = set()
    for point in list(mask):
        if point in visited:
            continue
        queue: deque[tuple[int, int]] = deque([point])
        visited.add(point)
        component: list[tuple[int, int]] = []
        while queue:
            x, y = queue.popleft()
            component.append((x, y))
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if (nx, ny) in mask and (nx, ny) not in visited:
                    visited.add((nx, ny))
                    queue.append((nx, ny))

        if len(component) < 500:
            continue
        xs = [p[0] for p in component]
        ys = [p[1] for p in component]
        if max(xs) - min(xs) > w * 0.55 and max(ys) - min(ys) > h * 0.55:
            left, right = min(xs), max(xs)
            top, bottom = min(ys), max(ys)
            for x, y in component:
                pixels[x, y] = (0, 0, 0, 0)
            for y in range(max(0, top - 3), min(h, bottom + 4)):
                for x in range(max(0, left - 3), min(w, left + 4)):
                    pixels[x, y] = (0, 0, 0, 0)
                for x in range(max(0, right - 3), min(w, right + 4)):
                    pixels[x, y] = (0, 0, 0, 0)
            for x in range(max(0, left - 3), min(w, right + 4)):
                for y in range(max(0, top - 3), min(h, top + 4)):
                    pixels[x, y] = (0, 0, 0, 0)
                for y in range(max(0, bottom - 3), min(h, bottom + 4)):
                    pixels[x, y] = (0, 0, 0, 0)
    return rgba


def erase_canvas_edge_lines(image: Image.Image, inset: int = 10) -> Image.Image:
    """Remove generator-drawn rectangular borders close to an animation cell edge."""
    rgba = image.convert("RGBA")
    w, h = rgba.size
    pixels = rgba.load()

    def erase_if_line_like(x: int, y: int) -> None:
        r, g, b, a = pixels[x, y]
        if a <= 12:
            return
        luminance = max(r, g, b)
        saturation = max(r, g, b) - min(r, g, b)
        cyan_line = g >= 70 and b >= 70 and r <= g + 28
        pale_line = luminance >= 84 and saturation <= 54
        if cyan_line or pale_line:
            pixels[x, y] = (0, 0, 0, 0)

    for y in range(h):
        for x in range(min(inset, w)):
            erase_if_line_like(x, y)
        for x in range(max(0, w - inset), w):
            erase_if_line_like(x, y)
    for x in range(w):
        for y in range(min(inset, h)):
            erase_if_line_like(x, y)
        for y in range(max(0, h - inset), h):
            erase_if_line_like(x, y)
    return rgba


def erase_long_border_lines(image: Image.Image,
                            max_thickness: int = 12,
                            min_span_ratio: float = 0.32) -> Image.Image:
    """Remove thin horizontal/vertical generator borders without touching effects."""
    rgba = image.convert("RGBA")
    w, h = rgba.size
    pixels = rgba.load()
    line_like: set[tuple[int, int]] = set()
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a <= 16:
                continue
            luminance = max(r, g, b)
            saturation = max(r, g, b) - min(r, g, b)
            if (g >= 54 and b >= 54 and r <= max(g, b) + 30) or (luminance >= 78 and saturation <= 62):
                line_like.add((x, y))

    visited: set[tuple[int, int]] = set()
    for point in list(line_like):
        if point in visited:
            continue
        queue: deque[tuple[int, int]] = deque([point])
        visited.add(point)
        component: list[tuple[int, int]] = []
        while queue:
            x, y = queue.popleft()
            component.append((x, y))
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if (nx, ny) in line_like and (nx, ny) not in visited:
                    visited.add((nx, ny))
                    queue.append((nx, ny))

        if len(component) < 16:
            continue
        xs = [p[0] for p in component]
        ys = [p[1] for p in component]
        width = max(xs) - min(xs) + 1
        height = max(ys) - min(ys) + 1
        horizontal_border = width >= w * min_span_ratio and height <= max_thickness
        vertical_border = height >= h * min_span_ratio and width <= max_thickness
        if not horizontal_border and not vertical_border:
            continue
        for x, y in component:
            pixels[x, y] = (0, 0, 0, 0)
    return rgba


def connected_dark_background(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    rgb = rgba.convert("RGB")
    w, h = rgb.size
    source = rgb.load()
    visited = bytearray(w * h)
    queue: deque[tuple[int, int]] = deque()

    border_samples = []
    for x in range(0, w, max(1, w // 64)):
        border_samples.extend((source[x, 0], source[x, h - 1]))
    for y in range(0, h, max(1, h // 64)):
        border_samples.extend((source[0, y], source[w - 1, y]))
    bg = tuple(sorted(sample[channel] for sample in border_samples)[len(border_samples) // 2]
               for channel in range(3))

    def background_like(x: int, y: int) -> bool:
        r, g, b = source[x, y]
        distance = math.sqrt((r - bg[0]) ** 2 + (g - bg[1]) ** 2 + (b - bg[2]) ** 2)
        # The Siren artwork uses very dark blue/black body details.  The old
        # threshold treated those edge-connected details as background and
        # left visibly hollow patches in the model.
        return distance < 20 and max(r, g, b) < 32

    for x in range(w):
        if background_like(x, 0):
            queue.append((x, 0))
        if background_like(x, h - 1):
            queue.append((x, h - 1))
    for y in range(h):
        if background_like(0, y):
            queue.append((0, y))
        if background_like(w - 1, y):
            queue.append((w - 1, y))

    while queue:
        x, y = queue.popleft()
        index = y * w + x
        if visited[index] or not background_like(x, y):
            continue
        visited[index] = 1
        if x > 0:
            queue.append((x - 1, y))
        if x + 1 < w:
            queue.append((x + 1, y))
        if y > 0:
            queue.append((x, y - 1))
        if y + 1 < h:
            queue.append((x, y + 1))

    alpha = Image.new("L", (w, h), 255)
    alpha_pixels = alpha.load()
    for y in range(h):
        for x in range(w):
            if visited[y * w + x]:
                alpha_pixels[x, y] = 0
    alpha = alpha.filter(ImageFilter.GaussianBlur(0.65))
    rgba.putalpha(alpha)
    return rgba


def solid_dark_background(image: Image.Image) -> Image.Image:
    """Remove a black backdrop while keeping dark details inside the object opaque."""
    rgba = image.convert("RGBA")
    rgb = rgba.convert("RGB")
    w, h = rgb.size
    pixels = rgb.load()

    foreground = Image.new("L", (w, h), 0)
    foreground_pixels = foreground.load()
    for y in range(h):
        for x in range(w):
            r, g, b = pixels[x, y]
            if max(r, g, b) > 8:
                foreground_pixels[x, y] = 255

    closed = foreground.filter(ImageFilter.MaxFilter(7)).filter(ImageFilter.MinFilter(7))
    silhouette = ImageChops.lighter(foreground, closed)
    silhouette_pixels = silhouette.load()
    outside = bytearray(w * h)
    queue: deque[tuple[int, int]] = deque()

    for x in range(w):
        if silhouette_pixels[x, 0] < 128:
            queue.append((x, 0))
        if silhouette_pixels[x, h - 1] < 128:
            queue.append((x, h - 1))
    for y in range(h):
        if silhouette_pixels[0, y] < 128:
            queue.append((0, y))
        if silhouette_pixels[w - 1, y] < 128:
            queue.append((w - 1, y))

    while queue:
        x, y = queue.popleft()
        index = y * w + x
        if outside[index] or silhouette_pixels[x, y] >= 128:
            continue
        outside[index] = 1
        if x > 0:
            queue.append((x - 1, y))
        if x + 1 < w:
            queue.append((x + 1, y))
        if y > 0:
            queue.append((x, y - 1))
        if y + 1 < h:
            queue.append((x, y + 1))

    alpha = Image.new("L", (w, h), 255)
    alpha_pixels = alpha.load()
    for y in range(h):
        for x in range(w):
            if outside[y * w + x]:
                alpha_pixels[x, y] = 0
    alpha = alpha.filter(ImageFilter.GaussianBlur(0.55))
    rgba.putalpha(alpha)
    return rgba


def glow_dark_background(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            r, g, b, _ = pixels[x, y]
            luminance = max(r, g, b)
            if luminance <= 8:
                alpha = 0
            elif luminance <= 72:
                alpha = round((luminance - 8) / 64 * 210)
            else:
                alpha = 255
            pixels[x, y] = (r, g, b, alpha)
    return rgba


def trim_cell_lines(image: Image.Image) -> Image.Image:
    # Generators occasionally draw separators exactly on cell borders.
    inset = max(2, min(image.size) // 160)
    return image.crop((inset, inset, image.width - inset, image.height - inset))


def remove_side_edge_fragments(image: Image.Image, alpha_threshold: int = 18) -> Image.Image:
    """Erase fragments that are connected to the left/right edge of a sprite-strip cell."""
    rgba = image.convert("RGBA")
    w, h = rgba.size
    pixels = rgba.load()
    def foreground(x: int, y: int) -> bool:
        r, g, b, a = pixels[x, y]
        return a > alpha_threshold and max(r, g, b) > 8

    visited = bytearray(w * h)
    queue: deque[tuple[int, int]] = deque()
    for y in range(h):
        if foreground(0, y):
            queue.append((0, y))
        if foreground(w - 1, y):
            queue.append((w - 1, y))

    while queue:
        x, y = queue.popleft()
        index = y * w + x
        if visited[index] or not foreground(x, y):
            continue
        visited[index] = 1
        pixels[x, y] = (0, 0, 0, 0)
        if x > 0:
            queue.append((x - 1, y))
        if x + 1 < w:
            queue.append((x + 1, y))
        if y > 0:
            queue.append((x, y - 1))
        if y + 1 < h:
            queue.append((x, y + 1))
    return rgba


def normalize_frames(frames: list[Image.Image], size: tuple[int, int]) -> list[Image.Image]:
    prepared = []
    max_content_width = 1
    max_content_height = 1
    for frame in frames:
        rgba = frame.convert("RGBA")
        bbox = rgba.getchannel("A").getbbox()
        prepared.append((rgba, bbox))
        if bbox:
            max_content_width = max(max_content_width, bbox[2] - bbox[0])
            max_content_height = max(max_content_height, bbox[3] - bbox[1])

    common_scale = min(
        size[0] * 0.86 / max_content_width,
        size[1] * 0.86 / max_content_height,
    )
    normalized = []
    for frame, bbox in prepared:
        canvas = Image.new("RGBA", size, (0, 0, 0, 0))
        if not bbox:
            normalized.append(canvas)
            continue
        content = frame.crop(bbox)
        scaled_size = (
            max(1, round(content.width * common_scale)),
            max(1, round(content.height * common_scale)),
        )
        content = content.resize(scaled_size, Image.Resampling.LANCZOS)
        x = (size[0] - content.width) // 2
        y = (size[1] - content.height) // 2
        canvas.alpha_composite(content, (x, y))
        normalized.append(canvas)
    return normalized


def save_sequence(relative: str, frames: list[Image.Image], loop: bool = False,
                  prepared: bool = False) -> None:
    target = GENERATED / relative
    target.mkdir(parents=True, exist_ok=True)
    for stale in target.glob("frame_*.png"):
        stale.unlink()
    for stale_name in ("preview.gif", "sprite_sheet.png"):
        stale = target / stale_name
        if stale.exists():
            stale.unlink()
    size = (384, 96) if relative.endswith("focus_meter") else (512, 512)
    if not prepared:
        frames = normalize_frames(frames, size)
    for index, frame in enumerate(frames, 1):
        frame.save(target / f"frame_{index:02d}.png", optimize=True)

    preview_frames = frames
    if loop and len(frames) > 2:
        preview_frames = frames + frames[-2:0:-1]
    preview_frames[0].save(
        target / "preview.gif",
        save_all=True,
        append_images=preview_frames[1:],
        duration=90 if "effect" in relative or "zone" in relative else 110,
        loop=0,
        disposal=2,
        transparency=0,
    )

    sheet = Image.new("RGBA", (size[0] * len(frames), size[1]), (0, 0, 0, 0))
    for index, frame in enumerate(frames):
        sheet.alpha_composite(frame, (index * size[0], 0))
    sheet.save(target / "sprite_sheet.png", optimize=True)


def rebuild_runtime_sheets() -> None:
    for target in GENERATED.rglob("*"):
        if not target.is_dir():
            continue
        frame_paths = sorted(target.glob("frame_*.png"))
        if not frame_paths:
            continue
        frames = [Image.open(path).convert("RGBA") for path in frame_paths]
        sheet = Image.new(
            "RGBA",
            (frames[0].width * len(frames), frames[0].height),
            (0, 0, 0, 0),
        )
        for index, frame in enumerate(frames):
            sheet.alpha_composite(frame, (index * frames[0].width, 0))
        sheet.save(target / "sprite_sheet.png", optimize=True)


def repair_directional_beam_alpha() -> None:
    """Recover the glow that is present in RGB but missing from alpha."""
    for relative in ("siren/soul_song_warning_beam", "siren/soul_song_beam"):
        target = GENERATED / relative
        for path in sorted(target.glob("frame_*.png")):
            rgba = Image.open(path).convert("RGBA")
            pixels = rgba.load()
            for y in range(rgba.height):
                for x in range(rgba.width):
                    r, g, b, old_alpha = pixels[x, y]
                    luminance = max(r, g, b)
                    if luminance <= 6:
                        recovered_alpha = 0
                    elif luminance >= 120:
                        recovered_alpha = 255
                    else:
                        recovered_alpha = round((luminance - 6) / 114 * 255)
                    pixels[x, y] = (r, g, b, max(old_alpha, recovered_alpha))
            rgba.save(path, optimize=True)


def find_generated_source(pattern: str) -> Path | None:
    candidates = [
        path for path in ROOT.rglob(pattern)
        if "boss" in str(path).lower() and "boss_generated" not in str(path)
    ]
    return sorted(candidates, key=lambda path: str(path))[0] if candidates else None


def draw_pillar_light(image: Image.Image, center: tuple[int, int], enabled: bool) -> None:
    cx, cy = center
    if not enabled:
        overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
        draw = ImageDraw.Draw(overlay)
        draw.ellipse((cx - 29, cy - 29, cx + 29, cy + 29),
                     fill=(2, 14, 18, 238))
        overlay = overlay.filter(ImageFilter.GaussianBlur(1.6))
        image.alpha_composite(overlay)
        return

    glow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(glow)
    draw.ellipse((cx - 43, cy - 43, cx + 43, cy + 43),
                 fill=(34, 230, 244, 82))
    glow = glow.filter(ImageFilter.GaussianBlur(11))
    image.alpha_composite(glow)

    core = Image.new("RGBA", image.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(core)
    draw.ellipse((cx - 27, cy - 27, cx + 27, cy + 27),
                 fill=(10, 130, 150, 210))
    draw.ellipse((cx - 21, cy - 21, cx + 21, cy + 21),
                 fill=(28, 222, 235, 235))
    draw.ellipse((cx - 12, cy - 12, cx + 12, cy + 12),
                 fill=(150, 255, 255, 245))
    image.alpha_composite(core)


def align_pillar_sequence(frames: list[Image.Image],
                          reference: Image.Image,
                          size: tuple[int, int] = (512, 512)) -> list[Image.Image]:
    """Scale one source group from its intact reference and keep a fixed base line."""
    reference_bbox = reference.getchannel("A").getbbox()
    if not reference_bbox:
        return [Image.new("RGBA", size, (0, 0, 0, 0)) for _ in frames]

    reference_width = reference_bbox[2] - reference_bbox[0]
    reference_height = reference_bbox[3] - reference_bbox[1]
    scale = min(
        size[0] * 0.72 / reference_width,
        size[1] * 0.86 / reference_height,
    )
    baseline = round(size[1] * 0.95)
    aligned: list[Image.Image] = []
    for frame in frames:
        rgba = frame.convert("RGBA")
        bbox = rgba.getchannel("A").getbbox()
        canvas = Image.new("RGBA", size, (0, 0, 0, 0))
        if not bbox:
            aligned.append(canvas)
            continue
        content = rgba.crop(bbox)
        content = content.resize(
            (max(1, round(content.width * scale)),
             max(1, round(content.height * scale))),
            Image.Resampling.LANCZOS,
        )
        x = (size[0] - content.width) // 2
        y = max(0, baseline - content.height)
        canvas.alpha_composite(content, (x, y))
        aligned.append(canvas)
    return aligned


def remove_left_bleed_components(image: Image.Image,
                                 limit_x: int = 152,
                                 alpha_threshold: int = 18) -> Image.Image:
    """Remove independent sprite-strip bleed pieces left of the pillar body."""
    rgba = image.convert("RGBA")
    w, h = rgba.size
    pixels = rgba.load()
    alpha = rgba.getchannel("A").load()
    visited = bytearray(w * h)
    for y0 in range(h):
        for x0 in range(limit_x):
            index0 = y0 * w + x0
            if visited[index0] or alpha[x0, y0] <= alpha_threshold:
                continue
            queue: deque[tuple[int, int]] = deque([(x0, y0)])
            visited[index0] = 1
            component: list[tuple[int, int]] = []
            while queue:
                x, y = queue.popleft()
                component.append((x, y))
                for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                    if nx < 0 or nx >= w or ny < 0 or ny >= h:
                        continue
                    index = ny * w + nx
                    if visited[index] or alpha[nx, ny] <= alpha_threshold:
                        continue
                    visited[index] = 1
                    queue.append((nx, ny))

            if not component:
                continue
            max_x = max(p[0] for p in component)
            min_x = min(p[0] for p in component)
            if max_x < limit_x or (min_x < limit_x * 0.75 and max_x < limit_x + 24):
                for x, y in component:
                    pixels[x, y] = (0, 0, 0, 0)
    return rgba


def clear_left_pillar_bleed_strip(image: Image.Image, limit_x: int = 142) -> Image.Image:
    """Clear the narrow left strip where the shatter sprite sheet bleeds into cells."""
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(min(limit_x, rgba.width)):
            pixels[x, y] = (0, 0, 0, 0)
    return rgba


def clear_top_death_bleed_strip(image: Image.Image, limit_y: int = 108) -> Image.Image:
    """Clear the empty top band where generated death sheets keep thin borders."""
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    for y in range(min(limit_y, rgba.height)):
        for x in range(rgba.width):
            pixels[x, y] = (0, 0, 0, 0)
    return rgba


def process_resonance_pillar_assets() -> None:
    """Build four charge states plus a short flash-and-collapse sequence."""
    pillar_folder = EXISTING / "cpp（boss设计——塞壬）" / "cpp（boss设计——塞壬，补）"
    charge_source = pillar_folder / "共鸣柱.png"
    shatter_source = find_generated_source("*1-0.png")
    if not charge_source.exists() or not shatter_source:
        return

    base = Image.open(charge_source).convert("RGBA")
    light_centers = ((544, 746), (544, 838), (544, 929))
    charge_frames: list[Image.Image] = []
    for charge_count in range(4):
        frame = base.copy()
        for light_index, center in enumerate(light_centers):
            enabled = light_index >= 3 - charge_count
            draw_pillar_light(frame, center, enabled)
        charge_frames.append(solid_dark_background(frame))

    strip = Image.open(shatter_source).convert("RGBA")
    shatter_frames: list[Image.Image] = []
    # The strip contains eight images.  Skip its first dark duplicate because
    # the three-light charge frame already supplies the intact starting pose.
    for index in range(1, 8):
        left = round(index * strip.width / 8)
        right = round((index + 1) * strip.width / 8)
        cell = strip.crop((left, 0, right, strip.height))
        cell = trim_cell_lines(cell)
        shatter_frames.append(solid_dark_background(cell))

    # The intact and shatter sources use very different native resolutions.
    # Normalize each group against its own intact pose so the pillar does not
    # suddenly shrink when the flash begins, while later rubble still collapses.
    charge_frames = align_pillar_sequence(charge_frames, charge_frames[0])
    shatter_frames = align_pillar_sequence(shatter_frames, shatter_frames[0])
    shatter_frames = [clear_left_pillar_bleed_strip(frame) for frame in shatter_frames]
    save_sequence("siren/resonance_pillar",
                  charge_frames + shatter_frames,
                  loop=False,
                  prepared=True)


def derive_phantom_stun_from_move() -> None:
    """Keep stunned phantoms on the same silhouette as moving phantoms."""
    source = GENERATED / "siren" / "phantom_move"
    frame_paths = sorted(source.glob("frame_*.png"))
    if not frame_paths:
        return

    frames = []
    for index, path in enumerate(frame_paths):
        base = Image.open(path).convert("RGBA")
        alpha = base.getchannel("A")
        tint_alpha = alpha.point(
            lambda value: min(88, round(value * (0.24 + 0.03 * (index % 3))))
        )
        tint = Image.new("RGBA", base.size, (156, 244, 255, 0))
        tint.putalpha(tint_alpha)

        stunned = Image.alpha_composite(base, tint)
        frames.append(stunned)

    save_sequence("siren/phantom_stun_dissolve", frames, loop=True)


def process_generated() -> None:
    looping = {"five_head_shark/summon_water", "siren/seaweed_zone",
               "siren/immunity_feedback", "siren/elegy_pull"}
    for source, (relative, columns, rows, key) in GENERATED_SHEETS.items():
        if not source.exists():
            continue
        sheet = Image.open(source)
        cell_w = sheet.width // columns
        cell_h = sheet.height // rows
        frames = []
        for row in range(rows):
            for column in range(columns):
                cell = sheet.crop((
                    column * cell_w,
                    row * cell_h,
                    (column + 1) * cell_w,
                    (row + 1) * cell_h,
                ))
                cell = trim_cell_lines(cell)
                frame = chroma_to_alpha(cell, key)
                if relative in {"five_head_shark/death", "siren/death",
                                "siren/seaweed_zone"}:
                    frame = remove_large_white_frame(frame)
                if relative == "five_head_shark/death":
                    frame = erase_canvas_edge_lines(frame)
                    frame = erase_long_border_lines(frame)
                    frame = clear_top_death_bleed_strip(frame)
                frames.append(frame)
        save_sequence(relative, frames, relative in looping)


def process_existing() -> None:
    looping = {
        "five_head_shark/idle",
        "siren/idle",
        "siren/phantom_move",
        "siren/seaweed_zone",
    }
    glow_effects = {
        "five_head_shark/bombardment",
        "siren/soul_song",
        "siren/elegy_wave",
        "siren/elegy_pull",
    }
    for relative, folder, prefix in EXISTING_GROUPS:
        files = sorted(folder.glob(f"{prefix}*.png"), key=numeric_key)
        if not files:
            continue
        converter = glow_dark_background if relative in glow_effects else connected_dark_background
        frames = [converter(Image.open(path)) for path in files]
        if relative == "siren/seaweed_zone":
            frames = [remove_large_white_frame(frame) for frame in frames]
        save_sequence(relative, frames, relative in looping)

    bombardment = EXISTING / "夺命五头鲨" / "轰炸.png"
    if bombardment.exists():
        sheet = Image.open(bombardment)
        frame_width = sheet.width // 8
        frames = [
            glow_dark_background(
                trim_cell_lines(sheet.crop((index * frame_width, 0,
                                             (index + 1) * frame_width, sheet.height)))
            )
            for index in range(8)
        ]
        save_sequence("five_head_shark/bombardment", frames)

    phantom = EXISTING / "cpp（boss设计——塞壬）" / "倩影.png"
    if phantom.exists():
        save_sequence("siren/phantom_idle", [connected_dark_background(Image.open(phantom))])


if __name__ == "__main__":
    process_generated()
    process_existing()
    process_resonance_pillar_assets()
    derive_phantom_stun_from_move()
    repair_directional_beam_alpha()
    rebuild_runtime_sheets()
    print(f"Processed Boss assets under: {GENERATED}")
