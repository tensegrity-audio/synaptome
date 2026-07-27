#pragma once

#include "Layer.h"
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>

class LayerFactory {
public:
    using Creator = std::function<std::unique_ptr<Layer>()>;

    LayerFactory() = default;
    LayerFactory(const LayerFactory&) = delete;
    LayerFactory& operator=(const LayerFactory&) = delete;
    LayerFactory(LayerFactory&&) = delete;
    LayerFactory& operator=(LayerFactory&&) = delete;

    void registerType(const std::string& type, Creator creator);
    bool contains(const std::string& type) const noexcept;
    std::unique_ptr<Layer> create(const std::string& type) const;

private:
    std::unordered_map<std::string, Creator> creators_;
};
