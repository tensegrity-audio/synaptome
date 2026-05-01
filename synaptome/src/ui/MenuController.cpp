#include "MenuController.h"
#include "HotkeyManager.h"
#include "ofEvents.h"
#include "ofLog.h"

#include <algorithm>
#include <utility>

namespace {
    bool scopeMatches(const std::string& bindingScope, const std::string& activeScope) {
        return bindingScope.empty() || bindingScope == activeScope;
    }

    int normalizeKey(int key) {
        // Preserve modifier flags in the high bits, normalize base key to lower-case
        const int mods = key & MenuController::HOTKEY_MOD_MASK;
        int base = key & 0xFFFF;
        // On Windows, Ctrl+<letter> may be delivered as control characters
        // with codes 1..26 (Ctrl+A..Ctrl+Z). If we see such a base value,
        // map it back to the corresponding lower-case letter and ensure the
        // Ctrl modifier flag is present so it matches registered bindings
        // like (HOTKEY_MOD_CTRL | 'b').
        int outMods = mods;
        if (base >= 1 && base <= 26) {
            base = 'a' + (base - 1);
            outMods |= MenuController::HOTKEY_MOD_CTRL;
        } else if (base >= 'A' && base <= 'Z') {
            base = base + ('a' - 'A');
        }
        return outMods | base;
    }
}

void MenuController::pushState(StatePtr state) {
    if (!state) return;
    stack_.push_back(std::move(state));
    stack_.back()->onEnter(*this);
    refreshViewModel();
}

void MenuController::popState() {
    if (stack_.empty()) return;
    stack_.back()->onExit(*this);
    stack_.pop_back();
    refreshViewModel();
}

void MenuController::replaceState(StatePtr state) {
    if (!state) return;
    if (!stack_.empty()) {
        stack_.back()->onExit(*this);
        stack_.pop_back();
    }
    stack_.push_back(std::move(state));
    stack_.back()->onEnter(*this);
    refreshViewModel();
}

void MenuController::clear() {
    while (!stack_.empty()) {
        stack_.back()->onExit(*this);
        stack_.pop_back();
    }
    refreshViewModel();
}

bool MenuController::handleInput(int key) {
    State* current = stack_.empty() ? nullptr : stack_.back().get();
    std::string scope = current ? current->scope() : std::string();
    bool captureFirst = current && current->capturesInputBeforeHotkeys();

    const int rawKey = key;
    const int baseKey = rawKey & 0xFFFF;

    auto handleStateInput = [&](State* state, int deliveredKey) {
        if (!state) {
            return false;
        }
        ofLogVerbose("MenuController") << "Dispatching input to state '" << state->id() << "' deliveredKey=" << deliveredKey << " label='" << HotkeyManager::keyLabel(deliveredKey) << "'";
        if (state->handleInput(*this, deliveredKey)) {
            refreshViewModel();
            return true;
        }
        return false;
    };

    auto dispatchToState = [&](State* state) {
        if (!state) {
            return false;
        }
        int delivered = state->wantsRawKeyInput() ? rawKey : baseKey;
        return handleStateInput(state, delivered);
    };

    if (captureFirst && dispatchToState(current)) {
        return true;
    }

    if (dispatchHotkey(rawKey, scope)) {
        refreshViewModel();
        return true;
    }

    if (!captureFirst && dispatchToState(current)) {
        return true;
    }

    // Allow the ESC key to close the current menu/state. Backspace should
    // not be used for navigation closing because it is commonly used for
    // editing text fields (and was interfering with text entry).
    if (current && baseKey == OF_KEY_ESC) {
        popState();
        return true;
    }
    return false;
}

MenuController::ViewModel MenuController::viewModel() const {
    return cachedViewModel_;
}

std::vector<std::string> MenuController::currentBreadcrumbs() const {
    return cachedViewModel_.breadcrumbs;
}

void MenuController::registerHotkey(const HotkeyBinding& binding) {
    HotkeyBinding normalized = binding;
    normalized.key = normalizeKey(binding.key);

    auto it = std::find_if(hotkeys_.begin(), hotkeys_.end(), [&](const HotkeyBinding& existing) {
        return !normalized.id.empty() && existing.id == normalized.id;
    });

    const HotkeyBinding* candidate = nullptr;
    if (it != hotkeys_.end()) {
        *it = normalized;
        candidate = &(*it);
    } else {
        hotkeys_.push_back(normalized);
        candidate = &hotkeys_.back();
    }

    auto scopesOverlap = [](const std::string& a, const std::string& b) {
        return a.empty() || b.empty() || a == b;
    };

    for (const auto& existing : hotkeys_) {
        if (&existing == candidate) {
            continue;
        }
        if (existing.key != candidate->key) {
            continue;
        }
        if (!scopesOverlap(existing.scope, candidate->scope)) {
            continue;
        }
        std::string scopeLabel = candidate->scope.empty() ? existing.scope : candidate->scope;
        if (scopeLabel.empty()) {
            scopeLabel = "*";
        }
        std::string lhs = existing.id.empty() ? "?" : existing.id;
        std::string rhs = candidate->id.empty() ? "?" : candidate->id;
        std::string message = "scope=" + scopeLabel + " key=" + std::to_string(candidate->key) + " (" + lhs + " vs " + rhs + ")";
        if (std::find(hotkeyConflicts_.begin(), hotkeyConflicts_.end(), message) == hotkeyConflicts_.end()) {
            hotkeyConflicts_.push_back(message);
            ofLogWarning("MenuController") << "Hotkey conflict: " << message;
        }
    }
}

void MenuController::unregisterHotkey(const std::string& bindingId) {
    if (bindingId.empty()) {
        return;
    }
    hotkeys_.erase(std::remove_if(hotkeys_.begin(), hotkeys_.end(), [&](const HotkeyBinding& binding) {
        return binding.id == bindingId;
    }), hotkeys_.end());
}

bool MenuController::removeState(const std::string& stateId) {
    if (stateId.empty()) {
        return false;
    }
    for (auto it = stack_.begin(); it != stack_.end(); ++it) {
        if ((*it)->id() == stateId) {
            (*it)->onExit(*this);
            stack_.erase(it);
            refreshViewModel();
            return true;
        }
    }
    return false;
}

std::vector<MenuController::HotkeyBinding> MenuController::activeHotkeys() const {
    std::vector<HotkeyBinding> results;
    std::string scope = stack_.empty() ? std::string() : stack_.back()->scope();
    results.reserve(hotkeys_.size());
    for (const auto& binding : hotkeys_) {
        if (scopeMatches(binding.scope, scope)) {
            results.push_back(binding);
        }
    }
    return results;
}

void MenuController::addViewModelListener(ViewModelListener listener) {
    listeners_.push_back(listener);
    if (listener) {
        listener(cachedViewModel_);
    }
}

void MenuController::clearViewModelListeners() {
    listeners_.clear();
}

void MenuController::requestViewModelRefresh() {
    refreshViewModel();
}

bool MenuController::isCurrent(const std::string& stateId) const {
    if (stack_.empty()) {
        return false;
    }
    return stack_.back()->id() == stateId;
}

bool MenuController::contains(const std::string& stateId) const {
    for (const auto& state : stack_) {
        if (state->id() == stateId) {
            return true;
        }
    }
    return false;
}

void MenuController::refreshViewModel() {
    ViewModel vm;
    if (!stack_.empty()) {
        vm.hasState = true;
        vm.scope = stack_.back()->scope();
        vm.state = stack_.back()->view();
        vm.breadcrumbs.reserve(stack_.size());
        for (const auto& state : stack_) {
            vm.breadcrumbs.push_back(state->label());
        }
    }
    cachedViewModel_ = std::move(vm);
    for (const auto& listener : listeners_) {
        if (listener) {
            listener(cachedViewModel_);
        }
    }
}

bool MenuController::dispatchHotkey(int key, const std::string& scope) {
    bool handled = false;
    int normalizedKey = normalizeKey(key);
    for (const auto& binding : hotkeys_) {
        if (binding.key != normalizedKey) {
            continue;
        }
        if (!scopeMatches(binding.scope, scope)) {
            continue;
        }
        if (!binding.callback) {
            continue;
        }
        ofLogNotice("MenuController") << "Dispatching hotkey: id='" << binding.id << "' key=" << binding.key << " label='" << HotkeyManager::keyLabel(binding.key) << "' scope='" << binding.scope << "'";
        handled = binding.callback(*this);
        ofLogNotice("MenuController") << "Hotkey callback returned: id='" << binding.id << "' handled=" << (handled ? "true" : "false");
        if (handled) {
            break;
        }
    }
    return handled;
}







MenuController::StateViewBuilder& MenuController::StateViewBuilder::addEntry(const EntryView& entry) {
    entries_.push_back(entry);
    if (entry.selected && selectedIndex_ < 0) {
        selectedIndex_ = static_cast<int>(entries_.size()) - 1;
    }
    return *this;
}

MenuController::StateViewBuilder& MenuController::StateViewBuilder::addDropdownEntry(const std::string& id,
                                                                                     const std::string& label,
                                                                                     const std::string& valueLabel,
                                                                                     bool selected,
                                                                                     int modifierCount,
                                                                                     bool pending,
                                                                                     const std::string& suffix) {
    EntryView entry;
    entry.id = id;
    entry.label = label;
    entry.description = valueLabel;
    if (!suffix.empty()) {
        entry.description += "  " + suffix;
    }
    entry.selectable = true;
    entry.selected = selected;
    entry.modifierCount = modifierCount;
    entry.pendingChanges = pending;
    return addEntry(entry);
}

MenuController::StateViewBuilder& MenuController::StateViewBuilder::addNumericEntry(const std::string& id,
                                                                                    const std::string& label,
                                                                                    const std::string& displayValue,
                                                                                    bool selected,
                                                                                    int modifierCount,
                                                                                    bool pending,
                                                                                    const std::string& suffix) {
    EntryView entry;
    entry.id = id;
    entry.label = label;
    entry.description = displayValue;
    if (!suffix.empty()) {
        entry.description += "  " + suffix;
    }
    entry.selectable = true;
    entry.selected = selected;
    entry.modifierCount = modifierCount;
    entry.pendingChanges = pending;
    return addEntry(entry);
}

MenuController::StateViewBuilder& MenuController::StateViewBuilder::addTextEntry(const std::string& id,
                                                                                 const std::string& label,
                                                                                 const std::string& value,
                                                                                 bool selected,
                                                                                 int modifierCount,
                                                                                 bool pending) {
    EntryView entry;
    entry.id = id;
    entry.label = label;
    entry.description = value;
    entry.selectable = true;
    entry.selected = selected;
    entry.modifierCount = modifierCount;
    entry.pendingChanges = pending;
    return addEntry(entry);
}

MenuController::StateViewBuilder& MenuController::StateViewBuilder::addHotkey(int key,
                                                                              const std::string& label,
                                                                              const std::string& description) {
    KeyHint hint;
    hint.key = key;
    hint.label = label;
    hint.description = description;
    hotkeys_.push_back(hint);
    return *this;
}

MenuController::StateViewBuilder& MenuController::StateViewBuilder::addHotkey(const KeyHint& hint) {
    hotkeys_.push_back(hint);
    return *this;
}

MenuController::StateViewBuilder& MenuController::StateViewBuilder::setSelectedIndex(int index) {
    selectedIndex_ = index;
    return *this;
}

MenuController::StateView MenuController::StateViewBuilder::build() const {
    StateView view;
    view.entries = entries_;
    view.hotkeys = hotkeys_;
    view.selectedIndex = selectedIndex_;
    return view;
}
