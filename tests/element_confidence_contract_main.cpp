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
    return {"grid", "grid", "legacy-setup-adapter", 60, 640, 360};
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

void registerFixture(LayerFactory& factory) {
#if defined(SYNAPTOME_CONFIDENCE_GRID)
    ElementTypeContract contract;
    contract.element =
        ElementDescriptor{"grid", ElementKind::Visual, {}};
    contract.parameters =
        synaptome::runtime::builtinElementParameterDeclarations("grid");
    factory.registerType(
        std::move(contract),
        [] { return std::make_unique<GridLayer>(); },
        LayerFactory::ParameterBindingMode::LegacySetupAdapter);
#else
    synaptome::runtime::registerGeneratedElementPackages(factory);
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
        factory.descriptors().size() == 1,
        "fixture registry compiled or registered unrelated elements");
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
        factory.descriptors().size() == 1,
        "descriptor fixture registered unrelated elements");
    const auto* record = factory.typeContract(selected.typeId);
    require(
        record != nullptr &&
            record->state ==
                LayerFactory::ParameterDeclarationState::Declared,
        "construction-free ElementTypeContract is unavailable");
    const auto& contract = record->contract;
    ofJson output;
    output["typeId"] = contract.element.typeId;
    output["kind"] =
        contract.element.kind == ElementKind::Visual
        ? "visual"
        : "effect";
    output["bindingMode"] =
        record->bindingMode == LayerFactory::ParameterBindingMode::Explicit
        ? "bind-only"
        : "legacy-setup-adapter";
    output["actions"] = ofJson::array();
    for (const auto& action : contract.element.actions) {
        output["actions"].push_back({
            {"id", action.id},
            {"label", action.label},
            {"groupId", action.groupId},
        });
    }
    output["parameterGroups"] = ofJson::array();
    for (const auto& group : contract.parameters.groups) {
        output["parameterGroups"].push_back({
            {"id", group.id},
            {"label", group.label},
            {"description", group.description},
        });
    }
    output["parameters"] = ofJson::array();
    for (const auto& parameter : contract.parameters.parameters) {
        ofJson range = nullptr;
        if (parameter.range) {
            range = {
                {"min", parameter.range->min},
                {"max", parameter.range->max},
                {
                    "step",
                    parameter.range->step
                        ? ofJson(*parameter.range->step)
                        : ofJson(nullptr),
                },
            };
        }
        ofJson options = ofJson::array();
        for (const auto& option : parameter.options) {
            options.push_back({
                {"value", parameterValueJson(option.value)},
                {"label", option.label},
                {"description", option.description},
            });
        }
        ofJson optionSource = nullptr;
        if (parameter.optionSource) {
            optionSource = {
                {"id", parameter.optionSource->id},
                {"valueField", parameter.optionSource->valueField},
                {"labelField", parameter.optionSource->labelField},
            };
        }
        ofJson deprecation = nullptr;
        if (parameter.deprecation) {
            deprecation = {
                {"replacementId", parameter.deprecation->replacementId},
                {"reason", parameter.deprecation->reason},
            };
        }
        output["parameters"].push_back({
            {"id", parameter.id},
            {
                "kind",
                parameter.kind == ParameterKind::Float
                    ? "float"
                    : parameter.kind == ParameterKind::Bool
                        ? "bool"
                        : "string",
            },
            {"groupId", parameter.groupId},
            {"label", parameter.label},
            {"default", parameterValueJson(parameter.defaultValue)},
            {"range", range},
            {"units", parameter.units},
            {"description", parameter.description},
            {"visible", parameter.visible},
            {"options", options},
            {"optionSource", optionSource},
            {
                "quickAccessOrder",
                parameter.quickAccessOrder
                    ? ofJson(*parameter.quickAccessOrder)
                    : ofJson(nullptr),
            },
            {"aliases", parameter.aliases},
            {"deprecation", deprecation},
        });
    }
    return output;
}

ofJson runGraphics(
    const Fixture& selected,
    const std::filesystem::path& destination) {
    ofGLFWWindowSettings settings;
    settings.setSize(64, 64);
    settings.visible = false;
    settings.setGLVersion(3, 2);
    const auto window = ofCreateWindow(settings);
    require(window != nullptr, "hidden graphics context creation failed");

    LayerFactory factory;
    registerFixture(factory);
    ParameterRegistry registry;
    Runtime runtime(factory, registry);
    Runtime::ElementRequest request;
    request.typeId = selected.typeId;
    request.definitionId =
        "confidence." + selected.profileId + ".graphics";
    request.instanceId =
        "confidence." + selected.profileId + ".graphics";
    request.registryPrefix = "console.layer1";
    request.enabled = true;
    auto prepared = runtime.prepareElement(request);
    require(
        static_cast<bool>(prepared),
        "graphics element preparation failed: " + prepared.error);
    const auto adoption = runtime.adoptPreparedElement(
        0,
        std::move(prepared),
        assignmentFor(request));
    require(
        static_cast<bool>(adoption),
        "graphics element adoption failed: " + adoption.error);
    for (int frame = 0; frame < selected.updateFrames; ++frame) {
        LayerUpdateParams update;
        update.dt = 1.0f / 60.0f;
        update.time = static_cast<float>(frame) * update.dt;
        update.bpm = 120.0f;
        update.speed = 1.0f;
        runtime.updateCompositionElements(update);
    }

    ofFbo::Settings fboSettings;
    fboSettings.width = selected.viewportWidth;
    fboSettings.height = selected.viewportHeight;
    fboSettings.internalformat = GL_RGBA;
    fboSettings.useDepth = true;
    fboSettings.useStencil = true;
    ofFbo fbo;
    fbo.allocate(fboSettings);
    require(fbo.isAllocated(), "real offscreen framebuffer allocation failed");
    ofCamera camera;
    camera.setPosition(0.0f, 350.0f, 650.0f);
    camera.lookAt(glm::vec3(0.0f));
    camera.setNearClip(0.1f);
    camera.setFarClip(4000.0f);
    LayerDrawParams draw{camera};
    draw.viewport = {
        selected.viewportWidth,
        selected.viewportHeight,
    };
    draw.time =
        static_cast<float>(selected.updateFrames - 1) / 60.0f;
    draw.beat = draw.time * 2.0f;
    draw.slotOpacity = 1.0f;

    fbo.begin();
    ofClear(0, 0, 0, 255);
    GraphicsStateGuard stateGuard;
    runtime.drawCompositionElement(0, draw);
    const auto leakedState = stateGuard.restoreAndVerify();
    fbo.end();
    require(
        leakedState.empty(),
        "graphics state restoration failed");

    ofPixels pixels;
    fbo.readToPixels(pixels);
    require(
        pixels.isAllocated() &&
            pixels.getNumChannels() == 4,
        "RGBA pixel readback failed");
    std::vector<std::uint32_t> colors;
    colors.reserve(pixels.getWidth() * pixels.getHeight());
    std::size_t nonblack = 0;
    for (std::size_t index = 0;
         index + 3 < pixels.size();
         index += 4) {
        const auto packed =
            static_cast<std::uint32_t>(pixels[index]) |
            (static_cast<std::uint32_t>(pixels[index + 1]) << 8) |
            (static_cast<std::uint32_t>(pixels[index + 2]) << 16) |
            (static_cast<std::uint32_t>(pixels[index + 3]) << 24);
        colors.push_back(packed);
        if (pixels[index] != 0 ||
            pixels[index + 1] != 0 ||
            pixels[index + 2] != 0) {
            ++nonblack;
        }
    }
    std::sort(colors.begin(), colors.end());
    const auto distinctEnd =
        std::unique(colors.begin(), colors.end());
    const auto distinct =
        static_cast<std::size_t>(distinctEnd - colors.begin());
    const auto pixelCount =
        static_cast<std::size_t>(
            pixels.getWidth() * pixels.getHeight());
    const double nonblackRatio =
        pixelCount == 0
        ? 0.0
        : static_cast<double>(nonblack) /
            static_cast<double>(pixelCount);
    require(distinct >= 2, "rendered output has fewer than two RGBA values");
    require(
        nonblackRatio >= 0.001,
        "rendered output is below the 0.1 percent nonblack gate");

    const auto pixelPath =
        destination.parent_path() /
        (selected.profileId + ".rgba");
    std::ofstream pixelStream(pixelPath, std::ios::binary);
    require(
        static_cast<bool>(pixelStream),
        "could not open pixel evidence output");
    pixelStream.write(
        reinterpret_cast<const char*>(pixels.getData()),
        static_cast<std::streamsize>(pixels.size()));
    require(
        static_cast<bool>(pixelStream),
        "could not write pixel evidence");

    const auto* renderer =
        reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto* vendor =
        reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* version =
        reinterpret_cast<const char*>(glGetString(GL_VERSION));
    ofJson output;
    output["status"] = "pass";
    output["profileId"] = selected.profileId;
    output["elementType"] = selected.typeId;
    output["renderer"] = renderer ? renderer : "";
    output["vendor"] = vendor ? vendor : "";
    output["version"] = version ? version : "";
    output["pixelFile"] = pixelPath.string();
    output["nonblank"] = {
        {"width", pixels.getWidth()},
        {"height", pixels.getHeight()},
        {"distinctRgbaValues", distinct},
        {"nonblackPixels", nonblack},
        {"nonblackRatio", nonblackRatio},
    };
    output["leakedState"] = leakedState;
    require(
        static_cast<bool>(runtime.clearCompositionLayer(0)),
        "graphics element teardown failed");
    return output;
}

std::uint64_t processWorkingSetBytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters = {};
    counters.cb = sizeof(counters);
    require(
        GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters)) != FALSE,
        "could not read process working set");
    return static_cast<std::uint64_t>(
        counters.WorkingSetSize);
#else
    throw std::runtime_error(
        "working-set collection is not implemented on this platform");
#endif
}

double percentile(
    std::vector<double> values,
    double fraction) {
    require(!values.empty(), "timing sample set is empty");
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>(
        std::ceil(fraction * static_cast<double>(values.size()))) - 1;
    return values[std::min(index, values.size() - 1)];
}

double median(std::vector<double> values) {
    require(!values.empty(), "timing sample set is empty");
    std::sort(values.begin(), values.end());
    const auto middle = values.size() / 2;
    return values.size() % 2 == 0
        ? (values[middle - 1] + values[middle]) * 0.5
        : values[middle];
}

double fittedSlope(const std::vector<std::uint64_t>& samples) {
    require(samples.size() >= 2, "memory sample set is too small");
    const double count = static_cast<double>(samples.size());
    double sumX = 0.0;
    double sumY = 0.0;
    double sumXy = 0.0;
    double sumX2 = 0.0;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        const double x = static_cast<double>(index + 1);
        const double y = static_cast<double>(samples[index]);
        sumX += x;
        sumY += y;
        sumXy += x * y;
        sumX2 += x * x;
    }
    const double denominator = count * sumX2 - sumX * sumX;
    require(denominator != 0.0, "memory slope denominator is zero");
    return (count * sumXy - sumX * sumY) / denominator;
}

struct CycleTiming {
    double updateMs = 0.0;
    double drawMs = 0.0;
};

CycleTiming runReloadCycle(
    Runtime& runtime,
    ParameterRegistry& registry,
    const Fixture& selected,
    ofFbo& fbo,
    ofCamera& camera,
    int cycle,
    bool recordTiming) {
    Runtime::ElementRequest request;
    request.typeId = selected.typeId;
    request.definitionId =
        "confidence." + selected.profileId + ".reload";
    request.instanceId =
        "confidence." + selected.profileId + ".reload." +
        std::to_string(cycle);
    request.registryPrefix = "console.layer1";
    request.enabled = true;
    auto prepared = runtime.prepareElement(request);
    require(
        static_cast<bool>(prepared),
        "reload preparation failed at cycle " +
            std::to_string(cycle) + ": " + prepared.error);
    const auto adoption = runtime.adoptPreparedElement(
        0,
        std::move(prepared),
        assignmentFor(request));
    require(
        static_cast<bool>(adoption),
        "reload adoption failed at cycle " +
            std::to_string(cycle) + ": " + adoption.error);

    LayerUpdateParams update;
    update.dt = 1.0f / 60.0f;
    update.time = static_cast<float>(cycle) * update.dt;
    update.bpm = 120.0f;
    update.speed = 1.0f;
    const auto updateStart = std::chrono::steady_clock::now();
    runtime.updateCompositionElements(update);
    const auto updateEnd = std::chrono::steady_clock::now();

    LayerDrawParams draw{camera};
    draw.viewport = {
        selected.viewportWidth,
        selected.viewportHeight,
    };
    draw.time = update.time;
    draw.beat = draw.time * 2.0f;
    draw.slotOpacity = 1.0f;
    fbo.begin();
    ofClear(0, 0, 0, 255);
    GraphicsStateGuard stateGuard;
    const auto drawStart = std::chrono::steady_clock::now();
    runtime.drawCompositionElement(0, draw);
    const auto drawEnd = std::chrono::steady_clock::now();
    const auto leakedState = stateGuard.restoreAndVerify();
    fbo.end();
    require(
        leakedState.empty(),
        "graphics state restoration failed at reload cycle " +
            std::to_string(cycle));

    require(
        static_cast<bool>(runtime.clearCompositionLayer(0)),
        "reload teardown failed at cycle " +
            std::to_string(cycle));
    const auto snapshot = runtime.compositionLayerSnapshot(0);
    require(
        snapshot && !snapshot->occupied && snapshot->actions.empty(),
        "reload retained a composition/action entry at cycle " +
            std::to_string(cycle));
    const auto prefix = request.registryPrefix + ".";
    const auto staleFloat = std::find_if(
        registry.floats().begin(),
        registry.floats().end(),
        [&](const auto& parameter) {
            return parameter.meta.id.rfind(prefix, 0) == 0;
        });
    const auto staleBool = std::find_if(
        registry.bools().begin(),
        registry.bools().end(),
        [&](const auto& parameter) {
            return parameter.meta.id.rfind(prefix, 0) == 0;
        });
    const auto staleString = std::find_if(
        registry.strings().begin(),
        registry.strings().end(),
        [&](const auto& parameter) {
            return parameter.meta.id.rfind(prefix, 0) == 0;
        });
    require(
        staleFloat == registry.floats().end() &&
            staleBool == registry.bools().end() &&
            staleString == registry.strings().end(),
        "reload retained registry entries at cycle " +
            std::to_string(cycle));

    CycleTiming timing;
    if (recordTiming) {
        timing.updateMs =
            std::chrono::duration<double, std::milli>(
                updateEnd - updateStart).count();
        timing.drawMs =
            std::chrono::duration<double, std::milli>(
                drawEnd - drawStart).count();
    }
    return timing;
}

ofJson runReload(const Fixture& selected) {
    ofGLFWWindowSettings settings;
    settings.setSize(64, 64);
    settings.visible = false;
    settings.setGLVersion(3, 2);
    const auto window = ofCreateWindow(settings);
    require(window != nullptr, "hidden reload context creation failed");
    LayerFactory factory;
    registerFixture(factory);
    require(
        factory.descriptors().size() == 1,
        "reload fixture registered unrelated elements");
    ParameterRegistry registry;
    Runtime runtime(factory, registry);
    ofFbo::Settings fboSettings;
    fboSettings.width = selected.viewportWidth;
    fboSettings.height = selected.viewportHeight;
    fboSettings.internalformat = GL_RGBA;
    fboSettings.useDepth = true;
    fboSettings.useStencil = true;
    ofFbo fbo;
    fbo.allocate(fboSettings);
    require(
        fbo.isAllocated(),
        "reload framebuffer allocation failed");
    ofCamera camera;
    camera.setPosition(0.0f, 350.0f, 650.0f);
    camera.lookAt(glm::vec3(0.0f));
    camera.setNearClip(0.1f);
    camera.setFarClip(4000.0f);

    constexpr int warmupCycles = 20;
    constexpr int measuredCycles = 200;
    for (int cycle = 0; cycle < warmupCycles; ++cycle) {
        runReloadCycle(
            runtime,
            registry,
            selected,
            fbo,
            camera,
            cycle,
            false);
    }
    const auto warmWorkingSet = processWorkingSetBytes();
    std::vector<std::uint64_t> workingSets;
    std::vector<double> updateTimings;
    std::vector<double> drawTimings;
    workingSets.reserve(measuredCycles);
    updateTimings.reserve(measuredCycles);
    drawTimings.reserve(measuredCycles);
    for (int cycle = 0; cycle < measuredCycles; ++cycle) {
        const auto timing = runReloadCycle(
            runtime,
            registry,
            selected,
            fbo,
            camera,
            warmupCycles + cycle,
            true);
        updateTimings.push_back(timing.updateMs);
        drawTimings.push_back(timing.drawMs);
        workingSets.push_back(processWorkingSetBytes());
    }
    const auto finalWorkingSet = workingSets.back();
    const auto slope = fittedSlope(workingSets);
    constexpr std::uint64_t maxFinalGrowth =
        16ull * 1024ull * 1024ull;
    constexpr double maxSlope = 64.0 * 1024.0;
    require(
        finalWorkingSet <= warmWorkingSet + maxFinalGrowth,
        "final working set exceeded the 16 MiB reload gate: warm=" +
            std::to_string(warmWorkingSet) + " final=" +
            std::to_string(finalWorkingSet));
    require(
        slope <= maxSlope,
        "working-set slope exceeded 64 KiB per reload: slope=" +
            std::to_string(slope));
    fbo.clear();
    require(
        !fbo.isAllocated(),
        "harness-owned graphics target was not released");

    ofJson output;
    output["status"] = "pass";
    output["profileId"] = selected.profileId;
    output["elementType"] = selected.typeId;
    output["reload"] = {
        {"warmupCount", warmupCycles},
        {"count", measuredCycles},
        {"warmWorkingSetBytes", warmWorkingSet},
        {"finalWorkingSetBytes", finalWorkingSet},
        {"growthSlopeBytesPerReload", slope},
        {"graphicsTargetsReleased", true},
        {"registryEntriesInvalidated", true},
        {"actionEntriesInvalidated", true},
        {"graphicsStateLeaks", ofJson::array()},
    };
    output["timings"] = {
        {"updateMs", {
            {"median", median(updateTimings)},
            {"p95", percentile(updateTimings, 0.95)},
            {"maximum", *std::max_element(
                updateTimings.begin(),
                updateTimings.end())},
        }},
        {"drawMs", {
            {"median", median(drawTimings)},
            {"p95", percentile(drawTimings, 0.95)},
            {"maximum", *std::max_element(
                drawTimings.begin(),
                drawTimings.end())},
        }},
    };
    return output;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto selected = fixture();
        const auto destination = outputPath(argc, argv);
        if (hasArgument(argc, argv, "--descriptor-only")) {
            const auto output = constructionFreeDescriptor(selected);
            std::filesystem::create_directories(
                destination.parent_path());
            std::ofstream stream(destination);
            require(
                static_cast<bool>(stream),
                "could not open descriptor evidence output");
            stream << output.dump(2) << "\n";
            std::cout << "[element_confidence_descriptor] PASS "
                      << selected.profileId << "\n";
            return 0;
        }
        if (hasArgument(argc, argv, "--reload")) {
            const auto output = runReload(selected);
            std::filesystem::create_directories(
                destination.parent_path());
            std::ofstream stream(destination);
            require(
                static_cast<bool>(stream),
                "could not open reload evidence output");
            stream << output.dump(2) << "\n";
            std::cout << "[element_confidence_reload] PASS "
                      << selected.profileId << "\n";
            return 0;
        }
        if (hasArgument(argc, argv, "--graphics")) {
            const auto output = runGraphics(selected, destination);
            std::filesystem::create_directories(
                destination.parent_path());
            std::ofstream stream(destination);
            require(
                static_cast<bool>(stream),
                "could not open graphics evidence output");
            stream << output.dump(2) << "\n";
            std::cout << "[element_confidence_graphics] PASS "
                      << selected.profileId << "\n";
            return 0;
        }
        const auto first = runRepetition(selected, 0);
        const auto second = runRepetition(selected, 1);
        require(
            first.state == second.state,
            "identical repetitions produced different state payloads");

        ofJson output;
        output["status"] = "pass";
        output["profileId"] = selected.profileId;
        output["elementType"] = selected.typeId;
        output["bindingMode"] = selected.bindingMode;
        output["parameterCount"] = first.parameterCount;
        output["declarationLiveComparison"] = {
            {"status", "pass"},
            {"diagnostic", "static declaration and live surface match"},
        };
        output["statePayloads"] = {first.state, second.state};
        output["registryInvalidated"] =
            first.registryInvalidated && second.registryInvalidated;
        output["actionsInvalidated"] =
            first.actionsInvalidated && second.actionsInvalidated;
        output["inputs"] = {
            {"viewportWidth", selected.viewportWidth},
            {"viewportHeight", selected.viewportHeight},
            {"updateFrames", selected.updateFrames},
            {"fixedStepSeconds", 1.0 / 60.0},
            {"bpm", 120.0},
            {"transportSpeed", 1.0},
            {"seed", 1001},
            {"determinismRepetitions", 2},
        };

        std::filesystem::create_directories(
            destination.parent_path());
        std::ofstream stream(destination);
        require(
            static_cast<bool>(stream),
            "could not open native evidence output");
        stream << output.dump(2) << "\n";
        std::cout << "[element_confidence_contract] PASS "
                  << selected.profileId << ": "
                  << first.parameterCount
                  << " parameters, two deterministic repetitions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[element_confidence_contract] FAIL: "
                  << error.what() << "\n";
        return 1;
    }
}
