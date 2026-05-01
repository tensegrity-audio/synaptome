#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class MenuController {
public:
    // Hotkey modifier flags packed into the integer key representation.
    // Low 16 bits: base key code. High bits: modifier flags.
    static constexpr int HOTKEY_MOD_CTRL = 1 << 24;
    static constexpr int HOTKEY_MOD_SHIFT = 1 << 25;
    static constexpr int HOTKEY_MOD_ALT = 1 << 26;
    static constexpr int HOTKEY_MOD_MASK = HOTKEY_MOD_CTRL | HOTKEY_MOD_SHIFT | HOTKEY_MOD_ALT;

    struct EntryView {
        std::string id;
        std::string label;
        std::string description;
        bool selectable = true;
        bool selected = false;
        int modifierCount = 0;
        bool pendingChanges = false;
    };

    struct KeyHint {
        int key = 0;
        std::string label;
        std::string description;
    };

    struct StateView {
        std::vector<EntryView> entries;
        std::vector<KeyHint> hotkeys;
        int selectedIndex = -1;
    };

    class StateViewBuilder {
    public:
        StateViewBuilder& addEntry(const EntryView& entry);
        StateViewBuilder& addDropdownEntry(const std::string& id,
                                           const std::string& label,
                                           const std::string& valueLabel,
                                           bool selected = false,
                                           int modifierCount = 0,
                                           bool pending = false,
                                           const std::string& suffix = std::string());
        StateViewBuilder& addNumericEntry(const std::string& id,
                                          const std::string& label,
                                          const std::string& displayValue,
                                          bool selected = false,
                                          int modifierCount = 0,
                                          bool pending = false,
                                          const std::string& suffix = std::string());
        StateViewBuilder& addTextEntry(const std::string& id,
                                       const std::string& label,
                                       const std::string& value,
                                       bool selected = false,
                                       int modifierCount = 0,
                                       bool pending = false);
        StateViewBuilder& addHotkey(int key, const std::string& label, const std::string& description);
        StateViewBuilder& addHotkey(const KeyHint& hint);
        StateViewBuilder& setSelectedIndex(int index);
        StateView build() const;

    private:
        std::vector<EntryView> entries_;
        std::vector<KeyHint> hotkeys_;
        int selectedIndex_ = -1;
    };
    struct ViewModel {
        bool hasState = false;
        std::vector<std::string> breadcrumbs;
        std::string scope;
        StateView state;
    };

    class State {
    public:
        virtual ~State() = default;
        virtual const std::string& id() const = 0;
        virtual const std::string& label() const = 0;
        virtual const std::string& scope() const = 0;
        virtual StateView view() const = 0;
        virtual bool handleInput(MenuController& controller, int key) = 0;
        virtual void onEnter(MenuController& controller) {}
        virtual void onExit(MenuController& controller) {}
        virtual bool capturesInputBeforeHotkeys() const { return false; }
        virtual bool wantsRawKeyInput() const { return false; }
    };

    using StatePtr = std::shared_ptr<State>;
    using HotkeyCallback = std::function<bool(MenuController&)>;

    struct HotkeyBinding {
        std::string id;
        std::string scope;
        int key = 0;
        std::string description;
        HotkeyCallback callback;
    };

    using ViewModelListener = std::function<void(const ViewModel&)>;

    void pushState(StatePtr state);
    void popState();
    void replaceState(StatePtr state);
    void clear();

    bool handleInput(int key);

    ViewModel viewModel() const;
    std::vector<std::string> currentBreadcrumbs() const;

    void registerHotkey(const HotkeyBinding& binding);
    void unregisterHotkey(const std::string& bindingId);
    std::vector<HotkeyBinding> activeHotkeys() const;
    const std::vector<HotkeyBinding>& allHotkeys() const { return hotkeys_; }
    const std::vector<std::string>& hotkeyConflicts() const { return hotkeyConflicts_; }
    bool removeState(const std::string& stateId);

    void addViewModelListener(ViewModelListener listener);
    void clearViewModelListeners();

    void requestViewModelRefresh();

    bool isCurrent(const std::string& stateId) const;
    bool contains(const std::string& stateId) const;

private:
    void refreshViewModel();
    bool dispatchHotkey(int key, const std::string& scope);

    std::vector<StatePtr> stack_;
    std::vector<HotkeyBinding> hotkeys_;
    ViewModel cachedViewModel_;
    std::vector<ViewModelListener> listeners_;
    std::vector<std::string> hotkeyConflicts_;
};
