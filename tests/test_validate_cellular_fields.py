import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import validate_cellular_fields as cellular


class CellularFieldsValidatorTests(unittest.TestCase):
    def test_parameter_parser_tracks_builder_direct_and_helper_registration(self) -> None:
        source = """
            common.visible(&visible);
            common.speed(&speed, {});
            common.alpha(&alpha);
            common.number("seed", &seed, {});
            common.boolean("reseed", &reseed, {});
            registry.addBool(prefix + ".paused", &paused, paused, meta);
            registerFloat(registry, prefix + ".gain", &gain, gain, "Gain", 0, 1, .1);
            addFloat("decay", &decay, "Decay", "Time", 0, 1, .1);
        """
        self.assertEqual(
            cellular.registered_parameters(source),
            {
                "visible": "Bool",
                "speed": "Float",
                "alpha": "Float",
                "seed": "Float",
                "reseed": "Bool",
                "paused": "Bool",
                "gain": "Float",
                "decay": "Float",
            },
        )

    def test_runtime_identities_remain_distinct(self) -> None:
        self.assertEqual(
            {runtime.runtime_type for runtime in cellular.RUNTIMES},
            {"gameOfLife", "excitableMedia"},
        )
        self.assertEqual(
            {runtime.asset_id for runtime in cellular.RUNTIMES},
            {"generative.gameOfLife", "generative.excitableMedia"},
        )


if __name__ == "__main__":
    unittest.main()
