"""Generate Windows ICO and the concept contact sheet (requires Pillow)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
icons = root / "assets/icons"
image = Image.open(icons / "gbwiz-icon.png").convert("RGBA")
image.save(icons / "gbwiz.ico", sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])
sheet = Image.new("RGB", (1200, 620), "#eef1f5")
draw = ImageDraw.Draw(sheet)
font_path = Path("C:/Windows/Fonts/meiryo.ttc")
font = ImageFont.truetype(str(font_path), 25) if font_path.exists() else ImageFont.load_default()
small = ImageFont.truetype(str(font_path), 18) if font_path.exists() else ImageFont.load_default()
for index, (variant, label) in enumerate((("a", "A  魔法の帽子＋十字キー"), ("b", "B  迷宮の入口"), ("c", "C  携帯ゲーム機"))):
    x = index * 400
    source = Image.open(icons / f"concept-{variant}.png").convert("RGBA")
    large = source.resize((280, 280), Image.Resampling.LANCZOS)
    sheet.paste(large, (x + 60, 38), large)
    draw.text((x + 38, 340), label, fill="#172438", font=font)
    if variant == "a":
        draw.text((x + 38, 381), "旧候補", fill="#526277", font=small)
    for size, offset in ((96, 42), (48, 166), (32, 258)):
        thumbnail = source.resize((size, size), Image.Resampling.LANCZOS)
        sheet.paste(thumbnail, (x + offset, 450), thumbnail)
        draw.text((x + offset, 555), f"{size}px", fill="#526277", font=small)
sheet.save(icons / "concepts.png")
