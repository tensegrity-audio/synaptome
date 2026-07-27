#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace synaptome::element {

using TelemetryValue =
    std::variant<bool, std::int64_t, double, std::string>;

struct TelemetryEntry {
    std::string id;
    std::string label;
    std::string groupId;
    std::string description;
    TelemetryValue value;
};

class TelemetrySink {
public:
    virtual ~TelemetrySink() = default;

    virtual void add(TelemetryEntry entry) = 0;
};

} // namespace synaptome::element
