#include <filesystem>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "control_mapping_hub_flow.cpp"

namespace {
struct TestCase {
    std::string name;
    std::function<bool()> run;
};

int run_test(const TestCase& test) {
    std::cout << "[browser_flow_native] RUN " << test.name << "\n";
    try {
        if (!test.run()) {
            std::cerr << "[browser_flow_native] FAIL " << test.name
                      << ": scenario returned false\n";
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[browser_flow_native] FAIL " << test.name << ": " << ex.what()
                  << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[browser_flow_native] FAIL " << test.name
                  << ": unknown exception\n";
        return 1;
    }
    std::cout << "[browser_flow_native] PASS " << test.name << "\n";
    return 0;
}
}

int main() {
    const std::vector<TestCase> tests = {
        { "browser_flow_pipeline", [] {
             return browser_flow::RunScenario("tests/artifacts/browser_flow.json");
         } },
        { "midi_mapping_flow", [] {
             return browser_flow::RunMidiMappingFlowScenario(
                 "tests/artifacts/midi_mapping_flow.json");
         } },
        { "slot_dropdown_focus", browser_flow::RunSlotDropdownFocusScenario },
        { "slot_binding_refresh", browser_flow::RunSlotBindingRefreshScenario },
        { "webcam_replay_flow", [] {
             return browser_flow::RunWebcamReplayScenario(
                 "tests/artifacts/webcam_replay_flow.json");
         } },
        { "osc_ingest_flow", [] {
             return browser_flow::RunOscIngestFlowScenario(
                 "tests/artifacts/osc_ingest_flow.json");
         } },
        { "console_slot_hotkeys", browser_flow::RunConsoleSlotHotkeyScenario },
        { "coverage_window_logic", browser_flow::RunCoverageWindowLogicScenario },
        { "console_store_persistence", browser_flow::RunConsoleStorePersistenceScenario },
        { "viewport_persistence", browser_flow::RunViewportPersistenceScenario },
        { "layer_opacity_parameter_row", browser_flow::RunLayerOpacityParameterScenario },
        { "hud_asset_catalog", browser_flow::RunHudAssetPlacementScenario },
        { "hud_inline_picker", browser_flow::RunHudInlinePickerScenario },
        { "hud_feed_telemetry", browser_flow::RunHudFeedTelemetryScenario },
        { "hud_routing_manifest", browser_flow::RunHudRoutingManifestScenario },
        { "dual_screen_phase2", browser_flow::RunDualScreenPhase2Scenario },
    };

    int failures = 0;
    for (const auto& test : tests) {
        failures += run_test(test);
    }
    if (failures != 0) {
        std::cerr << "[browser_flow_native] " << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "[browser_flow_native] all " << tests.size() << " tests passed\n";
    return 0;
}
