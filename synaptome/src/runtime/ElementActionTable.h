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
    explicit ElementActionTable(
        const std::vector<element::ActionDescriptor>& descriptors) {
        seed(descriptors);
    }
    ElementActionTable(const ElementActionTable&) = delete;
    ElementActionTable& operator=(const ElementActionTable&) = delete;

    ElementActionTable(ElementActionTable&& other) noexcept {
        swap(other);
    }

    ElementActionTable& operator=(ElementActionTable&& other) noexcept {
        if (this != &other) {
            clear();
            swap(other);
        }
        return *this;
    }

    void bind(
        std::string actionId,
        element::ActionHandler handler) override {
        if (!bindingError_.empty()) {
            return;
        }
        if (actionId.empty()) {
            bindingError_ =
                "element registered an empty action binding ID";
            return;
        }
        if (!handler) {
            bindingError_ =
                "element registered an empty action handler: " +
                actionId;
            return;
        }
        for (auto& entry : entries_) {
            if (entry.descriptor.id != actionId) {
                continue;
            }
            if (entry.handler) {
                bindingError_ =
                    "element registered a duplicate action binding: " +
                    actionId;
                return;
            }
            entry.handler = std::move(handler);
            return;
        }
        bindingError_ =
            "element registered an undeclared action binding: " +
            actionId;
    }

    void seed(
        const std::vector<element::ActionDescriptor>& descriptors) {
        std::vector<Entry> next;
        next.reserve(descriptors.size());
        for (const auto& descriptor : descriptors) {
            next.push_back({descriptor, {}});
        }
        entries_.swap(next);
        bindingError_.clear();
    }

    std::string contractError() const {
        if (!bindingError_.empty()) {
            return bindingError_;
        }
        for (const auto& entry : entries_) {
            if (!entry.handler) {
                return "element did not bind declared action: " +
                    entry.descriptor.id;
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
        bindingError_.clear();
    }

    void swap(ElementActionTable& other) noexcept {
        entries_.swap(other.entries_);
        bindingError_.swap(other.bindingError_);
    }

private:
    struct Entry {
        element::ActionDescriptor descriptor;
        element::ActionHandler handler;
    };

    std::vector<Entry> entries_;
    std::string bindingError_;
};

} // namespace synaptome::runtime
