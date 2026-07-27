#pragma once

#include <synaptome/element/ElementDescriptor.h>

#include <functional>
#include <string>
#include <utility>

namespace synaptome::element {

enum class ActionExecutionStatus {
    Succeeded,
    Rejected,
    Failed,
};

struct ActionExecutionResult {
    ActionExecutionStatus status = ActionExecutionStatus::Succeeded;
    std::string message;

    explicit operator bool() const noexcept {
        return status == ActionExecutionStatus::Succeeded;
    }

    static ActionExecutionResult succeeded() {
        return {};
    }

    static ActionExecutionResult rejected(std::string message = {}) {
        return {
            ActionExecutionStatus::Rejected,
            std::move(message),
        };
    }

    static ActionExecutionResult failed(std::string message = {}) {
        return {
            ActionExecutionStatus::Failed,
            std::move(message),
        };
    }
};

using ActionHandler = std::function<ActionExecutionResult()>;

class ActionRegistrar {
public:
    virtual ~ActionRegistrar() = default;

    virtual void bind(
        std::string actionId,
        ActionHandler handler) = 0;
};

} // namespace synaptome::element
