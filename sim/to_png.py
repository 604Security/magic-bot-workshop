#!/usr/bin/env python3
"""Turn the emulator's .ppm screens into PNGs inside a little yellow CYD frame (with the RGB light lit)."""
import glob, os, sys
from PIL import Image, ImageDraw

src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
for ppm in sorted(glob.glob(os.path.join(src, "*.ppm"))):
    name = os.path.splitext(os.path.basename(ppm))[0]
    scr = Image.open(ppm).convert("RGB")
    led = (40, 40, 40)
    try:
        r, g, b = map(int, open(os.path.join(src, name + ".led")).read().split())
        if r or g or b:
            led = (255 * r, 255 * g, 255 * b)
    except OSError:
        pass
    frame = Image.new("RGB", (320 + 52, 240 + 28), (255, 255, 255))
    d = ImageDraw.Draw(frame)
    d.rounded_rectangle((0, 0, frame.width - 1, frame.height - 1), 16, fill=(242, 194, 0), outline=(212, 166, 0), width=3)
    frame.paste(scr, (14, 14))
    d.ellipse((frame.width - 26, frame.height // 2 - 6, frame.width - 14, frame.height // 2 + 6), fill=led)
    frame.save(os.path.join(dst, name + ".png"))
