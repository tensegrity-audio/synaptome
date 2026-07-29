#pragma once

#include <memory>
#include <string>

#include "element_confidence/DeclaredSurfaceChecks.h"
#include "runtime/ElementParameterTable.h"
#include "visuals/LayerFactory.h"

namespace synaptome::tests::element_confidence {

struct PreparedDeclaredElement {
    std::unique_ptr<Layer> layer;
    std::unique_ptr<synaptome::runtime::ElementParameterTable>
        parameterTable;
    std::unique_ptr<ParameterRegistry> registry;
};

inline PreparedDeclaredElement prepareDeclaredElement(
    LayerFactory& factory,
    const std::string& typeId,
    const std::string& registryPrefix,
    const std::string& instanceId,
    const ofJson& config) {
    const auto* typeContract = factory.typeContract(typeId);
    require(
        typeContract &&
            typeContract->state ==
                LayerFactory::ParameterDeclarationState::Declared &&
            typeContract->bindingMode ==
                LayerFactory::ParameterBindingMode::Explicit,
        typeId + " is not a declared bind-only element");

    PreparedDeclaredElement prepared;
    prepared.layer = factory.create(typeId);
    require(
        prepared.layer != nullptr,
        "factory did not create " + typeId);
    prepared.layer->setRegistryPrefix(registryPrefix);
    prepared.layer->setInstanceId(instanceId);

    auto* bindable =
        dynamic_cast<synaptome::element::ParameterBindable*>(
            prepared.layer.get());
    require(
        bindable != nullptr,
        "declared element did not expose bind-only storage: " + typeId);
    prepared.parameterTable =
        std::make_unique<synaptome::runtime::ElementParameterTable>(
            typeContract->contract.parameters);
    bindable->bindParameters(*prepared.parameterTable);
    require(
        prepared.parameterTable->contractError().empty(),
        typeId + " live bindings diverged from its declaration: " +
            prepared.parameterTable->contractError());
    prepared.parameterTable->applyDeclarationDefaults();
    prepared.layer->configure(config);

    prepared.registry = std::make_unique<ParameterRegistry>();
    prepared.layer->setup(*prepared.registry);
    require(
        prepared.registry->floats().empty() &&
            prepared.registry->bools().empty() &&
            prepared.registry->strings().empty(),
        typeId + " setup retained metadata authority");
    prepared.parameterTable->populate(
        registryPrefix,
        *prepared.registry);
    return prepared;
}

} // namespace synaptome::tests::element_confidence
