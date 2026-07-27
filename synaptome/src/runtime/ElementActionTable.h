#pragma once

#include <synaptome/element/Action.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace synaptome::runtime {

class ElementActionTable final : public element::ActionRegistrar {
public:
    ElementActionTable() = default;
    ElementActionTable(const ElementActionTable&) = delete;
    ElementActionTable& operator=(const ElementActionTable&) = delete;

    ElementActionTable(ElementActionTable&& other) noexcept {
        entries_.swap(other.entries_);
    }

    ElementActionTable& operator=(ElementActionTable&& other) noexcept {
        if (this != &other) {
            entries_.clear();
            entries_.swap(other.entries_);
        }
        return *this;
    }

    void add(
        element::ActionDescriptor descriptor,
        element::ActionHandler handler) override {
        entries_.push_back({
            std::move(descriptor),
            std::move(handler),
        });
    }

    std::string contractError() const {
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            if (!isValidId(entry.descriptor.id)) {
                return "element registered an invalid action ID: " +
                    entry.descriptor.id;
            }
            if (entry.descriptor.label.empty()) {
                return "element registered an action with an empty label: " +
                    entry.descriptor.id;
            }
            if (!isValidGroupId(entry.descriptor.groupId)) {
                return "element registered an invalid action group ID: " +
                    entry.descriptor.groupId;
            }
            if (!entry.handler) {
                return "element registered an empty action handler: " +
                    entry.descriptor.id;
            }
            for (std::size_t prior = 0; prior < i; ++prior) {
                if (entries_[prior].descriptor.id ==
                    entry.descriptor.id) {
                    return "element registered a duplicate action ID: " +
                        entry.descriptor.id;
                }
            }
        }
        return {};
    }

    std::vector<element::ActionDescriptor> descriptors() const {
        std::vector<element::ActionDescriptor> result;
        result.reserve(entries_.size());
        for (const auto& entry : entries_) {
            result.push_back(entry.descriptor);
        }
        return result;
    }

    const element::ActionHandler* find(
        std::string_view id) const noexcept {
        for (const auto& entry : entries_) {
            if (entry.descriptor.id == id) {
                return &entry.handler;
            }
        }
        return nullptr;
    }

    void clear() noexcept {
        entries_.clear();
    }

    void swap(ElementActionTable& other) noexcept {
        entries_.swap(other.entries_);
    }

private:
    struct Entry {
        element::ActionDescriptor descriptor;
        element::ActionHandler handler;
    };

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

    std::vector<Entry> entries_;
};

} // namespace synaptome::runtime
