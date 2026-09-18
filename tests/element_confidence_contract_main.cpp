#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include "../synaptome/src/runtime/Runtime.h"
#include "../synaptome/src/runtime/BuiltinElementParameterContracts.h"
#include "../synaptome/src/visuals/LayerFactory.h"
#include "element_confidence/GraphicsStateGuard.h"
#include "ofAppGLFWWindow.h"

#define private public
#if defined(SYNAPTOME_CONFIDENCE_GRID)
#include "../synaptome/src/visuals/GridLayer.h"
#elif defined(SYNAPTOME_CONFIDENCE_STL)
#include "../synaptome/src/visuals/StlModelLayer.h"
#elif defined(SYNAPTOME_CONFIDENCE_LENIA)
#include "../synaptome/src/visuals/LeniaLayer.h"
#elif defined(SYNAPTOME_CONFIDENCE_SIGNAL_BLOOM)
#include "../docs/examples/layer_packages/signal_bloom/source/SignalBloomLayer.h"
#include "../synaptome/src/runtime/GeneratedElementPackageRegistrations.h"
#else
#error An element confidence fixture must be selected.
#endif
#undef private

namespace {

using synaptome::element::ElementDescriptor;
using synaptome::element::ElementKind;
using synaptome::element::ElementTypeContract;
using synaptome::element::ParameterDeclaration;
using synaptome::element::ParameterKind;
using synaptome::runtime::CompositionAssignment;
using synaptome::runtime::Runtime;
using synaptome::tests::element_confidence::GraphicsStateGuard;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Fixture {
    std::string typeId;
    std::string profileId;
    std::string bindingMode;
    int updateFrames = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
};

Fixture fixture() {
#if defined(SYNAPTOME_CONFIDENCE_GRID)
    return {"grid", "grid", "bind-only", 60, 640, 360};
#elif defined(SYNAPTOME_CONFIDENCE_STL)
    return {"stlModel", "stl-model", "bind-only", 60, 640, 360};
#elif defined(SYNAPTOME_CONFIDENCE_LENIA)
    return {"lenia", "lenia", "bind-only", 60, 640, 360};
#else
    return {
        "example.signalBloom",
        "examples.signal_bloom",
        "bind-only",
        120,
        1280,
        720,
    };
#endif
}

void configureRequest(Runtime::ElementRequest& request) {
#if defined(SYNAPTOME_CONFIDENCE_STL)
    request.config["assetPath"] = std::filesystem::absolute(
        "docs/examples/generated_layers/stl_models/tetrahedron.stl").string();
#elif defined(SYNAPTOME_CONFIDENCE_LENIA)
    // Exercise the exact lifecycle and renderer with the supported minimum
    // field size so 200 allocator cycles measure retention rather than the
    // Windows heap's caching of the production 160x90 working buffers. The
    // production Circuit Lenia dimensions are locked separately by its
    // catalog validator and real-host visual gate.
    request.config["textureSize"] = {32, 32};
    if (request.definitionId.find(".graphics") != std::string::npos) {
        request.config["presentation"] = "circuit";
    }
#else
    (void)request;
#endif
}

void registerFixture(LayerFactory& factory) {
#if defined(SYNAPTOME_CONFIDENCE_GRID) || \
    defined(SYNAPTOME_CONFIDENCE_STL) || \
    defined(SYNAPTOME_CONFIDENCE_LENIA)
    ElementTypeContract contract;
#if defined(SYNAPTOME_CONFIDENCE_GRID)
    constexpr const char* typeId = "grid";
#elif defined(SYNAPTOME_CONFIDENCE_STL)
    constexpr const char* typeId = "stlModel";
#else
    constexpr const char* typeId = "lenia";
#endif
    contract.element =
        ElementDescriptor{typeId, ElementKind::Visual, {}};
    contract.parameters =
        synaptome::runtime::builtinElementParameterDeclarations(typeId);
    factory.registerType(
        std::move(contract),
#if defined(SYNAPTOME_CONFIDENCE_GRID)
        [] { return std::make_unique<GridLayer>(); },
#elif defined(SYNAPTOME_CONFIDENCE_STL)
        [] { return std::make_unique<StlModelLayer>(); },
#else
        [] { return std::make_unique<LeniaLayer>(); },
#endif
        LayerFactory::ParameterBindingMode::Explicit);
#else
    synaptome::runtime::registerGeneratedElementPackages(factory);
#endif
}

std::size_t expectedFixtureTypeCount() {
#if defined(SYNAPTOME_CONFIDENCE_GRID) || \
    defined(SYNAPTOME_CONFIDENCE_STL) || \
    defined(SYNAPTOME_CONFIDENCE_LENIA)
    return 1;
#else
    std::size_t count = 0;
    synaptome::runtime::generatedElementPackageRegistrations(count);
    return count;
#endif
}

CompositionAssignment assignmentFor(
    const Runtime::ElementRequest& request) {
    CompositionAssignment assignment;
    assignment.definitionId = request.definitionId;
    assignment.label = request.typeId;
    assignment.typeId = request.typeId;
    assignment.registryPrefix = request.registryPrefix;
    assignment.active = true;
    assignment.opacity = 1.0f;
    return assignment;
}

bool nearlyEqual(float left, float right) {
    return std::fabs(left - right) < 0.0001f;
}

void verifyFloat(
    const ParameterDeclaration& declaration,
    const ParameterRegistry& registry,
    const std::string& id) {
    const auto* live = registry.findFloat(id);
    require(live != nullptr, "missing live float: " + id);
    const auto* defaultValue =
        std::get_if<float>(&declaration.defaultValue);
    require(
        defaultValue && nearlyEqual(*defaultValue, live->defaultValue),
        "float default drift: " + id);
    require(
        declaration.label == live->meta.label &&
            declaration.units == live->meta.units &&
            declaration.description == live->meta.description,
        "float metadata drift: " + id);
    require(
        declaration.range.has_value() &&
            nearlyEqual(declaration.range->min, live->meta.range.min) &&
            nearlyEqual(declaration.range->max, live->meta.range.max) &&
            (!declaration.range->step ||
             nearlyEqual(
                 *declaration.range->step,
                 live->meta.range.step)),
        "float range drift: " + id);
}

void verifyBool(
    const ParameterDeclaration& declaration,
    const ParameterRegistry& registry,
    const std::string& id) {
    const auto* live = registry.findBool(id);
    require(live != nullptr, "missing live bool: " + id);
    const auto* defaultValue =
        std::get_if<bool>(&declaration.defaultValue);
    require(
        defaultValue && *defaultValue == live->defaultValue,
        "bool default drift: " + id);
    require(
        declaration.label == live->meta.label &&
            declaration.description == live->meta.description,
        "bool metadata drift: " + id);
}

void verifyString(
    const ParameterDeclaration& declaration,
    const ParameterRegistry& registry,
    const std::string& id) {
    const auto* live = registry.findString(id);
    require(live != nullptr, "missing live string: " + id);
    const auto* defaultValue =
        std::get_if<std::string>(&declaration.defaultValue);
    require(
        defaultValue && *defaultValue == live->defaultValue,
        "string default drift: " + id);
    require(
        declaration.label == live->meta.label &&
            declaration.description == live->meta.description,
        "string metadata drift: " + id);
}

std::size_t verifyLiveSurface(
    const LayerFactory::ElementTypeContractRecord& contract,
    const ParameterRegistry& registry,
    const std::string& prefix) {
    for (const auto& declaration :
         contract.contract.parameters.parameters) {
        const auto id = prefix + "." + declaration.id;
        switch (declaration.kind) {
        case ParameterKind::Float:
            verifyFloat(declaration, registry, id);
            break;
        case ParameterKind::Bool:
            verifyBool(declaration, registry, id);
            break;
        case ParameterKind::String:
            verifyString(declaration, registry, id);
            break;
        }
    }
    const auto liveCount =
        registry.floats().size() +
        registry.bools().size() +
        registry.strings().size();
    require(
        liveCount ==
            contract.contract.parameters.parameters.size() + 1,
        "live surface includes missing or unexpected parameters");
    require(
        registry.findFloat(prefix + ".opacity") != nullptr,
        "Runtime-owned opacity parameter is missing");
    return contract.contract.parameters.parameters.size();
}

std::string statePayload(
    const ParameterRegistry& registry,
    const std::string& prefix,
    const Layer& layer) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::hexfloat;
    std::vector<std::string> values;
    for (const auto& parameter : registry.floats()) {
        if (parameter.meta.id.rfind(prefix + ".", 0) == 0 &&
            parameter.value) {
            std::ostringstream value;
            value.imbue(std::locale::classic());
            value << parameter.meta.id << "=" << std::hexfloat
                  << *parameter.value;
            values.push_back(value.str());
        }
    }
    for (const auto& parameter : registry.bools()) {
        if (parameter.meta.id.rfind(prefix + ".", 0) == 0 &&
            parameter.value) {
            values.push_back(
                parameter.meta.id + "=" +
                (*parameter.value ? "true" : "false"));
        }
    }
    for (const auto& parameter : registry.strings()) {
        if (parameter.meta.id.rfind(prefix + ".", 0) == 0 &&
            parameter.value) {
            values.push_back(parameter.meta.id + "=" + *parameter.value);
        }
    }
    std::sort(values.begin(), values.end());
    for (const auto& value : values) {
        out << value << ";";
    }

#if defined(SYNAPTOME_CONFIDENCE_GRID)
    const auto& grid = dynamic_cast<const GridLayer&>(layer);
    out << "segments=" << grid.segments_
        << ";enabled=" << grid.enabled_
        << ";wave=" << grid.wave_
        << ";bend=" << grid.bend_
        << ";deform=" << grid.deform_
        << ";twist=" << grid.twist_
        << ";bulge=" << grid.bulge_
        << ";summary=" << grid.deformationSummary();
#elif defined(SYNAPTOME_CONFIDENCE_STL)
    const auto& model = dynamic_cast<const StlModelLayer&>(layer);
    out << "meshLoaded=" << model.meshLoaded_
        << ";vertices=" << model.mesh_.getNumVertices()
        << ";asset=" << model.assetPath_;
#elif defined(SYNAPTOME_CONFIDENCE_LENIA)
    const auto& lenia = dynamic_cast<const LeniaLayer&>(layer);
    out << "signature=" << lenia.debugStateSignature()
        << ";circuit=" << lenia.debugUsesCircuitPresentation();
#else
    const auto& bloom = dynamic_cast<const SignalBloomLayer&>(layer);
    out << "phase=" << bloom.phase_
        << ";points=" << bloom.points_.size() << ";";
    for (const auto& point : bloom.points_) {
        out << point.x << "," << point.y << ";";
    }
#endif
    return out.str();
}

struct RepetitionEvidence {
    std::string state;
    std::size_t parameterCount = 0;
    bool registryInvalidated = false;
    bool actionsInvalidated = false;
};

RepetitionEvidence runRepetition(const Fixture& selected, int repetition) {
    LayerFactory factory;
    registerFixture(factory);
    require(
        factory.descriptors().size() == expectedFixtureTypeCount(),
        "fixture registry does not match its declared registration set");
    const auto* descriptor = factory.descriptor(selected.typeId);
    const auto* contract = factory.typeContract(selected.typeId);
    require(
        descriptor && contract &&
            descriptor->kind == ElementKind::Visual &&
            contract->state ==
                LayerFactory::ParameterDeclarationState::Declared,
        "construction-free declaration is unavailable");
    require(
        (selected.bindingMode == "bind-only" &&
         contract->bindingMode ==
             LayerFactory::ParameterBindingMode::Explicit) ||
            (selected.bindingMode == "legacy-setup-adapter" &&
             contract->bindingMode ==
                 LayerFactory::ParameterBindingMode::
                     LegacySetupAdapter),
        "binding mode drifted");

    ParameterRegistry registry;
    Runtime runtime(factory, registry);
    Runtime::ElementRequest request;
    request.typeId = selected.typeId;
    request.definitionId =
        "confidence." + selected.profileId + ".definition";
    request.instanceId =
        "confidence." + selected.profileId + "." +
        std::to_string(repetition);
    request.registryPrefix = "console.layer1";
    request.enabled = true;
    configureRequest(request);

    auto prepared = runtime.prepareElement(request);
    require(
        static_cast<bool>(prepared),
        "element preparation failed at " + prepared.stage + ": " +
            prepared.error);
    Layer* element = prepared.element();
    require(element != nullptr, "prepared element is missing");
    const auto adoption = runtime.adoptPreparedElement(
        0,
        std::move(prepared),
        assignmentFor(request));
    require(
        static_cast<bool>(adoption),
        "prepared element adoption failed: " + adoption.error);

    RepetitionEvidence evidence;
    evidence.parameterCount =
        verifyLiveSurface(*contract, registry, request.registryPrefix);
    for (int frame = 0; frame < selected.updateFrames; ++frame) {
        LayerUpdateParams update;
        update.dt = 1.0f / 60.0f;
        update.time = static_cast<float>(frame) * update.dt;
        update.bpm = 120.0f;
        update.speed = 1.0f;
        runtime.updateCompositionElements(update);
    }
    evidence.state =
        statePayload(registry, request.registryPrefix, *element);

    require(
        static_cast<bool>(runtime.clearCompositionLayer(0)),
        "element teardown failed");
    evidence.registryInvalidated =
        std::none_of(
            registry.floats().begin(),
            registry.floats().end(),
            [&](const auto& parameter) {
                return parameter.meta.id.rfind(
                    request.registryPrefix + ".",
                    0) == 0;
            }) &&
        std::none_of(
            registry.bools().begin(),
            registry.bools().end(),
            [&](const auto& parameter) {
                return parameter.meta.id.rfind(
                    request.registryPrefix + ".",
                    0) == 0;
            }) &&
        std::none_of(
            registry.strings().begin(),
            registry.strings().end(),
            [&](const auto& parameter) {
                return parameter.meta.id.rfind(
                    request.registryPrefix + ".",
                    0) == 0;
            });
    const auto snapshot = runtime.compositionLayerSnapshot(0);
    evidence.actionsInvalidated =
        snapshot && !snapshot->occupied && snapshot->actions.empty();
    require(
        evidence.registryInvalidated,
        "teardown retained registry entries");
    require(
        evidence.actionsInvalidated,
        "teardown retained action entries");
    return evidence;
}

std::filesystem::path outputPath(int argc, char** argv) {
    for (int index = 1; index + 1 < argc; ++index) {
        if (std::string(argv[index]) == "--output") {
            return argv[index + 1];
        }
    }
    throw std::runtime_error("--output PATH is required");
}

bool hasArgument(int argc, char** argv, const std::string& expected) {
    for (int index = 1; index < argc; ++index) {
        if (argv[index] == expected) {
            return true;
        }
    }
    return false;
}

ofJson parameterValueJson(
    const synaptome::element::ParameterValue& value) {
    return std::visit(
        [](const auto& selected) -> ofJson {
            return selected;
        },
        value);
}

ofJson constructionFreeDescriptor(const Fixture& selected) {
    LayerFactory factory;
    registerFixture(factory);
    require(
        factory.descriptors().size() == expectedFixtureTypeCount(),
        "descriptor fixture does not match its declared registration set");
    const auto* record = factory.typeContract(selected.typeId);
    require(
        record != nullptr &&
            record->state ==
                LayerFactory::ParameterDeclarationState::Declared,
        "construction-free ElementTypeContract is unavailable");
    const auto& contract = re