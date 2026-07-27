#include <synaptome/element/Action.h>
#include <synaptome/element/ElementDescriptor.h>
#include <synaptome/element/Parameter.h>
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/Telemetry.h>
#include <synaptome/element/compat/Layer.h>
#include <synaptome/element/compat/LayerParameterBuilder.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace {
struct ExpectedElementDescriptorShape {
    std::string typeId;
    synaptome::element::ElementKind kind;
    std::vector<synaptome::element::ActionDescriptor> actions;
};

struct ExpectedParameterRangeShape {
    float min;
    float max;
    std::optional<float> step;
};

struct ExpectedParameterOptionShape {
    synaptome::element::ParameterValue value;
    std::string label;
    std::string description;
};

struct ExpectedParameterOptionSourceShape {
    std::string id;
    std::string valueField;
    std::string labelField;
};

struct ExpectedParameterGroupDeclarationShape {
    std::string id;
    std::string label;
    std::string description;
};

struct ExpectedParameterDeprecationShape {
    std::string replacementId;
    std::string reason;
};

struct ExpectedParameterDeclarationShape {
    std::string id;
    synaptome::element::ParameterKind kind;
    std::string groupId;
    std::string label;
    synaptome::element::ParameterValue defaultValue;
    std::optional<synaptome::element::ParameterRange> range;
    std::string units;
    std::string description;
    std::vector<synaptome::element::ParameterOption> options;
    std::optional<synaptome::element::ParameterOptionSource> optionSource;
    std::optional<int> quickAccessOrder;
    std::vector<std::string> aliases;
    std::optional<synaptome::element::ParameterDeprecation> deprecation;
};

struct ExpectedParameterDeclarationSetShape {
    std::vector<synaptome::element::ParameterGroupDeclaration> groups;
    std::vector<synaptome::element::ParameterDeclaration> parameters;
};

struct ExpectedElementTypeContractShape {
    synaptome::element::ElementDescriptor element;
    synaptome::element::ParameterDeclarationSet parameters;
};
}

static_assert(std::has_virtual_destructor_v<Layer>);
static_assert(std::is_abstract_v<Layer>);
static_assert(std::is_constructible_v<LayerParameterBuilder,
                                     ParameterRegistry&,
                                     std::string>);
static_assert(std::is_move_constructible_v<
              synaptome::element::ActionDescriptor>);
static_assert(std::is_move_constructible_v<
              synaptome::element::ActionExecutionResult>);
static_assert(std::has_virtual_destructor_v<
              synaptome::element::ActionRegistrar>);
static_assert(std::has_virtual_destructor_v<
              synaptome::element::ParameterBinder>);
static_assert(std::is_abstract_v<
              synaptome::element::ParameterBinder>);
static_assert(std::has_virtual_destructor_v<
              synaptome::element::ParameterBindable>);
static_assert(std::is_abstract_v<
              synaptome::element::ParameterBindable>);
using FloatParameterBinding =
    void (synaptome::element::ParameterBinder::*)(
        std::string,
        float&);
using BoolParameterBinding =
    void (synaptome::element::ParameterBinder::*)(
        std::string,
        bool&);
using StringParameterBinding =
    void (synaptome::element::ParameterBinder::*)(
        std::string,
        std::string&);
static_assert(std::is_same_v<
              decltype(static_cast<FloatParameterBinding>(
                  &synaptome::element::ParameterBinder::bind)),
              FloatParameterBinding>);
static_assert(std::is_same_v<
              decltype(static_cast<BoolParameterBinding>(
                  &synaptome::element::ParameterBinder::bind)),
              BoolParameterBinding>);
static_assert(std::is_same_v<
              decltype(static_cast<StringParameterBinding>(
                  &synaptome::element::ParameterBinder::bind)),
              StringParameterBinding>);
static_assert(std::is_aggregate_v<
              synaptome::element::ElementDescriptor>);
static_assert(std::is_copy_constructible_v<
              synaptome::element::ElementDescriptor>);
static_assert(std::is_move_constructible_v<
              synaptome::element::ElementDescriptor>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ElementDescriptor::typeId),
              std::string>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ElementDescriptor::kind),
              synaptome::element::ElementKind>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ElementDescriptor::actions),
              std::vector<synaptome::element::ActionDescriptor>>);
static_assert(sizeof(synaptome::element::ElementDescriptor) ==
              sizeof(ExpectedElementDescriptorShape));
static_assert(alignof(synaptome::element::ElementDescriptor) ==
              alignof(ExpectedElementDescriptorShape));
static_assert(std::is_same_v<
              std::underlying_type_t<synaptome::element::ParameterKind>,
              std::uint8_t>);
static_assert(std::variant_size_v<
                  synaptome::element::ParameterValue> == 3);
static_assert(std::is_same_v<
              std::variant_alternative_t<
                  0,
                  synaptome::element::ParameterValue>,
              float>);
static_assert(std::is_same_v<
              std::variant_alternative_t<
                  1,
                  synaptome::element::ParameterValue>,
              bool>);
static_assert(std::is_same_v<
              std::variant_alternative_t<
                  2,
                  synaptome::element::ParameterValue>,
              std::string>);
static_assert(std::is_aggregate_v<
              synaptome::element::ParameterRange>);
static_assert(std::is_aggregate_v<
              synaptome::element::ParameterOption>);
static_assert(std::is_aggregate_v<
              synaptome::element::ParameterOptionSource>);
static_assert(std::is_aggregate_v<
              synaptome::element::ParameterGroupDeclaration>);
static_assert(std::is_aggregate_v<
              synaptome::element::ParameterDeprecation>);
static_assert(std::is_aggregate_v<
              synaptome::element::ParameterDeclaration>);
static_assert(std::is_aggregate_v<
              synaptome::element::ParameterDeclarationSet>);
static_assert(std::is_aggregate_v<
              synaptome::element::ElementTypeContract>);
static_assert(std::is_copy_constructible_v<
              synaptome::element::ParameterDeclaration>);
static_assert(std::is_move_constructible_v<
              synaptome::element::ParameterDeclaration>);
static_assert(std::is_copy_constructible_v<
              synaptome::element::ElementTypeContract>);
static_assert(std::is_move_constructible_v<
              synaptome::element::ElementTypeContract>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterRange::step),
              std::optional<float>>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterOption::value),
              synaptome::element::ParameterValue>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterOptionSource::valueField),
              std::string>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterOptionSource::labelField),
              std::string>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclaration::kind),
              synaptome::element::ParameterKind>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclaration::defaultValue),
              synaptome::element::ParameterValue>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclaration::range),
              std::optional<synaptome::element::ParameterRange>>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclaration::options),
              std::vector<synaptome::element::ParameterOption>>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclaration::optionSource),
              std::optional<
                  synaptome::element::ParameterOptionSource>>);
static_assert(std::is_same_v<
              decltype(
                  synaptome::element::ParameterDeclaration::quickAccessOrder),
              std::optional<int>>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclaration::aliases),
              std::vector<std::string>>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclaration::deprecation),
              std::optional<
                  synaptome::element::ParameterDeprecation>>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ParameterDeclarationSet::groups),
              std::vector<
                  synaptome::element::ParameterGroupDeclaration>>);
static_assert(std::is_same_v<
              decltype(
                  synaptome::element::ParameterDeclarationSet::parameters),
              std::vector<
                  synaptome::element::ParameterDeclaration>>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ElementTypeContract::element),
              synaptome::element::ElementDescriptor>);
static_assert(std::is_same_v<
              decltype(synaptome::element::ElementTypeContract::parameters),
              synaptome::element::ParameterDeclarationSet>);
static_assert(sizeof(synaptome::element::ParameterRange) ==
              sizeof(ExpectedParameterRangeShape));
static_assert(alignof(synaptome::element::ParameterRange) ==
              alignof(ExpectedParameterRangeShape));
static_assert(sizeof(synaptome::element::ParameterOption) ==
              sizeof(ExpectedParameterOptionShape));
static_assert(alignof(synaptome::element::ParameterOption) ==
              alignof(ExpectedParameterOptionShape));
static_assert(sizeof(synaptome::element::ParameterOptionSource) ==
              sizeof(ExpectedParameterOptionSourceShape));
static_assert(alignof(synaptome::element::ParameterOptionSource) ==
              alignof(ExpectedParameterOptionSourceShape));
static_assert(sizeof(synaptome::element::ParameterGroupDeclaration) ==
              sizeof(ExpectedParameterGroupDeclarationShape));
static_assert(alignof(synaptome::element::ParameterGroupDeclaration) ==
              alignof(ExpectedParameterGroupDeclarationShape));
static_assert(sizeof(synaptome::element::ParameterDeprecation) ==
              sizeof(ExpectedParameterDeprecationShape));
static_assert(alignof(synaptome::element::ParameterDeprecation) ==
              alignof(ExpectedParameterDeprecationShape));
static_assert(sizeof(synaptome::element::ParameterDeclaration) ==
              sizeof(ExpectedParameterDeclarationShape));
static_assert(alignof(synaptome::element::ParameterDeclaration) ==
              alignof(ExpectedParameterDeclarationShape));
static_assert(sizeof(synaptome::element::ParameterDeclarationSet) ==
              sizeof(ExpectedParameterDeclarationSetShape));
static_assert(alignof(synaptome::element::ParameterDeclarationSet) ==
              alignof(ExpectedParameterDeclarationSetShape));
static_assert(sizeof(synaptome::element::ElementTypeContract) ==
              sizeof(ExpectedElementTypeContractShape));
static_assert(alignof(synaptome::element::ElementTypeContract) ==
              alignof(ExpectedElementTypeContractShape));
static_assert(std::is_same_v<
              decltype(&synaptome::element::ActionRegistrar::bind),
              void (synaptome::element::ActionRegistrar::*)(
                  std::string,
                  synaptome::element::ActionHandler)>);
static_assert(std::variant_size_v<
                  synaptome::element::TelemetryValue> == 4);
static_assert(std::is_same_v<
              std::variant_alternative_t<
                  0,
                  synaptome::element::TelemetryValue>,
              bool>);
static_assert(std::is_same_v<
              std::variant_alternative_t<
                  1,
                  synaptome::element::TelemetryValue>,
              std::int64_t>);
static_assert(std::is_same_v<
              std::variant_alternative_t<
                  2,
                  synaptome::element::TelemetryValue>,
              double>);
static_assert(std::is_same_v<
              std::variant_alternative_t<
                  3,
                  synaptome::element::TelemetryValue>,
              std::string>);
static_assert(std::has_virtual_destructor_v<
              synaptome::element::TelemetrySink>);
static_assert(std::is_same_v<
              decltype(&Layer::collectTelemetry),
              void (Layer::*)(
                  synaptome::element::TelemetrySink&) const>);
