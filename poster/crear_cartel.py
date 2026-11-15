"""Genera un cartel A3 (297×420 mm) explicando el biodata."""

from pathlib import Path
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "fondo-biodata.jpg"
OUT_PNG = ROOT / "cartel-biodata-a3.png"
OUT_PDF = ROOT / "cartel-biodata-a3.pdf"
OUT_PREVIEW = ROOT / "preview.jpg"

DPI = 300
W = int(297 / 25.4 * DPI)
H = int(420 / 25.4 * DPI)

FONTS = Path(r"C:\Windows\Fonts")


def font(name: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONTS / name), size)


def wrap(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.ImageFont, max_w: int) -> list[str]:
    words = text.split()
    lines: list[str] = []
    cur: list[str] = []
    for word in words:
        trial = " ".join(cur + [word])
        if draw.textlength(trial, font=fnt) <= max_w:
            cur.append(word)
        else:
            if cur:
                lines.append(" ".join(cur))
            cur = [word]
    if cur:
        lines.append(" ".join(cur))
    return lines


def draw_text_block(
    draw: ImageDraw.ImageDraw,
    xy: tuple[float, float],
    lines: list[str],
    fnt: ImageFont.ImageFont,
    fill,
    line_gap: float = 1.28,
) -> float:
    x, y = xy
    ascent, descent = fnt.getmetrics()
    lh = (ascent + descent) * line_gap
    for line in lines:
        draw.text((x + 2, y + 2), line, font=fnt, fill=(0, 0, 0, 150))
        draw.text((x, y), line, font=fnt, fill=fill)
        y += lh
    return y


def make_background() -> Image.Image:
    base = Image.open(SRC).convert("RGB")
    src_w, src_h = base.size
    scale = max(W / src_w, H / src_h)
    nw, nh = int(src_w * scale), int(src_h * scale)
    base = base.resize((nw, nh), Image.Resampling.LANCZOS)
    left = (nw - W) // 2
    top = max(0, (nh - H) // 2 - int(H * 0.02))
    base = base.crop((left, top, left + W, top + H))
    base = ImageEnhance.Contrast(base).enhance(1.1)
    base = ImageEnhance.Color(base).enhance(1.15)
    return base.convert("RGBA")


def overlays(size: tuple[int, int]) -> Image.Image:
    w, h = size
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)

    # Título: velo suave arriba
    top_h = int(h * 0.28)
    for i in range(top_h):
        t = 1 - i / top_h
        a = int(125 * (t ** 1.7))
        d.rectangle([0, i, w, i + 1], fill=(4, 12, 8, a))

    # Contenido: gradiente inferior (deja ver musgo/dispositivo al inicio)
    band_h = int(h * 0.48)
    for i in range(band_h):
        t = i / band_h
        a = int(210 * (t ** 1.55))
        y = h - band_h + i
        d.rectangle([0, y, w, y + 1], fill=(3, 10, 7, a))

    # Laterales muy suaves
    for i in range(70):
        a = int(40 * (i / 70) ** 1.5)
        d.rectangle([i * 2, 0, i * 2 + 2, h], fill=(4, 12, 8, a))
        d.rectangle([w - i * 2 - 2, 0, w - i * 2, h], fill=(4, 12, 8, a))

    return layer.filter(ImageFilter.GaussianBlur(radius=22))


def main() -> None:
    canvas = make_background()
    canvas = Image.alpha_composite(canvas, overlays((W, H)))
    draw = ImageDraw.Draw(canvas)

    cream = (246, 243, 234, 250)
    mist = (226, 234, 222, 230)
    leaf = (186, 222, 188, 255)
    soft = (208, 218, 206, 220)

    title_f = font("georgia.ttf", 198)
    italic = "georgiai.ttf" if (FONTS / "georgiai.ttf").exists() else "Candarai.ttf"
    tag_f = font(italic, 50)
    h2_f = font("georgiab.ttf", 52)
    body_f = font("segoeui.ttf", 38)
    step_f = font("segoeui.ttf", 38)
    foot_f = font(italic, 38)
    num_f = font("georgiab.ttf", 42)

    # —— Marca ——
    title = "BIODATA"
    tw = draw.textlength(title, font=title_f)
    tx = (W - tw) / 2
    ty = int(H * 0.048)
    draw.text((tx + 3, ty + 3), title, font=title_f, fill=(0, 0, 0, 130))
    draw.text((tx, ty), title, font=title_f, fill=cream)

    line_y = ty + 215
    draw.line([(int(W * 0.36), line_y), (int(W * 0.64), line_y)], fill=leaf, width=3)

    tag = "La voz eléctrica de las plantas"
    tag_w = draw.textlength(tag, font=tag_f)
    draw_text_block(draw, ((W - tag_w) / 2, line_y + 26), [tag], tag_f, mist, 1.15)

    # —— Contenido ——
    margin = int(W * 0.085)
    content_w = W - 2 * margin
    y = float(int(H * 0.545))

    draw_text_block(draw, (margin, y), ["Qué es"], h2_f, leaf, 1.1)
    y += 68
    que_es = (
        "Las plantas tienen una vida eléctrica silenciosa. "
        "El biodata escucha esos pequeños cambios y los convierte en música. "
        "No hace falta ser técnico: solo curiosidad, una hoja y algo que pueda tocar notas."
    )
    y = draw_text_block(draw, (margin, y), wrap(draw, que_es, body_f, content_w), body_f, cream, 1.3)
    y += 42

    draw_text_block(draw, (margin, y), ["Cómo funciona"], h2_f, leaf, 1.1)
    y += 68

    steps = [
        "Se colocan dos electrodos en una hoja o en la corteza.",
        "El aparato mide las diminutas variaciones eléctricas de la planta.",
        "Esas señales se transforman en notas musicales (MIDI).",
        "La música llega a un teléfono, ordenador o sintetizador por Bluetooth, Wi‑Fi o cable.",
    ]
    for i, step in enumerate(steps, 1):
        num = f"{i}."
        draw.text((margin + 2, y + 2), num, font=num_f, fill=(0, 0, 0, 130))
        draw.text((margin, y), num, font=num_f, fill=leaf)
        step_lines = wrap(draw, step, step_f, content_w - 80)
        y = draw_text_block(draw, (margin + 70, y + 2), step_lines, step_f, cream, 1.28)
        y += 18

    y += 22
    cierre = (
        "Con el mando regulas la sensibilidad: más abierto, la planta «habla» más; "
        "más cerrado, solo se oyen los cambios grandes."
    )
    y = draw_text_block(draw, (margin, y), wrap(draw, cierre, body_f, content_w), body_f, soft, 1.28)

    foot = "Tecnología para escuchar a la naturaleza"
    fw = draw.textlength(foot, font=foot_f)
    foot_y = min(y + 40, H - 100)
    draw_text_block(draw, ((W - fw) / 2, foot_y), [foot], foot_f, leaf, 1.15)

    rgb = canvas.convert("RGB")
    rgb.save(OUT_PNG, "PNG", dpi=(DPI, DPI))
    rgb.save(OUT_PDF, "PDF", resolution=DPI)
    rgb.resize((700, int(700 * H / W)), Image.Resampling.LANCZOS).save(
        OUT_PREVIEW, "JPEG", quality=90
    )
    print(f"PNG: {OUT_PNG} ({W}x{H} @ {DPI} dpi)")
    print(f"PDF: {OUT_PDF}")


if __name__ == "__main__":
    main()
