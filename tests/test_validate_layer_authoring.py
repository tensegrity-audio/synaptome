import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import validate_layer_authoring as authoring


class LayerAuthoringWorkflowTests(unittest.TestCase):
    def write_profile(self, root: Path, stages: dict) -> Path:
        path = root / "profile.json"
        path.write_text(
            json.dumps({"name": "fixture family", "stages": stages}),
            encoding="utf-8",
        )
        return path

    def test_profile_expands_python_and_preserves_stage_order(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = self.write_profile(
                Path(temp),
                {
                    "fast": [
                        {"label": "one", "command": ["{python}", "-c", "pass"]},
                        {"label": "two", "command": ["tool", "--check"]},
                    ],
                    "native": [{"label": "three", "command": ["native-test"]}],
                },
            )
            profile = authoring.load_profile(path)
            commands = authoring.commands_for(profile, ["fast", "native"])
            self.assertEqual([command.label for command in commands], ["one", "two", "three"])
            self.assertEqual(commands[0].argv[0], sys.executable)

    def test_fail_fast_does_not_run_later_command(self) -> None:
        marker_root = Path(tempfile.mkdtemp())
        marker = marker_root / "should-not-exist"
        commands = [
            authoring.Command(
                "fast", "intentional failure", (sys.executable, "-c", "raise SystemExit(9)")
            ),
            authoring.Command(
                "fast",
                "must be skipped",
                (sys.executable, "-c", f"from pathlib import Path; Path({str(marker)!r}).touch()"),
            ),
        ]
        output = io.StringIO()
        result = authoring.run_commands(commands, output=output)
        self.assertEqual(result, 9)
        self.assertFalse(marker.exists())
        self.assertIn("stopped at first failure", output.getvalue())

    def test_invalid_command_shape_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = self.write_profile(
                Path(temp),
                {"fast": [{"label": "broken", "command": "not-an-array"}]},
            )
            profile = authoring.load_profile(path)
            with self.assertRaisesRegex(ValueError, "non-empty string array"):
                authoring.commands_for(profile, ["fast"])

    def test_profile_discovery_lists_all_migrated_families(self) -> None:
        output = io.StringIO()
        with redirect_stdout(output):
            result = authoring.main(["--list"])
        self.assertEqual(result, 0)
        self.assertEqual(
            set(output.getvalue().splitlines()),
            {
                "adaptive-trail",
                "cellular-fields",
                "circuit-lenia",
                "circuit-trace",
                "collective-motion",
            },
        )

    def test_family_profile_selects_only_its_focused_validator(self) -> None:
        cases = {
            "adaptive-trail": "agentField",
            "collective-motion": "flocking",
        }
        for profile_name, runtime_type in cases.items():
            with self.subTest(profile=profile_name):
                profile = authoring.load_profile(authoring.profile_path(profile_name))
                commands = authoring.commands_for(profile, ["fast", "native"])
                self.assertEqual(len(commands), 1)
                self.assertEqual(commands[0].stage, "fast")
                self.assertEqual(commands[0].argv[-2:], ("--family", runtime_type))

    def test_cellular_profile_uses_its_distinct_runtime_validator(self) -> None:
        profile = authoring.load_profile(authoring.profile_path("cellular-fields"))
        commands = authoring.commands_for(profile, ["fast", "native"])
        self.assertEqual(len(commands), 1)
        self.assertEqual(commands[0].stage, "fast")
        self.assertEqual(commands[0].argv[-1], "tools/validate_cellular_fields.py")


if __name__ == "__main__":
    unittest.main()
