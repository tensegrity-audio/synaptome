#include "HotkeyManager.h"

#include "ofEvents.h"
#include "ofFileUtils.h"
#include "ofJson.h"
#include "ofLog.h"
#include "ofUtils.h"
#include <cstdio>

#include <algorithm>
#include <unordered_set>

namespace {
    std::string scopeForLabel(const std::string& scope) {
        if (scope.empty()) {
            return "Global";
        }
        return scope;
    }
}

void HotkeyManager::setController(MenuController* controller) {
    controller_ = controller;
    if (!controller_) {
        return;
    }
    std::unordered_set<std::string> missing;
    for (const auto& id : order_) {
        auto it = bindings_.find(id);
        if (it == bindings_.end()) {
            missing.insert(id);
            ofLogWarning("HotkeyManager") << "Missing binding for id '" << id << "' while applying controller";
            continue;
        }
        applyBinding(it->second);
    }
    if (!missing.empty()) {
        order_.erase(std::remove_if(order_.begin(), order_.end(), [&](const std::string& value) {
            return missing.count(value) > 0;
        }), order_.end());
    }
}

void HotkeyManager::setStoragePath(std::string path) {
    storagePath_ = std::move(path);
}

bool HotkeyManager::defineBinding(Binding binding) {
    if (binding.id.empty()) {
        ofLogWarning("HotkeyManager") << "Attempted to define binding with empty id";
        return false;
    }
    if (binding.displayName.empty()) {
        binding.displayName = binding.id;
    }
    binding.defaultKey = normalizeKey(binding.defaultKey);
    if (binding.currentKey == 0) {
        binding.currentKey = binding.defaultKey;
    } else {
        binding.currentKey = normalizeKey(binding.currentKey);
    }

    auto savedIt = savedKeys_.find(binding.id);
    if (savedIt != savedKeys_.end()) {
        binding.currentKey = normalizeKey(savedIt->second);
    } else {
        savedKeys_[binding.id] = binding.defaultKey;
    }

    std::string bindingId = binding.id;
    bool replaced = bindings_.find(bindingId) != bindings_.end();
    bindings_[bindingId] = std::move(binding);
    if (!replaced) {
        order_.push_back(bindingId);
    }

    if (controller_) {
        auto it = bindings_.find(bindingId);
        if (it != bindings_.end()) {
            applyBinding(it->second);
        }
    }
    return true;
}

bool HotkeyManager::setBindingKey(const std::string& id, int key) {
    auto* binding = findBinding(id);
    if (!binding) {
        ofLogWarning("HotkeyManager") << "Requested key change for unknown binding " << id;
        return false;
    }
    binding->currentKey = normalizeKey(key);
    if (controller_) {
        applyBinding(*binding);
    }
    return true;
}

bool HotkeyManager::resetBindingToDefault(const std::string& id) {
    auto* binding = findBinding(id);
    if (!binding) {
        return false;
    }
    binding->currentKey = binding->defaultKey;
    if (controller_) {
        applyBinding(*binding);
    }
    return true;
}

const HotkeyManager::Binding* HotkeyManager::findBinding(const std::string& id) const {
    auto it = bindings_.find(id);
    if (it == bindings_.end()) {
        return nullptr;
    }
    return &it->second;
}

HotkeyManager::Binding* HotkeyManager::findBinding(const std::string& id) {
    auto it = bindings_.find(id);
    if (it == bindings_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::vector<const HotkeyManager::Binding*> HotkeyManager::orderedBindings() const {
    std::vector<const Binding*> results;
    results.reserve(order_.size());
    for (const auto& id : order_) {
        auto it = bindings_.find(id);
        if (it != bindings_.end()) {
            results.push_back(&it->second);
        }
    }
    return results;
}

std::vector<std::string> HotkeyManager::scopesInOrder() const {
    std::vector<std::string> scopes;
    for (const auto& id : order_) {
        auto it = bindings_.find(id);
        if (it == bindings_.end()) continue;
        const auto& scope = it->second.scope;
        if (std::find(scopes.begin(), scopes.end(), scope) == scopes.end()) {
            scopes.push_back(scope);
        }
    }
    return scopes;
}

bool HotkeyManager::loadFromDisk() {
    if (storagePath_.empty()) {
        ofLogWarning("HotkeyManager") << "Storage path not configured";
        return false;
    }
    if (!ofFile::doesFileExist(storagePath_)) {
        return false;
    }

    ofJson json;
    try {
        json = ofLoadJson(storagePath_);
    } catch (const std::exception& ex) {
        ofLogWarning("HotkeyManager") << "Failed to load hotkey map: " << ex.what();
        return false;
    }
    if (!json.is_object()) {
        ofLogWarning("HotkeyManager") << "Hotkey map is not an object";
        return false;
    }
    if (!json.contains("bindings")) {
        return false;
    }
    const auto& bindingsNode = json["bindings"];
    if (!bindingsNode.is_object()) {
        return false;
    }

    auto isBareModifierKey = [](int value) {
        const int mods = value & MenuController::HOTKEY_MOD_MASK;
        if (mods != 0) {
            return false;
        }
        int base = value & 0xFFFF;
        return base == OF_KEY_CONTROL || base == OF_KEY_SHIFT || base == OF_KEY_ALT;
    };

    for (auto it = bindingsNode.begin(); it != bindingsNode.end(); ++it) {
        if (!it.value().is_object()) {
            continue;
        }
        int rawKey = 0;
        const auto& node = it.value();
        if (node.contains("key") && node["key"].is_number_integer()) {
            rawKey = node["key"].get<int>();
        }
        rawKey = normalizeKey(rawKey);

        // Legacy hotkey maps may store only the base key (no modifier bits).
        // If the stored key has no modifier flags but the defined binding's
        // defaultKey includes modifiers (e.g. Ctrl), merge those modifiers so
        // defaults like Ctrl+C continue to work when users have an older
        // hotkeys.json that only contains 'b'. We store the effective key
        // back into savedKeys_ so future saves persist the corrected form.
        auto* binding = findBinding(it.key());
        int effectiveKey = rawKey;
        if (binding && isBareModifierKey(rawKey)) {
            ofLogWarning("HotkeyManager") << "Ignoring modifier-only binding for '" << binding->id
                                           << "' and reverting to default.";
            effectiveKey = binding->defaultKey;
        } else if (!binding && isBareModifierKey(rawKey)) {
            effectiveKey = 0;
        } else if (binding) {
            const int savedMods = rawKey & MenuController::HOTKEY_MOD_MASK;
            if (savedMods == 0) {
                const int defMods = binding->defaultKey & MenuController::HOTKEY_MOD_MASK;
                effectiveKey = rawKey | defMods;
            }
        }

        if (binding) {
            binding->currentKey = effectiveKey;
            if (controller_) {
                applyBinding(*binding);
            }
        }
        savedKeys_[it.key()] = effectiveKey;
    }
    return true;
}

bool HotkeyManager::saveToDisk() {
    if (storagePath_.empty()) {
        ofLogWarning("HotkeyManager") << "Cannot save hotkeys without storage path";
        return false;
    }

    ofJson root;
    root["version"] = 1;
    ofJson bindingsJson = ofJson::object();
    for (const auto& id : order_) {
        auto it = bindings_.find(id);
        if (it == bindings_.end()) continue;
        const auto& binding = it->second;
        ofJson node;
        node["key"] = binding.currentKey;
        node["label"] = keyLabel(binding.currentKey);
        node["scope"] = scopeForLabel(binding.scope);
        node["name"] = binding.displayName;
        bindingsJson[id] = node;
    }
    root["bindings"] = bindingsJson;

    auto directory = ofFilePath::getEnclosingDirectory(storagePath_, false);
    if (!directory.empty()) {
        ofDirectory::createDirectory(directory, true, true);
    }

    // Write atomically: write to a temp file then rename into place.
    std::string tmpPath = storagePath_ + ".tmp";
    bool ok = ofSavePrettyJson(tmpPath, root);
    if (!ok) {
        ofLogWarning("HotkeyManager") << "Failed to write temporary hotkey map to " << tmpPath;
        // attempt to remove temporary file if present
        try { ofFile::removeFile(tmpPath); } catch (...) {}
        return false;
    }
    // Rename temp into final path
    if (std::rename(tmpPath.c_str(), storagePath_.c_str()) != 0) {
        ofLogWarning("HotkeyManager") << "Failed to rename " << tmpPath << " -> " << storagePath_;
        try { ofFile::removeFile(tmpPath); } catch (...) {}
        return false;
    }

    for (const auto& id : order_) {
        auto it = bindings_.find(id);
        if (it == bindings_.end()) continue;
        savedKeys_[id] = it->second.currentKey;
    }
    return true;
}

bool HotkeyManager::saveIfDirty() {
    if (!isDirty()) {
        return true;
    }
    return saveToDisk();
}

bool HotkeyManager::isDirty() const {
    for (const auto& id : order_) {
        auto bindingIt = bindings_.find(id);
        if (bindingIt == bindings_.end()) continue;
        int savedKey = bindingIt->second.defaultKey;
        auto savedIt = savedKeys_.find(id);
        if (savedIt != savedKeys_.end()) {
            savedKey = savedIt->second;
        }
        if (bindingIt->second.currentKey != savedKey) {
            return true;
        }
    }
    return false;
}

bool HotkeyManager::bindingDirty(const std::string& id) const {
    auto bindingIt = bindings_.find(id);
    if (bindingIt == bindings_.end()) {
        return false;
    }
    int savedKey = bindingIt->second.defaultKey;
    auto savedIt = savedKeys_.find(id);
    if (savedIt != savedKeys_.end()) {
        savedKey = savedIt->second;
    }
    return bindingIt->second.currentKey != savedKey;
}

std::vector<std::string> HotkeyManager::bindingConflicts(const std::string& id) const {
    std::vector<std::string> conflicts;
    auto it = bindings_.find(id);
    if (it == bindings_.end()) {
        return conflicts;
    }
    const auto& target = it->second;
    if (target.currentKey == 0) {
        return conflicts;
    }
    for (const auto& entry : bindings_) {
        if (entry.first == id) continue;
        const auto& other = entry.second;
        if (other.currentKey == 0) continue;
        if (other.currentKey != target.currentKey) continue;
        if (!scopesOverlap(target.scope, other.scope)) continue;
        conflicts.push_back(other.displayName);
    }
    return conflicts;
}

std::string HotkeyManager::keyLabel(int key) {
    if (key == 0) {
        return "-";
    }
    // Show modifiers (Ctrl/Shift/Alt) as prefixes if present
    std::string prefix;
    if (key & MenuController::HOTKEY_MOD_CTRL) prefix += "Ctrl+";
    if (key & MenuController::HOTKEY_MOD_SHIFT) prefix += "Shift+";
    if (key & MenuController::HOTKEY_MOD_ALT) prefix += "Alt+";

    int base = key & 0xFFFF;
    switch (base) {
    case ' ': return "Space";
    case OF_KEY_RETURN: return "Enter";
    case OF_KEY_BACKSPACE: return "Backspace";
    case OF_KEY_ESC: return "Esc";
    case OF_KEY_TAB: return "Tab";
    case OF_KEY_LEFT: return "Left";
    case OF_KEY_RIGHT: return "Right";
    case OF_KEY_UP: return "Up";
    case OF_KEY_DOWN: return "Down";
    case OF_KEY_PAGE_UP: return "PageUp";
    case OF_KEY_PAGE_DOWN: return "PageDown";
    case OF_KEY_HOME: return "Home";
    case OF_KEY_END: return "End";
    case OF_KEY_F1: return "F1";
    case OF_KEY_F2: return "F2";
    case OF_KEY_F3: return "F3";
    case OF_KEY_F4: return "F4";
    case OF_KEY_F5: return "F5";
    case OF_KEY_F6: return "F6";
    case OF_KEY_F7: return "F7";
    case OF_KEY_F8: return "F8";
    case OF_KEY_F9: return "F9";
    case OF_KEY_F10: return "F10";
    case OF_KEY_F11: return "F11";
    case OF_KEY_F12: return "F12";
    default:
        if (base >= 32 && base <= 126) {
            return prefix + std::string(1, static_cast<char>(base));
        }
        return prefix + ofToString(base);
    }
}

void HotkeyManager::applyBinding(const Binding& binding) {
    if (!controller_) {
        return;
    }
    if (binding.id.empty()) {
        return;
    }
    if (binding.currentKey == 0) {
        controller_->unregisterHotkey(binding.id);
        return;
    }
    MenuController::HotkeyBinding hotkey;
    hotkey.id = binding.id;
    hotkey.scope = binding.scope;
    hotkey.key = binding.currentKey;
    hotkey.description = binding.description;
    hotkey.callback = binding.callback;
    ofLogNotice("HotkeyManager") << "Registering hotkey: id='" << binding.id << "' key=" << hotkey.key << " label='" << keyLabel(hotkey.key) << "' scope='" << binding.scope << "'";
    controller_->registerHotkey(hotkey);
}

int HotkeyManager::normalizeKey(int key) {
    // Preserve modifier flags (if any) and normalize the base key to lower-case
    const int mods = key & MenuController::HOTKEY_MOD_MASK;
    int base = key & 0xFFFF;
    int outMods = mods;
    // Handle control-character input (1..26 -> Ctrl+A..Ctrl+Z) which some
    // platforms (Windows) deliver when Ctrl+letter is pressed. Convert these
    // to the corresponding lower-case letter and set the Ctrl modifier so
    // the normalized form matches registered bindings like Ctrl+'b'.
    if (base >= 1 && base <= 26) {
        base = 'a' + (base - 1);
        outMods |= MenuController::HOTKEY_MOD_CTRL;
    } else if (base >= 'A' && base <= 'Z') {
        base = base + ('a' - 'A');
    }
    return outMods | base;
}

bool HotkeyManager::scopesOverlap(const std::string& a, const std::string& b) {
    return a.empty() || b.empty() || a == b;
}
