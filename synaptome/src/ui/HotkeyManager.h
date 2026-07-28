#pragma once

#include "MenuController.h"
#include "ofJson.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class HotkeyManager {
public:
    struct Binding {
        std::string id;
        std::string scope;
        std::string displayName;
        std::string description;
        int defaultKey = 0;
        int currentKey = 0;
        bool learnable = true;
        MenuController::HotkeyCallback callback;
    };

    void setController(MenuController* controller);
    void setStoragePath(std::string path);

    bool defineBinding(Binding binding);
    bool setBindingKey(const std::string& id, int key);
    bool resetBindingToDefault(const std::string& id);

    const Binding* findBinding(const std::string& id) const;
    Binding* findBinding(const std::string& id);

    std::vector<const Binding*> orderedBindings() const;
    std::vector<std::string> scopesInOrder() const;

    bool loadFromDisk();
    bool adoptPreferencesSnapshot(const ofJson& hotkeys);
    ofJson exportPreferencesSnapshot() const;
    void setPreferencesPersistenceCallback(
        std::function<bool(const ofJson&)> callback);
    bool saveToDisk();
    bool saveIfDirty();

    bool isDirty() const;
    bool bindingDirty(const std::string& id) const;
    std::vector<std::string> bindingConflicts(const std::string& id) const;

    static std::string keyLabel(int key);

private:
    MenuController* controller_ = nullptr;
    std::string storagePath_;
    std::unordered_map<std::string, Binding> bindings_;
    std::vector<std::string> order_;
    std::unordered_map<std::string, int> savedKeys_;
    std::function<bool(const ofJson&)>
        preferencesPersistenceCallback_;

    void applyBinding(const Binding& binding);
    static int normalizeKey(int key);
    static bool scopesOverlap(const std::string& a, const std::string& b);
};
