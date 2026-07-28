#include "MachineProfileDocument.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <set>
#include <string>
#include <utility>

namespace synaptome::state {
namespace {

MachineProfileDocumentResult machineProfileFailure(
    std::string error,
    MachineProfileDocumentError code =
        MachineProfileDocumentError::InvalidDocument) {
    MachineProfileDocumentResult result;
    result.errorCode = code;
    result.error = std::move(error);
    return result;
}

bool hasOnlyKeys(
    const ofJson& object,
    std::initializer_list<const char*> allowed) {
    std::set<std::string> allowedKeys;
    for (const char* key : allowed) {
        allowedKeys.emplace(key);
    }
    for (const auto& item : object.items()) {
        if (allowedKeys.find(item.key()) == allowedKeys.end()) {
            return false;
        }
    }
    return true;
}

bool isStableId(const std::string& value) {
    if (value.empty() || value.size() > 127) {
        return false;
    }
    const auto validFirst = [](unsigned char character) {
        return std::isalnum(character) != 0;
    };
    const auto validRest = [](unsigned char character) {
        return std::isalnum(character) != 0 ||
            character == '.' ||
            character == '_' ||
            character == '-';
    };
    if (!validFirst(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    for (unsigned char character : value) {
        if (!validRest(character)) {
            return false;
        }
    }
    return true;
}

bool isAssignmentKey(const std::string& value) {
    if (value.empty() ||
        value.size() > kMaxMachineProfileAssignmentKeyLength) {
        return false;
    }
    const auto isStableIdCharacter = [](unsigned char character) {
        return std::isalnum(character) != 0 ||
            character == '.' ||
            character == '_' ||
            character == '-';
    };
    bool segmentStart = true;
    bool separatorSeen = false;
    for (std::size_t index = 0; index < value.size(); ++index) {
        const unsigned char character =
            static_cast<unsigned char>(value[index]);
        if (segmentStart) {
            if (std::isalnum(character) == 0) {
                return false;
            }
            segmentStart = false;
            continue;
        }
        if (isStableIdCharacter(character)) {
            continue;
        }
        if (character == ':' &&
            !separatorSeen &&
            index + 1 < value.size() &&
            value[index + 1] == ':') {
            index += 1;
            segmentStart = true;
            separatorSeen = true;
            continue;
        }
        return false;
    }
    return !segmentStart;
}

bool readBoundedBindingId(
    const ofJson& object,
    const char* key,
    const std::string& path,
    std::string& destination,
    std::string& error) {
    if (!object.contains(key) || !object[key].is_string()) {
        error = path + "." + key + " must be a string";
        return false;
    }
    destination = object[key].get<std::string>();
    if (destination.empty() ||
        destination.size() > kMaxMachineProfileBindingIdLength ||
        std::none_of(
            destination.begin(),
            destination.end(),
            [](unsigned char character) {
                return std::isspace(character) == 0;
            })) {
        error =
            path + "." + key +
            " must be a nonblank string no longer than " +
            std::to_string(kMaxMachineProfileBindingIdLength) +
            " characters";
        return false;
    }
    return true;
}

bool readStableId(
    const ofJson& object,
    const char* key,
    const std::string& path,
    std::string& destination,
    std::string& error) {
    if (!object.contains(key) || !object[key].is_string()) {
        error = path + "." + key + " must be a stable ID string";
        return false;
    }
    destination = object[key].get<std::string>();
    if (!isStableId(destination)) {
        error =
            path + "." + key +
            " must begin with an alphanumeric character and contain only "
            "alphanumerics, '.', '_', or '-'";
        return false;
    }
    return true;
}

bool validateControlSlotAssignment(
    const ofJson& assignment,
    const std::string& path,
    std::string& assignmentKey,
    std::string& error) {
    if (!assignment.is_object() ||
        !hasOnlyKeys(
            assignment,
            {
                "assignmentKey",
                "deviceProfileId",
                "slotId",
                "analog",
            })) {
        error =
            path +
            " must contain only assignmentKey, deviceProfileId, slotId, "
            "and analog";
        return false;
    }
    if (!assignment.contains("assignmentKey") ||
        !assignment["assignmentKey"].is_string()) {
        error = path + ".assignmentKey must be a string";
        return false;
    }
    assignmentKey = assignment["assignmentKey"].get<std::string>();
    if (!isAssignmentKey(assignmentKey)) {
        error =
            path +
            ".assignmentKey must be a stable parameter ID or canonical "
            "assetId::parameterSuffix key";
        return false;
    }

    std::string ignored;
    if (!readBoundedBindingId(
            assignment,
            "deviceProfileId",
            path,
            ignored,
            error) ||
        !readBoundedBindingId(
            assignment,
            "slotId",
            path,
            ignored,
            error)) {
        return false;
    }
    if (!assignment.contains("analog") ||
        !assignment["analog"].is_boolean()) {
        error = path + ".analog must be a boolean";
        return false;
    }
    return true;
}

bool validateMidiInput(
    const ofJson& input,
    const std::string& path,
    std::string& error) {
    if (!input.is_object() ||
        !hasOnlyKeys(
            input,
            {"deviceProfileId", "portName"})) {
        error =
            path +
            " must contain only deviceProfileId and portName";
        return false;
    }
    std::string ignored;
    return readBoundedBindingId(
               input,
               "deviceProfileId",
               path,
               ignored,
               error) &&
        readBoundedBindingId(
               input,
               "portName",
               path,
               ignored,
               error);
}

bool validateUdp(
    const ofJson& udp,
    const std::string& path,
    std::string& error) {
    if (!udp.is_object() ||
        !hasOnlyKeys(udp, {"host", "port"})) {
        error = path + " must be an object containing only host and port";
        return false;
    }
    if (!udp.contains("host") ||
        !udp["host"].is_string() ||
        udp["host"].get<std::string>().empty()) {
        error = path + ".host must be a non-empty string";
        return false;
    }
    if (!udp.contains("port") ||
        !udp["port"].is_number_integer()) {
        error = path + ".port must be an integer";
        return false;
    }
    const int port = udp["port"].get<int>();
    if (port < 1 || port > 65535) {
        error = path + ".port must be between 1 and 65535";
        return false;
    }
    return true;
}

bool validateSerial(
    const ofJson& serial,
    const std::string& path,
    std::string& error) {
    if (!serial.is_object() ||
        !hasOnlyKeys(
            serial,
            {"autoPort", "baud", "port", "portNameContains"})) {
        error =
            path +
            " must contain only autoPort, baud, port, and portNameContains";
        return false;
    }
    if (!serial.contains("autoPort") ||
        !serial["autoPort"].is_boolean()) {
        error = path + ".autoPort must be a boolean";
        return false;
    }
    if (!serial.contains("baud") ||
        !serial["baud"].is_number_integer()) {
        error = path + ".baud must be an integer";
        return false;
    }
    const int baud = serial["baud"].get<int>();
    if (baud < 1200 || baud > 4000000) {
        error = path + ".baud must be between 1200 and 4000000";
        return false;
    }
    for (const char* key : {"port", "portNameContains"}) {
        if (serial.contains(key) &&
            (!serial[key].is_string() ||
             serial[key].get<std::string>().empty())) {
            error = path + "." + key + " must be a non-empty string";
            return false;
        }
    }
    const bool autoPort = serial["autoPort"].get<bool>();
    if (!autoPort &&
        !serial.contains("port") &&
        !serial.contains("portNameContains")) {
        error =
            path +
            " must provide port or portNameContains when autoPort is false";
        return false;
    }
    return true;
}

bool validateEndpoint(
    const ofJson& endpoint,
    const std::string& path,
    std::string& endpointId,
    std::string& error) {
    if (!endpoint.is_object() ||
        !hasOnlyKeys(
            endpoint,
            {"id", "enabled", "transport", "udp", "serial"})) {
        error =
            path +
            " must contain only id, enabled, transport, udp, and serial";
        return false;
    }
    if (!readStableId(
            endpoint,
            "id",
            path,
            endpointId,
            error)) {
        return false;
    }
    if (!endpoint.contains("enabled") ||
        !endpoint["enabled"].is_boolean()) {
        error = path + ".enabled must be a boolean";
        return false;
    }
    if (!endpoint.contains("transport") ||
        !endpoint["transport"].is_string()) {
        error = path + ".transport must be \"udp\" or \"serial\"";
        return false;
    }
    const std::string transport =
        endpoint["transport"].get<std::string>();
    if (transport == "udp") {
        if (!endpoint.contains("udp") ||
            endpoint.contains("serial")) {
            error =
                path +
                " with UDP transport must contain udp and must not contain "
                "serial";
            return false;
        }
        return validateUdp(endpoint["udp"], path + ".udp", error);
    }
    if (transport == "serial") {
        if (!endpoint.contains("serial") ||
            endpoint.contains("udp")) {
            error =
                path +
                " with serial transport must contain serial and must not "
                "contain udp";
            return false;
        }
        return validateSerial(
            endpoint["serial"],
            path + ".serial",
            error);
    }
    error = path + ".transport must be \"udp\" or \"serial\"";
    return false;
}

} // namespace

MachineProfileDocumentResult validateMachineProfileDocument(
    const ofJson& source) {
    if (!source.is_object()) {
        return machineProfileFailure(
            "machine-profile document must be an object");
    }

    if (!source.contains("schemaVersion")) {
        return machineProfileFailure(
            "machine-profile schemaVersion is required");
    }
    if (!source["schemaVersion"].is_number_integer()) {
        return machineProfileFailure(
            "machine-profile schemaVersion must be an integer");
    }

    MachineProfileDocumentResult result;
    result.sourceVersion = source["schemaVersion"].get<int>();
    if (result.sourceVersion >
        kCurrentMachineProfileSchemaVersion) {
        return machineProfileFailure(
            "unsupported future machine-profile schemaVersion " +
                std::to_string(result.sourceVersion),
            MachineProfileDocumentError::UnsupportedFutureVersion);
    }
    if (result.sourceVersion !=
        kCurrentMachineProfileSchemaVersion) {
        return machineProfileFailure(
            "machine-profile schemaVersion must be exactly 1");
    }
    if (!hasOnlyKeys(
            source,
            {
                "schemaVersion",
                "profileId",
                "osc",
                "midi",
                "controlSlots",
            })) {
        return machineProfileFailure(
            "machine-profile document contains an unknown root field");
    }

    std::string profileId;
    std::string error;
    if (!readStableId(
            source,
            "profileId",
            "$",
            profileId,
            error)) {
        return machineProfileFailure(std::move(error));
    }
    if (!source.contains("osc") ||
        !source["osc"].is_object()) {
        return machineProfileFailure(
            "$.osc must be an object");
    }
    const auto& osc = source["osc"];
    if (!hasOnlyKeys(
            osc,
            {"inputs", "activeInputId", "outputs"})) {
        return machineProfileFailure(
            "$.osc contains an unknown field");
    }
    if (!osc.contains("inputs") ||
        !osc["inputs"].is_array()) {
        return machineProfileFailure(
            "$.osc.inputs must be an array");
    }
    if (osc.contains("outputs") &&
        !osc["outputs"].is_array()) {
        return machineProfileFailure(
            "$.osc.outputs must be an array");
    }

    std::set<std::string> allEndpointIds;
    std::set<std::string> enabledInputIds;
    std::size_t index = 0;
    for (const auto& endpoint : osc["inputs"]) {
        std::string endpointId;
        if (!validateEndpoint(
                endpoint,
                "$.osc.inputs[" +
                    std::to_string(index) + "]",
                endpointId,
                error)) {
            return machineProfileFailure(std::move(error));
        }
        if (!allEndpointIds.insert(endpointId).second) {
            return machineProfileFailure(
                "duplicate OSC endpoint id " + endpointId);
        }
        if (endpoint["enabled"].get<bool>()) {
            enabledInputIds.insert(endpointId);
        }
        ++index;
    }

    if (osc.contains("outputs")) {
        index = 0;
        for (const auto& endpoint : osc["outputs"]) {
            std::string endpointId;
            if (!validateEndpoint(
                    endpoint,
                    "$.osc.outputs[" +
                        std::to_string(index) + "]",
                    endpointId,
                    error)) {
                return machineProfileFailure(std::move(error));
            }
            if (!allEndpointIds.insert(endpointId).second) {
                return machineProfileFailure(
                    "duplicate OSC endpoint id " + endpointId);
            }
            ++index;
        }
    }

    if (osc.contains("activeInputId")) {
        if (!osc["activeInputId"].is_string()) {
            return machineProfileFailure(
                "$.osc.activeInputId must be a stable ID string");
        }
        const std::string activeInputId =
            osc["activeInputId"].get<std::string>();
        if (!isStableId(activeInputId)) {
            return machineProfileFailure(
                "$.osc.activeInputId must be a stable ID string");
        }
        if (enabledInputIds.find(activeInputId) ==
            enabledInputIds.end()) {
            return machineProfileFailure(
                "$.osc.activeInputId must reference an enabled input");
        }
    } else if (!enabledInputIds.empty()) {
        return machineProfileFailure(
            "$.osc.activeInputId is required when an input is enabled");
    }

    if (source.contains("midi")) {
        const auto& midi = source["midi"];
        if (!midi.is_object() ||
            !hasOnlyKeys(midi, {"inputs"})) {
            return machineProfileFailure(
                "$.midi must contain only inputs");
        }
        if (!midi.contains("inputs") ||
            !midi["inputs"].is_array()) {
            return machineProfileFailure(
                "$.midi.inputs must be an array");
        }
        if (midi["inputs"].size() > 1) {
            return machineProfileFailure(
                "$.midi.inputs must contain at most one input in v1");
        }
        if (!midi["inputs"].empty() &&
            !validateMidiInput(
                midi["inputs"][0],
                "$.midi.inputs[0]",
                error)) {
            return machineProfileFailure(std::move(error));
        }
    }

    if (source.contains("controlSlots")) {
        const auto& controlSlots = source["controlSlots"];
        if (!controlSlots.is_object() ||
            !hasOnlyKeys(controlSlots, {"assignments"})) {
            return machineProfileFailure(
                "$.controlSlots must contain only assignments");
        }
        if (!controlSlots.contains("assignments") ||
            !controlSlots["assignments"].is_array()) {
            return machineProfileFailure(
                "$.controlSlots.assignments must be an array");
        }

        std::set<std::string> assignmentKeys;
        index = 0;
        for (const auto& assignment :
             controlSlots["assignments"]) {
            std::string assignmentKey;
            if (!validateControlSlotAssignment(
                    assignment,
                    "$.controlSlots.assignments[" +
                        std::to_string(index) + "]",
                    assignmentKey,
                    error)) {
                return machineProfileFailure(std::move(error));
            }
            if (!assignmentKeys.insert(assignmentKey).second) {
                return machineProfileFailure(
                    "duplicate control-slot assignmentKey " +
                    assignmentKey);
            }
            ++index;
        }
    }

    result.ok = true;
    result.kind = MachineProfileDocumentKind::CurrentV1;
    result.errorCode = MachineProfileDocumentError::None;
    result.document = source;
    return result;
}

} // namespace synaptome::state
