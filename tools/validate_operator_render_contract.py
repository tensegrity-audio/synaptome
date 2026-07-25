#!/usr/bin/env python3
"""Guard show-critical operator text scaling and lossless mirror behavior."""

from __future__ import annotations

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

    require(
        mirror,
        "return vec2(1.0 - uv.x, uv.y);",
        "lossless horizontal full-frame flip",
        errors,
    )
    require(
        mirror,
        "return vec2(uv.x, 1.0 - uv.y);",
        "lossless vertical full-frame flip",
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
        "Operator render contract OK: global UI text scaling is propagated and "
        "basic mirror modes preserve full-frame RGBA."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
