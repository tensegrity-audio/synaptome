#pragma once

#include <string>

namespace synaptome::element {

// Bindings are instance-local storage only. Parameter declarations own all
// public metadata, defaults, options, aliases, and deprecation information.
// A binder is call-scoped and must not be retained. Bound storage must keep a
// stable address for the lifetime of the live element instance.
class ParameterBinder {
public:
    virtual ~ParameterBinder() = default;

    virtual void bind(std::string parameterId, float& storage) = 0;
    virtual void bind(std::string parameterId, bool& storage) = 0;
    virtual void bind(std::string parameterId, std::string& storage) = 0;
};

// Transitional source-linked interface for elements whose parameter
// declarations are authoritative. Legacy Layer::setup discovery remains
// available to types that have not migrated yet.
class ParameterBindable {
public:
    virtual ~ParameterBindable() = default;

    virtual void bindParameters(ParameterBinder& binder) = 0;
};

} // namespace synaptome::element
