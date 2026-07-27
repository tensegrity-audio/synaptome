#include <synaptome/element/compat/Layer.h>
#include <synaptome/element/compat/LayerParameterBuilder.h>

#include <type_traits>

static_assert(std::has_virtual_destructor_v<Layer>);
static_assert(std::is_abstract_v<Layer>);
static_assert(std::is_constructible_v<LayerParameterBuilder,
                                     ParameterRegistry&,
                                     std::string>);
