#include "AssetBrowser.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include <algorithm>
#include <sstream>
#include <utility>

namespace {
    std::string displayCategory(const LayerLibrary::Entry& entry) {
        return entry.category.empty() ? "Unsorted" : entry.category;
    }

    std::string categoryKey(const std::string& category) {
        return "category:" + category;
    }

    std::string groupKey(const std::string& category, const std::string& group) {
        return "group:" + category + "/" + group;
    }

    std::string rowPrefix(int depth, bool expandable, bool expanded) {
        if (!expandable) {
            return std::string(depth * 2, ' ') + "  ";
        }
        return std::string(depth * 2, ' ') + (expanded ? "- " : "+ ");
    }

    std::string entryPathLabel(const LayerLibrary::Entry& entry) {
        std::ostringstream out;
        out << displayCategory(entry);
        if (!entry.layerGroup.empty()) {
            out << " / " << entry.layerGroup;
        }
        out << " / " << entry.label;
        return out.str();
    }

    std::string entryDescription(const LayerLibrary::Entry& entry) {
        if (!entry.layerGroup.empty()) {
            return displayCategory(entry) + " / " + entry.layerGroup;
        }
        return displayCategory(entry);
    }
}

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
    auto rows = visibleRows();
    const float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    const float headerStep = 20.0f * textScale;
    const float rowStep = 18.0f * textScale;
    int clampedSelected = rows.empty() ? 0 : ofClamp(selected_, 0, static_cast<int>(rows.size()) - 1);
    if (rows.empty()) {
        drawBitmapStringHighlightScaled("Asset library empty", 20.0f, 40.0f, textScale);
        return;
    }

    float y = 40.0f;
    std::string header = "Asset Browser  [Up/Down] select   [Left/Right] collapse/expand   [Enter] load/toggle";
    drawBitmapStringHighlightScaled(header, 20.0f, y, textScale);
    y += headerStep;

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        std::string line = (static_cast<int>(i) == clampedSelected ? "> " : "  ");
        line += rowPrefix(row.depth, row.expandable, row.expanded);
        if (row.entry) {
            const auto& entry = *row.entry;
            bool present = presenceQuery_ ? presenceQuery_(entry.id) : false;
            bool active = activeQuery_ ? activeQuery_(entry.id) : false;
            char status = ' ';
            if (present) status = active ? '*' : '~';
            line += "[";
            line.push_back(status);
            line += "]  " + entryPathLabel(entry) + "  (" + entry.id + ")";
        } else {
            line += row.label;
            if (!row.description.empty()) {
                line += "  " + row.description;
            }
        }

        drawBitmapStringHighlightScaled(line, 20.0f, y, textScale);
        y += rowStep;
    }

    const auto* selectedEntry = (clampedSelected >= 0 && clampedSelected < static_cast<int>(rows.size()))
        ? rows[static_cast<std::size_t>(clampedSelected)].entry
        : nullptr;
    if (selectedEntry) {
        y += rowStep;
        bool present = presenceQuery_ ? presenceQuery_(selectedEntry->id) : false;
        bool active = activeQuery_ ? activeQuery_(selectedEntry->id) : false;
        std::string status = "Status: ";
        if (present) {
            status += active ? "Live in console" : "Assigned (inactive)";
        } else {
            status += "Unassigned";
        }
        drawBitmapStringHighlightScaled(status, 20.0f, y, textScale);
        y += rowStep;
        if (selectedEntry->type == "media.webcam") {
            drawBitmapStringHighlightScaled("Webcam controls: [,] cycle device   -/= gain   M mirror", 20.0f, y, textScale);
            y += rowStep;
        } else if (selectedEntry->type == "media.clip") {
            drawBitmapStringHighlightScaled("Clip controls: [,] cycle clip   -/= gain   M mirror   L loop", 20.0f, y, textScale);
            y += rowStep;
        }
        drawBitmapStringHighlightScaled("Console loader: press Enter to install into the focused slot.", 20.0f, y, textScale);
        y += rowStep;
    }
}

MenuController::StateView AssetBrowser::view() const {
    MenuController::StateView state;
    if (!library_) {
        return state;
    }
    auto rows = visibleRows();
    int clampedSelected = rows.empty() ? 0 : ofClamp(selected_, 0, static_cast<int>(rows.size()) - 1);
    state.entries.reserve(rows.size());
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        MenuController::EntryView entryView;
        entryView.id = row.entry ? row.entry->id : row.key;
        if (row.entry) {
            entryView.label = rowPrefix(row.depth, row.expandable, row.expanded) + (row.entry->label.empty() ? row.entry->id : row.entry->label);
            entryView.description = entryDescription(*row.entry);
        } else {
            entryView.label = rowPrefix(row.depth, row.expandable, row.expanded) + row.label;
            entryView.description = row.description;
        }
        entryView.selectable = true;
        entryView.selected = (static_cast<int>(i) == clampedSelected);
        state.entries.push_back(std::move(entryView));
    }
    if (!rows.empty()) {
        state.selectedIndex = clampedSelected;
    }
    state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_UP, "Up", "Previous asset"});
    state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_DOWN, "Down", "Next asset"});
    state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_LEFT, "Left", "Collapse group"});
    state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_RIGHT, "Right", "Expand group"});
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
    auto rows = visibleRows();
    int rowIndex = rows.empty() ? 0 : ofClamp(selected_, 0, static_cast<int>(rows.size()) - 1);
    const Row* row = rows.empty() ? nullptr : &rows[static_cast<std::size_t>(rowIndex)];

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
    case OF_KEY_LEFT:
        if (row && row->expandable && row->expanded) {
            setExpanded(row->key, false);
            clampSelection();
            handled = true;
            selectionChanged = true;
        }
        break;
    case OF_KEY_RIGHT:
        if (row && row->expandable && !row->expanded) {
            setExpanded(row->key, true);
            handled = true;
            selectionChanged = true;
        }
        break;
    case OF_KEY_RETURN:
    case ' ':
        if (row && row->expandable) {
            setExpanded(row->key, !row->expanded);
            clampSelection();
            handled = true;
            selectionChanged = true;
        }
        break;
    default:
        break;
    }

    const auto* entry = currentEntry();
    if (!handled && commandHandler_ && entry) {
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
    auto rows = visibleRows();
    int maxIndex = rows.empty() ? 0 : static_cast<int>(rows.size()) - 1;
    selected_ = ofClamp(selected_, 0, maxIndex);
}

const LayerLibrary::Entry* AssetBrowser::currentEntry() const {
    auto rows = visibleRows();
    if (rows.empty()) {
        return nullptr;
    }
    int index = ofClamp(selected_, 0, static_cast<int>(rows.size()) - 1);
    return rows[static_cast<std::size_t>(index)].entry;
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

std::vector<AssetBrowser::Row> AssetBrowser::visibleRows() const {
    std::vector<Row> rows;
    auto entries = visibleEntries();
    rows.reserve(entries.size() + 8);

    std::string currentCategory;
    std::string currentGroup;
    bool categoryExpanded = true;
    bool groupExpanded = true;

    for (const auto& entryRef : entries) {
        const auto& entry = entryRef.get();
        const std::string category = displayCategory(entry);
        if (category != currentCategory) {
            currentCategory = category;
            currentGroup.clear();
            const std::string key = categoryKey(category);
            categoryExpanded = isExpanded(key);
            Row row;
            row.kind = Row::Category;
            row.key = key;
            row.label = category;
            row.description = "category";
            row.depth = 0;
            row.expandable = true;
            row.expanded = categoryExpanded;
            rows.push_back(std::move(row));
        }

        if (!categoryExpanded) {
            continue;
        }

        if (!entry.layerGroup.empty()) {
            if (entry.layerGroup != currentGroup) {
                currentGroup = entry.layerGroup;
                const std::string key = groupKey(currentCategory, currentGroup);
                groupExpanded = isExpanded(key);
                Row row;
                row.kind = Row::Group;
                row.key = key;
                row.label = currentGroup;
                row.description = "layer group";
                row.depth = 1;
                row.expandable = true;
                row.expanded = groupExpanded;
                rows.push_back(std::move(row));
            }
            if (!groupExpanded) {
                continue;
            }
        }

        Row row;
        row.kind = Row::Asset;
        row.key = entry.id;
        row.label = entry.label.empty() ? entry.id : entry.label;
        row.description = entryDescription(entry);
        row.depth = entry.layerGroup.empty() ? 1 : 2;
        row.entry = &entry;
        rows.push_back(std::move(row));
    }

    return rows;
}

bool AssetBrowser::isExpanded(const std::string& key) const {
    return collapsedKeys_.find(key) == collapsedKeys_.end();
}

void AssetBrowser::setExpanded(const std::string& key, bool expanded) {
    if (expanded) {
        collapsedKeys_.erase(key);
    } else {
        collapsedKeys_.insert(key);
    }
}
