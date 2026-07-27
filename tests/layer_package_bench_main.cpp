#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "ofFbo.h"
#include "stubs/SynaptomeTestPaths.h"

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647692f
#endif
inline float ofDegToRad(float degrees) { return degrees * 0.01745329251994329577f; }

#define private public
#include "../synaptome/src/visuals/SignalBloomLayer.h"
#undef private
#include "../synaptome/src/runtime/BuiltinElements.h"
#include "../synaptome/src/visuals/LayerFactory.h"

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
}

int main() {
    try {
        LayerFactory& factory = LayerFactory::instance();
        synaptome::runtime::registerBuiltinElements(factory);

        auto layer = factory.create("example.signalBloom");
        require(layer != nullptr, "factory did not create Signal Bloom");
        layer->setRegistryPrefix("bench.signal_bloom");
        layer->setInstanceId("bench-1");

        ofJson config = {
            {"defaults", {
                {"visible", true},
                {"speed", 0.9},
                {"bpmSync", true},
                {"bpmMultiplier", 2.0},
                {"scale", 0.75},
                {"color", {0.2, 0.8, 1.0}},
                {"backgroundColor", {0.01, 0.02, 0.06}}
            }}
        };
        layer->configure(config);

        ParameterRegistry registry;
        layer->setup(registry);
        require(registry.floats().size() == 16, "unexpected float parameter count");
        require(registry.bools().size() == 2, "unexpected bool parameter count");
        require(registry.findFloat("bench.signal_bloom.bpmMultiplier") != nullptr,
                "named BPM multiplier parameter missing");

        const auto packagePath = synaptome_test_paths::appRoot().parent_path() /
            "docs" / "examples" / "layer_packages" / "signal_bloom" / "layer.package.json";
        std::ifstream packageStream(packagePath);
        require(static_cast<bool>(packageStream), "could not read package declaration");
        ofJson package;
        packageStream >> package;
        std::unordered_map<std::string, ofJson> declared;
        for (const auto& parameter : package["parameters"]) {
            declared[parameter.value("id", std::string())] = parameter;
        }
        require(declared.size() == registry.floats().size() + registry.bools().size(),
                "package/runtime parameter counts drifted");
        const std::string prefix = "bench.signal_bloom.";
        for (const auto& parameter : registry.floats()) {
            require(parameter.meta.id.rfind(prefix, 0) == 0,
                    "runtime float escaped the bench registry prefix");
            const std::string suffix = parameter.meta.id.substr(prefix.size());
            const auto it = declared.find(suffix);
            require(it != declared.end(), "runtime float is absent from package: " + suffix);
            require(it->second.value("kind", std::string()) == "float",
                    "package kind drift for: " + suffix);
            const auto range = it->second.value("range", ofJson::object());
            require(std::fabs(range.value("min", 0.0f) - parameter.meta.range.min) < 0.0001f &&
                    std::fabs(range.value("max", 0.0f) - parameter.meta.range.max) < 0.0001f,
                    "package/runtime range drift for: " + suffix);
        }
        for (const auto& parameter : registry.bools()) {
            require(parameter.meta.id.rfind(prefix, 0) == 0,
                    "runtime bool escaped the bench registry prefix");
            const std::string suffix = parameter.meta.id.substr(prefix.size());
            const auto it = declared.find(suffix);
            require(it != declared.end(), "runtime bool is absent from package: " + suffix);
            require(it->second.value("kind", std::string()) == "bool",
                    "package kind drift for: " + suffix);
        }

        // Scene/operator values are applied after package defaults and preset
        // values, so this explicit value must win for the live instance.
        registry.setFloatBase("bench.signal_bloom.speed", 2.5f, true);
        require(std::fabs(registry.getFloatBase("bench.signal_bloom.speed") - 2.5f) < 0.0001f,
                "explicit scene value did not win");

        for (int frame = 0; frame < 240; ++frame) {
            LayerUpdateParams update;
            update.dt = 1.0f / 60.0f;
            update.time = static_cast<float>(frame) * update.dt;
            update.bpm = 128.0f;
            update.speed = 1.0f;
            layer->update(update);
        }

        auto* signalBloom = dynamic_cast<SignalBloomLayer*>(layer.get());
        require(signalBloom != nullptr, "factory returned wrong layer type");
        require(signalBloom->points_.size() == 96, "point field was not initialized");
        require(std::isfinite(signalBloom->phase_) && signalBloom->phase_ > 0.0f,
                "update lifecycle did not advance a finite phase");

        ofFbo fbo;
        fbo.allocate(640, 360);
        require(fbo.isAllocated(), "offscreen framebuffer allocation failed");
        ofCamera camera;
        LayerDrawParams draw{camera};
        draw.viewport = {640, 360};
        draw.slotOpacity = 0.85f;
        fbo.begin();
        layer->draw(draw);
        fbo.end();

        layer->setExternalEnabled(false);
        require(!layer->isEnabled(), "external visibility toggle failed");

        bool duplicateRejected = false;
        try {
            factory.registerType("example.signalBloom", [] {
                return std::make_unique<SignalBloomLayer>();
            });
        } catch (const std::logic_error&) {
            duplicateRejected = true;
        }
        require(duplicateRejected, "duplicate factory registration was not rejected");

        std::cout << "[layer_package_bench] PASS examples.signal_bloom: "
                  << registry.floats().size() + registry.bools().size()
                  << " parameters, 240 updates, offscreen draw\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[layer_package_bench] FAIL: " << e.what() << "\n";
        return 1;
    }
}
