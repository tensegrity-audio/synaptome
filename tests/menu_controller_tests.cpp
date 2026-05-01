#include <cassert>
#include <memory>

#include "../synaptome/src/ui/MenuController.h"
#include "ofEvents.h"

struct RecordingState : MenuController::State {
    std::string stateId = "ui.parameters.test";
    std::string labelText = "Parameters";
    std::string scopeText = "Parameters";
    mutable MenuController::StateView stateView;
    int enterCount = 0;
    int downCount = 0;
    int upCount = 0;
    int exitCount = 0;

    RecordingState() {
        stateView.selectedIndex = 0;
        stateView.entries.resize(3);
        stateView.entries[0].id = "a";
        stateView.entries[0].label = "A";
        stateView.entries[1].id = "b";
        stateView.entries[1].label = "B";
        stateView.entries[2].id = "c";
        stateView.entries[2].label = "C";
    }

    const std::string& id() const override { return stateId; }
    const std::string& label() const override { return labelText; }
    const std::string& scope() const override { return scopeText; }
    MenuController::StateView view() const override { return stateView; }

    bool handleInput(MenuController& controller, int key) override {
        if (key == OF_KEY_RETURN) {
            ++enterCount;
            return true;
        }
        if (key == OF_KEY_DOWN) {
            ++downCount;
            if (!stateView.entries.empty()) {
                stateView.selectedIndex = (stateView.selectedIndex + 1) % static_cast<int>(stateView.entries.size());
                controller.requestViewModelRefresh();
            }
            return true;
        }
        if (key == OF_KEY_UP) {
            ++upCount;
            if (!stateView.entries.empty()) {
                int count = static_cast<int>(stateView.entries.size());
                stateView.selectedIndex = (stateView.selectedIndex - 1 + count) % count;
                controller.requestViewModelRefresh();
            }
            return true;
        }
        return false;
    }

    void onExit(MenuController& controller) override {
        (void)controller;
        ++exitCount;
    }
};

struct CaptureState : MenuController::State {
    MenuController::StateView stateView;
    bool captureBefore = false;
    bool consumeInput = false;
    mutable int lastKey = 0;

    const std::string stateId = "ui.capture";
    const std::string stateLabel = "Capture";
    const std::string scopeLabel = "Capture";

    const std::string& id() const override { return stateId; }
    const std::string& label() const override { return stateLabel; }
    const std::string& scope() const override { return scopeLabel; }
    MenuController::StateView view() const override { return stateView; }
    bool handleInput(MenuController&, int key) override {
        lastKey = key;
        return consumeInput;
    }
    bool capturesInputBeforeHotkeys() const override { return captureBefore; }
};

int main() {
    MenuController controller;
    auto state = std::make_shared<RecordingState>();
    controller.pushState(state);

    auto vm = controller.viewModel();
    assert(vm.hasState);
    assert(vm.state.selectedIndex == 0);

    bool handled = controller.handleInput(OF_KEY_DOWN);
    assert(handled);
    assert(state->downCount == 1);
    vm = controller.viewModel();
    assert(vm.state.selectedIndex == 1);

    handled = controller.handleInput(OF_KEY_UP);
    assert(handled);
    assert(state->upCount == 1);
    vm = controller.viewModel();
    assert(vm.state.selectedIndex == 0);

    handled = controller.handleInput(OF_KEY_RETURN);
    assert(handled);
    assert(state->enterCount == 1);

    handled = controller.handleInput(OF_KEY_BACKSPACE);
    assert(handled);
    vm = controller.viewModel();
    assert(!vm.hasState);
    assert(state->exitCount == 1);

    handled = controller.handleInput(OF_KEY_BACKSPACE);
    assert(!handled);

    {
        MenuController captureController;
        auto captureState = std::make_shared<CaptureState>();
        captureState->captureBefore = true;
        captureState->consumeInput = true;
        captureController.pushState(captureState);

        bool hotkeyTriggered = false;
        MenuController::HotkeyBinding binding;
        binding.id = "capture.test";
        binding.scope = "";
        binding.key = 'X';
        binding.callback = [&hotkeyTriggered](MenuController&) {
            hotkeyTriggered = true;
            return true;
        };
        captureController.registerHotkey(binding);

        bool consumed = captureController.handleInput('X');
        assert(consumed);
        assert(captureState->lastKey == 'X');
        assert(!hotkeyTriggered);
    }

    {
        MenuController captureController;
        auto captureState = std::make_shared<CaptureState>();
        captureState->captureBefore = false;
        captureState->consumeInput = false;
        captureController.pushState(captureState);

        bool hotkeyTriggered = false;
        MenuController::HotkeyBinding binding;
        binding.id = "capture.default";
        binding.scope = "";
        binding.key = 'X';
        binding.callback = [&hotkeyTriggered](MenuController&) {
            hotkeyTriggered = true;
            return true;
        };
        captureController.registerHotkey(binding);

        bool consumed = captureController.handleInput('X');
        assert(consumed);
        assert(hotkeyTriggered);
        assert(captureState->lastKey == 'X');
    }

    {
        MenuController captureController;
        auto captureState = std::make_shared<CaptureState>();
        captureState->captureBefore = true;
        captureState->consumeInput = false;
        captureController.pushState(captureState);

        bool consumed = captureController.handleInput(OF_KEY_BACKSPACE);
        assert(consumed);
        assert(!captureController.contains(captureState->id()));
    }

    {
        MenuController conflictController;
        MenuController::HotkeyBinding bindingA;
        bindingA.id = "conflict.A";
        bindingA.scope = "";
        bindingA.key = 'Q';
        bindingA.callback = [](MenuController&) { return true; };
        conflictController.registerHotkey(bindingA);

        MenuController::HotkeyBinding bindingB = bindingA;
        bindingB.id = "conflict.B";
        conflictController.registerHotkey(bindingB);

        const auto& conflicts = conflictController.hotkeyConflicts();
        assert(!conflicts.empty());
    }

    return 0;
}
