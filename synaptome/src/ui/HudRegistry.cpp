#include "HudRegistry.h"

#include "ofFileUtils.h"
#include "ofJson.h"
#include "ofLog.h"
#include "ofUtils.h"
#include "overlays/OverlayManager.h"

#include <algorithm>
#include <utility>

void HudRegistry::setOverlayManager(OverlayManager* manager) {
    if (overlayManager_) {
        overlayManager_->setLayoutDirtyCallback(nullptr);
    }
    overlayManager_ = manager;
    if (!overlayManager_) {
        layoutTrackingActive_ = false;
        for (auto& pair : widgets_) {
            pair.second.registered = false;
        }
        return;
    }
    overlayManager_->setLayoutDirtyCallback([this]() { markLayoutDirty(); });
    overlayManager_->setHudVisible(hudVisible_);
    layoutTrackingActive_ = true;
    for (const auto& id : widgetOrder_) {
        auto it = widgets_.find(id);
        if (it != widgets_.end()) {
            registerWidgetWithOverlay(id, it->second);
        }
    }
}

void HudRegistry::setLayoutChangedCallback(std::function<void()> cb) {
    layoutChangedCallback_ = std::move(cb);
}

bool HudRegistry::registerWidget(WidgetDescriptor widget) {
    if (!widget.factory) {
        ofLogWarning("HudRegistry") << "Widget '" << widget.metadata.id << "' missing factory";
        return false;
    }

    if (widget.metadata.id.empty()) {
        auto instance = widget.factory();
        if (!instance) {
            ofLogWarning("HudRegistry") << "Widget factory returned null instance";
            return false;
        }
        widget.metadata = instance->metadata();
        if (widget.metadata.id.empty()) {
            ofLogWarning("HudRegistry") << "Widget metadata missing id";
            return false;
        }
    }

    auto existing = widgets_.find(widget.metadata.id);
    std::string previousToggle;
    if (existing == widgets_.end()) {
        WidgetEntry entry;
        entry.metadata = widget.metadata;
        entry.factory = widget.factory;
        entry.toggleId = widget.toggleId;
        entry.registered = false;
        widgets_.emplace(widget.metadata.id, std::move(entry));
        widgetOrder_.push_back(widget.metadata.id);
        existing = widgets_.find(widget.metadata.id);
    } else {
        previousToggle = existing->second.toggleId;
        existing->second.metadata = widget.metadata;
        existing->second.factory = widget.factory;
        existing->second.toggleId = widget.toggleId;
        existing->second.registered = false;
    }

    if (!previousToggle.empty() && previousToggle != widget.toggleId) {
        auto& prevLinks = toggleToWidgets_[previousToggle];
        prevLinks.erase(std::remove(prevLinks.begin(), prevLinks.end(), widget.metadata.id), prevLinks.end());
    }

    if (!widget.toggleId.empty()) {
        auto& links = toggleToWidgets_[widget.toggleId];
        if (std::find(links.begin(), links.end(), widget.metadata.id) == links.end()) {
            links.push_back(widget.metadata.id);
        }
    }

    if (overlayManager_) {
        registerWidgetWithOverlay(widget.metadata.id, existing->second);
    }

    return true;
}
std::vector<HudRegistry::WidgetInfo> HudRegistry::widgets() const {
    std::vector<WidgetInfo> result;
    result.reserve(widgetOrder_.size());
    for (const auto& id : widgetOrder_) {
        auto it = widgets_.find(id);
        if (it == widgets_.end()) {
            continue;
        }
        WidgetInfo info;
        info.metadata = it->second.metadata;
        info.toggleId = it->second.toggleId;
        info.registered = it->second.registered;
        info.band = info.metadata.band;
        info.columnIndex = info.metadata.defaultColumn;
        info.visible = info.toggleId.empty() ? true : isEnabled(info.toggleId);
        if (overlayManager_) {
            info.band = overlayManager_->widgetBand(info.metadata.id);
            info.columnIndex = overlayManager_->widgetColumn(info.metadata.id);
            info.collapsed = overlayManager_->widgetCollapsed(info.metadata.id);
            const auto& columns = overlayManager_->columnSpecs();
            if (info.columnIndex >= 0 && info.columnIndex < static_cast<int>(columns.size())) {
                info.columnId = columns[info.columnIndex].id;
            }
        }
        if (info.columnId.empty()) {
            info.columnId = "Column " + std::to_string(info.columnIndex + 1);
        }
        result.push_back(std::move(info));
    }
    return result;
}


void HudRegistry::setStoragePath(std::string path) {
    storagePath_ = std::move(path);
}

void HudRegistry::setLayoutStoragePath(std::string path) {
    layoutStoragePath_ = std::move(path);
    layoutDirty_ = false;
    layoutTrackingActive_ = false;
    layoutLoaded_ = false;
}

void HudRegistry::setHudVisible(bool visible) {
    if (hudVisible_ == visible) {
        return;
    }
    hudVisible_ = visible;
    if (overlayManager_) {
        overlayManager_->setHudVisible(visible);
    }
}

bool HudRegistry::loadFromDisk() {
    if (storagePath_.empty()) {
        ofLogWarning("HudRegistry") << "Storage path not configured";
        return false;
    }
    if (!ofFile::doesFileExist(storagePath_)) {
        return false;
    }

    ofJson json;
    try {
        json = ofLoadJson(storagePath_);
    } catch (const std::exception& ex) {
        ofLogWarning("HudRegistry") << "Failed to load HUD registry: " << ex.what();
        return false;
    }
    if (!json.is_object()) {
        ofLogWarning("HudRegistry") << "HUD registry file malformed";
        return false;
    }
    if (!json.contains("toggles")) {
        return false;
    }
    const auto& togglesNode = json["toggles"];
    if (!togglesNode.is_object()) {
        return false;
    }

    for (auto it = togglesNode.begin(); it != togglesNode.end(); ++it) {
        if (!it.value().is_object()) {
            continue;
        }
        const auto& node = it.value();
        if (!node.contains("value") || !node["value"].is_boolean()) {
            continue;
        }
        bool stored = node["value"].get<bool>();
        savedValues_[it.key()] = stored;
        auto toggleIt = toggles_.find(it.key());
        if (toggleIt != toggles_.end()) {
            writeValue(toggleIt->second, stored);
            applyToggleToWidgets(it.key(), stored);
        }
    }

    return true;
}

bool HudRegistry::saveToDisk() {
    if (storagePath_.empty()) {
        ofLogWarning("HudRegistry") << "Cannot save without storage path";
        return false;
    }

    ofJson root;
    root["version"] = 1;
    ofJson togglesNode = ofJson::object();

    for (const auto& id : order_) {
        auto it = toggles_.find(id);
        if (it == toggles_.end()) {
            continue;
        }
        bool value = readValue(it->second);
        ofJson node;
        node["value"] = value;
        node["label"] = it->second.label;
        node["description"] = it->second.description;
        togglesNode[id] = node;
        savedValues_[id] = value;
    }

    root["toggles"] = togglesNode;

    auto directory = ofFilePath::getEnclosingDirectory(storagePath_, false);
    if (!directory.empty()) {
        ofDirectory::createDirectory(directory, true, true);
    }

    // Write atomically: save to temp file then rename
    std::string tmpPath = storagePath_ + ".tmp";
    bool ok = ofSavePrettyJson(tmpPath, root);
    if (!ok) {
        ofLogWarning("HudRegistry") << "Failed to write temporary HUD registry to " << tmpPath;
        try { ofFile::removeFile(tmpPath); } catch (...) {}
        return false;
    }
    if (std::rename(tmpPath.c_str(), storagePath_.c_str()) != 0) {
        ofLogWarning("HudRegistry") << "Failed to rename " << tmpPath << " -> " << storagePath_;
        try { ofFile::removeFile(tmpPath); } catch (...) {}
        return false;
    }
    return true;
}

bool HudRegistry::saveIfDirty() {
    bool ok = true;
    if (isDirty()) {
        ok = saveToDisk() && ok;
    }
    if (isLayoutDirty()) {
        ok = saveLayoutToDisk() && ok;
    }
    return ok;
}

bool HudRegistry::loadLayoutFromDisk() {
    if (layoutStoragePath_.empty()) {
        ofLogWarning("HudRegistry") << "Layout storage path not configured";
        layoutTrackingActive_ = false;
        return false;
    }
    if (!overlayManager_) {
        ofLogWarning("HudRegistry") << "Overlay manager not configured for layout load";
        return false;
    }
    bool loaded = false;
    if (ofFile::doesFileExist(layoutStoragePath_)) {
        ofJson json;
        try {
            json = ofLoadJson(layoutStoragePath_);
        } catch (const std::exception& ex) {
            ofLogWarning("HudRegistry") << "Failed to load overlay layout: " << ex.what();
            layoutTrackingActive_ = true;
            return false;
        }
        if (!json.is_object()) {
            ofLogWarning("HudRegistry") << "Overlay layout file malformed";
        } else {
            OverlayManager::LayoutState state;
            if (json.contains("columns") && json["columns"].is_array()) {
                for (const auto& columnNode : json["columns"]) {
                    if (!columnNode.is_object()) {
                        continue;
                    }
                    OverlayManager::ColumnSpec column;
                    if (columnNode.contains("id") && columnNode["id"].is_string()) {
                        column.id = columnNode["id"].get<std::string>();
                    }
                    if (column.id.empty()) {
                        column.id = "column" + std::to_string(state.columns.size());
                    }
                    if (columnNode.contains("width") && columnNode["width"].is_number()) {
                        column.width = columnNode["width"].get<float>();
                    }
                    state.columns.push_back(column);
                }
            }
            if (json.contains("widgets") && json["widgets"].is_array()) {
                for (const auto& widgetNode : json["widgets"]) {
                    if (!widgetNode.is_object()) {
                        continue;
                    }
                    if (!widgetNode.contains("id") || !widgetNode["id"].is_string()) {
                        continue;
                    }
                    OverlayManager::LayoutState::WidgetState widget;
                    widget.id = widgetNode["id"].get<std::string>();
                    if (widget.id == "telemetry") {
                        widget.id = "hud.telemetry";
                    }
                    if (widgetNode.contains("column") && widgetNode["column"].is_number_integer()) {
                        widget.columnIndex = widgetNode["column"].get<int>();
                    }
                    if (widgetNode.contains("visible") && widgetNode["visible"].is_boolean()) {
                        widget.visible = widgetNode["visible"].get<bool>();
                    }
                    if (widgetNode.contains("collapsed") && widgetNode["collapsed"].is_boolean()) {
                        widget.collapsed = widgetNode["collapsed"].get<bool>();
                    }
                    if (widgetNode.contains("band") && widgetNode["band"].is_string()) {
                        widget.bandId = widgetNode["band"].get<std::string>();
                    }
                    state.widgets.push_back(widget);
                }
            }
            overlayManager_->applyState(state, false);
            loaded = true;
        }
    }
    layoutLoaded_ = loaded;
    layoutDirty_ = false;
    layoutTrackingActive_ = true;
    return loaded;
}

bool HudRegistry::saveLayoutToDisk() {
    if (layoutStoragePath_.empty()) {
        ofLogWarning("HudRegistry") << "Cannot save layout without storage path";
        return false;
    }
    if (!overlayManager_) {
        ofLogWarning("HudRegistry") << "Cannot save layout without overlay manager";
        return false;
    }

    OverlayManager::LayoutState state = overlayManager_->captureState();

    ofJson root;
    root["version"] = 1;
    ofJson columns = ofJson::array();
    for (const auto& column : state.columns) {
        ofJson node;
        node["id"] = column.id;
        node["width"] = column.width;
        columns.push_back(node);
    }
    root["columns"] = std::move(columns);

    ofJson widgets = ofJson::array();
    for (const auto& widget : state.widgets) {
        ofJson node;
        node["id"] = widget.id;
        node["column"] = widget.columnIndex;
        node["visible"] = widget.visible;
        node["collapsed"] = widget.collapsed;
        if (!widget.bandId.empty()) {
            node["band"] = widget.bandId;
        }
        widgets.push_back(node);
    }
    root["widgets"] = std::move(widgets);

    auto directory = ofFilePath::getEnclosingDirectory(layoutStoragePath_, false);
    if (!directory.empty()) {
        ofDirectory::createDirectory(directory, true, true);
    }

    if (!ofSavePrettyJson(layoutStoragePath_, root)) {
        ofLogWarning("HudRegistry") << "Failed to save overlay layout to " << layoutStoragePath_;
        return false;
    }

    layoutDirty_ = false;
    return true;
}

bool HudRegistry::saveLayoutIfDirty() {
    if (!isLayoutDirty()) {
        return true;
    }
    return saveLayoutToDisk();
}

bool HudRegistry::isLayoutDirty() const {
    return layoutDirty_;
}

bool HudRegistry::registerToggle(const Toggle& toggle) {
    if (toggle.id.empty()) {
        ofLogWarning("HudRegistry") << "Cannot register toggle with empty id";
        return false;
    }
    if (!toggle.valuePtr) {
        ofLogWarning("HudRegistry") << "Toggle '" << toggle.id << "' is missing value pointer";
        return false;
    }

    InternalToggle internal;
    internal.id = toggle.id;
    internal.label = toggle.label.empty() ? toggle.id : toggle.label;
    internal.description = toggle.description;
    internal.defaultValue = toggle.defaultValue;
    internal.valuePtr = toggle.valuePtr;

    auto existing = toggles_.find(toggle.id);
    if (existing == toggles_.end()) {
        toggles_.emplace(toggle.id, internal);
        if (std::find(order_.begin(), order_.end(), toggle.id) == order_.end()) {
            order_.push_back(toggle.id);
        }
    } else {
        existing->second = internal;
    }

    auto storedIt = toggles_.find(toggle.id);
    if (storedIt == toggles_.end()) {
        return false;
    }

    auto savedIt = savedValues_.find(toggle.id);
    bool targetValue = toggle.defaultValue;
    if (savedIt != savedValues_.end()) {
        targetValue = savedIt->second;
    } else {
        savedValues_[toggle.id] = targetValue;
    }
    writeValue(storedIt->second, targetValue);
    applyToggleToWidgets(toggle.id, targetValue);
    return true;
}

bool HudRegistry::toggle(const std::string& id) {
    auto it = toggles_.find(id);
    if (it == toggles_.end()) {
        return false;
    }
    bool value = !readValue(it->second);
    writeValue(it->second, value);
    applyToggleToWidgets(id, value);
    return true;
}

bool HudRegistry::setValue(const std::string& id, bool enabled) {
    auto it = toggles_.find(id);
    if (it == toggles_.end()) {
        return false;
    }
    writeValue(it->second, enabled);
    applyToggleToWidgets(id, enabled);
    return true;
}

bool HudRegistry::resetToDefault(const std::string& id) {
    auto it = toggles_.find(id);
    if (it == toggles_.end()) {
        return false;
    }
    writeValue(it->second, it->second.defaultValue);
    applyToggleToWidgets(id, it->second.defaultValue);
    return true;
}

bool HudRegistry::setWidgetColumn(const std::string& id, int columnIndex) {
    if (!overlayManager_) {
        return false;
    }
    std::string resolvedId = id;
    auto ensureWidget = [&](const std::string& candidate) -> bool {
        if (overlayManager_->hasWidget(candidate)) {
            resolvedId = candidate;
            return true;
        }
        return false;
    };
    if (!ensureWidget(resolvedId)) {
        if (resolvedId.rfind("hud.", 0) == 0) {
            std::string stripped = resolvedId.substr(4);
            if (!ensureWidget(stripped)) {
                std::string fallback = "hud." + stripped;
                ensureWidget(fallback);
            }
        } else {
            ensureWidget("hud." + resolvedId);
        }
    }
    if (!overlayManager_->hasWidget(resolvedId)) {
        ofLogWarning("HudRegistry") << "Cannot set column for unknown widget '" << id << "'";
        return false;
    }
    overlayManager_->setWidgetColumn(resolvedId, columnIndex);
    return true;
}

bool HudRegistry::isEnabled(const std::string& id) const {
    auto it = toggles_.find(id);
    if (it == toggles_.end()) {
        return false;
    }
    return readValue(it->second);
}

std::vector<HudRegistry::ViewEntry> HudRegistry::entries() const {
    std::vector<ViewEntry> views;
    views.reserve(order_.size());
    for (const auto& id : order_) {
        auto it = toggles_.find(id);
        if (it == toggles_.end()) {
            continue;
        }
        ViewEntry entry;
        entry.id = id;
        entry.label = it->second.label;
        entry.description = it->second.description;
        entry.enabled = readValue(it->second);
        entry.dirty = entryDirty(id);
        views.push_back(std::move(entry));
    }
    return views;
}

bool HudRegistry::isDirty() const {
    for (const auto& id : order_) {
        if (entryDirty(id)) {
            return true;
        }
    }
    return false;
}

bool HudRegistry::entryDirty(const std::string& id) const {
    auto it = toggles_.find(id);
    if (it == toggles_.end()) {
        return false;
    }
    bool current = readValue(it->second);
    auto savedIt = savedValues_.find(id);
    bool saved = (savedIt != savedValues_.end()) ? savedIt->second : it->second.defaultValue;
    return current != saved;
}

void HudRegistry::registerWidgetWithOverlay(const std::string& id, WidgetEntry& entry) {
    if (!overlayManager_) {
        entry.registered = false;
        return;
    }
    if (!overlayManager_->hasWidget(id)) {
        if (!entry.factory) {
            ofLogWarning("HudRegistry") << "Widget factory missing for '" << id << "'";
            return;
        }
        auto widget = entry.factory();
        if (!widget) {
            ofLogWarning("HudRegistry") << "Factory for widget '" << id << "' returned null";
            return;
        }
        if (!overlayManager_->registerWidget(std::move(widget))) {
            ofLogWarning("HudRegistry") << "Overlay manager rejected widget '" << id << "'";
            return;
        }
    }
    entry.registered = true;
    if (!entry.toggleId.empty()) {
        bool visible = true;
        auto toggleIt = toggles_.find(entry.toggleId);
        if (toggleIt != toggles_.end()) {
            visible = readValue(toggleIt->second);
        }
        overlayManager_->setWidgetVisible(id, visible);
    }
}

void HudRegistry::applyToggleToWidgets(const std::string& toggleId, bool value) {
    if (!overlayManager_) {
        return;
    }
    auto mapIt = toggleToWidgets_.find(toggleId);
    if (mapIt == toggleToWidgets_.end()) {
        return;
    }
    for (const auto& widgetId : mapIt->second) {
        overlayManager_->setWidgetVisible(widgetId, value);
    }
}

void HudRegistry::markLayoutDirty() {
    layoutDirty_ = true;
    if (layoutChangedCallback_) {
        layoutChangedCallback_();
    }
}

bool HudRegistry::readValue(const InternalToggle& toggle) const {
    if (!toggle.valuePtr) {
        return toggle.defaultValue;
    }
    return *toggle.valuePtr;
}

void HudRegistry::writeValue(const InternalToggle& toggle, bool value) const {
    if (!toggle.valuePtr) {
        return;
    }
    *toggle.valuePtr = value;
}
