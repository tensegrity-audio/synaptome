#pragma once

#include <synaptome/element/Parameter.h>

#include <string>
#include <string_view>
#include <vector>

namespace synaptome::runtime {

const element::ParameterDeclarationSet&
builtinElementParameterDeclarations(std::string_view typeId);

std::vector<std::string> builtinElementParameterTypeIds();

} // namespace synaptome::runtime
