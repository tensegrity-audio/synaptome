#pragma once

#include "Layer.h"
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>

class LayerFactory {
public:
    using Creator = std::function<std::unique_ptr<Layer>()>;

    static LayerFactory& instance();

    void registerType(const std::string& type, Creator creator);
    std::unique_ptr<Layer> create(const std::string& type) const;

private:
    LayerFactory() = default;
    LayerFactory(const LayerFactory&) = delete;
    LayerFactory& operator=(const LayerFactory&) = delete;

    std::unordered_map<std::string, Creator> creators_;
};
