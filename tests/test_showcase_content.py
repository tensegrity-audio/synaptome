"""Integration checks for shipped catalog looks, mappings and effect scenes."""
from pathlib import Path
from copy import deepcopy
import json
import sys

import jsonschema
from referencing import Registry, Resource

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import generate_showcase_content
import generate_element_package_registrations


def read(path):
    return json.loads(path.read_text())


def packages():
    return [read(ROOT / "packages/showcase" / name / "layer.package.json")
            for name in generate_showcase_content.NAMES]


def test_shipped_looks_and_mapping_inspection_are_current():
    for path, expected in generate_showcase_content.outputs().items():
        assert path.read_text() == expected, str(path)
    inspection = read(generate_showcase_content.INSPECTION)
    # The pre-existing generated tetrahedron reports runtimeLoadable=true,
    # which predates an update to this draft inspection schema. Its dedicated
    # regression gate owns that baseline. Validate every added show entry here.
    show_inspection = deepcopy(inspection)
    show_inspection["entries"] = [
        entry for entry in inspection["entries"]
        if entry["assetId"].startswith("show.")
    ]
    assert len(show_inspection["entries"]) == 36
    schema_dir = ROOT / "docs/schemas"
    # Resolve the payload's package mapping references from the local contract,
    # with no network access or implicit current-directory resolution.
    registry = Registry().with_resource(
        "layer_package.schema.json",
        Resource.from_contents(read(schema_dir / "layer_package.schema.json")),
    )
    jsonschema.Draft7Validator(
        read(schema_dir / "layer_browser_inspection_payload.schema.json"),
        registry=registry,
    ).validate(show_inspection)


def test_source_packages_are_registered_without_missing_or_duplicate_types():
    registered = {r.type_id: r for r in generate_element_package_registrations.load_records()}
    assert len(packages()) == 12
    assert len({p["asset"]["model"] for p in packages()}) == 12
    for package in packages():
        record = registered[package["element"]["id"]]
        assert record.definition_id == package["asset"]["id"]
        assert record.binding_mode == "bind-only"


def test_every_shipped_value_and_mapping_is_in_the_live_declaration():
    manifest = read(ROOT / "docs/contracts/parameter_manifest.json")
    declared_targets = {p["id"] for p in manifest["parameters"]}
    console_suffixes = {p["suffix"] for p in manifest["consoleSlotTemplates"]}
    for package in packages():
        declarations = {p["id"]: p for p in package["parameters"]}
        assert not ({"visible", "opacity", "alpha", "enabled"} & declarations.keys())
        assert all(p["label"].startswith(p["label"].split(":")[0] + ": ") for p in declarations.values())
        assets = [read(p) for p in generate_showcase_content.ASSETS.glob("*.json")
                  if read(p)["type"] == package["element"]["id"]]
        assert len(assets) == 3
        for asset in assets:
            assert asset["defaults"].keys() == declarations.keys()
            for suffix, value in asset["defaults"].items():
                parameter = declarations[suffix]
                assert asset["registryPrefix"] + "." + suffix in declared_targets
                assert suffix in console_suffixes
                if parameter["kind"] == "float":
                    assert type(value) in (int, float)
                    assert parameter["range"]["min"] <= value <= parameter["range"]["max"]
                elif parameter["kind"] == "bool":
                    assert type(value) is bool
        for preset in package["mappingPresets"]:
            assert preset["applyMode"] == "suggestion-only"
            for mapping in preset["mappings"]:
                assert mapping["target"]["id"] in declarations


def test_audio_profiles_can_be_combined_without_source_profile_conflicts():
    profiles = {}
    for package in packages():
        defaults = {p["id"]: p["default"] for p in package["parameters"]}
        for preset in package["mappingPresets"]:
            for mapping in preset["mappings"]:
                source = mapping["source"]
                key = source["pattern"]
                if key in profiles:
                    assert profiles[key] == source
                profiles[key] = source
                assert source["relative"] is True and source["blend"] == "scale"
                assert 0 < source["out"][0] < 1 < source["out"][1]
                assert defaults[mapping["target"]["id"]] != 0, "Relative scaling of zero has no audible response"


def test_effect_looks_use_real_processors_and_complete_registered_values():
    all_parameters = read(ROOT / "docs/contracts/parameter_manifest.json")["parameters"]
    registered = {p["id"] for p in all_parameters}
    canonical = {read(p)["id"]: read(p) for p in (ROOT / "synaptome/bin/data/layers/fx").glob("*.json")}
    scenes = sorted((ROOT / "synaptome/bin/data/layers/scenes").glob("show-fx-*.json"))
    assert len(scenes) == 6
    for path in scenes:
        scene = read(path)
        assert "mappings" not in scene and "globals" not in scene
        slots = scene["console"]["slots"]
        assert [slot["index"] for slot in slots] == [1, 2]
        effect = slots[1]
        assert effect["assetId"] in canonical
        prefix = canonical[effect["assetId"]]["registryPrefix"]
        expected = {key for key in registered if key.startswith(prefix + ".")}
        assert set(effect["parameters"]) == expected
        assert effect["active"] is True
