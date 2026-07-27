#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../synaptome/src/runtime/Runtime.h"
#include "../synaptome/src/visuals/LayerFactory.h"

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void require(
    const synaptome::runtime::CompositionMutationResult& result,
    const std::string& message) {
    if (!result) {
        throw std::runtime_error(
            message + (result.error.empty() ? "" : ": " + result.error));
    }
}

synaptome::element::ElementDescriptor visualDescriptor(
    std::string typeId,
    std::vector<synaptome::element::ActionDescriptor> actions = {}) {
    synaptome::element::ElementDescriptor descriptor;
    descriptor.typeId = std::move(typeId);
    descriptor.kind = synaptome::element::ElementKind::Visual;
    descriptor.actions = std::move(actions);
    return descriptor;
}

synaptome::element::ElementDescriptor effectDescriptor(
    std::string typeId,
    std::vector<synaptome::element::ActionDescriptor> actions = {}) {
    synaptome::element::ElementDescriptor descriptor;
    descriptor.typeId = std::move(typeId);
    descriptor.kind = synaptome::element::ElementKind::Effect;
    descriptor.actions = std::move(actions);
    return descriptor;
}

synaptome::element::ParameterDeclaration validFloatDeclaration(
    std::string id = "amount",
    std::string groupId = "controls") {
    synaptome::element::ParameterDeclaration declaration;
    declaration.id = std::move(id);
    declaration.kind = synaptome::element::ParameterKind::Float;
    declaration.groupId = std::move(groupId);
    declaration.label = "Amount";
    declaration.defaultValue = 0.5f;
    declaration.range =
        synaptome::element::ParameterRange{0.0f, 1.0f, 0.1f};
    declaration.units = "multiplier";
    declaration.description = "A valid declaration fixture.";
    declaration.options = {
        {0.25f, "Low", "Low fixture value."},
        {0.75f, "High", "High fixture value."},
    };
    declaration.quickAccessOrder = 0;
    declaration.aliases = {"legacyAmount"};
    return declaration;
}

synaptome::element::ElementTypeContract validParameterContract(
    std::string typeId) {
    synaptome::element::ElementTypeContract contract;
    contract.element = visualDescriptor(std::move(typeId));
    contract.parameters.groups = {
        {"controls", "Controls", "Primary controls."},
        {"advanced", "Advanced", "Advanced controls."},
    };
    contract.parameters.parameters = {
        validFloatDeclaration(),
        {
            "gate",
            synaptome::element::ParameterKind::Bool,
            "controls",
            "Gate",
            true,
            std::nullopt,
            {},
            "A valid bool fixture.",
            {},
            std::nullopt,
            1,
            {},
            synaptome::element::ParameterDeprecation{
                "alternateGate",
                "Use the replacement bool fixture.",
            },
        },
        {
            "alternateGate",
            synaptome::element::ParameterKind::Bool,
            "advanced",
            "Alternate Gate",
            false,
            std::nullopt,
            {},
            "A valid replacement bool fixture.",
            {},
            std::nullopt,
            std::nullopt,
            {},
            std::nullopt,
        },
    };
    return contract;
}

std::vector<synaptome::element::ActionDescriptor>
actionContractDescriptors(bool secondVersion) {
    return {
        {
            secondVersion ? "version.second" : "version.first",
            secondVersion ? "Second Version" : "First Version",
            "version",
            "Proves replacement publishes a new action table.",
        },
        {
            "execution.reject",
            "Reject",
            "execution",
            "Returns an intentional rejection.",
        },
        {
            "execution.fail",
            "Fail",
            "execution",
            "Returns an intentional execution failure.",
        },
        {
            "render.reset2D",
            "Reset 2D",
            "render",
            "Pins accepted lowerCamel identifiers with digits.",
        },
        {
            "execution.throw",
            "Throw",
            "execution",
            "Throws to prove Runtime contains action exceptions.",
        },
    };
}

bool sameActionDescriptor(
    const synaptome::element::ActionDescriptor& left,
    const synaptome::element::ActionDescriptor& right) {
    return left.id == right.id &&
        left.label == right.label &&
        left.groupId == right.groupId &&
        left.description == right.description;
}

template <typename T>
T telemetryValue(
    const synaptome::runtime::Runtime& runtime,
    std::size_t zeroBasedIndex,
    std::string_view id) {
    const auto result =
        runtime.compositionElementTelemetry(zeroBasedIndex);
    if (!result) {
        throw std::runtime_error(
            "telemetry query failed for " + std::string(id) +
            (result.error.empty() ? "" : ": " + result.error));
    }
    const auto* value = result.valueAs<T>(id);
    if (!value) {
        throw std::runtime_error(
            "telemetry value missing or wrong type: " +
            std::string(id));
    }
    return *value;
}

synaptome::runtime::CompositionAssignment assignmentFor(
    const synaptome::runtime::Runtime::ElementRequest& request,
    const std::string& label = "RuntimeCore Element",
    float opacity = 1.0f) {
    synaptome::runtime::CompositionAssignment assignment;
    assignment.kind = synaptome::runtime::CompositionKind::Element;
    assignment.definitionId = request.definitionId;
    assignment.label = label;
    assignment.typeId = request.typeId;
    assignment.registryPrefix = request.registryPrefix;
    assignment.active = request.enabled;
    assignment.opacity = opacity;
    return assignment;
}

class ContractElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Contract Value";
        registry.addFloat(registryPrefix() + ".value", &value_, 0.25f, descriptor);
        descriptor.label = "Contract Gate";
        registry.addBool(registryPrefix() + ".gate", &gate_, true, descriptor);
    }
    void update(const LayerUpdateParams&) override { ++updateCount; }
    void draw(const LayerDrawParams&) override { ++drawCount; }
    void onWindowResized(int width, int height) override {
        ++resizeCount;
        lastWidth = width;
        lastHeight = height;
    }
    void collectTelemetry(
        synaptome::element::TelemetrySink& sink) const override {
        sink.add({
            "tests.updateCount",
            "Update Count",
            "tests",
            "Number of routed updates.",
            static_cast<std::int64_t>(updateCount),
        });
        sink.add({
            "tests.drawCount",
            "Draw Count",
            "tests",
            "Number of routed draws.",
            static_cast<std::int64_t>(drawCount),
        });
        sink.add({
            "tests.resizeCount",
            "Resize Count",
            "tests",
            "Number of routed resizes.",
            static_cast<std::int64_t>(resizeCount),
        });
        sink.add({
            "tests.lastWidth",
            "Last Width",
            "tests",
            "Last routed width.",
            static_cast<std::int64_t>(lastWidth),
        });
        sink.add({
            "tests.lastHeight",
            "Last Height",
            "tests",
            "Last routed height.",
            static_cast<std::int64_t>(lastHeight),
        });
        sink.add({
            "tests.enabled",
            "Enabled",
            "tests",
            "Current external enabled state.",
            enabled_,
        });
    }
    void setExternalEnabled(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

    int updateCount = 0;
    int drawCount = 0;
    int resizeCount = 0;
    int lastWidth = 0;
    int lastHeight = 0;

private:
    float value_ = 0.25f;
    bool gate_ = true;
    bool enabled_ = true;
};

class FailingElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Partial Value";
        registry.addFloat("outside.partial", &value_, 0.5f, descriptor);
        throw std::runtime_error("intentional setup failure");
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.5f;
};

class EmptyElement final : public Layer {
public:
    void setup(ParameterRegistry&) override {}
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}
};

struct SlotReplacementLifetimeState {
    int constructionCount = 0;
    std::array<bool, 12> destroyed{};
};

class SlotReplacementLifetimeElement final : public Layer {
public:
    SlotReplacementLifetimeElement(
        SlotReplacementLifetimeState& state,
        int serial)
        : state_(state),
          serial_(serial) {}

    ~SlotReplacementLifetimeElement() override {
        state_.destroyed[static_cast<std::size_t>(serial_)] = true;
    }

    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Slot Replacement Lifetime";
        descriptor.range.min = 0.0f;
        descriptor.range.max = 1.0f;
        descriptor.range.step = 0.05f;
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            value_,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}
    void collectTelemetry(
        synaptome::element::TelemetrySink& sink) const override {
        sink.add({
            "tests.serial",
            "Serial",
            "tests",
            "Fixture construction serial.",
            static_cast<std::int64_t>(serial_),
        });
    }

    int serial() const noexcept { return serial_; }

private:
    SlotReplacementLifetimeState& state_;
    int serial_ = 0;
    float value_ = 0.25f;
};

class ForeignParameterElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Foreign Value";
        registry.addFloat("foreign.value", &value_, 0.75f, descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.75f;
};

class DestructiveFailingElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        registry.removeById("host.live");
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Partial Value";
        registry.addFloat(
            registryPrefix() + ".partial",
            &value_,
            0.5f,
            descriptor);
        throw std::runtime_error("destructive setup failure");
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.5f;
};

class RegistryAwareElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        setupRegistry = &registry;
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Registry Aware Value";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            0.4f,
            descriptor);
    }
    void onParameterRegistryCommitted(
        ParameterRegistry& registry) noexcept override {
        committedRegistry = &registry;
        ++commitCount;
    }
    void update(const LayerUpdateParams&) override {
        sawLiveRegistry =
            committedRegistry &&
            committedRegistry != setupRegistry &&
            committedRegistry->findFloat(
                registryPrefix() + ".value") != nullptr;
    }
    void draw(const LayerDrawParams&) override {}
    void collectTelemetry(
        synaptome::element::TelemetrySink& sink) const override {
        sink.add({
            "tests.commitCount",
            "Commit Count",
            "tests",
            "Number of live registry commit callbacks.",
            static_cast<std::int64_t>(commitCount),
        });
        sink.add({
            "tests.committedToLiveRegistry",
            "Committed To Live Registry",
            "tests",
            "Whether the committed registry differs from setup staging.",
            committedRegistry &&
                committedRegistry != setupRegistry,
        });
        sink.add({
            "tests.sawLiveRegistry",
            "Saw Live Registry",
            "tests",
            "Whether update observed committed live storage.",
            sawLiveRegistry,
        });
    }

    ParameterRegistry* setupRegistry = nullptr;
    ParameterRegistry* committedRegistry = nullptr;
    int commitCount = 0;
    bool sawLiveRegistry = false;

private:
    float value_ = 0.4f;
};

class ReservedOpacityElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Element-owned Opacity";
        registry.addFloat(
            registryPrefix() + ".opacity",
            &opacity_,
            0.2f,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float opacity_ = 0.2f;
};

class HostCollisionElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Host Collision";
        registry.addFloat(
            registryPrefix() + ".hostOwned",
            &value_,
            0.2f,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.2f;
};

bool retainedRegistryDestructorRan = false;
bool retainedRegistryWasAlive = false;

class RetainedSetupRegistryElement final : public Layer {
public:
    ~RetainedSetupRegistryElement() override {
        retainedRegistryDestructorRan = true;
        retainedRegistryWasAlive =
            setupRegistry_ &&
            setupRegistry_->findFloat(registryPrefix() + ".retained") != nullptr;
    }

    void setup(ParameterRegistry& registry) override {
        setupRegistry_ = &registry;
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Retained Setup Registry";
        registry.addFloat(
            registryPrefix() + ".retained",
            &value_,
            0.3f,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    ParameterRegistry* setupRegistry_ = nullptr;
    float value_ = 0.3f;
};

class ScopedRegistryElementA final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Scoped Registry A";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            value_,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.1f;
};

class ScopedRegistryElementB final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Scoped Registry B";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            value_,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.9f;
};

struct ActionContractState {
    int constructionCount = 0;
    std::array<int, 16> successCount{};
    std::array<bool, 16> destroyed{};
    std::array<bool, 16> handlersGoneBeforeElement{};
    std::array<std::weak_ptr<int>, 16> handlerLifetimes;
};

class ActionContractElement final : public Layer {
public:
    ActionContractElement(
        ActionContractState& state,
        int serial,
        bool secondVersion)
        : state_(state),
          serial_(serial),
          secondVersion_(secondVersion) {}

    ~ActionContractElement() override {
        const auto index = static_cast<std::size_t>(serial_);
        state_.handlersGoneBeforeElement[index] =
            state_.handlerLifetimes[index].expired();
        state_.destroyed[index] = true;
    }

    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Action Contract Value";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            value_,
            descriptor);
    }

    void registerActions(
        synaptome::element::ActionRegistrar& registrar) override {
        auto lifetime = std::make_shared<int>(serial_);
        state_.handlerLifetimes[static_cast<std::size_t>(serial_)] =
            lifetime;
        const std::string versionedId = secondVersion_
            ? "version.second"
            : "version.first";
        registrar.bind(
            versionedId,
            [this, lifetime] {
                (void)lifetime;
                ++state_.successCount[static_cast<std::size_t>(serial_)];
                return synaptome::element::ActionExecutionResult::succeeded();
            });
        registrar.bind(
            "execution.reject",
            [] {
                return synaptome::element::ActionExecutionResult::rejected(
                    "intentional rejection");
            });
        registrar.bind(
            "execution.fail",
            [] {
                return synaptome::element::ActionExecutionResult::failed(
                    "intentional failure");
            });
        registrar.bind(
            "render.reset2D",
            [] {
                return synaptome::element::ActionExecutionResult::succeeded();
            });
        registrar.bind(
            "execution.throw",
            []() -> synaptome::element::ActionExecutionResult {
                throw std::runtime_error("intentional action exception");
            });
    }

    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    ActionContractState& state_;
    int serial_ = 0;
    bool secondVersion_ = false;
    float value_ = 0.5f;
};

enum class InvalidActionMode {
    UnknownBinding,
    DuplicateBinding,
    EmptyHandler,
    MissingBinding,
    ThrowDuringRegistration,
};

class InvalidActionElement final : public Layer {
public:
    explicit InvalidActionElement(InvalidActionMode mode)
        : mode_(mode) {}

    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Invalid Action Candidate";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            value_,
            descriptor);
    }

    void registerActions(
        synaptome::element::ActionRegistrar& registrar) override {
        if (mode_ == InvalidActionMode::ThrowDuringRegistration) {
            throw std::runtime_error(
                "intentional action registration failure");
        }
        if (mode_ == InvalidActionMode::MissingBinding) {
            return;
        }
        const std::string id =
            mode_ == InvalidActionMode::UnknownBinding
            ? "unknown.action"
            : "valid.action";
        synaptome::element::ActionHandler handler = [] {
            return synaptome::element::ActionExecutionResult::succeeded();
        };
        if (mode_ == InvalidActionMode::EmptyHandler) {
            handler = {};
        }
        registrar.bind(id, std::move(handler));
        if (mode_ == InvalidActionMode::DuplicateBinding) {
            registrar.bind(
                id,
                [] {
                    return synaptome::element::
                        ActionExecutionResult::succeeded();
                });
        }
    }

    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    InvalidActionMode mode_;
    float value_ = 0.5f;
};

struct TelemetryInstanceState {
    std::int64_t serial = 0;
    bool ready = false;
    double load = 0.0;
    std::string source;
    int collectionCount = 0;
};

enum class TelemetryFixtureMode {
    Valid,
    InvalidId,
    DuplicateId,
    EmptyLabel,
    InvalidGroupId,
    Throw,
};

class TelemetryContractElement final : public Layer {
public:
    TelemetryContractElement(
        std::shared_ptr<TelemetryInstanceState> state,
        TelemetryFixtureMode mode)
        : state_(std::move(state)),
          mode_(mode) {}

    void setup(ParameterRegistry&) override {}
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

    void collectTelemetry(
        synaptome::element::TelemetrySink& sink) const override {
        ++state_->collectionCount;
        if (mode_ == TelemetryFixtureMode::Throw) {
            throw std::runtime_error(
                "intentional telemetry collection failure");
        }

        std::string id = "tests.ready";
        std::string label = "Ready";
        std::string groupId = "tests";
        if (mode_ == TelemetryFixtureMode::InvalidId) {
            id = "tests_invalid";
        } else if (mode_ == TelemetryFixtureMode::EmptyLabel) {
            label.clear();
        } else if (mode_ == TelemetryFixtureMode::InvalidGroupId) {
            groupId = "Invalid Group";
        }
        sink.add({
            id,
            label,
            groupId,
            "Fixture readiness.",
            state_->ready,
        });
        if (mode_ == TelemetryFixtureMode::DuplicateId) {
            sink.add({
                id,
                "Duplicate Ready",
                "tests",
                "Duplicate fixture field.",
                false,
            });
            return;
        }
        if (mode_ != TelemetryFixtureMode::Valid) {
            return;
        }
        sink.add({
            "tests.serial",
            "Serial",
            "tests",
            "Fixture serial.",
            state_->serial,
        });
        sink.add({
            "tests.load",
            "Load",
            "tests",
            "Fixture load.",
            state_->load,
        });
        sink.add({
            "tests.source",
            "Source",
            "tests",
            "Fixture source.",
            state_->source,
        });
    }

private:
    std::shared_ptr<TelemetryInstanceState> state_;
    TelemetryFixtureMode mode_ = TelemetryFixtureMode::Valid;
};

void RunElementDescriptorRegistryScenario() {
    LayerFactory factory;
    int originalConstructions = 0;
    int duplicateConstructions = 0;

    factory.registerType(
        visualDescriptor("tests.runtime.descriptor.valid"),
        [&] {
            ++originalConstructions;
            return std::make_unique<EmptyElement>();
        });
    const auto* stableDescriptor =
        factory.descriptor("tests.runtime.descriptor.valid");
    factory.registerType(
        effectDescriptor("tests.runtime.descriptor.effect"),
        [] {
            return std::make_unique<EmptyElement>();
        });
    require(
        stableDescriptor &&
            stableDescriptor ==
                factory.descriptor("tests.runtime.descriptor.valid") &&
            stableDescriptor->kind ==
                synaptome::element::ElementKind::Visual &&
            factory.descriptor("tests.runtime.descriptor.effect")->kind ==
                synaptome::element::ElementKind::Effect &&
            originalConstructions == 0,
        "descriptor registration invoked a creator or invalidated lookup");

    bool duplicateRejected = false;
    try {
        factory.registerType(
            visualDescriptor("tests.runtime.descriptor.valid"),
            [&] {
                ++duplicateConstructions;
                return std::make_unique<EmptyElement>();
            });
    } catch (const std::logic_error&) {
        duplicateRejected = true;
    }

    auto rejectsDescriptor = [&](std::string typeId) {
        try {
            factory.registerType(
                visualDescriptor(std::move(typeId)),
                [] {
                    return std::make_unique<EmptyElement>();
                });
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };
    auto invalidKind =
        visualDescriptor("tests.runtime.descriptor.invalidKind");
    invalidKind.kind =
        static_cast<synaptome::element::ElementKind>(255);
    bool invalidKindRejected = false;
    try {
        factory.registerType(
            std::move(invalidKind),
            [] {
                return std::make_unique<EmptyElement>();
            });
    } catch (const std::invalid_argument&) {
        invalidKindRejected = true;
    }
    bool emptyCreatorRejected = false;
    try {
        factory.registerType(
            visualDescriptor("tests.runtime.descriptor.emptyCreator"),
            LayerFactory::Creator{});
    } catch (const std::invalid_argument&) {
        emptyCreatorRejected = true;
    }

    require(
        duplicateRejected &&
            invalidKindRejected &&
            emptyCreatorRejected &&
            rejectsDescriptor("") &&
            rejectsDescriptor("Invalid.start") &&
            rejectsDescriptor("invalid_type") &&
            rejectsDescriptor("invalid-type") &&
            rejectsDescriptor("invalid.") &&
            factory.descriptors().size() == 2 &&
            originalConstructions == 0 &&
            duplicateConstructions == 0,
        "invalid or duplicate descriptors mutated the registry");

    auto created = factory.create("tests.runtime.descriptor.valid");
    require(
        created &&
            originalConstructions == 1 &&
            duplicateConstructions == 0,
        "duplicate registration replaced the original descriptor creator");

    const auto* legacyContract =
        factory.typeContract("tests.runtime.descriptor.valid");
    int declaredConstructions = 0;
    factory.registerType(
        validParameterContract("tests.runtime.parameters.declared"),
        [&] {
            ++declaredConstructions;
            return std::make_unique<EmptyElement>();
        });
    const auto* stableDeclaredContract =
        factory.typeContract("tests.runtime.parameters.declared");
    factory.registerType(
        synaptome::element::ElementTypeContract{
            visualDescriptor("tests.runtime.parameters.declaredEmpty"),
            {},
        },
        [] {
            return std::make_unique<EmptyElement>();
        });
    const auto* declaredEmptyContract =
        factory.typeContract("tests.runtime.parameters.declaredEmpty");
    auto typeContractCopies = factory.typeContracts();
    require(
        legacyContract &&
            legacyContract->state ==
                LayerFactory::ParameterDeclarationState::
                    LegacySetupDiscovery &&
            legacyContract->contract.parameters.groups.empty() &&
            legacyContract->contract.parameters.parameters.empty() &&
            stableDeclaredContract &&
            stableDeclaredContract->state ==
                LayerFactory::ParameterDeclarationState::Declared &&
            stableDeclaredContract->contract.element.typeId ==
                "tests.runtime.parameters.declared" &&
            stableDeclaredContract->contract.parameters.groups.size() == 2 &&
            stableDeclaredContract->contract.parameters.parameters.size() == 3 &&
            stableDeclaredContract->contract.parameters.parameters[0].id ==
                "amount" &&
            declaredEmptyContract &&
            declaredEmptyContract->state ==
                LayerFactory::ParameterDeclarationState::Declared &&
            declaredEmptyContract->contract.parameters.groups.empty() &&
            declaredEmptyContract->contract.parameters.parameters.empty() &&
            typeContractCopies.size() == 4 &&
            declaredConstructions == 0,
        "construction-free parameter contract lookup lost declared or legacy state");
    typeContractCopies[2].state =
        LayerFactory::ParameterDeclarationState::LegacySetupDiscovery;
    typeContractCopies[2].contract.element.typeId = "copy.mutated";
    typeContractCopies[2].contract.parameters.groups[0].id = "copyMutated";
    typeContractCopies[2].contract.parameters.parameters[0].id = "copyMutated";
    require(
        factory.typeContract("tests.runtime.parameters.declared") ==
                stableDeclaredContract &&
            stableDeclaredContract->state ==
                LayerFactory::ParameterDeclarationState::Declared &&
            stableDeclaredContract->contract.element.typeId ==
                "tests.runtime.parameters.declared" &&
            stableDeclaredContract->contract.parameters.groups[0].id ==
                "controls" &&
            stableDeclaredContract->contract.parameters.parameters[0].id ==
                "amount" &&
            declaredConstructions == 0,
        "mutating copied parameter contracts changed stable factory storage");

    using ContractMutation =
        std::function<void(synaptome::element::ElementTypeContract&)>;
    const float quietNan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const std::vector<std::pair<std::string, ContractMutation>>
        invalidDeclarations = {
            {"empty parameter ID", [](auto& value) {
                 value.parameters.parameters[0].id.clear();
             }},
            {"invalid parameter ID", [](auto& value) {
                 value.parameters.parameters[0].id = "Invalid_id";
             }},
            {"reserved active parameter", [](auto& value) {
                 value.parameters.parameters[0].id = "active";
             }},
            {"reserved opacity parameter", [](auto& value) {
                 value.parameters.parameters[0].id = "opacity";
             }},
            {"duplicate parameter ID", [](auto& value) {
                 value.parameters.parameters[1].id =
                     value.parameters.parameters[0].id;
             }},
            {"invalid parameter kind", [](auto& value) {
                 value.parameters.parameters[0].kind =
                     static_cast<synaptome::element::ParameterKind>(255);
             }},
            {"empty parameter label", [](auto& value) {
                 value.parameters.parameters[0].label.clear();
             }},
            {"invalid group ID", [](auto& value) {
                 value.parameters.groups[0].id = "Invalid_group";
             }},
            {"empty group label", [](auto& value) {
                 value.parameters.groups[0].label.clear();
             }},
            {"duplicate group ID", [](auto& value) {
                 value.parameters.groups[1].id =
                     value.parameters.groups[0].id;
             }},
            {"undeclared parameter group", [](auto& value) {
                 value.parameters.parameters[0].groupId = "missing";
             }},
            {"default kind mismatch", [](auto& value) {
                 value.parameters.parameters[0].defaultValue = true;
             }},
            {"non-finite default", [quietNan](auto& value) {
                 value.parameters.parameters[0].defaultValue = quietNan;
             }},
            {"range on bool", [](auto& value) {
                 value.parameters.parameters[1].range =
                     synaptome::element::ParameterRange{0.0f, 1.0f, 1.0f};
             }},
            {"non-finite range minimum", [quietNan](auto& value) {
                 value.parameters.parameters[0].range->min = quietNan;
             }},
            {"non-finite range maximum", [infinity](auto& value) {
                 value.parameters.parameters[0].range->max = infinity;
             }},
            {"reversed range", [](auto& value) {
                 value.parameters.parameters[0].range->min = 2.0f;
             }},
            {"non-finite range step", [quietNan](auto& value) {
                 value.parameters.parameters[0].range->step = quietNan;
             }},
            {"zero range step", [](auto& value) {
                 value.parameters.parameters[0].range->step = 0.0f;
             }},
            {"default outside range", [](auto& value) {
                 value.parameters.parameters[0].defaultValue = 2.0f;
             }},
            {"options and source", [](auto& value) {
                 value.parameters.parameters[0].optionSource =
                     synaptome::element::ParameterOptionSource{
                         "tests.options",
                         "value",
                         "label",
                     };
             }},
            {"option kind mismatch", [](auto& value) {
                 value.parameters.parameters[0].options[0].value = true;
             }},
            {"empty option label", [](auto& value) {
                 value.parameters.parameters[0].options[0].label.clear();
             }},
            {"non-finite option", [infinity](auto& value) {
                 value.parameters.parameters[0].options[0].value = infinity;
             }},
            {"option outside range", [](auto& value) {
                 value.parameters.parameters[0].options[0].value = 2.0f;
             }},
            {"duplicate option value", [](auto& value) {
                 value.parameters.parameters[0].options[1].value =
                     value.parameters.parameters[0].options[0].value;
             }},
            {"invalid option source ID", [](auto& value) {
                 auto& parameter = value.parameters.parameters[0];
                 parameter.options.clear();
                 parameter.optionSource =
                     synaptome::element::ParameterOptionSource{
                         "Invalid_source",
                         "value",
                         "label",
                     };
             }},
            {"empty option value field", [](auto& value) {
                 auto& parameter = value.parameters.parameters[0];
                 parameter.options.clear();
                 parameter.optionSource =
                     synaptome::element::ParameterOptionSource{
                         "tests.options",
                         {},
                         "label",
                     };
             }},
            {"empty option label field", [](auto& value) {
                 auto& parameter = value.parameters.parameters[0];
                 parameter.options.clear();
                 parameter.optionSource =
                     synaptome::element::ParameterOptionSource{
                         "tests.options",
                         "value",
                         {},
                     };
             }},
            {"negative quick-access order", [](auto& value) {
                 value.parameters.parameters[0].quickAccessOrder = -1;
             }},
            {"duplicate quick-access order", [](auto& value) {
                 value.parameters.parameters[1].quickAccessOrder =
                     value.parameters.parameters[0].quickAccessOrder;
             }},
            {"invalid alias", [](auto& value) {
                 value.parameters.parameters[0].aliases = {"Invalid_alias"};
             }},
            {"reserved active alias", [](auto& value) {
                 value.parameters.parameters[0].aliases = {"active"};
             }},
            {"reserved opacity alias", [](auto& value) {
                 value.parameters.parameters[0].aliases = {"opacity"};
             }},
            {"colliding alias", [](auto& value) {
                 value.parameters.parameters[0].aliases = {"gate"};
             }},
            {"duplicate alias", [](auto& value) {
                 value.parameters.parameters[0].aliases = {
                     "formerAmount",
                     "formerAmount",
                 };
             }},
            {"deprecation without reason", [](auto& value) {
                 value.parameters.parameters[1].deprecation->reason.clear();
             }},
            {"invalid replacement ID", [](auto& value) {
                 value.parameters.parameters[1].deprecation->replacementId =
                     "Invalid_replacement";
             }},
            {"undeclared replacement", [](auto& value) {
                 value.parameters.parameters[1].deprecation->replacementId =
                     "missing";
             }},
            {"replacement kind mismatch", [](auto& value) {
                 value.parameters.parameters[1].deprecation->replacementId =
                     "amount";
             }},
            {"self replacement", [](auto& value) {
                 value.parameters.parameters[1].deprecation->replacementId =
                     "gate";
             }},
            {"reserved replacement", [](auto& value) {
                 value.parameters.parameters[1].deprecation->replacementId =
                     "opacity";
             }},
        };

    const auto* stableLegacyContract =
        factory.typeContract("tests.runtime.descriptor.valid");
    const std::size_t stableContractCount =
        factory.typeContracts().size();
    int invalidCreatorCalls = 0;
    for (std::size_t index = 0;
         index < invalidDeclarations.size();
         ++index) {
        const std::string typeId =
            "tests.runtime.parameters.invalid" +
            std::to_string(index);
        auto contract = validParameterContract(typeId);
        invalidDeclarations[index].second(contract);
        bool rejected = false;
        try {
            factory.registerType(
                std::move(contract),
                [&] {
                    ++invalidCreatorCalls;
                    return std::make_unique<EmptyElement>();
                });
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(
            rejected &&
                !factory.contains(typeId) &&
                factory.typeContract(typeId) == nullptr &&
                factory.typeContracts().size() == stableContractCount &&
                factory.typeContract("tests.runtime.descriptor.valid") ==
                    stableLegacyContract &&
                factory.typeContract("tests.runtime.parameters.declared") ==
                    stableDeclaredContract &&
                invalidCreatorCalls == 0,
            "invalid parameter declaration was not rejected atomically: " +
                invalidDeclarations[index].first);
    }
}

void RunScopedElementTypeRegistryIsolationScenario() {
    // Runtime receives its type registry by reference, so independently
    // constructed registries must never share or fall back to global state.
    LayerFactory registryA;
    LayerFactory registryB;
    constexpr const char* kSharedType = "tests.runtime.scoped.shared";
    constexpr const char* kOnlyInAType = "tests.runtime.scoped.onlyA";

    registryA.registerType(visualDescriptor(kSharedType), [] {
        return std::make_unique<ScopedRegistryElementA>();
    });
    registryB.registerType(visualDescriptor(kSharedType), [] {
        return std::make_unique<ScopedRegistryElementB>();
    });
    registryA.registerType(visualDescriptor(kOnlyInAType), [] {
        return std::make_unique<ScopedRegistryElementA>();
    });
    int lookupConstructionCount = 0;
    registryA.registerType(
        visualDescriptor(
            "tests.runtime.scoped.lookupOnly",
            {
                {
                    "lookup.first",
                    "Lookup First",
                    "lookup",
                    "Pins static descriptor action order.",
                },
                {
                    "lookup.second",
                    "Lookup Second",
                    "lookup",
                    "Pins static descriptor action order.",
                },
            }),
        [&] {
            ++lookupConstructionCount;
            return std::make_unique<ScopedRegistryElementA>();
        });

    const auto* lookupDescriptor =
        registryA.descriptor("tests.runtime.scoped.lookupOnly");
    auto descriptorCopies = registryA.descriptors();
    require(
        lookupDescriptor &&
            lookupDescriptor->typeId ==
                "tests.runtime.scoped.lookupOnly" &&
            lookupDescriptor->kind ==
                synaptome::element::ElementKind::Visual &&
            lookupDescriptor->actions.size() == 2 &&
            lookupDescriptor->actions[0].id == "lookup.first" &&
            lookupDescriptor->actions[1].id == "lookup.second" &&
            descriptorCopies.size() == 3 &&
            descriptorCopies[0].typeId == kSharedType &&
            descriptorCopies[1].typeId == kOnlyInAType &&
            descriptorCopies[2].typeId ==
                "tests.runtime.scoped.lookupOnly" &&
            lookupConstructionCount == 0,
        "descriptor lookup constructed an element or lost static/deterministic order");
    descriptorCopies[2].typeId = "copy.mutated";
    descriptorCopies[2].actions[0].id = "copy.mutated";
    require(
        registryA.descriptor("tests.runtime.scoped.lookupOnly")->typeId ==
                "tests.runtime.scoped.lookupOnly" &&
            registryA.descriptor(
                "tests.runtime.scoped.lookupOnly")->actions[0].id ==
                "lookup.first" &&
            lookupConstructionCount == 0,
        "mutating enumerated descriptor copies changed the registry");

    ParameterRegistry parametersA;
    ParameterRegistry parametersB;
    synaptome::runtime::Runtime runtimeA(registryA, parametersA);
    synaptome::runtime::Runtime runtimeB(registryB, parametersB);
    require(
        runtimeA.hasElementType("tests.runtime.scoped.lookupOnly") &&
            !runtimeB.hasElementType("tests.runtime.scoped.lookupOnly") &&
            lookupConstructionCount == 0,
        "scoped type lookup constructed an element or leaked across runtimes");

    synaptome::runtime::Runtime::ElementRequest request;
    request.typeId = kSharedType;
    request.definitionId = "tests.definition.scoped.shared";
    request.instanceId = "tests.instance.scoped.a";
    request.registryPrefix = "console.layer1";
    auto preparedA = runtimeA.prepareElement(request);
    require(
        preparedA &&
            dynamic_cast<ScopedRegistryElementA*>(preparedA.element()) !=
                nullptr,
        "runtime A did not resolve its scoped shared type");

    request.instanceId = "tests.instance.scoped.b";
    auto preparedB = runtimeB.prepareElement(request);
    require(
        preparedB &&
            dynamic_cast<ScopedRegistryElementB*>(preparedB.element()) !=
                nullptr,
        "runtime B leaked runtime A's shared type registration");

    request.typeId = kOnlyInAType;
    request.definitionId = "tests.definition.scoped.only-a";
    request.instanceId = "tests.instance.scoped.only-a";
    request.registryPrefix = "console.layer2";
    auto onlyInA = runtimeA.prepareElement(request);
    require(onlyInA &&
                dynamic_cast<ScopedRegistryElementA*>(onlyInA.element()) !=
                    nullptr,
            "runtime A could not resolve its private type registration");

    request.instanceId = "tests.instance.scoped.missing-in-b";
    auto missingInB = runtimeB.prepareElement(request);
    require(
        !missingInB &&
            missingInB.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    TypeNotRegistered &&
            missingInB.stage == "descriptor",
        "runtime B observed a type registered only in runtime A's scope");
}

void RunEffectCoverageWindowScenario(
    const synaptome::runtime::Runtime& runtime) {
    auto expectWindow = [&](std::size_t effectLayerIndex,
                            float coverage,
                            std::size_t expectedFirst,
                            std::size_t expectedEnd,
                            int expectedRequested,
                            bool expectedAll) {
        const auto window =
            runtime.resolveEffectCoverage(effectLayerIndex, coverage);
        require(
            window.effectLayerIndex == effectLayerIndex &&
                window.firstInputLayerIndex == expectedFirst &&
                window.inputEndLayerIndex == expectedEnd &&
                window.requestedLayers == expectedRequested &&
                window.includesAllPrior == expectedAll,
            "runtime effect coverage window did not match the requested range");
        return window;
    };

    expectWindow(3, 0.0f, 0, 3, 0, true);
    expectWindow(3, 1.0f, 2, 3, 1, false);
    const auto nearestTwo =
        expectWindow(4, 2.0f, 2, 4, 2, false);
    expectWindow(4, 10.0f, 0, 4, 10, true);
    expectWindow(4, 2.9f, 2, 4, 2, false);
    expectWindow(0, 2.0f, 0, 0, 2, true);
    expectWindow(2, -1.0f, 0, 2, 0, true);

    require(
        nearestTwo.contains(2) &&
            nearestTwo.contains(3) &&
            !nearestTwo.contains(1) &&
            !nearestTwo.contains(4),
        "runtime effect coverage window boundaries were not half-open");

    const auto invalid = runtime.resolveEffectCoverage(
        synaptome::runtime::kCompositionLayerCount,
        2.0f);
    require(
        invalid.firstInputLayerIndex == 0 &&
            invalid.inputEndLayerIndex == 0 &&
            invalid.requestedLayers == 0 &&
            !invalid.includesAllPrior &&
            !invalid.contains(0),
        "runtime effect coverage accepted an out-of-range effect layer");
}

void RunCompositionSnapshotScenario() {
    LayerFactory factory;
    int constructionCount = 0;
    factory.registerType(visualDescriptor("tests.runtime.snapshot"), [&] {
        ++constructionCount;
        return std::make_unique<ContractElement>();
    });
    ParameterRegistry parameters;
    synaptome::runtime::Runtime runtime(factory, parameters);

    auto emptySnapshot = runtime.compositionSnapshot();
    require(
        emptySnapshot.layers.size() ==
            synaptome::runtime::kCompositionLayerCount &&
            constructionCount == 0,
        "empty composition snapshot constructed an element or changed capacity");
    for (std::size_t i = 0; i < emptySnapshot.layers.size(); ++i) {
        const auto& layer = emptySnapshot.layers[i];
        require(
            layer.zeroBasedIndex == i &&
                !layer.occupied &&
                !layer.hasElement &&
                layer.kind == synaptome::runtime::CompositionKind::Element &&
                layer.definitionId.empty() &&
                layer.label.empty() &&
                layer.typeId.empty() &&
                layer.registryPrefix.empty() &&
                !layer.active &&
                std::fabs(layer.opacity - 1.0f) < 0.0001f &&
                !layer.coverage.defined &&
                layer.coverage.mode == "upstream" &&
                layer.coverage.columns == 0,
            "empty composition snapshot did not describe an empty layer");
    }
    require(
        runtime.compositionLayerSnapshot(0).has_value() &&
            !runtime.compositionLayerSnapshot(
                synaptome::runtime::kCompositionLayerCount).has_value() &&
            !runtime.compositionLayerSnapshot(
                synaptome::runtime::kCompositionLayerCount + 100).has_value() &&
            constructionCount == 0,
        "single-layer snapshot bounds check constructed or returned a layer");

    synaptome::runtime::Runtime::ElementRequest elementRequest;
    elementRequest.typeId = "tests.runtime.snapshot";
    elementRequest.definitionId = "tests.definition.snapshot";
    elementRequest.instanceId = "tests.instance.snapshot";
    elementRequest.registryPrefix = "console.layer1";
    elementRequest.enabled = true;
    auto prepared = runtime.prepareElement(elementRequest);
    require(
        runtime.adoptPreparedElement(
            0,
            std::move(prepared),
            assignmentFor(
                elementRequest,
                "Snapshot Element",
                0.37f)),
        "snapshot scenario did not adopt its element");

    synaptome::runtime::CompositionAssignment effect;
    effect.kind = synaptome::runtime::CompositionKind::Effect;
    effect.definitionId = "tests.effect.snapshot";
    effect.label = "Snapshot Effect";
    effect.typeId = "fx.snapshot";
    effect.registryPrefix = "effects.snapshot";
    effect.active = true;
    effect.opacity = 0.64f;
    effect.coverage.defined = true;
    effect.coverage.mode = "upstream";
    effect.coverage.columns = 2;
    require(
        runtime.assignCompositionEntry(1, effect),
        "snapshot scenario did not assign its effect");

    synaptome::runtime::CompositionAssignment overlay;
    overlay.kind = synaptome::runtime::CompositionKind::Overlay;
    overlay.definitionId = "tests.overlay.snapshot";
    overlay.label = "Snapshot Overlay";
    overlay.typeId = "ui.snapshot";
    overlay.registryPrefix = "ui.snapshot";
    overlay.active = false;
    overlay.opacity = 0.83f;
    require(
        runtime.assignCompositionEntry(2, overlay),
        "snapshot scenario did not assign its overlay");

    auto assignedSnapshot = runtime.compositionSnapshot();
    const auto& elementState = assignedSnapshot.layers[0];
    const auto& effectState = assignedSnapshot.layers[1];
    const auto& overlayState = assignedSnapshot.layers[2];
    require(
        elementState.occupied &&
            elementState.hasElement &&
            elementState.kind ==
                synaptome::runtime::CompositionKind::Element &&
            elementState.definitionId == elementRequest.definitionId &&
            elementState.label == "Snapshot Element" &&
            elementState.typeId == elementRequest.typeId &&
            elementState.registryPrefix == elementRequest.registryPrefix &&
            elementState.active &&
            std::fabs(elementState.opacity - 0.37f) < 0.0001f,
        "composition snapshot did not describe its element assignment");
    require(
        effectState.occupied &&
            !effectState.hasElement &&
            effectState.kind ==
                synaptome::runtime::CompositionKind::Effect &&
            effectState.definitionId == effect.definitionId &&
            effectState.typeId == effect.typeId &&
            effectState.registryPrefix == effect.registryPrefix &&
            effectState.active &&
            effectState.coverage.defined &&
            effectState.coverage.mode == "upstream" &&
            effectState.coverage.columns == 2,
        "composition snapshot did not describe its effect assignment");
    require(
        overlayState.occupied &&
            !overlayState.hasElement &&
            overlayState.kind ==
                synaptome::runtime::CompositionKind::Overlay &&
            overlayState.definitionId == overlay.definitionId &&
            overlayState.typeId == overlay.typeId &&
            overlayState.registryPrefix == overlay.registryPrefix &&
            !overlayState.active &&
            !overlayState.coverage.defined,
        "composition snapshot did not describe its overlay assignment");
    require(
        constructionCount == 1,
        "composition snapshot constructed an extra element");

    assignedSnapshot.layers[0].definitionId = "caller.mutated";
    assignedSnapshot.layers[0].label = "Caller Mutated";
    assignedSnapshot.layers[0].active = false;
    assignedSnapshot.layers[1].coverage.mode = "caller-mutated";
    const auto freshAfterCopyMutation = runtime.compositionSnapshot();
    require(
        freshAfterCopyMutation.layers[0].definitionId ==
                elementRequest.definitionId &&
            freshAfterCopyMutation.layers[0].label == "Snapshot Element" &&
            freshAfterCopyMutation.layers[0].active &&
            freshAfterCopyMutation.layers[1].coverage.mode == "upstream",
        "mutating a composition snapshot changed Runtime state");

    require(
        runtime.setCompositionLayerLabel(0, "Runtime Mutation") &&
            runtime.setCompositionLayerActive(0, false),
        "snapshot scenario Runtime mutation failed");
    synaptome::runtime::CompositionCoverage changedCoverage;
    changedCoverage.defined = true;
    changedCoverage.mode = "upstream";
    changedCoverage.columns = 5;
    require(
        runtime.setCompositionLayerCoverage(1, changedCoverage),
        "snapshot scenario coverage mutation failed");
    parameters.setFloatBase("console.layer1.opacity", 0.58f, true);

    const auto elementMutation = runtime.compositionLayerSnapshot(0);
    const auto effectMutation = runtime.compositionLayerSnapshot(1);
    require(
        elementMutation &&
            elementMutation->label == "Runtime Mutation" &&
            !elementMutation->active &&
            std::fabs(elementMutation->opacity - 0.58f) < 0.0001f &&
            effectMutation &&
            effectMutation->coverage.columns == 5,
        "composition snapshots did not reflect Runtime or parameter mutation");

    require(
        runtime.clearCompositionLayer(0) &&
            runtime.clearCompositionLayer(1) &&
            runtime.clearCompositionLayer(2),
        "snapshot scenario clear failed");
    const auto clearedSnapshot = runtime.compositionSnapshot();
    for (std::size_t i = 0; i < 3; ++i) {
        const auto& layer = clearedSnapshot.layers[i];
        require(
            !layer.occupied &&
                !layer.hasElement &&
                layer.kind == synaptome::runtime::CompositionKind::Element &&
                layer.definitionId.empty() &&
                layer.typeId.empty() &&
                layer.registryPrefix.empty() &&
                !layer.active &&
                std::fabs(layer.opacity - 1.0f) < 0.0001f &&
                !layer.coverage.defined &&
                layer.coverage.mode == "upstream" &&
                layer.coverage.columns == 0,
            "composition snapshot did not reflect a cleared assignment");
    }
    require(
        runtime.clearCompositionLayer(0) &&
            constructionCount == 1,
        "idempotent clear or snapshot query constructed an element");
}

void RunCompositionSlotReplacementScenario() {
    LayerFactory factory;
    SlotReplacementLifetimeState lifetime;
    factory.registerType(
        visualDescriptor("tests.runtime.slotReplacement"),
        [&] {
        const int serial = lifetime.constructionCount++;
        return std::make_unique<SlotReplacementLifetimeElement>(
            lifetime,
            serial);
    });
    ParameterRegistry parameters;
    synaptome::runtime::Runtime runtime(factory, parameters);

    synaptome::runtime::Runtime::ElementRequest request;
    request.typeId = "tests.runtime.slotReplacement";
    request.definitionId = "tests.definition.slot-initial";
    request.instanceId = "tests.instance.slot-initial";
    request.registryPrefix = "console.layer1";
    request.enabled = true;

    const auto outOfBounds =
        runtime.prepareCompositionElementReplacement(
            synaptome::runtime::kCompositionLayerCount,
            request);
    const auto empty =
        runtime.prepareCompositionElementReplacement(0, request);
    require(
        !outOfBounds &&
            outOfBounds.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    InvalidRequest &&
            outOfBounds.stage == "validate" &&
            outOfBounds.typeId == request.typeId &&
            outOfBounds.definitionId == request.definitionId &&
            outOfBounds.instanceId == request.instanceId &&
            outOfBounds.registryPrefix == request.registryPrefix &&
            !empty &&
            empty.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    InvalidRequest &&
            empty.stage == "validate" &&
            lifetime.constructionCount == 0,
        "slot replacement bounds or empty-slot validation constructed an element");

    synaptome::runtime::CompositionAssignment nonElement;
    nonElement.kind = synaptome::runtime::CompositionKind::Effect;
    nonElement.definitionId = "tests.effect.slot-replacement";
    nonElement.label = "Replacement Effect";
    nonElement.typeId = "fx.slot-replacement";
    nonElement.registryPrefix = "effects.slot-replacement";
    nonElement.active = true;
    require(runtime.assignCompositionEntry(1, nonElement),
            "slot replacement scenario could not assign its effect");
    auto effectReplacement =
        runtime.prepareCompositionElementReplacement(1, request);
    require(
        !effectReplacement &&
            effectReplacement.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    InvalidRequest &&
            lifetime.constructionCount == 0,
        "effect slot replacement constructed an element");
    require(runtime.clearCompositionLayer(1),
            "slot replacement scenario could not clear its effect");

    nonElement.kind = synaptome::runtime::CompositionKind::Overlay;
    nonElement.definitionId = "tests.overlay.slot-replacement";
    nonElement.typeId = "ui.slot-replacement";
    nonElement.registryPrefix = "ui.slot-replacement";
    require(runtime.assignCompositionEntry(1, nonElement),
            "slot replacement scenario could not assign its overlay");
    auto overlayReplacement =
        runtime.prepareCompositionElementReplacement(1, request);
    require(
        !overlayReplacement &&
            overlayReplacement.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    InvalidRequest &&
            lifetime.constructionCount == 0,
        "overlay slot replacement constructed an element");
    require(runtime.clearCompositionLayer(1),
            "slot replacement scenario could not clear its overlay");

    auto initial = runtime.prepareElement(request);
    require(initial && lifetime.constructionCount == 1,
            "slot replacement scenario did not prepare its initial element");
    require(
        runtime.adoptPreparedElement(
            0,
            std::move(initial),
            assignmentFor(request, "Slot Initial", 0.41f)),
        "slot replacement scenario did not adopt its initial element");
    require(
        telemetryValue<std::int64_t>(
            runtime,
            0,
            "tests.serial") == 0,
        "initial slot telemetry did not identify the live element");
    const auto* initialParam =
        parameters.findFloat("console.layer1.value");
    require(
        initialParam && initialParam->value,
        "slot replacement scenario did not publish initial parameter storage");
    float* const initialValueStorage = initialParam->value;
    const ParameterRegistry::Range initialRange = initialParam->meta.range;

    auto wrongPrefixRequest = request;
    wrongPrefixRequest.registryPrefix = "console.layer2";
    auto wrongPrefix =
        runtime.prepareCompositionElementReplacement(
            0,
            wrongPrefixRequest);
    require(
        !wrongPrefix &&
            wrongPrefix.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    InvalidRequest &&
            lifetime.constructionCount == 1 &&
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.serial") == 0,
        "slot replacement accepted a mismatched prefix or changed the live element");

    {
        auto abandoned =
            runtime.prepareCompositionElementReplacement(0, request);
        require(
            abandoned &&
                lifetime.constructionCount == 2 &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.serial") == 0,
            "slot replacement preparation changed the live element");
    }
    require(
        lifetime.destroyed[1] &&
            !lifetime.destroyed[0] &&
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.serial") == 0,
        "aborting slot replacement damaged the live element or leaked its candidate");

    request.definitionId = "tests.definition.slot-replacement";
    request.instanceId = "tests.instance.slot-replacement";
    std::vector<std::string> progress;
    {
        auto prepared =
            runtime.prepareCompositionElementReplacement(
                0,
                request,
                [&](std::string_view step) {
                    progress.emplace_back(step);
                });
        require(
            prepared &&
                lifetime.constructionCount == 3 &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.serial") == 0 &&
                parameters.findFloat("console.layer1.value") != nullptr,
            "slot replacement preparation published candidate state");
        const auto wrongSlotAdoption = runtime.adoptPreparedElement(
            1,
            std::move(prepared),
            assignmentFor(request, "Slot Replacement", 0.67f));
        require(
            !wrongSlotAdoption &&
                prepared &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.serial") == 0,
            "cross-slot adoption consumed or replaced the prepared candidate");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(prepared),
                assignmentFor(request, "Slot Replacement", 0.67f)),
            "slot-based replacement did not commit");
        const auto* retired =
            dynamic_cast<const SlotReplacementLifetimeElement*>(
                prepared.element());
        const auto* replacementParam =
            parameters.findFloat("console.layer1.value");
        require(
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.serial") == 2 &&
                retired &&
                retired->serial() == 0 &&
                !lifetime.destroyed[0] &&
                replacementParam &&
                replacementParam->value &&
                replacementParam->value != initialValueStorage &&
                std::fabs(*initialValueStorage - 0.25f) < 0.0001f &&
                std::fabs(initialRange.min - 0.0f) < 0.0001f &&
                std::fabs(initialRange.max - 1.0f) < 0.0001f &&
                std::fabs(initialRange.step - 0.05f) < 0.0001f &&
                parameters.findFloat("console.layer1.opacity") != nullptr,
            "slot adoption did not replace registry storage while retaining the retired element");
    }
    require(
        lifetime.destroyed[0] &&
            progress ==
                std::vector<std::string>(
                    {"create", "configure", "setup", "enable"}),
        "slot replacement retirement or lifecycle progress ordering drifted");

    request.definitionId = "tests.definition.slot-stale";
    request.instanceId = "tests.instance.slot-stale";
    {
        const auto staleRequest = request;
        auto stale =
            runtime.prepareCompositionElementReplacement(
                0,
                staleRequest);
        request.definitionId = "tests.definition.slot-winner";
        request.instanceId = "tests.instance.slot-winner";
        auto winner =
            runtime.prepareCompositionElementReplacement(0, request);
        require(stale && winner && lifetime.constructionCount == 5,
                "parallel slot replacement preparation failed");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(winner),
                assignmentFor(request, "Slot Winner", 0.79f)),
            "winning slot replacement did not commit");
        require(
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.serial") == 4,
            "winning replacement telemetry was not published");
        const auto staleCommit = runtime.adoptPreparedElement(
            0,
            std::move(stale),
            assignmentFor(
                staleRequest,
                "Slot Stale",
                0.79f));
        require(
            !staleCommit &&
                staleCommit.errorCode ==
                    synaptome::runtime::CompositionMutationError::
                        ElementMismatch &&
                staleCommit.error ==
                    "prepared replacement generation is stale" &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.serial") == 4 &&
                !lifetime.destroyed[2],
            "stale slot replacement changed the winning live element");
        runtime.releasePreparedElement(stale);
        require(
            lifetime.destroyed[3] &&
                !lifetime.destroyed[2],
            "stale candidate abort destroyed the winner's retired element");
    }
    require(
        lifetime.destroyed[2],
        "winning replacement did not retain its retired element guard");

    request.definitionId = "tests.definition.slot-clear-stale";
    request.instanceId = "tests.instance.slot-clear-stale";
    auto clearStale =
        runtime.prepareCompositionElementReplacement(0, request);
    require(
        clearStale &&
            lifetime.constructionCount == 6 &&
            runtime.clearCompositionLayer(0) &&
            parameters.findFloat("console.layer1.value") == nullptr,
        "clear-stale replacement setup failed");
    const auto clearStaleCommit = runtime.adoptPreparedElement(
        0,
        std::move(clearStale),
        assignmentFor(
            request,
            "Slot Clear Stale",
            0.79f));
    require(
        !clearStaleCommit &&
            clearStaleCommit.errorCode ==
                synaptome::runtime::CompositionMutationError::
                    ElementMismatch &&
            clearStaleCommit.error ==
                "prepared replacement generation is stale" &&
            !runtime.compositionLayerSnapshot(0)->occupied,
        "cleared slot accepted a stale replacement token");
    runtime.releasePreparedElement(clearStale);
    require(
        lifetime.destroyed[4] &&
            lifetime.destroyed[5],
        "clear-stale replacement leaked its live or candidate element");

    synaptome::runtime::Runtime::ElementResult outlivesRuntime;
    ParameterRegistry expiryParameters;
    {
        synaptome::runtime::Runtime expiryRuntime(
            factory,
            expiryParameters);
        auto expiryRequest = request;
        expiryRequest.definitionId =
            "tests.definition.slot-expiry-live";
        expiryRequest.instanceId =
            "tests.instance.slot-expiry-live";
        auto expiryInitial =
            expiryRuntime.prepareElement(expiryRequest);
        require(
            expiryRuntime.adoptPreparedElement(
                0,
                std::move(expiryInitial),
                assignmentFor(
                    expiryRequest,
                    "Slot Expiry Live")),
            "Runtime-expiry scenario did not adopt its live element");
        expiryRequest.definitionId =
            "tests.definition.slot-expiry-candidate";
        expiryRequest.instanceId =
            "tests.instance.slot-expiry-candidate";
        outlivesRuntime =
            expiryRuntime.prepareCompositionElementReplacement(
                0,
                expiryRequest);
        require(
            outlivesRuntime &&
                lifetime.constructionCount == 8 &&
                !lifetime.destroyed[6] &&
                !lifetime.destroyed[7],
            "Runtime-expiry replacement preparation failed");
    }
    require(
        lifetime.destroyed[6] &&
            !lifetime.destroyed[7] &&
            static_cast<bool>(outlivesRuntime),
        "Runtime expiry destroyed or invalidated its prepared replacement");
    outlivesRuntime =
        synaptome::runtime::Runtime::ElementResult{};
    require(
        lifetime.destroyed[7],
        "expired Runtime replacement did not release its candidate safely");
}

void RunCompositionActionScenario() {
    LayerFactory factory;
    ActionContractState state;
    factory.registerType(
        visualDescriptor(
            "tests.runtime.actions.v1",
            actionContractDescriptors(false)),
        [&] {
        const int serial = state.constructionCount++;
        return std::make_unique<ActionContractElement>(
            state,
            serial,
            false);
    });
    factory.registerType(
        visualDescriptor(
            "tests.runtime.actions.v2",
            actionContractDescriptors(true)),
        [&] {
        const int serial = state.constructionCount++;
        return std::make_unique<ActionContractElement>(
            state,
            serial,
            true);
    });
    const std::vector<synaptome::element::ActionDescriptor>
        oneDeclaredAction = {
            {
                "valid.action",
                "Valid Action",
                "tests",
                "Contract fixture.",
            },
        };
    auto registerInvalidBindingType = [&](
        std::string typeId,
        InvalidActionMode mode) {
        factory.registerType(
            visualDescriptor(std::move(typeId), oneDeclaredAction),
            [mode] {
                return std::make_unique<InvalidActionElement>(mode);
            });
    };
    registerInvalidBindingType(
        "tests.runtime.actions.unknownBinding",
        InvalidActionMode::UnknownBinding);
    registerInvalidBindingType(
        "tests.runtime.actions.duplicateBinding",
        InvalidActionMode::DuplicateBinding);
    registerInvalidBindingType(
        "tests.runtime.actions.emptyHandler",
        InvalidActionMode::EmptyHandler);
    registerInvalidBindingType(
        "tests.runtime.actions.missingBinding",
        InvalidActionMode::MissingBinding);
    registerInvalidBindingType(
        "tests.runtime.actions.throwRegister",
        InvalidActionMode::ThrowDuringRegistration);
    int effectConstructionCount = 0;
    factory.registerType(
        effectDescriptor("tests.runtime.actions.effect"),
        [&] {
            ++effectConstructionCount;
            return std::make_unique<EmptyElement>();
        });

    LayerFactory invalidDeclarationFactory;
    int invalidDeclarationConstructionCount = 0;
    auto rejectsStaticDeclaration = [&](
        synaptome::element::ElementDescriptor descriptor) {
        const auto typeId = descriptor.typeId;
        try {
            invalidDeclarationFactory.registerType(
                std::move(descriptor),
                [&] {
                    ++invalidDeclarationConstructionCount;
                    return std::make_unique<EmptyElement>();
                });
        } catch (const std::exception&) {
            return !invalidDeclarationFactory.contains(typeId);
        }
        return false;
    };
    require(
        rejectsStaticDeclaration(visualDescriptor(
            "tests.runtime.actions.staticInvalidStart",
            {{
                "Invalid.action",
                "Invalid Start",
                "tests",
                "Static declaration fixture.",
            }})) &&
            rejectsStaticDeclaration(visualDescriptor(
                "tests.runtime.actions.staticInvalidUnderscore",
                {{
                    "invalid_action",
                    "Invalid Underscore",
                    "tests",
                    "Static declaration fixture.",
                }})) &&
            rejectsStaticDeclaration(visualDescriptor(
                "tests.runtime.actions.staticInvalidTrailingDot",
                {{
                    "invalid.",
                    "Invalid Trailing Dot",
                    "tests",
                    "Static declaration fixture.",
                }})) &&
            rejectsStaticDeclaration(visualDescriptor(
                "tests.runtime.actions.staticEmptyLabel",
                {{
                    "valid.action",
                    "",
                    "tests",
                    "Static declaration fixture.",
                }})) &&
            rejectsStaticDeclaration(visualDescriptor(
                "tests.runtime.actions.staticInvalidGroup",
                {{
                    "valid.action",
                    "Invalid Group",
                    "Invalid Group",
                    "Static declaration fixture.",
                }})) &&
            rejectsStaticDeclaration(visualDescriptor(
                "tests.runtime.actions.staticDuplicate",
                {
                    {
                        "valid.action",
                        "First",
                        "tests",
                        "Static declaration fixture.",
                    },
                    {
                        "valid.action",
                        "Second",
                        "tests",
                        "Static declaration fixture.",
                    },
                })) &&
            invalidDeclarationConstructionCount == 0,
        "invalid static action declarations were accepted or constructed an element");

    ParameterRegistry parameters;
    synaptome::runtime::Runtime runtime(factory, parameters);

    synaptome::runtime::Runtime::ElementRequest invalidRequest;
    invalidRequest.definitionId = "tests.definition.actions.invalid";
    invalidRequest.instanceId = "tests.instance.actions.invalid";
    invalidRequest.registryPrefix = "console.layer1";
    for (const std::string typeId : {
             "tests.runtime.actions.unknownBinding",
             "tests.runtime.actions.duplicateBinding",
             "tests.runtime.actions.emptyHandler",
             "tests.runtime.actions.missingBinding",
         }) {
        invalidRequest.typeId = typeId;
        const auto invalid = runtime.prepareElement(invalidRequest);
        require(
            !invalid &&
                invalid.errorCode ==
                    synaptome::runtime::Runtime::ElementErrorCode::
                        ContractViolation &&
                invalid.stage == "actions" &&
                parameters.findFloat("console.layer1.value") == nullptr,
            "invalid live action binding escaped staging or used the wrong error");
    }
    invalidRequest.typeId = "tests.runtime.actions.throwRegister";
    const auto throwingRegistration =
        runtime.prepareElement(invalidRequest);
    require(
        !throwingRegistration &&
            throwingRegistration.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    LifecycleFailure &&
            throwingRegistration.stage == "actions" &&
            parameters.findFloat("console.layer1.value") == nullptr,
        "throwing action registration escaped Runtime lifecycle containment");

    invalidRequest.typeId = "tests.runtime.actions.effect";
    const auto effectPreparation = runtime.prepareElement(invalidRequest);
    require(
        !effectPreparation &&
            effectPreparation.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    ContractViolation &&
            effectPreparation.stage == "descriptor" &&
            effectConstructionCount == 0 &&
            parameters.findFloat("console.layer1.value") == nullptr,
        "effect descriptor reached the visual element creator or wrong failure stage");

    const auto outOfRange = runtime.invokeCompositionAction(
        synaptome::runtime::kCompositionLayerCount,
        "version.first");
    const auto empty =
        runtime.invokeCompositionAction(0, "version.first");
    require(
        !outOfRange &&
            outOfRange.errorCode ==
                synaptome::runtime::CompositionActionError::
                    IndexOutOfRange &&
            outOfRange.actionId == "version.first" &&
            !empty &&
            empty.errorCode ==
                synaptome::runtime::CompositionActionError::SlotEmpty,
        "action invocation bounds or empty-slot errors drifted");

    synaptome::runtime::CompositionAssignment nonElement;
    nonElement.kind = synaptome::runtime::CompositionKind::Effect;
    nonElement.definitionId = "tests.effect.actions";
    nonElement.label = "Action Effect";
    nonElement.typeId = "fx.actions";
    nonElement.registryPrefix = "effects.actions";
    nonElement.active = true;
    require(runtime.assignCompositionEntry(4, nonElement),
            "action scenario could not assign its effect");
    const auto effectAction =
        runtime.invokeCompositionAction(4, "version.first");
    require(
        !effectAction &&
            effectAction.errorCode ==
                synaptome::runtime::CompositionActionError::KindMismatch,
        "effect assignment accepted an Element action");
    require(runtime.clearCompositionLayer(4),
            "action scenario could not clear its effect");
    nonElement.kind = synaptome::runtime::CompositionKind::Overlay;
    nonElement.definitionId = "tests.overlay.actions";
    nonElement.typeId = "ui.actions";
    nonElement.registryPrefix = "ui.actions";
    require(runtime.assignCompositionEntry(4, nonElement),
            "action scenario could not assign its overlay");
    const auto overlayAction =
        runtime.invokeCompositionAction(4, "version.first");
    require(
        !overlayAction &&
            overlayAction.errorCode ==
                synaptome::runtime::CompositionActionError::KindMismatch,
        "overlay assignment accepted an Element action");
    require(runtime.clearCompositionLayer(4),
            "action scenario could not clear its overlay");

    synaptome::runtime::Runtime::ElementRequest firstRequest;
    firstRequest.typeId = "tests.runtime.actions.v1";
    firstRequest.definitionId = "tests.definition.actions.first";
    firstRequest.instanceId = "tests.instance.actions.first";
    firstRequest.registryPrefix = "console.layer1";
    firstRequest.enabled = false;
    auto firstPrepared = runtime.prepareElement(firstRequest);
    require(
        firstPrepared &&
            runtime.compositionLayerSnapshot(0)->actions.empty(),
        "prepared actions became discoverable before adoption");
    require(
        runtime.adoptPreparedElement(
            0,
            std::move(firstPrepared),
            assignmentFor(firstRequest, "Action First")),
        "action scenario did not adopt its first element");

    const std::vector<std::string> expectedFirstIds = {
        "version.first",
        "execution.reject",
        "execution.fail",
        "render.reset2D",
        "execution.throw",
    };
    const auto firstSnapshot = runtime.compositionLayerSnapshot(0);
    const auto* firstTypeDescriptor =
        factory.descriptor("tests.runtime.actions.v1");
    require(
        firstSnapshot &&
            firstTypeDescriptor &&
            !firstSnapshot->active &&
            firstSnapshot->actions.size() == expectedFirstIds.size() &&
            firstSnapshot->actions.size() ==
                firstTypeDescriptor->actions.size(),
        "action snapshot did not publish the inactive element declarations");
    require(
        firstSnapshot->actions.front().label == "First Version" &&
            firstSnapshot->actions.front().groupId == "version" &&
            firstSnapshot->actions.front().description ==
                "Proves replacement publishes a new action table.",
        "action snapshot did not copy complete descriptor metadata");
    for (std::size_t i = 0; i < expectedFirstIds.size(); ++i) {
        require(
            firstSnapshot->actions[i].id == expectedFirstIds[i] &&
                sameActionDescriptor(
                    firstSnapshot->actions[i],
                    firstTypeDescriptor->actions[i]),
            "action snapshot drifted from static metadata or order");
    }
    auto mutatedCopy = *firstSnapshot;
    mutatedCopy.actions.front().id = "copy.mutated";
    require(
        runtime.compositionLayerSnapshot(0)->actions.front().id ==
            "version.first",
        "mutating a copied action descriptor changed Runtime state");

    auto invalidReplacementRequest = firstRequest;
    invalidReplacementRequest.typeId =
        "tests.runtime.actions.missingBinding";
    invalidReplacementRequest.definitionId =
        "tests.definition.actions.invalid-replacement";
    invalidReplacementRequest.instanceId =
        "tests.instance.actions.invalid-replacement";
    const auto invalidReplacement =
        runtime.prepareCompositionElementReplacement(
            0,
            invalidReplacementRequest);
    require(
        !invalidReplacement &&
            invalidReplacement.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    ContractViolation &&
            invalidReplacement.stage == "actions" &&
            runtime.compositionLayerSnapshot(0)->actions.size() ==
                firstTypeDescriptor->actions.size() &&
            sameActionDescriptor(
                runtime.compositionLayerSnapshot(0)->actions.front(),
                firstTypeDescriptor->actions.front()),
        "failed action replacement changed the live declaration table");

    const auto success =
        runtime.invokeCompositionAction(0, "version.first");
    const auto rejected =
        runtime.invokeCompositionAction(0, "execution.reject");
    const auto failed =
        runtime.invokeCompositionAction(0, "execution.fail");
    const auto threw =
        runtime.invokeCompositionAction(0, "execution.throw");
    const auto missing =
        runtime.invokeCompositionAction(0, "version.missing");
    const auto usableAfterThrow =
        runtime.invokeCompositionAction(0, "version.first");
    require(
        success &&
            usableAfterThrow &&
            state.successCount[0] == 2 &&
            !rejected &&
            rejected.errorCode ==
                synaptome::runtime::CompositionActionError::Rejected &&
            rejected.actionId == "execution.reject" &&
            rejected.error == "intentional rejection" &&
            !failed &&
            failed.errorCode ==
                synaptome::runtime::CompositionActionError::
                    ExecutionFailure &&
            failed.actionId == "execution.fail" &&
            failed.error == "intentional failure" &&
            !threw &&
            threw.errorCode ==
                synaptome::runtime::CompositionActionError::
                    ExecutionFailure &&
            threw.actionId == "execution.throw" &&
            threw.error.find("intentional action exception") !=
                std::string::npos &&
            !missing &&
            missing.errorCode ==
                synaptome::runtime::CompositionActionError::ActionNotFound &&
            missing.actionId == "version.missing",
        "action execution status translation or exception containment drifted");

    auto secondRequest = firstRequest;
    secondRequest.definitionId = "tests.definition.actions.sibling";
    secondRequest.instanceId = "tests.instance.actions.sibling";
    secondRequest.registryPrefix = "console.layer2";
    auto secondPrepared = runtime.prepareElement(secondRequest);
    require(
        runtime.adoptPreparedElement(
            1,
            std::move(secondPrepared),
            assignmentFor(secondRequest, "Action Sibling")),
        "action scenario did not adopt its sibling element");
    require(
        runtime.invokeCompositionAction(1, "version.first") &&
            state.successCount[0] == 2 &&
            state.successCount[1] == 1,
        "same local action ID did not remain scoped to its composition slot");

    auto replacementRequest = firstRequest;
    replacementRequest.typeId = "tests.runtime.actions.v2";
    replacementRequest.definitionId =
        "tests.definition.actions.replacement";
    replacementRequest.instanceId =
        "tests.instance.actions.replacement";
    {
        auto aborted =
            runtime.prepareCompositionElementReplacement(
                0,
                replacementRequest);
        require(
            aborted &&
                runtime.compositionLayerSnapshot(0)->actions.front().id ==
                    "version.first",
            "prepared replacement published candidate action declarations");
        runtime.releasePreparedElement(aborted);
        require(
            state.destroyed[2] &&
                state.handlersGoneBeforeElement[2] &&
                runtime.invokeCompositionAction(0, "version.first"),
            "aborted action candidate damaged the live table or lifetime order");
    }

    {
        auto replacement =
            runtime.prepareCompositionElementReplacement(
                0,
                replacementRequest);
        const auto retiredHandlerLifetime =
            state.handlerLifetimes[0];
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(replacement),
                assignmentFor(
                    replacementRequest,
                    "Action Replacement")),
            "action replacement did not commit");
        const auto replacedSnapshot =
            runtime.compositionLayerSnapshot(0);
        const auto retiredId =
            runtime.invokeCompositionAction(0, "version.first");
        require(
            replacedSnapshot &&
                replacedSnapshot->actions.front().id ==
                    "version.second" &&
                !retiredId &&
                retiredId.errorCode ==
                    synaptome::runtime::CompositionActionError::
                        ActionNotFound &&
                runtime.invokeCompositionAction(0, "version.second") &&
                !state.destroyed[0] &&
                !retiredHandlerLifetime.expired(),
            "action replacement did not atomically publish and retain tables");
    }
    require(
        state.destroyed[0] &&
            state.handlersGoneBeforeElement[0],
        "retired action handlers did not precede matching element destruction");

    const auto clearResult = runtime.clearCompositionLayer(0);
    const auto clearedAction =
        runtime.invokeCompositionAction(0, "version.second");
    require(
        clearResult &&
            runtime.compositionLayerSnapshot(0)->actions.empty() &&
            !clearedAction &&
            clearedAction.errorCode ==
                synaptome::runtime::CompositionActionError::SlotEmpty &&
            state.destroyed[3] &&
            state.handlersGoneBeforeElement[3],
        "clear retained action discovery, invocation, or handler lifetime");
    runtime.shutdownComposition();
    require(
        runtime.compositionLayerSnapshot(1)->actions.empty() &&
            state.destroyed[1] &&
            state.handlersGoneBeforeElement[1] &&
            state.constructionCount == 4,
        "shutdown retained a live action table or destroyed it out of order");

    ActionContractState expiryState;
    synaptome::runtime::Runtime::ElementResult outlivesRuntime;
    std::weak_ptr<int> expiryHandlerLifetime;
    LayerFactory expiryFactory;
    expiryFactory.registerType(
        visualDescriptor(
            "tests.runtime.actions.expiry",
            actionContractDescriptors(false)),
        [&] {
        const int serial = expiryState.constructionCount++;
        return std::make_unique<ActionContractElement>(
            expiryState,
            serial,
            false);
    });
    ParameterRegistry expiryParameters;
    {
        synaptome::runtime::Runtime expiryRuntime(
            expiryFactory,
            expiryParameters);
        auto expiryRequest = firstRequest;
        expiryRequest.typeId = "tests.runtime.actions.expiry";
        expiryRequest.definitionId =
            "tests.definition.actions.expiry";
        expiryRequest.instanceId =
            "tests.instance.actions.expiry";
        outlivesRuntime = expiryRuntime.prepareElement(expiryRequest);
        expiryHandlerLifetime = expiryState.handlerLifetimes[0];
        require(
            outlivesRuntime &&
                !expiryHandlerLifetime.expired() &&
                !expiryState.destroyed[0],
            "action-bearing candidate was not retained before Runtime expiry");
    }
    require(
        outlivesRuntime &&
            !expiryHandlerLifetime.expired() &&
            !expiryState.destroyed[0],
        "Runtime expiry prematurely released its action-bearing candidate");
    outlivesRuntime =
        synaptome::runtime::Runtime::ElementResult{};
    require(
        expiryHandlerLifetime.expired() &&
            expiryState.destroyed[0] &&
            expiryState.handlersGoneBeforeElement[0],
        "expired Runtime released an action-bearing candidate out of order");

    ActionContractState destructorState;
    LayerFactory destructorFactory;
    destructorFactory.registerType(
        visualDescriptor(
            "tests.runtime.actions.destructor",
            actionContractDescriptors(false)),
        [&] {
        const int serial = destructorState.constructionCount++;
        return std::make_unique<ActionContractElement>(
            destructorState,
            serial,
            false);
    });
    ParameterRegistry destructorParameters;
    {
        synaptome::runtime::Runtime destructorRuntime(
            destructorFactory,
            destructorParameters);
        auto destructorRequest = firstRequest;
        destructorRequest.typeId =
            "tests.runtime.actions.destructor";
        destructorRequest.definitionId =
            "tests.definition.actions.destructor";
        destructorRequest.instanceId =
            "tests.instance.actions.destructor";
        auto destructorPrepared =
            destructorRuntime.prepareElement(destructorRequest);
        require(
            destructorRuntime.adoptPreparedElement(
                0,
                std::move(destructorPrepared),
                assignmentFor(
                    destructorRequest,
                    "Action Destructor")),
            "action destructor scenario did not adopt its element");
    }
    require(
        destructorState.destroyed[0] &&
            destructorState.handlersGoneBeforeElement[0] &&
            destructorParameters.findFloat(
                "console.layer1.value") == nullptr,
        "Runtime destructor retained actions, element, or parameters");
}

void RunCompositionTelemetryScenario() {
    LayerFactory factory;
    std::vector<std::shared_ptr<TelemetryInstanceState>> instances;
    std::int64_t nextSerial = 0;
    auto registerTelemetryType = [&](
        const std::string& typeId,
        TelemetryFixtureMode mode) {
        factory.registerType(visualDescriptor(typeId), [&, mode] {
            auto state = std::make_shared<TelemetryInstanceState>();
            state->serial = nextSerial++;
            state->ready = (state->serial % 2) == 0;
            state->load = 0.25 + static_cast<double>(state->serial);
            state->source =
                "source-" + std::to_string(state->serial);
            instances.push_back(state);
            return std::make_unique<TelemetryContractElement>(
                state,
                mode);
        });
    };
    registerTelemetryType(
        "tests.runtime.telemetry.valid",
        TelemetryFixtureMode::Valid);
    registerTelemetryType(
        "tests.runtime.telemetry.invalidId",
        TelemetryFixtureMode::InvalidId);
    registerTelemetryType(
        "tests.runtime.telemetry.duplicateId",
        TelemetryFixtureMode::DuplicateId);
    registerTelemetryType(
        "tests.runtime.telemetry.emptyLabel",
        TelemetryFixtureMode::EmptyLabel);
    registerTelemetryType(
        "tests.runtime.telemetry.invalidGroup",
        TelemetryFixtureMode::InvalidGroupId);
    registerTelemetryType(
        "tests.runtime.telemetry.throw",
        TelemetryFixtureMode::Throw);
    factory.registerType(
        visualDescriptor("tests.runtime.telemetry.empty"),
        [] {
        return std::make_unique<EmptyElement>();
    });

    ParameterRegistry parameters;
    synaptome::runtime::Runtime runtime(factory, parameters);

    const auto outOfRange = runtime.compositionElementTelemetry(
        synaptome::runtime::kCompositionLayerCount);
    const auto empty = runtime.compositionElementTelemetry(0);
    require(
        !outOfRange &&
            outOfRange.errorCode ==
                synaptome::runtime::CompositionTelemetryError::
                    IndexOutOfRange &&
            !empty &&
            empty.errorCode ==
                synaptome::runtime::CompositionTelemetryError::SlotEmpty,
        "telemetry bounds or empty-slot errors drifted");

    synaptome::runtime::CompositionAssignment nonElement;
    nonElement.kind = synaptome::runtime::CompositionKind::Effect;
    nonElement.definitionId = "tests.effect.telemetry";
    nonElement.label = "Telemetry Effect";
    nonElement.typeId = "fx.telemetry";
    nonElement.registryPrefix = "effects.telemetry";
    nonElement.active = true;
    require(runtime.assignCompositionEntry(4, nonElement),
            "telemetry scenario could not assign its effect");
    require(
        runtime.compositionElementTelemetry(4).errorCode ==
            synaptome::runtime::CompositionTelemetryError::KindMismatch,
        "effect assignment exposed Element telemetry");
    require(runtime.clearCompositionLayer(4),
            "telemetry scenario could not clear its effect");
    nonElement.kind = synaptome::runtime::CompositionKind::Overlay;
    nonElement.definitionId = "tests.overlay.telemetry";
    nonElement.typeId = "ui.telemetry";
    nonElement.registryPrefix = "ui.telemetry";
    require(runtime.assignCompositionEntry(4, nonElement),
            "telemetry scenario could not assign its overlay");
    require(
        runtime.compositionElementTelemetry(4).errorCode ==
            synaptome::runtime::CompositionTelemetryError::KindMismatch,
        "overlay assignment exposed Element telemetry");
    require(runtime.clearCompositionLayer(4),
            "telemetry scenario could not clear its overlay");

    synaptome::runtime::Runtime::ElementRequest request;
    request.typeId = "tests.runtime.telemetry.empty";
    request.definitionId = "tests.definition.telemetry.empty";
    request.instanceId = "tests.instance.telemetry.empty";
    request.registryPrefix = "console.layer5";
    request.enabled = true;
    auto emptyPrepared = runtime.prepareElement(request);
    require(
        runtime.adoptPreparedElement(
            4,
            std::move(emptyPrepared),
            assignmentFor(request, "No Telemetry")) &&
            runtime.compositionElementTelemetry(4) &&
            runtime.compositionElementTelemetry(4).entries.empty(),
        "element with no telemetry did not return an empty success");
    require(runtime.clearCompositionLayer(4),
            "telemetry scenario could not clear empty telemetry");

    request.typeId = "tests.runtime.telemetry.valid";
    request.definitionId = "tests.definition.telemetry.first";
    request.instanceId = "tests.instance.telemetry.first";
    request.registryPrefix = "console.layer1";
    request.enabled = false;
    auto firstPrepared = runtime.prepareElement(request);
    require(
        firstPrepared &&
            instances.size() == 1 &&
            instances[0]->collectionCount == 0,
        "telemetry collection ran during candidate preparation");
    require(
        runtime.adoptPreparedElement(
            0,
            std::move(firstPrepared),
            assignmentFor(request, "Telemetry First")) &&
            instances[0]->collectionCount == 0,
        "telemetry collection ran during adoption");
    runtime.compositionSnapshot();
    require(
        instances[0]->collectionCount == 0,
        "ordinary composition snapshot collected volatile telemetry");

    const auto firstTelemetry =
        runtime.compositionElementTelemetry(0);
    const auto inactiveTelemetrySnapshot =
        runtime.compositionLayerSnapshot(0);
    require(
        inactiveTelemetrySnapshot &&
            !inactiveTelemetrySnapshot->active &&
            firstTelemetry &&
            instances[0]->collectionCount == 1 &&
            firstTelemetry.entries.size() == 4 &&
            firstTelemetry.entries[0].id == "tests.ready" &&
            firstTelemetry.entries[0].label == "Ready" &&
            firstTelemetry.entries[0].groupId == "tests" &&
            firstTelemetry.entries[0].description ==
                "Fixture readiness." &&
            firstTelemetry.valueAs<bool>("tests.ready") &&
            *firstTelemetry.valueAs<bool>("tests.ready") &&
            firstTelemetry.valueAs<std::int64_t>("tests.serial") &&
            *firstTelemetry.valueAs<std::int64_t>("tests.serial") == 0 &&
            firstTelemetry.valueAs<double>("tests.load") &&
            std::fabs(*firstTelemetry.valueAs<double>("tests.load") - 0.25) <
                0.0001 &&
            firstTelemetry.valueAs<std::string>("tests.source") &&
            *firstTelemetry.valueAs<std::string>("tests.source") ==
                "source-0" &&
            firstTelemetry.valueAs<double>("tests.ready") == nullptr &&
            firstTelemetry.find("tests.missing") == nullptr,
        "adopted inactive element telemetry or typed value projection drifted");

    auto mutatedCopy = firstTelemetry;
    mutatedCopy.entries.front().id = "copy.mutated";
    std::get<bool>(mutatedCopy.entries.front().value) = false;
    instances[0]->source = "source-live";
    instances[0]->ready = false;
    const auto refreshed = runtime.compositionElementTelemetry(0);
    require(
        refreshed &&
            instances[0]->collectionCount == 2 &&
            refreshed.entries.front().id == "tests.ready" &&
            refreshed.valueAs<bool>("tests.ready") &&
            !*refreshed.valueAs<bool>("tests.ready") &&
            refreshed.valueAs<std::string>("tests.source") &&
            *refreshed.valueAs<std::string>("tests.source") ==
                "source-live",
        "telemetry copy isolation or on-demand freshness drifted");

    auto siblingRequest = request;
    siblingRequest.definitionId =
        "tests.definition.telemetry.sibling";
    siblingRequest.instanceId =
        "tests.instance.telemetry.sibling";
    siblingRequest.registryPrefix = "console.layer2";
    auto siblingPrepared = runtime.prepareElement(siblingRequest);
    require(
        runtime.adoptPreparedElement(
            1,
            std::move(siblingPrepared),
            assignmentFor(siblingRequest, "Telemetry Sibling")) &&
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.serial") == 0 &&
            telemetryValue<std::int64_t>(
                runtime,
                1,
                "tests.serial") == 1,
        "same telemetry IDs leaked across composition slots");

    for (const auto& [typeId, expectedError] :
         std::vector<std::pair<
             std::string,
             synaptome::runtime::CompositionTelemetryError>>{
             {
                 "tests.runtime.telemetry.invalidId",
                 synaptome::runtime::CompositionTelemetryError::
                     ContractViolation,
             },
             {
                 "tests.runtime.telemetry.duplicateId",
                 synaptome::runtime::CompositionTelemetryError::
                     ContractViolation,
             },
             {
                 "tests.runtime.telemetry.emptyLabel",
                 synaptome::runtime::CompositionTelemetryError::
                     ContractViolation,
             },
             {
                 "tests.runtime.telemetry.invalidGroup",
                 synaptome::runtime::CompositionTelemetryError::
                     ContractViolation,
             },
             {
                 "tests.runtime.telemetry.throw",
                 synaptome::runtime::CompositionTelemetryError::
                     CollectionFailure,
             },
         }) {
        auto invalidRequest = request;
        invalidRequest.typeId = typeId;
        invalidRequest.definitionId =
            "tests.definition.telemetry.invalid";
        invalidRequest.instanceId =
            "tests.instance.telemetry.invalid";
        invalidRequest.registryPrefix = "console.layer3";
        auto invalidPrepared = runtime.prepareElement(invalidRequest);
        require(
            runtime.adoptPreparedElement(
                2,
                std::move(invalidPrepared),
                assignmentFor(
                    invalidRequest,
                    "Telemetry Invalid")),
            "telemetry invalid fixture could not be adopted");
        const auto invalidTelemetry =
            runtime.compositionElementTelemetry(2);
        require(
            !invalidTelemetry &&
                invalidTelemetry.errorCode == expectedError &&
                runtime.compositionElementTelemetry(0),
            "invalid telemetry was accepted or left Runtime unusable");
        require(runtime.clearCompositionLayer(2),
                "telemetry invalid fixture could not be cleared");
    }

    auto replacementRequest = request;
    replacementRequest.definitionId =
        "tests.definition.telemetry.replacement";
    replacementRequest.instanceId =
        "tests.instance.telemetry.replacement";
    {
        auto aborted =
            runtime.prepareCompositionElementReplacement(
                0,
                replacementRequest);
        require(
            aborted &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.serial") == 0,
            "prepared telemetry candidate became visible before adoption");
        runtime.releasePreparedElement(aborted);
        require(
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.serial") == 0,
            "aborted telemetry candidate changed the live query");
    }
    auto replacement =
        runtime.prepareCompositionElementReplacement(
            0,
            replacementRequest);
    require(
        runtime.adoptPreparedElement(
            0,
            std::move(replacement),
            assignmentFor(
                replacementRequest,
                "Telemetry Replacement")) &&
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.serial") > 1,
        "telemetry replacement did not publish the adopted instance");

    require(
        runtime.clearCompositionLayer(0) &&
            runtime.compositionElementTelemetry(0).errorCode ==
                synaptome::runtime::CompositionTelemetryError::SlotEmpty,
        "clear retained live telemetry");
    runtime.shutdownComposition();
    require(
        runtime.compositionElementTelemetry(1).errorCode ==
            synaptome::runtime::CompositionTelemetryError::SlotEmpty,
        "shutdown retained live telemetry");
}
}

int main() {
    try {
        RunElementDescriptorRegistryScenario();
        RunScopedElementTypeRegistryIsolationScenario();
        RunCompositionSnapshotScenario();
        RunCompositionSlotReplacementScenario();
        RunCompositionActionScenario();
        RunCompositionTelemetryScenario();

        LayerFactory factory;
        factory.registerType(visualDescriptor("tests.runtime.good"), [] {
            return std::make_unique<ContractElement>();
        });
        factory.registerType(visualDescriptor("tests.runtime.failing"), [] {
            return std::make_unique<FailingElement>();
        });
        factory.registerType(visualDescriptor("tests.runtime.empty"), [] {
            return std::make_unique<EmptyElement>();
        });
        factory.registerType(visualDescriptor("tests.runtime.foreign"), [] {
            return std::make_unique<ForeignParameterElement>();
        });
        factory.registerType(
            visualDescriptor("tests.runtime.destructiveFailing"),
            [] {
            return std::make_unique<DestructiveFailingElement>();
        });
        factory.registerType(
            visualDescriptor("tests.runtime.registryAware"),
            [] {
            return std::make_unique<RegistryAwareElement>();
        });
        factory.registerType(
            visualDescriptor("tests.runtime.reservedOpacity"),
            [] {
            return std::make_unique<ReservedOpacityElement>();
        });
        factory.registerType(
            visualDescriptor("tests.runtime.hostCollision"),
            [] {
            return std::make_unique<HostCollisionElement>();
        });
        factory.registerType(
            visualDescriptor("tests.runtime.retainedRegistry"),
            [] {
            return std::make_unique<RetainedSetupRegistryElement>();
        });

        ParameterRegistry parameters;
        float siblingFloat = 0.1f;
        bool siblingBool = true;
        std::string siblingString = "live";
        ParameterRegistry::Descriptor siblingDescriptor;
        siblingDescriptor.label = "Sibling";
        parameters.addFloat(
            "console.layer10.float",
            &siblingFloat,
            siblingFloat,
            siblingDescriptor);
        parameters.addBool(
            "console.layer10.bool",
            &siblingBool,
            siblingBool,
            siblingDescriptor);
        parameters.addString(
            "console.layer10.string",
            &siblingString,
            siblingString,
            siblingDescriptor);

        synaptome::runtime::Runtime runtime(factory, parameters);
        RunEffectCoverageWindowScenario(runtime);
        synaptome::runtime::Runtime::ElementRequest request;
        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.good";
        request.instanceId = "tests.instance.good";
        request.registryPrefix = "console.layer1";
        request.enabled = false;

        std::vector<std::string> progress;
        auto prepared = runtime.prepareElement(
            request,
            [&](std::string_view step) { progress.emplace_back(step); });
        require(static_cast<bool>(prepared), prepared.error);
        require(prepared.element()->instanceId() == request.instanceId,
                "instance identity was not assigned");
        require(prepared.element()->registryPrefix() == request.registryPrefix,
                "registry prefix was not assigned");
        require(!prepared.element()->isEnabled(), "requested enable state was not applied");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "candidate setup mutated the live parameter registry");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(prepared),
                assignmentFor(request, "Good Element", 0.8f)),
            "runtime did not commit the prepared element");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "adoption did not commit staged parameters");
        require(parameters.findBool("console.layer1.gate") != nullptr,
                "adoption did not commit staged bool parameters");
        require(parameters.findFloat("console.layer1.opacity") != nullptr,
                "adoption did not register spine-owned opacity");
        require(
            progress == std::vector<std::string>({"create", "configure", "setup", "enable"}),
            "lifecycle progress order drifted");

        auto collision = runtime.prepareElement(request);
        require(!collision, "occupied parameter prefix was accepted");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "prefix collision damaged the live registration");

        const auto firstClear = runtime.clearCompositionLayer(0);
        const auto firstClearedSnapshot =
            runtime.compositionLayerSnapshot(0);
        require(
            firstClear &&
                firstClearedSnapshot &&
                !firstClearedSnapshot->hasElement,
            "clear did not destroy the element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "clear did not remove instance parameters");
        require(parameters.findBool("console.layer1.gate") == nullptr,
                "clear did not remove instance bool parameters");
        require(parameters.findFloat("console.layer1.opacity") == nullptr,
                "clear retained spine-owned opacity");
        request.definitionId = "tests.definition.reloaded";
        request.instanceId = "tests.instance.reloaded";
        auto reloaded = runtime.prepareElement(request);
        require(static_cast<bool>(reloaded),
                "slot prefix could not be reused after Runtime clear");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(reloaded),
                assignmentFor(request)),
                "reloaded element was not adopted");
        require(runtime.clearCompositionLayer(0),
                "reloaded element was not cleared");
        require(parameters.findFloat("console.layer10.float") != nullptr,
                "release removed a sibling float namespace");
        require(parameters.findBool("console.layer10.bool") != nullptr,
                "release removed a sibling bool namespace");
        require(parameters.findString("console.layer10.string") != nullptr,
                "release removed a sibling string namespace");

        request.typeId = "tests.runtime.failing";
        request.definitionId = "tests.definition.failing";
        request.instanceId = "tests.instance.failing";
        request.registryPrefix = "console.layer2";
        auto failed = runtime.prepareElement(request);
        require(!failed, "throwing setup was reported as successful");
        require(
            failed.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::LifecycleFailure,
            "setup failure did not preserve its structured error code");
        require(failed.stage == "setup",
                "setup failure did not preserve its lifecycle stage");
        require(failed.error == "intentional setup failure",
                "structured setup failure was not preserved");
        require(parameters.findFloat("outside.partial") == nullptr,
                "failed setup leaked an out-of-namespace registration");

        request.typeId = "tests.runtime.foreign";
        request.definitionId = "tests.definition.foreign";
        request.instanceId = "tests.instance.foreign";
        request.registryPrefix = "console.layer2";
        auto foreign = runtime.prepareElement(request);
        require(!foreign, "out-of-namespace registration was accepted");
        require(
            foreign.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::ContractViolation,
            "out-of-namespace registration did not report a contract violation");
        require(parameters.findFloat("foreign.value") == nullptr,
                "contract violation leaked its foreign registration");

        request.typeId = "tests.runtime.reservedOpacity";
        request.definitionId = "tests.definition.reserved-opacity";
        request.instanceId = "tests.instance.reserved-opacity";
        request.registryPrefix = "console.layer2";
        auto reservedOpacity = runtime.prepareElement(request);
        require(!reservedOpacity,
                "element claimed the layer-container opacity parameter");
        require(
            reservedOpacity.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::ContractViolation,
            "reserved opacity did not report a contract violation");
        require(
            reservedOpacity.error.find("layer-container reserved") !=
                std::string::npos,
            "reserved opacity did not explain the ownership violation");
        require(parameters.findFloat("console.layer2.opacity") == nullptr,
                "reserved opacity leaked into the live registry");

        retainedRegistryDestructorRan = false;
        retainedRegistryWasAlive = false;
        request.typeId = "tests.runtime.retainedRegistry";
        request.definitionId = "tests.definition.retained-registry";
        request.instanceId = "tests.instance.retained-registry";
        request.registryPrefix = "console.layer2";
        {
            auto retained = runtime.prepareElement(request);
            require(static_cast<bool>(retained), retained.error);
        }
        require(
            retainedRegistryDestructorRan && retainedRegistryWasAlive,
            "prepared element was not destroyed before its staging registry");

        retainedRegistryDestructorRan = false;
        retainedRegistryWasAlive = false;
        synaptome::runtime::Runtime::ElementResult outlivesRuntime;
        {
            ParameterRegistry temporaryParameters;
            synaptome::runtime::Runtime temporaryRuntime(
                factory,
                temporaryParameters);
            auto retained = temporaryRuntime.prepareElement(request);
            require(static_cast<bool>(retained), retained.error);
            outlivesRuntime = std::move(retained);
        }
        outlivesRuntime =
            synaptome::runtime::Runtime::ElementResult{};
        require(
            retainedRegistryDestructorRan && retainedRegistryWasAlive,
            "expired Runtime destroyed staging before its prepared element");

        request.typeId = "tests.runtime.empty";
        request.definitionId = "tests.definition.empty";
        request.instanceId = "tests.instance.empty";
        request.registryPrefix = "console.layer3";
        auto empty = runtime.prepareElement(request);
        require(static_cast<bool>(empty), "zero-parameter element failed to prepare");
        auto emptyCollision = runtime.prepareElement(request);
        require(!emptyCollision, "zero-parameter element did not reserve its prefix");
        runtime.releasePreparedElement(empty);

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.abandoned";
        request.instanceId = "tests.instance.abandoned";
        request.registryPrefix = "console.layer4";
        {
            auto abandoned = runtime.prepareElement(request);
            require(static_cast<bool>(abandoned), "abandoned result did not prepare");
            require(parameters.findFloat("console.layer4.value") == nullptr,
                    "abandoned candidate mutated live parameters");
        }
        require(parameters.findFloat("console.layer4.value") == nullptr,
                "abandoned result did not release owned parameters");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.composition";
        request.instanceId = "tests.instance.composition";
        request.registryPrefix = "console.layer5";
        {
            auto mismatched = runtime.prepareElement(request);
            require(static_cast<bool>(mismatched),
                    "mismatched composition candidate did not prepare");
            require(
                !runtime.adoptPreparedElement(
                    0,
                    std::move(mismatched),
                    assignmentFor(request)),
                "runtime accepted a candidate for the wrong composition address");
        }
        require(parameters.findFloat("console.layer5.value") == nullptr,
                "rejected composition candidate leaked parameters");

        ParameterRegistry foreignParameters;
        synaptome::runtime::Runtime foreignRuntime(factory, foreignParameters);
        request.definitionId = "tests.definition.foreign-runtime";
        request.instanceId = "tests.instance.foreign-runtime";
        request.registryPrefix = "console.layer1";
        {
            auto foreignRuntimeCandidate = foreignRuntime.prepareElement(request);
            require(static_cast<bool>(foreignRuntimeCandidate),
                    "foreign-runtime composition candidate did not prepare");
            require(
                !runtime.adoptPreparedElement(
                    0,
                    std::move(foreignRuntimeCandidate),
                    assignmentFor(request)),
                "runtime adopted an element owned by another runtime");
            require(
                foreignParameters.findFloat("console.layer1.value") == nullptr,
                "cross-runtime candidate escaped its staging registry");
        }
        require(
            foreignParameters.findFloat("console.layer1.value") == nullptr,
            "rejected cross-runtime candidate leaked source parameters");

        request.definitionId = "tests.definition.composition";
        request.instanceId = "tests.instance.composition";
        request.registryPrefix = "console.layer1";
        auto compositionPrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(compositionPrepared),
                assignmentFor(request, "Composition Element", 0.72f)),
            "runtime did not adopt the prepared composition element");
        const auto initialCompositionSnapshot =
            runtime.compositionLayerSnapshot(0);
        require(
            initialCompositionSnapshot &&
                initialCompositionSnapshot->hasElement &&
                initialCompositionSnapshot->definitionId ==
                    "tests.definition.composition" &&
                !runtime.compositionLayerSnapshot(
                    synaptome::runtime::kCompositionLayerCount) &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.updateCount") == 0,
                "runtime did not retain composition element ownership");
        require(runtime.setCompositionLayerActive(0, true),
                "runtime did not activate the composition element");

        float liveHostValue = 0.65f;
        ParameterRegistry::Descriptor liveHostDescriptor;
        liveHostDescriptor.label = "Live Host Value";
        parameters.addFloat(
            "host.live",
            &liveHostValue,
            liveHostValue,
            liveHostDescriptor);
        request.typeId = "tests.runtime.destructiveFailing";
        request.definitionId = "tests.definition.failed-replacement";
        request.instanceId = "tests.instance.failed-replacement";
        auto failedReplacement =
            runtime.prepareCompositionElementReplacement(0, request);
        require(!failedReplacement,
                "throwing replacement was reported as prepared");
        require(
            runtime.compositionLayerSnapshot(0)->definitionId ==
                    "tests.definition.composition" &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.updateCount") == 0,
            "failed replacement destroyed the live element");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "failed replacement removed the live element parameter");
        require(parameters.findFloat("console.layer1.partial") == nullptr,
                "failed replacement leaked a staged parameter");
        require(parameters.findFloat("host.live") != nullptr &&
                    liveHostValue == 0.65f,
                "failed replacement mutated a live host registration");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.abandoned-replacement";
        request.instanceId = "tests.instance.abandoned-replacement";
        {
            auto abandonedReplacement =
                runtime.prepareCompositionElementReplacement(0, request);
            require(static_cast<bool>(abandonedReplacement),
                    abandonedReplacement.error);
        }
        require(
            runtime.compositionLayerSnapshot(0)->definitionId ==
                    "tests.definition.composition" &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.updateCount") == 0 &&
                parameters.findFloat("console.layer1.value") != nullptr,
            "abandoned replacement disturbed the live slot");

        modifier::Modifier stableModifier;
        auto& stableRuntimeModifier = parameters.addFloatModifier(
            "console.layer1.value",
            stableModifier);
        stableRuntimeModifier.ownerTag = "tests.stable-slot-mapping";
        auto& stableBoolRuntimeModifier = parameters.addBoolModifier(
            "console.layer1.gate",
            stableModifier);
        stableBoolRuntimeModifier.ownerTag = "tests.stable-bool-mapping";

        ParameterRegistry::Descriptor replacementHostDescriptor;
        replacementHostDescriptor.label = "Host Opacity";
        const auto* replacementOpacityParam =
            parameters.findFloat("console.layer1.opacity");
        require(
            replacementOpacityParam &&
                replacementOpacityParam->value &&
                std::fabs(*replacementOpacityParam->value - 0.72f) < 0.0001f,
            "Runtime-owned replacement opacity was not available");
        float hostOwnedValue = 0.91f;
        parameters.addFloat(
            "console.layer1.hostOwned",
            &hostOwnedValue,
            hostOwnedValue,
            replacementHostDescriptor);
        request.typeId = "tests.runtime.hostCollision";
        request.definitionId = "tests.definition.conflicting";
        request.instanceId = "tests.instance.conflicting";
        {
            auto conflictingReplacement =
                runtime.prepareCompositionElementReplacement(0, request);
            require(static_cast<bool>(conflictingReplacement),
                    conflictingReplacement.error);
            require(
                !runtime.adoptPreparedElement(
                    0,
                    std::move(conflictingReplacement),
                    assignmentFor(request, "Conflicting Element", 0.2f)),
                "replacement overwrote a host-owned parameter");
            require(
                runtime.compositionLayerSnapshot(0)->definitionId ==
                        "tests.definition.composition" &&
                    telemetryValue<std::int64_t>(
                        runtime,
                        0,
                        "tests.updateCount") == 0,
                "failed commit replaced the live element");
            require(
                    parameters.findFloat("console.layer1.value") != nullptr &&
                    parameters.findFloat("console.layer1.opacity") != nullptr &&
                    parameters.findFloat("console.layer1.hostOwned") != nullptr &&
                    std::fabs(
                        *parameters.findFloat("console.layer1.opacity")->value -
                        0.72f) < 0.0001f &&
                    hostOwnedValue == 0.91f,
                "failed commit mutated the live registry");
        }
        parameters.removeById("console.layer1.hostOwned");
        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.replacement";
        request.instanceId = "tests.instance.replacement";
        request.enabled = true;
        auto replacement =
            runtime.prepareCompositionElementReplacement(0, request);
        require(static_cast<bool>(replacement), replacement.error);
        require(
            runtime.compositionLayerSnapshot(0)->definitionId ==
                    "tests.definition.composition" &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.updateCount") == 0,
            "preparation replaced the live element before commit");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(replacement),
                assignmentFor(request, "Replacement Element", 0.72f)),
                "same-address replacement did not commit");
        require(
            runtime.compositionLayerSnapshot(0)->definitionId ==
                    "tests.definition.replacement" &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.updateCount") == 0,
            "replacement did not publish fresh live element state");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "replacement did not install candidate parameters");
        const auto* replacementModifiers =
            parameters.floatModifiers("console.layer1.value");
        require(
            replacementModifiers &&
                replacementModifiers->size() == 1 &&
                replacementModifiers->front().ownerTag ==
                    "tests.stable-slot-mapping",
            "replacement did not preserve matching slot modifiers");
        const auto* replacementBoolModifiers =
            parameters.boolModifiers("console.layer1.gate");
        require(
            replacementBoolModifiers &&
                replacementBoolModifiers->size() == 1 &&
                replacementBoolModifiers->front().ownerTag ==
                    "tests.stable-bool-mapping",
            "replacement did not preserve matching bool modifiers");
        const auto* committedReplacementOpacity =
            parameters.findFloat("console.layer1.opacity");
        require(
            committedReplacementOpacity &&
                committedReplacementOpacity->value &&
                std::fabs(*committedReplacementOpacity->value - 0.72f) <
                    0.0001f,
            "replacement lost spine-owned same-address opacity");

        runtime.updateCompositionElements(LayerUpdateParams{});
        runtime.resizeCompositionElements(1280, 720);
        ofCamera camera;
        runtime.drawCompositionElement(
            0,
            LayerDrawParams{camera, {1280, 720}, 0.0f, 0.0f, 1.0f});
        require(
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.updateCount") == 1,
                "runtime did not route composition update");
        require(
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.drawCount") == 1,
                "runtime did not route composition draw");
        require(
            telemetryValue<std::int64_t>(
                runtime,
                0,
                "tests.resizeCount") == 1 &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.lastWidth") == 1280 &&
                telemetryValue<std::int64_t>(
                    runtime,
                    0,
                    "tests.lastHeight") == 720,
            "runtime did not route composition resize");
        const auto compositionClear = runtime.clearCompositionLayer(0);
        const auto compositionClearedSnapshot =
            runtime.compositionLayerSnapshot(0);
        require(
            compositionClear &&
                compositionClearedSnapshot &&
                !compositionClearedSnapshot->hasElement,
            "runtime did not clear its composition element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "composition clear leaked element parameters");
        require(parameters.findFloat("console.layer1.opacity") == nullptr,
                "composition clear retained spine-owned opacity");
        parameters.removeById("host.live");

        request.typeId = "tests.runtime.registryAware";
        request.definitionId = "tests.definition.registry-aware";
        request.instanceId = "tests.instance.registry-aware";
        request.registryPrefix = "console.layer2";
        auto registryAwarePrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(
                1,
                std::move(registryAwarePrepared),
                assignmentFor(request, "Registry Aware")),
            "runtime did not adopt the registry-aware element");
        require(
            telemetryValue<std::int64_t>(
                runtime,
                1,
                "tests.commitCount") == 1 &&
                telemetryValue<bool>(
                    runtime,
                    1,
                    "tests.committedToLiveRegistry"),
            "runtime did not rebind the staged registry after commit");
        require(runtime.setCompositionLayerActive(1, true),
                "runtime did not activate the registry-aware element");
        runtime.updateCompositionElements(LayerUpdateParams{});
        require(
                telemetryValue<bool>(
                    runtime,
                    1,
                    "tests.sawLiveRegistry"),
                "registry-aware update did not observe the live registry");
        require(runtime.clearCompositionLayer(1),
                "runtime did not clear the registry-aware element");

        require(runtime.compositionLayerCount() == 8,
                "runtime composition capacity drifted");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.control-plane-a";
        request.instanceId = "tests.instance.control-plane-a";
        request.registryPrefix = "console.layer3";
        request.enabled = true;
        synaptome::runtime::CompositionAssignment assignment;
        assignment.kind = synaptome::runtime::CompositionKind::Element;
        assignment.definitionId = request.definitionId;
        assignment.label = "Control Plane A";
        assignment.typeId = request.typeId;
        assignment.registryPrefix = request.registryPrefix;
        assignment.active = request.enabled;
        assignment.opacity = 0.42f;
        auto controlPlanePrepared = runtime.prepareElement(request);
        const auto controlPlaneCommit = runtime.adoptPreparedElement(
            2,
            std::move(controlPlanePrepared),
            assignment);
        require(
            controlPlaneCommit &&
                controlPlaneCommit.elementChanged &&
                controlPlaneCommit.parametersChanged,
            "control-plane adoption did not report its committed mutation");
        const auto controlPlaneLayer =
            runtime.compositionLayerSnapshot(2);
        require(
            controlPlaneLayer &&
                controlPlaneLayer->hasElement &&
                controlPlaneLayer->kind ==
                    synaptome::runtime::CompositionKind::Element &&
                controlPlaneLayer->definitionId == assignment.definitionId &&
                controlPlaneLayer->label == assignment.label &&
                controlPlaneLayer->typeId == assignment.typeId &&
                controlPlaneLayer->registryPrefix == assignment.registryPrefix &&
                controlPlaneLayer->active &&
                std::fabs(controlPlaneLayer->opacity - 0.42f) < 0.0001f,
            "control-plane adoption did not publish the assignment");
        auto* controlPlaneOpacity =
            parameters.findFloat("console.layer3.opacity");
        require(
            controlPlaneOpacity &&
                controlPlaneOpacity->value &&
                std::fabs(*controlPlaneOpacity->value - 0.42f) < 0.0001f &&
                std::fabs(controlPlaneOpacity->baseValue - 0.42f) < 0.0001f,
            "Runtime did not bind spine-owned opacity to stable slot storage");
        float* const stableOpacityAddress = controlPlaneOpacity->value;
        modifier::Modifier opacityModifier;
        auto& opacityRuntimeModifier = parameters.addFloatModifier(
            "console.layer3.opacity",
            opacityModifier);
        opacityRuntimeModifier.ownerTag = "tests.stable-opacity-mapping";

        request.definitionId = "tests.definition.control-plane-b";
        request.instanceId = "tests.instance.control-plane-b";
        request.enabled = false;
        auto replacementPrepared =
            runtime.prepareCompositionElementReplacement(2, request);
        synaptome::runtime::CompositionAssignment replacementAssignment;
        replacementAssignment.kind =
            synaptome::runtime::CompositionKind::Element;
        replacementAssignment.definitionId = request.definitionId;
        replacementAssignment.label = "Control Plane B";
        replacementAssignment.typeId = request.typeId;
        replacementAssignment.registryPrefix = request.registryPrefix;
        replacementAssignment.active = request.enabled;
        replacementAssignment.opacity = 0.73f;
        const auto replacementCommit = runtime.adoptPreparedElement(
            2,
            std::move(replacementPrepared),
            replacementAssignment);
        require(static_cast<bool>(replacementCommit),
                "control-plane replacement was rejected");
        controlPlaneOpacity = parameters.findFloat("console.layer3.opacity");
        require(
            controlPlaneOpacity &&
                controlPlaneOpacity->value == stableOpacityAddress &&
                std::fabs(*stableOpacityAddress - 0.73f) < 0.0001f &&
                std::fabs(controlPlaneOpacity->baseValue - 0.73f) < 0.0001f &&
                controlPlaneOpacity->modifiers.size() == 1 &&
                controlPlaneOpacity->modifiers.front().ownerTag ==
                    "tests.stable-opacity-mapping",
            "replacement did not preserve opacity address and modifiers");

        request.definitionId = "tests.definition.control-plane-c";
        request.instanceId = "tests.instance.control-plane-c";
        auto mismatchedPrepared =
            runtime.prepareCompositionElementReplacement(2, request);
        auto mismatchedAssignment = replacementAssignment;
        mismatchedAssignment.definitionId = "tests.definition.wrong";
        const auto rejectedCommit = runtime.adoptPreparedElement(
            2,
            std::move(mismatchedPrepared),
            mismatchedAssignment);
        const auto rejectedCommitSnapshot =
            runtime.compositionLayerSnapshot(2);
        require(
            !rejectedCommit &&
                rejectedCommit.errorCode ==
                    synaptome::runtime::CompositionMutationError::
                        ElementMismatch &&
                rejectedCommitSnapshot &&
                rejectedCommitSnapshot->definitionId ==
                    replacementAssignment.definitionId &&
                telemetryValue<std::int64_t>(
                    runtime,
                    2,
                    "tests.updateCount") == 0 &&
                parameters.findFloat("console.layer3.opacity")->value ==
                    stableOpacityAddress,
            "rejected assignment changed live control-plane state");

        require(runtime.setCompositionLayerLabel(2, "Performance Layer"),
                "Runtime label command rejected presentation state");
        const auto relabeledSnapshot =
            runtime.compositionLayerSnapshot(2);
        require(
            relabeledSnapshot &&
                relabeledSnapshot->label == "Performance Layer",
            "Runtime label command did not update presentation state");
        require(runtime.setCompositionLayerActive(2, true),
                "Runtime active command rejected element state");
        const auto activatedSnapshot =
            runtime.compositionLayerSnapshot(2);
        require(
            activatedSnapshot &&
                activatedSnapshot->active &&
                telemetryValue<bool>(
                    runtime,
                    2,
                    "tests.enabled"),
            "Runtime active command did not update element state");

        float siblingState = 0.25f;
        ParameterRegistry::Descriptor controlPlaneSiblingDescriptor;
        controlPlaneSiblingDescriptor.label = "Sibling State";
        parameters.addFloat(
            "console.layer30.keep",
            &siblingState,
            siblingState,
            controlPlaneSiblingDescriptor);
        const auto clearElement = runtime.clearCompositionLayer(2);
        const auto clearedElementSnapshot =
            runtime.compositionLayerSnapshot(2);
        require(
            clearElement &&
                clearElement.elementChanged &&
                clearElement.parametersChanged &&
                clearedElementSnapshot &&
                !clearedElementSnapshot->hasElement &&
                !clearedElementSnapshot->occupied &&
                clearedElementSnapshot->definitionId.empty() &&
                parameters.findFloat("console.layer3.value") == nullptr &&
                parameters.findFloat("console.layer3.opacity") == nullptr &&
                parameters.findFloat("console.layer30.keep") != nullptr,
            "Runtime clear did not reset exact element/container ownership");

        float sharedEffectRoute = 1.0f;
        parameters.addFloat(
            "effects.tests.route",
            &sharedEffectRoute,
            sharedEffectRoute,
            controlPlaneSiblingDescriptor);
        synaptome::runtime::CompositionAssignment effectAssignment;
        effectAssignment.kind =
            synaptome::runtime::CompositionKind::Effect;
        effectAssignment.definitionId = "fx.tests";
        effectAssignment.label = "Test Effect";
        effectAssignment.typeId = "fx.tests";
        effectAssignment.registryPrefix = "effects.tests";
        effectAssignment.active = true;
        effectAssignment.opacity = 0.6f;
        effectAssignment.coverage.defined = true;
        effectAssignment.coverage.mode.clear();
        effectAssignment.coverage.columns = -4;
        require(
            static_cast<bool>(
                runtime.assignCompositionEntry(2, effectAssignment)),
                "Runtime rejected a valid effect assignment");
        const auto assignedEffectSnapshot =
            runtime.compositionLayerSnapshot(2);
        require(
            assignedEffectSnapshot &&
                assignedEffectSnapshot->kind ==
                    synaptome::runtime::CompositionKind::Effect &&
                assignedEffectSnapshot->coverage.defined &&
                assignedEffectSnapshot->coverage.mode == "upstream" &&
                assignedEffectSnapshot->coverage.columns == 0 &&
                parameters.findFloat("effects.tests.opacity") == nullptr,
            "effect assignment normalization or opacity ownership drifted");
        synaptome::runtime::CompositionCoverage effectCoverage;
        effectCoverage.defined = true;
        effectCoverage.mode.clear();
        effectCoverage.columns = 3;
        require(runtime.setCompositionLayerCoverage(2, effectCoverage),
                "Runtime coverage command rejected effect coverage");
        const auto changedEffectSnapshot =
            runtime.compositionLayerSnapshot(2);
        require(
            changedEffectSnapshot &&
                changedEffectSnapshot->coverage.mode == "upstream" &&
                changedEffectSnapshot->coverage.columns == 3,
            "Runtime coverage command did not normalize effect coverage");
        const auto clearEffect = runtime.clearCompositionLayer(2);
        require(
            clearEffect &&
                !clearEffect.elementChanged &&
                !clearEffect.parametersChanged &&
                parameters.findFloat("effects.tests.route") != nullptr,
            "clearing an effect removed shared effect parameters");

        synaptome::runtime::CompositionAssignment overlayAssignment;
        overlayAssignment.kind =
            synaptome::runtime::CompositionKind::Overlay;
        overlayAssignment.definitionId = "ui.tests";
        overlayAssignment.label = "Test Overlay";
        overlayAssignment.typeId = "ui.tests";
        overlayAssignment.registryPrefix = "ui.tests";
        overlayAssignment.active = true;
        require(runtime.assignCompositionEntry(2, overlayAssignment),
                "Runtime rejected a valid overlay assignment");
        const auto overlaySnapshot =
            runtime.compositionLayerSnapshot(2);
        require(
            overlaySnapshot &&
                overlaySnapshot->kind ==
                    synaptome::runtime::CompositionKind::Overlay &&
                !runtime.setCompositionLayerCoverage(2, effectCoverage),
            "overlay assignment accepted effect-only coverage");
        require(
            static_cast<bool>(runtime.clearCompositionLayer(2)),
                "Runtime did not clear the overlay assignment");
        parameters.removeById("effects.tests.route");
        parameters.removeById("console.layer30.keep");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.shutdown";
        request.instanceId = "tests.instance.shutdown";
        request.registryPrefix = "console.layer1";
        auto shutdownPrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(shutdownPrepared),
                assignmentFor(request, "Shutdown Element")),
            "runtime did not adopt the shutdown contract element");
        runtime.shutdownComposition();
        const auto shutdownSnapshot =
            runtime.compositionLayerSnapshot(0);
        require(shutdownSnapshot && !shutdownSnapshot->hasElement,
                "composition shutdown retained an element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "composition shutdown leaked element parameters");
        require(parameters.findFloat("console.layer1.opacity") == nullptr,
                "composition shutdown leaked spine-owned opacity");
        runtime.shutdownComposition();

        ParameterRegistry destructorParameters;
        {
            synaptome::runtime::Runtime destructorRuntime(
                factory,
                destructorParameters);
            synaptome::runtime::Runtime::ElementRequest destructorRequest;
            destructorRequest.typeId = "tests.runtime.good";
            destructorRequest.definitionId =
                "tests.definition.runtime-destructor";
            destructorRequest.instanceId =
                "tests.instance.runtime-destructor";
            destructorRequest.registryPrefix = "console.layer1";
            destructorRequest.enabled = true;
            auto destructorPrepared =
                destructorRuntime.prepareElement(destructorRequest);
            require(
                destructorRuntime.adoptPreparedElement(
                    0,
                    std::move(destructorPrepared),
                    assignmentFor(
                        destructorRequest,
                        "Runtime Destructor Element",
                        0.55f)),
                "destructor cleanup element was not adopted");
            require(
                destructorParameters.findFloat(
                    "console.layer1.opacity") != nullptr,
                "destructor cleanup setup lacked spine-owned opacity");
        }
        require(
            destructorParameters.findFloat("console.layer1.value") == nullptr &&
                destructorParameters.findBool("console.layer1.gate") == nullptr &&
                destructorParameters.findFloat(
                    "console.layer1.opacity") == nullptr,
            "Runtime destructor left element/container parameter pointers live");

        std::cout
            << "[runtime_core] PASS lifecycle, ownership, composition routing, "
               "live actions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[runtime_core] FAIL " << error.what() << "\n";
        return 1;
    }
}
