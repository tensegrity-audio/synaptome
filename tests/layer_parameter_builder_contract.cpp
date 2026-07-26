#include "../synaptome/src/visuals/LayerParameterBuilder.h"
#include <stdexcept>

namespace layer_parameter_builder_contract {
namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
}

bool RunLayerParameterBuilderScenario() {
    ParameterRegistry registry;
    bool visible = true;
    bool reseed = false;
    float speed = 3.5f;
    float alpha = 0.75f;
    float seed = 8128.0f;

    LayerParameterBuilder parameters(registry, "test.layer", "Generative");
    parameters.visible(&visible);
    parameters.speed(&speed, { 0.0f, 40.0f, 0.1f }, "Time: Field Speed");
    parameters.alpha(&alpha, "Visibility: Field Opacity");
    parameters.seed(&seed);
    parameters.reseed(&reseed);

    const auto* visibleParam = registry.findBool("test.layer.visible");
    const auto* speedParam = registry.findFloat("test.layer.speed");
    const auto* alphaParam = registry.findFloat("test.layer.alpha");
    const auto* seedParam = registry.findFloat("test.layer.seed");
    const auto* reseedParam = registry.findBool("test.layer.reseed");
    require(visibleParam && speedParam && alphaParam && seedParam && reseedParam,
            "common helpers must preserve prefix.suffix parameter IDs");
    require(speedParam->value == &speed && speedParam->defaultValue == 3.5f,
            "builder must preserve live pointer and configured default");
    require(speedParam->meta.group == "Generative" &&
            speedParam->meta.label == "Time: Field Speed",
            "builder must preserve descriptor metadata");
    require(speedParam->meta.range.min == 0.0f &&
            speedParam->meta.range.max == 40.0f &&
            speedParam->meta.range.step == 0.1f,
            "builder must preserve range metadata");
    require(alphaParam->meta.range.min == 0.0f &&
            alphaParam->meta.range.max == 1.0f,
            "alpha helper must use the normalized range");
    require(seedParam->defaultValue == 8128.0f &&
            seedParam->meta.range.step == 1.0f,
            "seed helper must retain integer-like persisted defaults");

    registry.setFloatBase("test.layer.speed", 9.0f, true);
    require(speed == 9.0f,
            "builder parameters must retain normal registry base/live behavior");
    require(registry.snapshotFloatBases().size() == 3 &&
            registry.snapshotBoolBases().size() == 2,
            "builder parameters must retain normal persistence snapshots");
    return true;
}
}
