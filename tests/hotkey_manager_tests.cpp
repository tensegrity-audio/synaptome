#include <cassert>
#include <cstdio>
#include <iostream>

#include "../synaptome/src/ui/HotkeyManager.h"

int main() {
    HotkeyManager mgr;
    HotkeyManager::Binding binding;
    binding.id = "test.bind";
    binding.displayName = "Test Bind";
    binding.defaultKey = 'x';
    binding.currentKey = binding.defaultKey;
    binding.scope = "";
    binding.learnable = true;

    mgr.defineBinding(binding);

    std::string tmpPath = "tests/tmp_hotkeys.json";
    std::remove(tmpPath.c_str());
    std::remove((tmpPath + ".tmp").c_str());
    mgr.setStoragePath(tmpPath);

    // change the key and save
    mgr.setBindingKey("test.bind", 'a');
    bool ok = mgr.saveToDisk();
    assert(ok && "saveToDisk failed");

    // change key locally and then load from disk to ensure reload
    mgr.setBindingKey("test.bind", 'b');
    bool loaded = mgr.loadFromDisk();
    assert(loaded && "loadFromDisk failed");

    auto* b = mgr.findBinding("test.bind");
    assert(b && "binding not found");
    assert(b->currentKey == 'a' && "binding key did not reload from disk");

    std::remove(tmpPath.c_str());
    std::remove((tmpPath + ".tmp").c_str());

    std::cout << "hotkey manager test passed\n";
    return 0;
}
