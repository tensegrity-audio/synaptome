#include "LayerFactory.h"
#include <stdexcept>
#include <utility>

LayerFactory& LayerFactory::instance() {
    static LayerFactory factory;
    return factory;
}

void LayerFactory::registerType(const std::string& type, Creator creator) {
    if (type.empty()) {
        throw std::invalid_argument("LayerFactory::registerType requires non-empty type");
    }
    if (!creator) {
        throw std::invalid_argument("LayerFactory::registerType requires valid creator");
    }
    if (creators_.find(type) != creators_.end()) {
        throw std::logic_error("LayerFactory::registerType duplicate type: " + type);
    }
    creators_[type] = std::move(creator);
}

std::unique_ptr<Layer> LayerFactory::create(const std::string& type) const {
    auto it = creators_.find(type);
    if (it == creators_.end()) {
        return nullptr;
    }
    return it->second();
}
