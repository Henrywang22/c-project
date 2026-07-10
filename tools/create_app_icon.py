from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
WATER = ROOT / "assets" / "water" / "sea_background_refined.png"
TITLE = ROOT / "ui图" / "生成素材" / "主菜单" / "title_board.png"
ANCHOR = ROOT / "ui图" / "生成素材" / "主菜单" / "icon_anchor.png"
PNG_OUT = ROOT / "assets" / "app_icon.png"
ICO_OUT = ROOT / "assets" / "app_icon.ico"


def cover_square(image: Image.Image, size: int) -> Image.Image:
    image = image.convert("RGBA")
    source_w, source_h = image.size
    scale = max(size / source_w, size / source_h)
    scaled = image.resize(
        (round(source_w * scale), round(source_h * scale)), Image.Resampling.LANCZOS
    )
    left = (scaled.width - size) // 2
    top = (scaled.height - size) // 2
    return scaled.crop((left, top, left + size, top + size))


def main() -> None:
    canvas = cover_square(Image.open(WATER), 256)
    draw = ImageDraw.Draw(canvas, "RGBA")

    # 深色航海边框和金色内圈，让小尺寸下仍有清晰的游戏图标轮廓。
    draw.rounded_rectangle((3, 3, 252, 252), radius=36, fill=(42, 24, 9, 238), outline=(230, 171, 61, 255), width=5)
    draw.rounded_rectangle((12, 12, 243, 243), radius=29, outline=(102, 57, 19, 235), width=5)
    draw.ellipse((53, 76, 203, 226), fill=(17, 70, 101, 218), outline=(244, 191, 72, 230), width=4)
    draw.ellipse((61, 84, 195, 218), outline=(108, 187, 213, 135), width=2)

    anchor = Image.open(ANCHOR).convert("RGBA").resize((108, 108), Image.Resampling.NEAREST)
    shadow = Image.new("RGBA", anchor.size, (0, 0, 0, 0))
    shadow.putalpha(anchor.getchannel("A").point(lambda value: value * 150 // 255))
    canvas.alpha_composite(shadow, (77, 108))
    canvas.alpha_composite(anchor, (74, 103))

    title = Image.open(TITLE).convert("RGBA").resize((242, 66), Image.Resampling.LANCZOS)
    canvas.alpha_composite(title, (7, 10))

    # 右下角的金色铆钉与左下角海浪，呼应游戏的木质码头界面。
    draw = ImageDraw.Draw(canvas, "RGBA")
    for x, y in ((24, 229), (232, 229), (24, 73), (232, 73)):
        draw.ellipse((x - 5, y - 5, x + 5, y + 5), fill=(77, 42, 13, 255), outline=(251, 207, 104, 255), width=2)
    for offset in (0, 12, 24):
        draw.arc((75 + offset, 198, 151 + offset, 232), 195, 340, fill=(116, 212, 232, 170), width=3)

    PNG_OUT.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(PNG_OUT)
    canvas.save(ICO_OUT, sizes=[(16, 16), (20, 20), (24, 24), (32, 32), (40, 40), (48, 48), (64, 64), (128, 128), (256, 256)])


if __name__ == "__main__":
    main()
