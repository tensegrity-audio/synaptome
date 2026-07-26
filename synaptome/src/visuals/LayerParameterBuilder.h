#pragma once

#include "../core/ParameterRegistry.h"
#include <string>
#include <utility>

// Thin, state-free registration helper for Layer implementations. It only
// builds descriptors and delegates to ParameterRegistry, so persistence,
// mapping IDs, base/live value semantics, and duplicate detection remain
// exactly where they were. visible()/alpha() exist to migrate established
// layer-owned IDs; new whole-layer visibility/opacity normally belongs to the
// Console slot contract rather than being duplicated inside every Layer.
class LayerParameterBuilder {
public:
    struct FloatOptions {
        std::string label;
        std::string group;
        ParameterRegistry::Range range;
        std::string units;
        std::string description;
        bool quickAccess = false;
        int quickAccessOrder = 0;
    };

    struct BoolOptions {
        std::string label;
        std::string group;
        std::string description;
    };

    LayerParameterBuilder(ParameterRegistry& registry,
                          std::string prefix,
                          std::string defaultGroup = {})
        : registry_(registry)
        , prefix_(std::move(prefix))
        , defaultGroup_(std::move(defaultGroup)) {
    }

    ParameterRegistry::FloatParam& number(const std::string& suffix,
                                           float* value,
                                           FloatOptions options) {
        ParameterRegistry::Descriptor meta = floatDescriptor(std::move(options));
        return registry_.addFloat(id(suffix), value, *value, meta);
    }

    ParameterRegistry::BoolParam& boolean(const std::string& suffix,
                                          bool* value,
                                          BoolOptions options) {
        ParameterRegistry::Descriptor meta = boolDescriptor(std::move(options));
        return registry_.addBool(id(suffix), value, *value, meta);
    }

    ParameterRegistry::Descriptor floatDescriptor(FloatOptions options) const {
        ParameterRegistry::Descriptor meta;
        meta.label = std::move(options.label);
        meta.group = resolvedGroup(options.group);
        meta.range = options.range;
        meta.units = std::move(options.units);
        meta.description = std::move(options.description);
        meta.quickAccess = options.quickAccess;
        meta.quickAccessOrder = options.quickAccessOrder;
        return meta;
    }

    ParameterRegistry::Descriptor boolDescriptor(BoolOptions options) const {
        ParameterRegistry::Descriptor meta;
        meta.label = std::move(options.label);
        meta.group = resolvedGroup(options.group);
        meta.description = std::move(options.description);
        return meta;
    }

    ParameterRegistry::BoolParam& visible(
        bool* value,
        std::string label = "Action: Visible",
        std::string group = {}) {
        return boolean("visible", value, { std::move(label), std::move(group), {} });
    }

    ParameterRegistry::FloatParam& speed(
        float* value,
        ParameterRegistry::Range range,
        std::string label = "Time: Speed",
        std::string group = {},
        bool quickAccess = false,
        int quickAccessOrder = 0) {
        return number("speed", value,
                      { std::move(label), std::move(group), range, {}, {},
                        quickAccess, quickAccessOrder });
    }

    ParameterRegistry::FloatParam& alpha(
        float* value,
        std::string label = "Visibility: Opacity",
        std::string group = {}) {
        return number("alpha", value,
                      { std::move(label), std::move(group), { 0.0f, 1.0f, 0.01f } });
    }

    ParameterRegistry::FloatParam& seed(
        float* value,
        ParameterRegistry::Range range = { 1.0f, 999999.0f, 1.0f },
        std::string label = "Seed: Deterministic Seed",
        std::string group = {}) {
        return number("seed", value,
                      { std::move(label), std::move(group), range, {},
                        "The same seed and parameters reproduce the same state" });
    }

    ParameterRegistry::BoolParam& reseed(
        bool* value,
        std::string label = "Action: Reseed",
        std::string group = {}) {
        return boolean("reseed", value,
                       { std::move(label), std::move(group),
                         "Momentary action; the stored seed is unchanged" });
    }

    std::string id(const std::string& suffix) const {
        if (prefix_.empty()) {
            return suffix;
        }
        if (suffix.empty()) {
            return prefix_;
        }
        return prefix_ + "." + suffix;
    }

private:
    std::string resolvedGroup(const std::string& group) const {
        return group.empty() ? defaultGroup_ : group;
    }

    ParameterRegistry& registry_;
    std::string prefix_;
    std::string defaultGroup_;
};
