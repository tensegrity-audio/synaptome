#pragma once

#include <synaptome/element/Telemetry.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace synaptome::runtime {

class ElementTelemetryBuffer final : public element::TelemetrySink {
public:
    void add(element::TelemetryEntry entry) override {
        entries_.push_back(std::move(entry));
    }

    std::string contractError() const {
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            if (!isValidId(entry.id)) {
                return "element collected an invalid telemetry ID: " +
                    entry.id;
            }
            if (entry.label.empty()) {
                return
                    "element collected telemetry with an empty label: " +
                    entry.id;
            }
            if (!isValidGroupId(entry.groupId)) {
                return
                    "element collected an invalid telemetry group ID: " +
                    entry.groupId;
            }
            for (std::size_t prior = 0; prior < i; ++prior) {
                if (entries_[prior].id == entry.id) {
                    return
                        "element collected a duplicate telemetry ID: " +
                        entry.id;
                }
            }
        }
        return {};
    }

    std::vector<element::TelemetryEntry> takeEntries() noexcept {
        return std::move(entries_);
    }

private:
    static bool isValidId(std::string_view id) noexcept {
        bool atSegmentStart = true;
        for (const char character : id) {
            if (atSegmentStart) {
                if (character < 'a' || character > 'z') {
                    return false;
                }
                atSegmentStart = false;
                continue;
            }
            if (character == '.') {
                atSegmentStart = true;
                continue;
            }
            const bool lowercase =
                character >= 'a' && character <= 'z';
            const bool uppercase =
                character >= 'A' && character <= 'Z';
            const bool digit =
                character >= '0' && character <= '9';
            if (!lowercase && !uppercase && !digit) {
                return false;
            }
        }
        return !id.empty() && !atSegmentStart;
    }

    static bool isValidGroupId(std::string_view id) noexcept {
        if (id.empty() || id.front() < 'a' || id.front() > 'z') {
            return false;
        }
        for (std::size_t i = 1; i < id.size(); ++i) {
            const char character = id[i];
            const bool lowercase =
                character >= 'a' && character <= 'z';
            const bool uppercase =
                character >= 'A' && character <= 'Z';
            const bool digit =
                character >= '0' && character <= '9';
            if (!lowercase && !uppercase && !digit) {
                return false;
            }
        }
        return true;
    }

    std::vector<element::TelemetryEntry> entries_;
};

} // namespace synaptome::runtime
