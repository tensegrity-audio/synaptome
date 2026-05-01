#include <filesystem>
#include <stdexcept>

#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "../../tests/control_mapping_hub_flow.cpp"

namespace {
void test_browser_flow_pipeline() {
    try {
        const std::filesystem::path artifact = "tests/artifacts/browser_flow.json";
        TEST_ASSERT_TRUE(browser_flow::RunScenario(artifact));
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_midi_mapping_flow() {
    try {
        const std::filesystem::path artifact = "tests/artifacts/midi_mapping_flow.json";
        TEST_ASSERT_TRUE(browser_flow::RunMidiMappingFlowScenario(artifact));
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_slot_dropdown_focus() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunSlotDropdownFocusScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_slot_binding_refresh() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunSlotBindingRefreshScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_webcam_replay_flow() {
    try {
        const std::filesystem::path artifact = "tests/artifacts/webcam_replay_flow.json";
        TEST_ASSERT_TRUE(browser_flow::RunWebcamReplayScenario(artifact));
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_osc_ingest_flow() {
    try {
        const std::filesystem::path artifact = "tests/artifacts/osc_ingest_flow.json";
        TEST_ASSERT_TRUE(browser_flow::RunOscIngestFlowScenario(artifact));
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_console_slot_hotkeys() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunConsoleSlotHotkeyScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_coverage_window_logic() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunCoverageWindowLogicScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_console_store_persistence() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunConsoleStorePersistenceScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_viewport_persistence() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunViewportPersistenceScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_layer_opacity_parameter_row() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunLayerOpacityParameterScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_hud_asset_catalog() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunHudAssetPlacementScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_hud_inline_picker() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunHudInlinePickerScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_hud_feed_telemetry() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunHudFeedTelemetryScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_hud_routing_manifest() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunHudRoutingManifestScenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}

void test_dual_screen_phase2() {
    try {
        TEST_ASSERT_TRUE(browser_flow::RunDualScreenPhase2Scenario());
    } catch (const std::exception& ex) {
        TEST_FAIL_MESSAGE(ex.what());
    }
}
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_browser_flow_pipeline);
    RUN_TEST(test_midi_mapping_flow);
    RUN_TEST(test_slot_dropdown_focus);
    RUN_TEST(test_slot_binding_refresh);
    RUN_TEST(test_webcam_replay_flow);
    RUN_TEST(test_osc_ingest_flow);
    RUN_TEST(test_console_slot_hotkeys);
    RUN_TEST(test_coverage_window_logic);
    RUN_TEST(test_console_store_persistence);
    RUN_TEST(test_viewport_persistence);
    RUN_TEST(test_layer_opacity_parameter_row);
    RUN_TEST(test_hud_asset_catalog);
    RUN_TEST(test_hud_inline_picker);
    RUN_TEST(test_hud_feed_telemetry);
    RUN_TEST(test_hud_routing_manifest);
    RUN_TEST(test_dual_screen_phase2);
    return UNITY_END();
}
