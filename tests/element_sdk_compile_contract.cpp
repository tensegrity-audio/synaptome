#include <synaptome/element/Action.h>
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
