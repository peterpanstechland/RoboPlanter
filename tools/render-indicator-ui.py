"""Convert native firmware framebuffer captures; no independent UI drawing."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
THEMES = ["work-garden", "purple-afternoon", "mint-breeze", "warm-tea",
          "night-shift", "berry-note", "moss-study", "pixel-idle"]


def convert(output: Path) -> None:
    frames = output / "frames"
    manifest = json.loads((frames / "captures.json").read_text(encoding="utf-8"))
    animations = {i: [] for i in range(8)}
    for capture in manifest["captures"]:
        name = capture["name"]
        # Only process the basename emitted by our native renderer.
        if Path(name).name != name or not all(c.isalnum() or c == "-" for c in name):
            raise ValueError("Unexpected capture filename")
        source = frames / f"{name}.ppm"
        with Image.open(source) as raw:
            image = raw.convert("RGB")
        if image.size != (480, 480):
            raise ValueError(f"Unexpected framebuffer dimensions: {name}")
        destination = frames if capture["kind"] == "animation" else output
        image.save(destination / f"{name}.png")
        if capture["kind"] == "animation":
            animations[capture["theme"]].append(image)
        # Remove only the exact intermediate successfully converted above.
        source.unlink()
    for theme, images in animations.items():
        # One common palette avoids frame-to-frame palette color shifts.
        palette = images[0].quantize(colors=256)
        indexed = [im.quantize(palette=palette, dither=Image.Dither.NONE) for im in images]
        indexed[0].save(output / f"theme-{theme}-{THEMES[theme]}.gif", save_all=True,
                        append_images=indexed[1:], duration=manifest["frame_duration_ms"],
                        loop=0, disposal=2, optimize=False)
    # Remove only these superseded generated filenames from the first UI draft.
    for theme, old in enumerate(["forest", "comic", "mint", "ocean", "space", "paper", "nature", "cyber"]):
        obsolete = output / f"theme-{theme}-{old}.gif"
        if obsolete.is_file():
            obsolete.unlink()
    # Contact sheets only arrange captured pixels and labels outside each screen.
    pages = ["face", "today", "info", "photos", "focus", "themes", "controls", "confirm-water", "confirm-print"]
    for page in pages:
        sheet = Image.new("RGB", (480 * 4, 512 * 2), "#d9d9d9")
        labels = ImageDraw.Draw(sheet)
        for theme, title in enumerate(THEMES):
            x, y = theme % 4 * 480, theme // 4 * 512
            with Image.open(output / f"theme-{theme}-{page}.png") as screen:
                sheet.paste(screen, (x, y + 32))
            labels.text((x + 12, y + 8), f"{theme}: {title} / {page}", fill="#222222")
        sheet.save(output / f"all-themes-{page}.png")
    overview = Image.new("RGB", (480 * 3, 512 * 3), "#d9d9d9")
    labels = ImageDraw.Draw(overview)
    for index, page in enumerate(pages):
        x, y = index % 3 * 480, index // 3 * 512
        with Image.open(output / f"theme-0-{page}.png") as screen:
            overview.paste(screen, (x, y + 32))
        labels.text((x + 12, y + 8), f"Default theme / {page}", fill="#222222")
    overview.save(output / "theme-0-overview.png")
    source_files = [ROOT / "firmware/indicator/src" / f for f in
                    ["main.cpp", "ui_theme.cpp", "ui_text.cpp", "ui_photos.cpp"]]
    source_files += [ROOT / "firmware/indicator/include" / f for f in
                     ["ui_glyphs.h", "ui_model.h", "ui_theme.h", "ui_text.h", "ui_photos.h"]]
    gfx = ROOT / "firmware/indicator/.pio/libdeps/sensecap_indicator/GFX Library for Arduino"
    source_files += [gfx / f for f in ["library.properties", "src/Arduino_G.cpp",
                    "src/Arduino_GFX.cpp", "src/canvas/Arduino_Canvas.cpp"]]
    manifest["source_sha256"] = {
        str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in source_files
    }
    manifest["limitations"] = [
        "Actual firmware drawing and pinned Arduino_GFX raster algorithms; host Canvas RGB565 output.",
        "Network and hardware disconnected; sensor and actuator values remain unknown.",
        "Does not emulate LCD timing, PSRAM/DMA, backlight, touch hardware, or on-device performance.",
        "PNG is lossless; GIF uses a 256-color palette. Contact sheet labels are outside screen pixels.",
        "GIF uses 100 ms host-simulated timestamps, not a measured physical-device frame rate.",
        "Confirmation screenshots force the modal open for layout QA while keeping actions disabled.",
    ]
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (output / "REVIEW.txt").write_text(
        "Native firmware pixel captures, disconnected / unknown data.\n"
        "Start with all-themes-face.png, all-themes-today.png, all-themes-focus.png,\n"
        "all-themes-confirm-water.png, all-themes-confirm-print.png, and theme-*.gif.\n"
        "Other all-themes-*.png sheets cover information, photos, theme menu and controls.\n"
        "The individual 480x480 images and frames/ directory are for zoomed inspection only.\n"
        "manifest.json records firmware and rasterizer hashes plus limitations.\n",
        encoding="utf-8",
    )
    print(f"Exported {len(manifest['captures'])} PNG captures, 8 animation GIFs, and {len(pages)} contact sheets.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build/indicator-preview")
    convert(parser.parse_args().output.resolve())
