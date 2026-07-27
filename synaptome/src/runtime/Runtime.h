#pragma once

#include <synaptome/element/compat/Layer.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class LayerFactory;
class ParameterRegistry;

namespace synaptome::runtime {

class Runtime {
public:
    enum class ElementErrorCode {
        None,
        InvalidRequest,
        PrefixInUse,
        TypeNotRegistered,
        LifecycleFailure,
        ContractViolation,
    };

    struct ElementRequest {
        std::string typeId;
        std::string definitionId;
        std::string instanceId;
        std::string registryPrefix;
        ofJson config = ofJson::object();
        bool enabled = true;
    };

    struct ElementResult {
        std::unique_ptr<Layer> element;
        ElementErrorCode errorCode = ElementErrorCode::None;
        std::string stage;
        std::string typeId;
        std::string definitionId;
        std::string instanceId;
        std::string registryPrefix;
        std::string error;

        ElementResult() = default;
        ~ElementResult();
        ElementResult(const ElementResult&) = delete;
        ElementResult& operator=(const ElementResult&) = delete;
        ElementResult(ElementResult&& other) noexcept;
        ElementResult& operator=(ElementResult&& other) noexcept;

        explicit operator bool() const { return element != nullptr; }

    private:
        friend class Runtime;
        Runtime* runtime_ = nullptr;
        std::weak_ptr<void> runtimeLifetime_;
    };

    using ProgressCallback = std::function<void(std::string_view step)>;

    Runtime(LayerFactory& factory, ParameterRegistry& parameters);
    ~Runtime() noexcept;

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    ElementResult prepareElement(
        const ElementRequest& request,
        const ProgressCallback& progress = {});

    void releaseElement(std::unique_ptr<Layer>& element) noexcept;

private:
    enum class ParameterKind {
        Float,
        Bool,
        String,
    };

    struct ParameterKey {
        ParameterKind kind;
        std::string id;
    };

    struct ElementOwnership {
        std::string prefix;
        std::vector<ParameterKey> parameters;
    };

    bool prefixIsAvailable(const std::string& prefix) const;
    std::vector<ParameterKey> parameterSnapshot() const;
    static std::vector<ParameterKey> parameterDelta(
        const std::vector<ParameterKey>& before,
        const std::vector<ParameterKey>& after);
    static bool idBelongsToPrefix(
        const std::string& id,
        const std::string& prefix);
    void removeParameters(const std::vector<ParameterKey>& parameters) noexcept;
    void releaseTrackedElement(Layer* element) noexcept;

    LayerFactory& factory_;
    ParameterRegistry& parameters_;
    std::unordered_map<Layer*, ElementOwnership> ownership_;
    std::unordered_set<std::string> activePrefixes_;
    std::shared_ptr<void> lifetime_ = std::make_shared<int>(0);
};

} // namespace synaptome::runtime
