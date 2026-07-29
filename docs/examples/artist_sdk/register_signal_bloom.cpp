#include "SignalBloomLayer.h"
#include "../../../synaptome/src/visuals/LayerFactory.h"

#include <synaptome/element/Parameter.h>

#include <memory>

namespace {

using synaptome::element::ElementKind;
using synaptome::element::ElementTypeContract;
using synaptome::element::ParameterDeclaration;
using synaptome::element::ParameterDeprecation;
using synaptome::element::ParameterGroupDeclaration;
using synaptome::element::ParameterKind;
using synaptome::element::ParameterOption;
using synaptome::element::ParameterOptionSource;

ParameterDeclaration boolParameter(
    const char* id,
    const char* groupId,
    const char* label,
    bool defaultValue,
    const char* description) {
    ParameterDeclaration parameter;
    parameter.id = id;
    parameter.kind = ParameterKind::Bool;
    parameter.groupId = groupId;
    parameter.label = label;
    parameter.defaultValue = defaultValue;
    parameter.description = description;
    return parameter;
}

ParameterDeclaration floatParameter(
    const char* id,
    const char* groupId,
    const char* label,
    float defaultValue,
    float minimum,
    float maximum,
    float step,
    const char* units,
    const char* description) {
    ParameterDeclaration parameter;
    parameter.id = id;
    parameter.kind = ParameterKind::Float;
    parameter.groupId = groupId;
    parameter.label = label;
    parameter.defaultValue = defaultValue;
    parameter.range = synaptome::element::ParameterRange{
        minimum,
        maximum,
        step,
    };
    parameter.units = units;
    parameter.description = description;
    return parameter;
}

ElementTypeContract signalBloomTypeContract() {
    ElementTypeContract contract;
    contract.element = {
        "example.signalBloom",
        ElementKind::Visual,
        {},
    };
    contract.parameters.groups = {
        ParameterGroupDeclaration{"example", "Example", ""},
        ParameterGroupDeclaration{
            "exampleMotion",
            "Example Motion",
            "",
        },
        ParameterGroupDeclaration{
            "exampleTransform",
            "Example Transform",
            "",
        },
        ParameterGroupDeclaration{
            "exampleColor",
            "Example Color",
            "",
        },
        ParameterGroupDeclaration{
            "exampleModulation",
            "Example Modulation",
            "",
        },
    };

    auto visible = boolParameter(
        "visible",
        "example",
        "Visible",
        true,
        "Whether the layer contributes to the current slot.");
    auto speed = floatParameter(
        "speed",
        "exampleMotion",
        "Speed",
        0.65f,
        0.0f,
        4.0f,
        0.01f,
        "multiplier",
        "Local signal motion speed.");
    auto bpmSync = boolParameter(
        "bpmSync",
        "exampleMotion",
        "BPM Sync",
        true,
        "Whether local motion follows the global transport BPM.");
    auto bpmMultiplier = floatParameter(
        "bpmMultiplier",
        "exampleMotion",
        "BPM Multiplier",
        1.0f,
        0.25f,
        8.0f,
        0.25f,
        "multiplier",
        "Multiplier applied to global transport BPM when sync is enabled. "
        "Browser inspection resolves the app-owned provider to labeled "
        "multiplier choices.");
    bpmMultiplier.optionSource = ParameterOptionSource{
        "transport.bpmMultipliers",
        "multiplier",
        "label",
    };

    auto scale = floatParameter(
        "scale",
        "exampleTransform",
        "Size",
        0.82f,
        0.1f,
        2.0f,
        0.01f,
        "multiplier",
        "Layer-local visual scale.");
    scale.options = {
        ParameterOption{
            0.5f,
            "Compact",
            "Reduce the layer footprint.",
        },
        ParameterOption{
            0.82f,
            "Default",
            "Use the reviewed package scale.",
        },
        ParameterOption{
            1.0f,
            "Full",
            "Use the full normalized layer scale.",
        },
    };

    auto rotation = floatParameter(
        "rotationDeg",
        "exampleTransform",
        "Rotation",
        0.0f,
        -180.0f,
        180.0f,
        1.0f,
        "deg",
        "Layer-local rotation in degrees.");
    auto alpha = floatParameter(
        "alpha",
        "exampleColor",
        "Alpha",
        0.86f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Legacy layer-local alpha exposed by the current SDK example.");
    alpha.deprecation = ParameterDeprecation{
        "opacity",
        "New layers should prefer opacity/visibility naming, but this "
        "fixture preserves the current public SDK parameter.",
    };
    auto gain = floatParameter(
        "gain",
        "exampleModulation",
        "Sensor Gain",
        0.62f,
        0.0f,
        2.0f,
        0.01f,
        "multiplier",
        "Gain applied to generic input modulation.");
    auto lineOpacity = floatParameter(
        "lineOpacity",
        "exampleColor",
        "Line Opacity",
        0.72f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Opacity for the signal line stroke.");
    auto colorR = floatParameter(
        "colorR",
        "exampleColor",
        "Red",
        0.12f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Primary color red channel.");
    auto colorG = floatParameter(
        "colorG",
        "exampleColor",
        "Green",
        0.78f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Primary color green channel.");
    auto colorB = floatParameter(
        "colorB",
        "exampleColor",
        "Blue",
        1.0f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Primary color blue channel.");
    auto backgroundR = floatParameter(
        "bgColorR",
        "exampleColor",
        "Background Red",
        0.02f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Background color red channel.");
    auto backgroundG = floatParameter(
        "bgColorG",
        "exampleColor",
        "Background Green",
        0.04f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Background color green channel.");
    auto backgroundB = floatParameter(
        "bgColorB",
        "exampleColor",
        "Background Blue",
        0.08f,
        0.0f,
        1.0f,
        0.01f,
        "",
        "Background color blue channel.");
    auto xInput = floatParameter(
        "xInput",
        "exampleModulation",
        "X",
        0.5f,
        0.0f,
        1.0f,
        0.001f,
        "",
        "Generic horizontal modulation input.");
    auto yInput = floatParameter(
        "yInput",
        "exampleModulation",
        "Y",
        0.5f,
        0.0f,
        1.0f,
        0.001f,
        "",
        "Generic vertical modulation input.");
    auto speedInput = floatParameter(
        "speedInput",
        "exampleModulation",
        "Speed",
        0.0f,
        0.0f,
        1.0f,
        0.001f,
        "",
        "Generic motion modulation input.");

    contract.parameters.parameters = {
        visible,
        speed,
        bpmSync,
        bpmMultiplier,
        scale,
        rotation,
        alpha,
        gain,
        lineOpacity,
        colorR,
        colorG,
        colorB,
        backgroundR,
        backgroundG,
        backgroundB,
        xInput,
        yInput,
        speedInput,
    };
    return contract;
}

} // namespace

void synaptomeRegisterElementPackage_examples_signal_bloom(
    LayerFactory& factory) {
    factory.registerType(
        signalBloomTypeContract(),
        []() {
            return std::make_unique<SignalBloomLayer>();
        });
}
