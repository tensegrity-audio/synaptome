#!/usr/bin/env python3
"""Guard show-critical operator text scaling and half-screen mirror behavior."""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(source: str, fragment: str, label: str, errors: list[str]) -> None:
    if fragment not in source:
        errors.append(f"missing {label}")


def main() -> int:
    errors: list[str] = []
    app = (ROOT / "synaptome/src/ofApp.cpp").read_text(encoding="utf-8")
    mirror = (ROOT / "synaptome/src/visuals/effects/PostEffectChain.cpp").read_text(
        encoding="utf-8"
    )
    menu_skin = (ROOT / "synaptome/src/ui/MenuSkin.h").read_text(encoding="utf-8")
    console = (ROOT / "synaptome/src/ui/ConsoleState.h").read_text(encoding="utf-8")
    control_hub = (ROOT / "synaptome/src/ui/ControlMappingHubState.h").read_text(
        encoding="utf-8"
    )

    require(app, '"App Text Size"', "global App Text Size label", errors)
    for target in (
        "overlayManager.setHudSkin(menuSkin.hud)",
        "consoleState->setMenuSkin(menuSkin)",
        "controlMappingHub->setMenuSkin(menuSkin)",
        "assetBrowser->setMenuSkin(menuSkin)",
        "keyMappingUi->setMenuSkin(menuSkin)",
        "devicesPanel->setMenuSkin(menuSkin)",
        "hudLayoutEditor->setMenuSkin(menuSkin)",
    ):
        require(app, target, f"text-scale propagation: {target}", errors)

    for fragment, label in (
        ('const std::string quitLine = "QUIT: CTRL+Q"', "quit confirmation action"),
        ('const std::string cancelLine = "ESC: CANCEL"', "quit cancellation action"),
        ("drawQuitConfirmationOverlay", "focused-window quit modal"),
        ("promptBase == OF_KEY_ESC", "Escape quit cancellation"),
    ):
        require(app, fragment, label, errors)

    for fragment, label in (
        ("class UiFontRenderer", "shared operator font renderer"),
        ('kFontDataPath = "fonts/unifont-17.0.05.otf"', "GNU Unifont data path"),
        ("ofTrueTypeFontSettings settings", "target-size TrueType rasterization"),
        ("kBasePixelSize * safeScale", "text-scale to pixel-size conversion"),
        ("loadedPixelSize_ == pixelSize", "font-size cache"),
        ("drawBitmapFallback", "built-in bitmap fallback"),
        ("measureUiStringWidth", "shared rendered-width measurement"),
    ):
        require(menu_skin, fragment, label, errors)

    for source, label in (
        (console, "Console"),
        (control_hub, "Control Hub"),
    ):
        require(
            source,
            "measureUiStringWidth(candidate, textScale)",
            f"{label} ellipsis uses rendered font metrics",
            errors,
        )

    font_path = ROOT / "synaptome/bin/data/fonts/unifont-17.0.05.otf"
    expected_font_sha256 = (
        "85701ab9b1e251ee16f4df00b13f22eac311d72b7dab427a7d975fe7f5064702"
    )
    if not font_path.is_file():
        errors.append("missing bundled GNU Unifont 17.0.05")
    elif hashlib.sha256(font_path.read_bytes()).hexdigest() != expected_font_sha256:
        errors.append("bundled GNU Unifont 17.0.05 checksum does not match")

    if not (ROOT / "synaptome/bin/data/fonts/OFL-1.1.txt").is_file():
        errors.append("missing bundled GNU Unifont OFL license")

    require(
        mirror,
        "return vec2(min(uv.x, 1.0 - uv.x), uv.y);",
        "lossless horizontal half-screen symmetry",
        errors,
    )
    require(
        mirror,
        "return vec2(uv.x, min(uv.y, 1.0 - uv.y));",
        "lossless vertical half-screen symmetry",
        errors,
    )
    require(
        mirror,
        "if (mirrorMode <= 1) {\n        fragColor = mix(original, mirrored, amount);\n        return;",
        "basic mirror bypass around artistic detail processing",
        errors,
    )

    ui_root = ROOT / "synaptome/src/ui"
    ui_sources = list(ui_root.rglob("*.cpp")) + list(ui_root.rglob("*.h"))
    for path in ui_sources:
        if path.name == "MenuSkin.h":
            continue
        source = path.read_text(encoding="utf-8", errors="replace")
        if "ofDrawBitmapString(" in source or "ofDrawBitmapStringHighlight(" in source:
            errors.append(
                f"{path.relative_to(ROOT)} bypasses the shared scaled text helpers"
            )

    if errors:
        for error in errors:
            print(f"error: {error}")
        return 1

    print(
        "Operator render contract OK: GNU Unifont is rasterized at the global "
        "UI size with fallback, and basic mirror modes reflect one source half "
        "pixel-for-pixel into the other."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
