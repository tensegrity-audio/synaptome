#pragma once

#include <synaptome/element/ElementDescriptor.h>

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

    LayerFactory() = default;
    LayerFactory(const LayerFactory&) = delete;
    LayerFactory& operator=(const LayerFactory&) = delete;
    LayerFactory(LayerFactory&&) = delete;
    LayerFactory& operator=(LayerFactory&&) = delete;

    void registerType(
        synaptome::element::ElementDescriptor descriptor,
        Creator creator);
    bool contains(std::string_view typeId) const noexcept;
    const synaptome::element::ElementDescriptor* descriptor(
        std::string_view typeId) const noexcept;
    std::vector<synaptome::element::ElementDescriptor>
        descriptors() const;
    std::unique_ptr<Layer> create(std::string_view typeId) const;

private:
    struct Registration {
        synaptome::element::ElementDescriptor descriptor;
        Creator creator;
    };

    const Registration* registration(
        std::string_view typeId) const noexcept;

    std::deque<Registration> registrations_;
};
