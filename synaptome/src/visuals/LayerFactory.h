#pragma once

#include <synaptome/element/Parameter.h>

#include "Layer.h"

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class LayerFactory {
public:
    using Creator = std::function<std::unique_ptr<Layer>()>;

    enum class ParameterDeclarationState {
        LegacySetupDiscovery,
        Declared,
    };

    enum class ParameterBindingMode {
        Explicit,
        LegacySetupAdapter,
    };

    struct ElementTypeContractRecord {
        ParameterDeclarationState state =
            ParameterDeclarationState::LegacySetupDiscovery;
        ParameterBindingMode bindingMode =
            ParameterBindingMode::LegacySetupAdapter;
        synaptome::element::ElementTypeContract contract;
    };

    LayerFactory() = default;
    LayerFactory(const LayerFactory&) = delete;
    LayerFactory& operator=(const LayerFactory&) = delete;
    LayerFactory(LayerFactory&&) = delete;
    LayerFactory& operator=(LayerFactory&&) = delete;

    void registerType(
        synaptome::element::ElementDescriptor descriptor,
        Creator creator);
    void registerType(
        synaptome::element::ElementTypeContract contract,
        Creator creator,
        ParameterBindingMode bindingMode =
            ParameterBindingMode::Explicit);
    bool contains(std::string_view typeId) const noexcept;
    const synaptome::element::ElementDescriptor* descriptor(
        std::string_view typeId) const noexcept;
    std::vector<synaptome::element::ElementDescriptor>
        descriptors() const;
    const ElementTypeContractRecord* typeContract(
        std::string_view typeId) const noexcept;
    std::vector<ElementTypeContractRecord> typeContracts() const;
    std::unique_ptr<Layer> create(std::string_view typeId) const;

private:
    struct Registration {
        ElementTypeContractRecord typeContract;
        Creator creator;
    };

    const Registration* registration(
        std::string_view typeId) const noexcept;
    void registerTypeRecord(
        ElementTypeContractRecord record,
        Creator creator);

    std::deque<Registration> registrations_;
};
