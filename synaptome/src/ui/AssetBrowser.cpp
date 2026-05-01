#include "AssetBrowser.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include <algorithm>
#include <utility>

void AssetBrowser::setLibrary(const LayerLibrary* library) {
    library_ = library;
    clampSelection();
    if (active_) {
        notifyViewModel();
    }
}

void AssetBrowser::setPresenceQuery(std::function<bool(const std::string&)> query) {
    presenceQuery_ = std::move(query);
    if (active_) {
        notifyViewModel();
    }
}

void AssetBrowser::setActiveQuery(std::function<bool(const std::string&)> query) {
    activeQuery_ = std::move(query);
    if (active_) {
        notifyViewModel();
    }
}

void AssetBrowser::setCommandHandler(std::function<void(const LayerLibrary::Entry&, int)> handler) {
    commandHandler_ = std::move(handler);
}

void AssetBrowser::setAllowEntryPredicate(std::function<bool(const LayerLibrary::Entry&)> predicate) {
    allowEntryPredicate_ = std::move(predicate);
    if (active_) notifyViewModel();
}

void AssetBrowser::draw() const {
    if (!active_) return;
    if (!library_) return;
    auto entries = visibleEntries();
    int clampedSelected = entries.empty() ? 0 : ofClamp(selected_, 0, static_cast<int>(entries.size()) - 1);
    if (entries.empty()) {
        ofDrawBitmapStringHighlight("Asset library empty", 20, 40);
        return;
    }

    float y = 40.0f;
    std::string header = "Asset Browser  [Up/Down] select   [Enter] load into console slot";
    ofDrawBitmapStringHighlight(header, 20, y);
    y += 20.0f;

    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i].get();
        bool present = presenceQuery_ ? presenceQuery_(entry.id) : false;
        bool active = activeQuery_ ? activeQuery_(entry.id) : false;
        char status = ' ';
        if (present) status = active ? '*' : '~';

        std::string line = (static_cast<int>(i) == clampedSelected ? "> " : "  ");
        line += "[";
        line.push_back(status);
        line += "]  " + entry.category + " / " + entry.label + "  (" + entry.id + ")";

        ofDrawBitmapStringHighlight(line, 20, y);
        y += 18.0f;
    }

    const auto* selectedEntry = (clampedSelected >= 0 && clampedSelected < static_cast<int>(entries.size()))
        ? &entries[static_cast<std::size_t>(clampedSelected)].get()
        : nullptr;
    if (selectedEntry) {
        y += 18.0f;
        bool present = presenceQuery_ ? presenceQuery_(selectedEntry->id) : false;
        bool active = activeQuery_ ? activeQuery_(selectedEntry->id) : false;
        std::string status = "Status: ";
        if (present) {
            status += active ? "Live in console" : "Assigned (inactive)";
        } else {
            status += "Unassigned";
        }
        ofDrawBitmapStringHighlight(status, 20, y);
        y += 18.0f;
        if (selectedEntry->type == "media.webcam") {
            ofDrawBitmapStringHighlight("Webcam controls: [,] cycle device   -/= gain   M mirror", 20, y);
            y += 18.0f;
        } else if (selectedEntry->type == "media.clip") {
            ofDrawBitmapStringHighlight("Clip controls: [,] cycle clip   -/= gain   M mirror   L loop", 20, y);
            y += 18.0f;
        }
        ofDrawBitmapStringHighlight("Console loader: press Enter to install into the focused slot.", 20, y);
        y += 18.0f;
    }
}

MenuController::StateView AssetBrowser::view() const {
    MenuController::StateView state;
    if (!library_) {
        return state;
    }
    auto entries = visibleEntries();
    int clampedSelected = entries.empty() ? 0 : ofClamp(selected_, 0, static_cast<int>(entries.size()) - 1);
    state.entries.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i].get();
        MenuController::EntryView entryView;
        entryView.id = e.id;
        entryView.label = e.label.empty() ? e.id : e.label;
        entryView.description = e.category;
        entryView.selectable = true;
        entryView.selected = (static_cast<int>(i) == clampedSelected);
        state.entries.push_back(std::move(entryView));
    }
    if (!entries.empty()) {
        state.selectedIndex = clampedSelected;
    }
    state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_UP, "Up", "Previous asset"});
    state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_DOWN, "Down", "Next asset"});
    state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_RETURN, "Enter", "Load into console slot"});
    return state;
}

bool AssetBrowser::handleInput(MenuController& controller, int key) {
    (void)controller;
    if (!active_) {
        return false;
    }

    bool handled = false;
    bool selectionChanged = false;

    switch (key) {
    case OF_KEY_UP:
        selected_ -= 1;
        clampSelection();
        selectionChanged = true;
        handled = true;
        break;
    case OF_KEY_DOWN:
        selected_ += 1;
        clampSelection();
        selectionChanged = true;
        handled = true;
        break;
    default:
        break;
    }

    const auto* entry = currentEntry();
    if (commandHandler_ && entry) {
        commandHandler_(*entry, key);
        if (key != OF_KEY_BACKSPACE && key != OF_KEY_ESC) {
            handled = true;
        }
    }

    if (handled || selectionChanged) {
        notifyViewModel();
    }

    return handled;
}

void AssetBrowser::onEnter(MenuController& controller) {
    controller_ = &controller;
    active_ = true;
    clampSelection();
    notifyViewModel();
}

void AssetBrowser::onExit(MenuController& controller) {
    (void)controller;
    active_ = false;
    controller_ = nullptr;
}

void AssetBrowser::clampSelection() {
    auto entries = visibleEntries();
    int maxIndex = entries.empty() ? 0 : static_cast<int>(entries.size()) - 1;
    selected_ = ofClamp(selected_, 0, maxIndex);
}

const LayerLibrary::Entry* AssetBrowser::currentEntry() const {
    auto entries = visibleEntries();
    if (entries.empty()) {
        return nullptr;
    }
    int index = ofClamp(selected_, 0, static_cast<int>(entries.size()) - 1);
    return &entries[static_cast<std::size_t>(index)].get();
}

void AssetBrowser::notifyViewModel() {
    if (controller_) {
        controller_->requestViewModelRefresh();
    }
}

std::vector<std::reference_wrapper<const LayerLibrary::Entry>> AssetBrowser::visibleEntries() const {
    std::vector<std::reference_wrapper<const LayerLibrary::Entry>> entries;
    if (!library_) {
        return entries;
    }
    const auto& allEntries = library_->entries();
    entries.reserve(allEntries.size());
    for (const auto& e : allEntries) {
        if (allowEntryPredicate_ && !allowEntryPredicate_(e)) {
            continue;
        }
        entries.push_back(std::cref(e));
    }
    return entries;
}
