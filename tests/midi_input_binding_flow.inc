bool RunMidiInputBindingTransactionScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const auto fixturePath =
        synaptome_test_paths::appRoot().parent_path() /
        "tools" / "testdata" / "machine_profile" /
        "midi_binding_cases.json";
    const ofJson fixture = ofLoadJson(fixturePath.string());
    require(
        fixture.is_object() &&
            fixture.value("policyVersion", 0) == 1 &&
            fixture.contains("resolutionCases") &&
            fixture["resolutionCases"].is_array() &&
            fixture.contains("reconnectCase") &&
            fixture["reconnectCase"].is_object() &&
            fixture.contains("legacyRouteFiltering") &&
            fixture["legacyRouteFiltering"].is_object(),
        "Malformed physical-MIDI runtime fixture");

    const auto exactCase = std::find_if(
        fixture["acceptedDocuments"].begin(),
        fixture["acceptedDocuments"].end(),
        [](const ofJson& candidate) {
            return candidate.is_object() &&
                candidate.value("id", std::string()) ==
                    "midi-exact-single-input";
        });
    require(
        exactCase != fixture["acceptedDocuments"].end() &&
            (*exactCase)["document"].contains("midi"),
        "Physical-MIDI fixture lacks its canonical exact binding");
    const ofJson canonicalBinding =
        (*exactCase)["document"]["midi"];
    const ofJson canonicalBindingBefore = canonicalBinding;

    MidiRouter router;
    float compatibleValue = 0.0f;
    float activeProfileValue = 0.0f;
    float otherProfileValue = 0.0f;
    router.bindFloat(
        "tests.midi.compatible",
        &compatibleValue,
        0.0f,
        1.0f);
    router.bindFloat(
        "tests.midi.active-profile",
        &activeProfileValue,
        0.0f,
        1.0f);
    router.bindFloat(
        "tests.midi.other-profile",
        &otherProfileValue,
        0.0f,
        1.0f);
    const ofJson routeSnapshot = {
        {"schemaVersion", 1},
        {"cc",
         ofJson::array({
             {
                 {"num", 41},
                 {"target", "tests.midi.compatible"},
             },
             {
                 {"num", 41},
                 {"target", "tests.midi.active-profile"},
                 {"device", "MIDI Mix 0"},
             },
             {
                 {"num", 41},
                 {"target", "tests.midi.other-profile"},
                 {"device", "Other Controller"},
             },
         })},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array()},
        {"osc", ofJson::array()},
    };
    require(
        router.importMappingSnapshot(routeSnapshot, true),
        "Could not seed physical-MIDI route filtering fixture");

    ControlMappingHubState hub;
    hub.slotAssignmentsLoaded_ = true;
    ControlMappingHubState::LogicalSlotBinding logical;
    logical.deviceId = "MIDI Mix 0";
    logical.deviceName = "MIDI Mix 0";
    logical.slotId = "K1";
    logical.slotLabel = "Knob 1";
    logical.analog = true;
    hub.slotAssignments_["tests.midi::compatible"] =
        std::move(logical);
    const ofJson logicalAssignmentsBefore =
        hub.exportSlotAssignmentSnapshot();
    const ofJson routesBefore =
        router.exportMappingSnapshot();

    for (const auto& resolutionCase :
         fixture["resolutionCases"]) {
        require(
            resolutionCase.is_object() &&
                resolutionCase.contains("id") &&
                resolutionCase["id"].is_string() &&
                resolutionCase.contains("availablePorts") &&
                resolutionCase["availablePorts"].is_array() &&
                resolutionCase.contains("expectedState") &&
                resolutionCase["expectedState"].is_string(),
            "Malformed physical-MIDI resolution case");
        std::vector<std::string> ports;
        for (const auto& port :
             resolutionCase["availablePorts"]) {
            require(
                port.is_string(),
                "Physical-MIDI available port must be a string");
            ports.push_back(port.get<std::string>());
        }
        router.setTestPortList(ports);
        const auto status = router.resolveInputBinding(
            "MIDI Mix 0",
            "MIDI Mix 0");
        const std::string expected =
            resolutionCase["expectedState"].
                get<std::string>();
        const std::string caseId =
            resolutionCase["id"].get<std::string>();
        if (expected == "resolved") {
            require(
                status.authoritative &&
                    status.configured &&
                    status.resolved &&
                    !status.ambiguous &&
                    status.exactMatchCount == 1 &&
                    status.portName == "MIDI Mix 0",
                "Unique exact physical-MIDI match did not resolve: " +
                    caseId);
        } else if (expected == "ambiguous") {
            require(
                status.authoritative &&
                    status.configured &&
                    !status.resolved &&
                    status.ambiguous &&
                    status.exactMatchCount > 1,
                "Duplicate exact physical-MIDI names were not "
                "ambiguous: " +
                    caseId);
        } else {
            require(
                status.authoritative &&
                    status.configured &&
                    !status.resolved &&
                    !status.ambiguous &&
                    status.exactMatchCount == 0,
                "Missing or renamed physical-MIDI port resolved "
                "through a forbidden fallback: " +
                    caseId);
        }
        require(
            hub.exportSlotAssignmentSnapshot() ==
                    logicalAssignmentsBefore &&
                router.exportMappingSnapshot() ==
                    routesBefore,
            "Physical-MIDI resolution changed logical assignments or "
            "routes: " +
                caseId);
    }

    const auto& reconnect = fixture["reconnectCase"];
    std::vector<std::string> initialPorts;
    for (const auto& port : reconnect["initialPorts"]) {
        initialPorts.push_back(port.get<std::string>());
    }
    router.setTestPortList(initialPorts);
    const auto adoptedMissing =
        router.adoptInputBindingSnapshot(canonicalBinding);
    require(
        adoptedMissing.ok &&
            adoptedMissing.status.authoritative &&
            adoptedMissing.status.configured &&
            !adoptedMissing.status.resolved &&
            router.exportInputBindingSnapshot() ==
                canonicalBinding &&
            canonicalBinding == canonicalBindingBefore,
        "Unplugged canonical MIDI binding was not retained unresolved "
        "or mutated its source");

    std::vector<std::string> reconnectPorts;
    for (const auto& port : reconnect["reconnectPorts"]) {
        reconnectPorts.push_back(port.get<std::string>());
    }
    router.setTestPortList(reconnectPorts);
    const auto reconnectedStatus =
        router.inputBindingStatus();
    require(
        reconnectedStatus.authoritative &&
            reconnectedStatus.configured &&
            reconnectedStatus.resolved &&
            reconnectedStatus.exactMatchCount == 1 &&
            reconnectedStatus.portName ==
                reconnect.value(
                    "expectedPortName",
                    std::string()),
        "Canonical physical-MIDI binding did not resolve after exact "
        "device reconnect");
    require(
        hub.exportSlotAssignmentSnapshot() ==
                logicalAssignmentsBefore &&
            router.exportMappingSnapshot() == routesBefore,
        "Unplugged/reconnect transition changed assignments or routes");

    compatibleValue = 0.0f;
    activeProfileValue = 0.0f;
    otherProfileValue = 0.0f;
    ofxMidiMessage message;
    message.status = MIDI_CONTROL_CHANGE;
    message.channel = 1;
    message.control = 41;
    message.value = 127;
    router.newMidiMessage(message);
    require(
        compatibleValue > 0.99f &&
            activeProfileValue > 0.99f &&
            otherProfileValue < 0.01f,
        "Active physical-MIDI profile did not retain compatible "
        "unscoped routes while filtering another device profile");

    const ofJson bindingBeforeInvalid =
        router.exportInputBindingSnapshot();
    const auto statusBeforeInvalid =
        router.inputBindingStatus();
    const ofJson routesBeforeInvalid =
        router.exportMappingSnapshot();
    const ofJson slotsBeforeInvalid =
        hub.exportSlotAssignmentSnapshot();
    int persistenceCalls = 0;
    for (const auto& invalidCase :
         fixture["invalidDocuments"]) {
        const ofJson& document = invalidCase["document"];
        require(
            document.contains("midi"),
            "Invalid physical-MIDI runtime fixture omitted midi");
        const ofJson source = document["midi"];
        const ofJson sourceBefore = source;
        const auto rejected =
            router.publishInputBindingSnapshot(
                source,
                [&](const ofJson&) {
                    ++persistenceCalls;
                    return true;
                });
        require(
            !rejected.ok &&
                !rejected.error.empty() &&
                persistenceCalls == 0,
            "Malformed or duplicate physical-MIDI binding reached "
            "persistence: " +
                invalidCase.value("id", std::string()));
        require(
            source == sourceBefore &&
                router.exportInputBindingSnapshot() ==
                    bindingBeforeInvalid &&
                router.exportMappingSnapshot() ==
                    routesBeforeInvalid &&
                hub.exportSlotAssignmentSnapshot() ==
                    slotsBeforeInvalid,
            "Rejected physical-MIDI binding mutated source, binding, "
            "assignments, or routes");
    }
    const auto statusAfterInvalid =
        router.inputBindingStatus();
    require(
        statusAfterInvalid.authoritative ==
                statusBeforeInvalid.authoritative &&
            statusAfterInvalid.configured ==
                statusBeforeInvalid.configured &&
            statusAfterInvalid.resolved ==
                statusBeforeInvalid.resolved &&
            statusAfterInvalid.ambiguous ==
                statusBeforeInvalid.ambiguous &&
            statusAfterInvalid.deviceProfileId ==
                statusBeforeInvalid.deviceProfileId &&
            statusAfterInvalid.portName ==
                statusBeforeInvalid.portName,
        "Rejected physical-MIDI binding changed adopted status");

    const ofJson replacementBinding = {
        {"inputs",
         ofJson::array({
             {
                 {"deviceProfileId", "Other Controller"},
                 {"portName", "Other Controller"},
             },
         })},
    };
    router.setTestPortList(
        {"MIDI Mix 0", "Other Controller"});
    persistenceCalls = 0;
    const ofJson replacementBefore =
        replacementBinding;
    const auto persistenceRejected =
        router.publishInputBindingSnapshot(
            replacementBinding,
            [&](const ofJson& published) {
                ++persistenceCalls;
                require(
                    published == replacementBinding,
                    "Physical-MIDI persistence received a "
                    "non-canonical snapshot");
                return false;
            });
    require(
        !persistenceRejected.ok &&
            persistenceRejected.rollbackSucceeded &&
            !persistenceRejected.error.empty() &&
            persistenceCalls == 1 &&
            replacementBinding == replacementBefore,
        "Physical-MIDI persistence rejection did not report one "
        "source-immutable rollback");
    require(
        router.exportInputBindingSnapshot() ==
                bindingBeforeInvalid &&
            router.exportMappingSnapshot() ==
                routesBeforeInvalid &&
            hub.exportSlotAssignmentSnapshot() ==
                slotsBeforeInvalid,
        "Physical-MIDI persistence failure did not restore binding, "
        "routes, or logical assignments");

    const ofJson explicitlyEmpty = {
        {"inputs", ofJson::array()},
    };
    const ofJson routesBeforeClear =
        router.exportMappingSnapshot();
    const ofJson slotsBeforeClear =
        hub.exportSlotAssignmentSnapshot();
    const auto cleared =
        router.adoptInputBindingSnapshot(explicitlyEmpty);
    const auto clearedStatus =
        router.inputBindingStatus();
    require(
        cleared.ok &&
            clearedStatus.authoritative &&
            !clearedStatus.configured &&
            !clearedStatus.resolved &&
            router.exportInputBindingSnapshot() ==
                explicitlyEmpty,
        "Explicit-empty physical-MIDI binding did not disable the "
        "canonical input");
    require(
        router.exportMappingSnapshot() == routesBeforeClear &&
            hub.exportSlotAssignmentSnapshot() ==
                slotsBeforeClear,
        "Explicit-empty physical-MIDI binding cleared routes or "
        "logical assignments");

    return true;
}

bool RunDevicesPanelMidiBindingScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const std::string selectedPort =
        "Synthetic Controller Port 7";
    const std::string otherPort = "Unmapped Surface";
    const std::string selectedProfile =
        "synthetic-two-bank-controller";
    const ofJson expectedSelection = {
        {"inputs",
         ofJson::array({
             {
                 {"deviceProfileId", selectedProfile},
                 {"portName", selectedPort},
             },
         })},
    };

    MidiRouter router;
    router.setTestPortList({selectedPort, otherPort});

    DevicesPanel panel;
    panel.setMidiRouter(&router);
    panel.setDeviceMapsDirectory(
        (synaptome_test_paths::appRoot().parent_path() /
         "tools" / "testdata" / "device_maps")
            .string());
    MenuController controller;
    panel.onEnter(controller);

    auto view = panel.view();
    auto entryIndexContaining =
        [&](const std::string& text) {
            for (std::size_t i = 0;
                 i < view.entries.size();
                 ++i) {
                const auto& entry = view.entries[i];
                if (entry.id.find(text) !=
                        std::string::npos ||
                    entry.label.find(text) !=
                        std::string::npos) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        };
    auto selectEntry = [&](int targetIndex) {
        require(
            targetIndex >= 0 &&
                targetIndex <
                    static_cast<int>(view.entries.size()) &&
                view.entries[
                    static_cast<std::size_t>(targetIndex)]
                    .selectable,
            "DevicesPanel MIDI binding target was not selectable");
        int guard = 0;
        while (view.selectedIndex != targetIndex &&
               guard++ <
                   static_cast<int>(view.entries.size()) + 1) {
            require(
                panel.handleInput(controller, OF_KEY_DOWN),
                "DevicesPanel did not handle device-list navigation");
            view = panel.view();
        }
        require(
            view.selectedIndex == targetIndex,
            "DevicesPanel could not select the requested device row");
    };

    const int otherIndex =
        entryIndexContaining(otherPort);
    require(
        otherIndex >= 0,
        "DevicesPanel did not list the unmatched online MIDI port");
    selectEntry(otherIndex);
    require(
        panel.handleInput(controller, OF_KEY_RETURN),
        "DevicesPanel could not activate the non-target device row");
    require(
        panel.handleInput(controller, OF_KEY_ESC),
        "DevicesPanel could not return to its device list");

    view = panel.view();
    const int selectedIndex =
        entryIndexContaining(selectedProfile);
    require(
        selectedIndex >= 0,
        "DevicesPanel did not list the mapped online MIDI profile");
    selectEntry(selectedIndex);

    int preflightPersistenceCalls = 0;
    router.setInputBindingPersistenceCallback(
        [&](const ofJson&) {
            ++preflightPersistenceCalls;
            return true;
        });
    const ofJson bindingBeforePreflight =
        router.exportInputBindingSnapshot();

    router.setTestPortList({otherPort});
    panel.markRosterDirty();
    view = panel.view();
    selectEntry(entryIndexContaining(selectedProfile));
    require(
        panel.handleInput(controller, 'A'),
        "DevicesPanel did not handle unavailable-port bind preflight");
    require(
        preflightPersistenceCalls == 0 &&
            router.exportInputBindingSnapshot() ==
                bindingBeforePreflight,
        "DevicesPanel unavailable-port preflight reached persistence "
        "or changed the binding");

    router.setTestPortList(
        {selectedPort, selectedPort, otherPort});
    panel.markRosterDirty();
    view = panel.view();
    selectEntry(entryIndexContaining(selectedProfile));
    require(
        panel.handleInput(controller, 'A'),
        "DevicesPanel did not handle ambiguous-port bind preflight");
    const auto ambiguous =
        router.resolveInputBinding(
            selectedProfile,
            selectedPort);
    require(
        ambiguous.ambiguous &&
            !ambiguous.resolved &&
            preflightPersistenceCalls == 0 &&
            router.exportInputBindingSnapshot() ==
                bindingBeforePreflight,
        "DevicesPanel ambiguous exact-port preflight reached "
        "persistence or changed the binding");

    router.setTestPortList({selectedPort, otherPort});
    panel.markRosterDirty();
    view = panel.view();
    selectEntry(entryIndexContaining(selectedProfile));

    int persistenceCalls = 0;
    ofJson published = ofJson();
    router.setInputBindingPersistenceCallback(
        [&](const ofJson& snapshot) {
            ++persistenceCalls;
            published = snapshot;
            return true;
        });
    require(
        panel.handleInput(controller, 'A'),
        "DevicesPanel did not handle explicit MIDI bind");
    const auto selectedStatus =
        router.inputBindingStatus();
    require(
        persistenceCalls == 1 &&
            published == expectedSelection &&
            router.exportInputBindingSnapshot() ==
                expectedSelection,
        "DevicesPanel explicit bind did not publish exactly once "
        "with the selected row's canonical profile and exact "
        "displayed port");
    require(
        selectedStatus.authoritative &&
            selectedStatus.configured &&
            selectedStatus.resolved &&
            selectedStatus.deviceProfileId ==
                selectedProfile &&
            selectedStatus.portName == selectedPort,
        "DevicesPanel explicit bind did not adopt the selected exact "
        "MIDI input");

    const ofJson priorBinding = {
        {"inputs",
         ofJson::array({
             {
                 {"deviceProfileId", "prior-profile"},
                 {"portName", "Prior Exact Port"},
             },
         })},
    };
    router.setTestPortList(
        {"Prior Exact Port", selectedPort, otherPort});
    const auto priorAdopted =
        router.adoptInputBindingSnapshot(priorBinding);
    require(
        priorAdopted.ok &&
            router.exportInputBindingSnapshot() ==
                priorBinding,
        "Could not seed the prior MIDI binding for UI rollback");

    panel.markRosterDirty();
    view = panel.view();
    const int reboundIndex =
        entryIndexContaining(selectedProfile);
    selectEntry(reboundIndex);

    int rejectedCalls = 0;
    ofJson rejectedSnapshot = ofJson();
    router.setInputBindingPersistenceCallback(
        [&](const ofJson& snapshot) {
            ++rejectedCalls;
            rejectedSnapshot = snapshot;
            return false;
        });
    require(
        panel.handleInput(controller, 'a'),
        "DevicesPanel did not handle rejected explicit MIDI rebind");
    const auto rolledBackStatus =
        router.inputBindingStatus();
    require(
        rejectedCalls == 1 &&
            rejectedSnapshot == expectedSelection,
        "DevicesPanel rejected rebind did not publish the selected "
        "canonical snapshot exactly once");
    require(
        router.exportInputBindingSnapshot() ==
                priorBinding &&
            rolledBackStatus.authoritative &&
            rolledBackStatus.configured &&
            rolledBackStatus.resolved &&
            rolledBackStatus.deviceProfileId ==
                "prior-profile" &&
            rolledBackStatus.portName ==
                "Prior Exact Port",
        "DevicesPanel persistence rejection did not restore the "
        "prior physical MIDI binding");

    panel.onExit(controller);
    return true;
}
