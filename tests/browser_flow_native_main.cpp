#include <filesystem>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "control_mapping_hub_flow.cpp"
#include "circuit_trace_contract.cpp"
#include "layer_parameter_builder_contract.cpp"

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
        { "midi_namespace_cleanup", browser_flow::RunMidiNamespaceCleanupScenario },
        { "parameter_registry_storage_invalidation",
          browser_flow::RunParameterRegistryStorageInvalidationScenario },
        { "builtin_element_host_parameters_without_text_element",
          browser_flow::RunBuiltinElementHostParametersWithoutTextElementScenario },
        { "text_layer_state_transaction",
          browser_flow::RunTextLayerStateTransactionScenario },
        { "element_parameter_registry_contract",
          browser_flow::RunElementParameterRegistryContractScenario },
        { "element_parameter_midi_rebind",
          browser_flow::RunElementParameterMidiRebindScenario },
        { "slot_dropdown_focus", browser_flow::RunSlotDropdownFocusScenario },
        { "slot_binding_refresh", browser_flow::RunSlotBindingRefreshScenario },
        { "slot_assignment_transaction",
          browser_flow::RunSlotAssignmentTransactionScenario },
        { "slot_assignment_ui_rollback",
          browser_flow::RunSlotAssignmentUiRollbackScenario },
        { "midi_input_binding_transaction",
          browser_flow::RunMidiInputBindingTransactionScenario },
        { "devices_panel_midi_binding",
          browser_flow::RunDevicesPanelMidiBindingScenario },
        { "webcam_replay_flow", [] {
             return browser_flow::RunWebcamReplayScenario(
                 "tests/artifacts/webcam_replay_flow.json");
         } },
        { "osc_ingest_flow", [] {
             return browser_flow::RunOscIngestFlowScenario(
                 "tests/artifacts/osc_ingest_flow.json");
         } },
        { "synaptome_mesh_osc_compatibility",
          browser_flow::RunSynaptomeMeshOscCompatibilityScenario },
        { "console_slot_hotkeys", browser_flow::RunConsoleSlotHotkeyScenario },
        { "scene_parameter_persistence", browser_flow::RunSceneParameterPersistenceScenario },
        { "scene_state_document_versions", browser_flow::RunSceneStateDocumentVersionScenario },
        { "scene_slot_assignment_ownership",
          browser_flow::RunSceneSlotAssignmentOwnershipScenario },
        { "machine_profile_document_versions",
          browser_flow::RunMachineProfileDocumentVersionScenario },
        { "mapping_bank_document_versions", browser_flow::RunMappingBankDocumentVersionScenario },
        { "preferences_document_versions", browser_flow::RunPreferencesDocumentVersionScenario },
        { "bank_definitions_document", browser_flow::RunBankDefinitionsDocumentScenario },
        { "mapping_snapshot_round_trip", browser_flow::RunMappingSnapshotRoundTripScenario },
        { "mapping_store_recovery", browser_flow::RunMappingStoreRecoveryScenario },
        { "console_store_persistence", browser_flow::RunConsoleStorePersistenceScenario },
        { "viewport_persistence", browser_flow::RunViewportPersistenceScenario },
        { "collapsed_browser_startup", browser_flow::RunCollapsedBrowserStartupScenario },
        { "layer_opacity_parameter_row", browser_flow::RunLayerOpacityParameterScenario },
        { "hud_asset_catalog", browser_flow::RunHudAssetPlacementScenario },
        { "hud_inline_picker", browser_flow::RunHudInlinePickerScenario },
        { "hud_feed_telemetry", browser_flow::RunHudFeedTelemetryScenario },
        { "hud_routing_manifest", browser_flow::RunHudRoutingManifestScenario },
        { "dual_screen_phase2", browser_flow::RunDualScreenPhase2Scenario },
        { "window_monitor_placement", browser_flow::RunWindowMonitorPlacementScenario },
        { "layer_package_read_only_inspection", browser_flow::RunLayerPackageReadOnlyInspectionScenario },
        { "controlled_package_discovery", browser_flow::RunControlledPackageDiscoveryScenario },
        { "labeled_parameter_selection", browser_flow::RunLabeledParameterSelectionScenario },
        { "layer_package_preset_bank_selection", browser_flow::RunLayerPackagePresetBankSelectionScenario },
        { "package_control_transactions", browser_flow::RunPackageControlTransactionScenario },
        { "opt_in_layer_package_activation", browser_flow::RunOptInLayerPackageActivationScenario },
        { "collapsed_asset_browser_startup", browser_flow::RunCollapsedAssetBrowserStartupScenario },
        { "asset_browser_search", browser_flow::RunAssetBrowserSearchScenario },
        { "focused_layer_edit", browser_flow::RunFocusedLayerEditScenario },
        { "circuit_variant_lifecycle", browser_flow::RunCircuitVariantLifecycleScenario },
        { "circuit_lenia_lifecycle", browser_flow::RunCircuitLeniaLifecycleScenario },
        { "circuit_lenia_osc_defaults", browser_flow::RunCircuitLeniaOscDefaultsScenario },
        { "eight_direction_motion", circuit_trace_contract::RunEightDirectionMotionScenario },
        { "layer_parameter_builder", layer_parameter_builder_contract::RunLayerParameterBuilderScenario },
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
