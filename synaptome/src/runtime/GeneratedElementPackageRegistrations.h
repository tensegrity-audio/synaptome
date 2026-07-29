#pragma once

#include <cstddef>

class LayerFactory;

namespace synaptome::runtime {

struct GeneratedElementPackageRegistration {
    const char* packageId;
    const char* packageVersion;
    const char* implementationVersion;
    const char* typeId;
    const char* kind;
    const char* bindingMode;
    const char* definitionId;
    const char* registryPrefix;
    const char* sourceRegistration;
    const char* descriptorSignature;
};

const GeneratedElementPackageRegistration*
generatedElementPackageRegistrations(std::size_t& count) noexcept;

void registerGeneratedElementPackages(LayerFactory& elementTypes);

} // namespace synaptome::runtime
