#include <synaptome/element/Action.h>
#include <synaptome/element/Telemetry.h>
#include <synaptome/element/compat/Layer.h>
#include <synaptome/element/compat/LayerParameterBuilder.h>

#include <type_traits>

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
