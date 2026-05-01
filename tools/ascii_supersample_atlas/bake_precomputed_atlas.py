#!/usr/bin/env python3
"""
Bake precomputed ASCII supersample atlases + descriptor LUTs.

The script mirrors the legacy runtime atlas builder so Release builds can ship
known-good glyph masks. It renders the printable ASCII range into a grid,
computes the 2x3 descriptor taps, validates descriptor uniqueness, and writes:

- atlas.png  : RGBA glyph atlas ready for runtime sampling
- atlas.lut.json : descriptor data + metadata for PostEffectChain
- manifest.json (one level up) : index of all prebaked atlases
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import math
import sys
from pathlib import Path
from typing import Dict, List, Sequence, Tuple

from PIL import Image, ImageDraw, ImageFont

ASCII_FIRST = 32
ASCII_COUNT = 95
CELL_WIDTH = 48
CELL_HEIGHT = 64
COLUMNS = 16
ROWS = math.ceil(ASCII_COUNT / COLUMNS)
DESCRIPTOR_COLS = 2
DESCRIPTOR_ROWS = 3
DESCRIPTOR_TAPS = DESCRIPTOR_COLS * DESCRIPTOR_ROWS


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Bake prebaked ASCII supersample atlases."
    )
    default_data = Path("synaptome") / "bin" / "data"
    parser.add_argument(
        "--data-root",
        type=Path,
        default=default_data,
        help="Root of the openFrameworks bin/data directory (default: %(default)s)",
    )
    parser.add_argument(
        "--fonts-dir",
        type=Path,
        help="Directory containing source fonts (defaults to <data-root>/fonts)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help=(
            "Directory to place prebaked atlases (defaults to "
            "<data-root>/fonts/ascii_supersample/prebaked)"
        ),
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        help=(
            "Path to manifest.json (defaults to "
            "<data-root>/fonts/ascii_supersample/manifest.json)"
        ),
    )
    parser.add_argument(
        "--font",
        action="append",
        dest="fonts",
        default=[],
        help="Font filename(s) to bake (defaults to all .ttf/.otf in fonts dir)",
    )
    parser.add_argument(
        "--font-size",
        type=int,
        default=48,
        help="Font size used when rendering glyphs (default: %(default)s)",
    )
    return parser.parse_args()


def sanitize_id(stem: str) -> str:
    sanitized = "".join(ch if ch.isalnum() else "_" for ch in stem)
    sanitized = sanitized.strip("_")
    return sanitized or "font"


def relpath(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.resolve().as_posix()


def sha256_for_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def render_glyph_image(
    font: ImageFont.FreeTypeFont, glyph: str, size: Tuple[int, int]
) -> Image.Image:
    width, height = size
    glyph_img = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(glyph_img)
    ascent, descent = font.getmetrics()
    line_height = ascent + descent
    margin_top = max(0.0, (height - line_height) * 0.5)
    baseline_y = margin_top + ascent
    try:
        bbox = font.getbbox(glyph, anchor="ls")
    except TypeError:
        bbox = font.getbbox(glyph, anchor="lt")
    if bbox:
        glyph_width = max(1.0, bbox[2] - bbox[0])
        offset_x = (width - glyph_width) * 0.5 - bbox[0]
    else:
        glyph_width = width * 0.3
        offset_x = (width - glyph_width) * 0.5
    anchor = "ls"
    try:
        draw.text((offset_x, baseline_y), glyph, font=font, fill=255, anchor=anchor)
    except TypeError:
        draw.text((offset_x, baseline_y - ascent), glyph, font=font, fill=255)
    return glyph_img


def sample_descriptor(pixels: Image.Image) -> List[float]:
    width, height = pixels.size
    data = pixels.load()
    samples: List[float] = []
    for row in range(DESCRIPTOR_ROWS):
        for col in range(DESCRIPTOR_COLS):
            x0 = int(col * width / DESCRIPTOR_COLS)
            y0 = int(row * height / DESCRIPTOR_ROWS)
            x1 = int((col + 1) * width / DESCRIPTOR_COLS)
            y1 = int((row + 1) * height / DESCRIPTOR_ROWS)
            x1 = min(x1, width)
            y1 = min(y1, height)
            total = 0.0
            count = 0
            for y in range(y0, y1):
                for x in range(x0, x1):
                    total += data[x, y]
                    count += 1
            samples.append(total / (count * 255.0) if count else 0.0)
    return samples


def summarize_uniqueness(descriptors: Sequence[Sequence[float]]) -> Dict[str, object]:
    buckets: Dict[Tuple[float, ...], List[int]] = {}
    for glyph_index, values in enumerate(descriptors):
        key = tuple(round(v, 6) for v in values)
        buckets.setdefault(key, []).append(glyph_index)
    duplicate_groups = []
    for descriptor_key, glyph_indices in buckets.items():
        if len(glyph_indices) > 1:
            duplicate_groups.append(
                {
                    "glyph_indices": glyph_indices,
                    "glyphs": [
                        chr(ASCII_FIRST + idx) for idx in glyph_indices
                    ],
                    "descriptor": list(descriptor_key),
                }
            )
    return {
        "unique_descriptor_vectors": len(buckets),
        "duplicate_groups": duplicate_groups,
    }


def bake_font(
    font_path: Path,
    output_root: Path,
    data_root: Path,
    font_size: int,
) -> Dict[str, object]:
    label = font_path.stem
    font_id = sanitize_id(label)
    layout_engine = getattr(ImageFont, "LAYOUT_BASIC", None)
    font_kwargs = {"size": font_size}
    if layout_engine is not None:
        font_kwargs["layout_engine"] = layout_engine
    font = ImageFont.truetype(str(font_path), **font_kwargs)
    atlas_width = COLUMNS * CELL_WIDTH
    atlas_height = ROWS * CELL_HEIGHT
    atlas_image = Image.new("RGBA", (atlas_width, atlas_height), (0, 0, 0, 0))
    descriptors: List[List[float]] = []
    descriptor_means: List[float] = []
    rects: List[List[float]] = []

    for glyph_index in range(ASCII_COUNT):
        glyph = chr(ASCII_FIRST + glyph_index)
        glyph_l = render_glyph_image(font, glyph, (CELL_WIDTH, CELL_HEIGHT))
        desc = sample_descriptor(glyph_l)
        descriptors.append(desc)
        descriptor_means.append(sum(desc) / len(desc))
        atlas_col = glyph_index % COLUMNS
        atlas_row = glyph_index // COLUMNS
        u0 = (atlas_col * CELL_WIDTH) / atlas_width
        v0 = (atlas_row * CELL_HEIGHT) / atlas_height
        u1 = ((atlas_col + 1) * CELL_WIDTH) / atlas_width
        v1 = ((atlas_row + 1) * CELL_HEIGHT) / atlas_height
        rects.append([u0, v0, u1, v1])
        glyph_rgba = Image.merge("RGBA", (glyph_l, glyph_l, glyph_l, glyph_l))
        atlas_image.paste(
            glyph_rgba,
            (atlas_col * CELL_WIDTH, atlas_row * CELL_HEIGHT),
        )

    uniqueness = summarize_uniqueness(descriptors)
    timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat()
    font_output_dir = output_root / font_id
    font_output_dir.mkdir(parents=True, exist_ok=True)
    atlas_path = font_output_dir / "atlas.png"
    lut_path = font_output_dir / "atlas.lut.json"
    atlas_image.save(atlas_path)

    lut_doc = {
        "version": 1,
        "generated_at": timestamp,
        "font": {
            "label": label,
            "id": font_id,
            "source": font_path.name,
        },
        "glyphs": {
            "first": ASCII_FIRST,
            "count": ASCII_COUNT,
            "codes": [ASCII_FIRST + i for i in range(ASCII_COUNT)],
        },
        "atlas": {
            "cell_size": [CELL_WIDTH, CELL_HEIGHT],
            "columns": COLUMNS,
            "rows": ROWS,
            "image_size": [atlas_width, atlas_height],
        },
        "descriptor_grid": {"columns": DESCRIPTOR_COLS, "rows": DESCRIPTOR_ROWS},
        "rects": rects,
        "descriptors": descriptors,
        "descriptor_means": descriptor_means,
        "glyph_uniqueness": uniqueness,
    }
    lut_path.write_text(json.dumps(lut_doc, indent=2))

    manifest_entry = {
        "id": font_id,
        "label": label,
        "source": relpath(font_path, data_root),
        "glyph_count": ASCII_COUNT,
        "descriptor_taps": DESCRIPTOR_TAPS,
        "descriptor_grid": {"columns": DESCRIPTOR_COLS, "rows": DESCRIPTOR_ROWS},
        "atlas_cell": [CELL_WIDTH, CELL_HEIGHT],
        "atlas_grid": {"columns": COLUMNS, "rows": ROWS},
        "atlas": relpath(atlas_path, data_root),
        "lut": relpath(lut_path, data_root),
        "checksums": {
            "atlas_sha256": sha256_for_file(atlas_path),
            "lut_sha256": sha256_for_file(lut_path),
        },
        "glyph_uniqueness": uniqueness,
        "generated_at": timestamp,
    }
    return {
        "manifest_entry": manifest_entry,
        "atlas_path": atlas_path,
        "lut_path": lut_path,
    }


def main() -> None:
    args = parse_args()
    data_root = args.data_root.resolve()
    fonts_dir = (args.fonts_dir or (data_root / "fonts")).resolve()
    output_dir = (
        args.output_dir
        or (data_root / "fonts" / "ascii_supersample" / "prebaked")
    ).resolve()
    manifest_path = (
        args.manifest
        or (data_root / "fonts" / "ascii_supersample" / "manifest.json")
    ).resolve()

    if not fonts_dir.exists():
        print(f"Font directory '{fonts_dir}' not found.", file=sys.stderr)
        sys.exit(1)

    if args.fonts:
        font_candidates = [fonts_dir / Path(name) for name in args.fonts]
    else:
        font_candidates = list(fonts_dir.glob("*.ttf")) + list(fonts_dir.glob("*.otf"))

    if not font_candidates:
        print("No fonts found to bake.", file=sys.stderr)
        sys.exit(1)

    manifest_fonts = []
    for font_path in font_candidates:
        if not font_path.exists():
            print(f"Skipping missing font {font_path}", file=sys.stderr)
            continue
        baked = bake_font(
            font_path,
            output_dir,
            data_root,
            font_size=args.font_size,
        )
        uniqueness = baked["manifest_entry"]["glyph_uniqueness"]
        unique = uniqueness["unique_descriptor_vectors"]
        duplicates = len(uniqueness["duplicate_groups"])
        print(
            f"[ok] {font_path.name}: "
            f"{unique} unique descriptor vectors "
            f"({duplicates} duplicate groups)"
        )
        manifest_fonts.append(baked["manifest_entry"])

    if not manifest_fonts:
        print("No atlases baked successfully.", file=sys.stderr)
        sys.exit(1)

    manifest_doc = {
        "version": 1,
        "generated_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "fonts": manifest_fonts,
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest_doc, indent=2))
    print(f"Wrote manifest: {manifest_path}")


if __name__ == "__main__":
    main()
