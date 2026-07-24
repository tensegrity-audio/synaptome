#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

#include "ofJson.h"

// Runtime-owned option lists referenced by package metadata.
//
// Providers remain generic JSON arrays because each optionsSource declaration
// names the fields that carry its value and label.
class OptionProviderRegistry {
public:
    bool setProvider(const std::string& id, ofJson options) {
        if (id.empty() || !options.is_array()) {
            return false;
        }
        for (const auto& option : options) {
            if (!option.is_object()) {
                return false;
            }
        }
        providers_[id] = std::move(options);
        ++revision_;
        return true;
    }

    const ofJson* find(const std::string& id) const {
        const auto it = providers_.find(id);
        return it != providers_.end() ? &it->second : nullptr;
    }

    void clear() {
        if (providers_.empty()) {
            return;
        }
        providers_.clear();
        ++revision_;
    }

    std::size_t revision() const {
        return revision_;
    }

private:
    std::unordered_map<std::string, ofJson> providers_;
    std::size_t revision_ = 0;
};
