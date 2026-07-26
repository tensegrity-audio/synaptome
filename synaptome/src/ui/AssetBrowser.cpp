#include "AssetBrowser.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include <algorithm>
#include <cctype>
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

    std::string lowercase(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    std::vector<std::string> searchTokens(const std::string& query) {
        std::vector<std::string> tokens;
        std::istringstream input(lowercase(query));
        std::string token;
        while (input >> token) {
            tokens.push_back(std::move(token));
        }
        return tokens;
    }

    std::string searchableText(const LayerLibrary::Entry& entry) {
        const std::string description = entry.config.is_object()
            ? entry.config.value("description", std::string())
            : std::string();
        return lowercase(entry.label + " " + entry.id + " " + entry.category + " " +
                         entry.layerGroup + " " + entry.type + " " + entry.model + " " +
                         entry.stateModel + " " + description);
    }

    bool matchesSearch(const LayerLibrary::Entry& entry,
                       const std::vector<std::string>& tokens) {
        if (tokens.empty()) {
            return true;
        }
        const std::string text = searchableText(entry);
        return std::all_of(tokens.begin(), tokens.end(), [&](const std::string& token) {
            return text.find(token) != std::string::npos;
        });
    }

    int searchRank(const LayerLibrary::Entry& entry, const std::string& query) {
        const std::string needle = lowercase(query);
        const std::string label = lowercase(entry.label);
        const std::string id = lowercase(entry.id);
        if (label == needle || id == needle) {
            return 0;
        }
        if (label.rfind(needle, 0) == 0 || id.rfind(needle, 0) == 0) {
            return 1;
        }
        return 2;
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

void AssetBrowser::setSearchQuery(const std::string& query) {
    searchQuery_ = query;
    selected_ = 0;
    clampSelection();
    if (active_) {
        notifyViewModel();
    }
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
        const std::string emptyMessage = searchQuery_.empty()
            ? "Asset library empty"
            : "No assets match: " + searchQuery_ + "  [Backspace] edit  [Esc] clear";
        drawBitmapStringHighlightScaled(emptyMessage, 20.0f, 40.0f, textScale);
        return;
    }

    float y = 40.0f;
    std::string header;
    if (searchQuery_.empty()) {
        header = "Asset Browser  [Type] search   [Up/Down] select   [Left/Right] groups   [Enter] load";
    } else {
        header = "Search: " + searchQuery_ + "  (" + std::to_string(rows.size()) +
                 " matches)  [Backspace] edit   [Esc] clear   [Enter] load";
    }
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
    state.hotkeys.push_back(MenuController::KeyHint{
        0,
        "Type",
        searchQuery_.empty() ? "Search assets" : "Search: " + searchQuery_
    });
    if (!searchQuery_.empty()) {
        state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_BACKSPACE, "Backspace", "Edit search"});
        state.hotkeys.push_back(MenuController::KeyHint{OF_KEY_ESC, "Esc", "Clear search"});
    }
    return state;
}

bool AssetBrowser::handleInput(MenuController& controller, int key) {
    (void)controller;
    if (!active_) {
        return false;
    }

    const int baseKey = key & 0xFFFF;
    const int modifiers = key & MenuController::HOTKEY_MOD_MASK;
    const bool textModifiersOnly =
        (modifiers & ~MenuController::HOTKEY_MOD_SHIFT) == 0;
    bool handled = false;
    bool selectionChanged = false;
    auto rows = visibleRows();
    int rowIndex = rows.empty() ? 0 : ofClamp(selected_, 0, static_cast<int>(rows.size()) - 1);
    const Row* row = rows.empty() ? nullptr : &rows[static_cast<std::size_t>(rowIndex)];

    if (modifiers == 0 && baseKey == OF_KEY_BACKSPACE && !searchQuery_.empty()) {
        searchQuery_.pop_back();
        selected_ = 0;
        clampSelection();
        handled = true;
        selectionChanged = true;
    } else if (modifiers == 0 && baseKey == OF_KEY_ESC && !searchQuery_.empty()) {
        searchQuery_.clear();
        selected_ = 0;
        clampSelection();
        handled = true;
        selectionChanged = true;
    } else if (textModifiersOnly && baseKey >= 32 && baseKey <= 126 &&
               (baseKey != ' ' || !searchQuery_.empty())) {
        searchQuery_.push_back(static_cast<char>(baseKey));
        selected_ = 0;
        clampSelection();
        handled = true;
        selectionChanged = true;
    } else switch (baseKey) {
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
        commandHandler_(*entry, baseKey);
        if (baseKey != OF_KEY_BACKSPACE && baseKey != OF_KEY_ESC) {
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
    searchQuery_.clear();
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
    const auto tokens = searchTokens(searchQuery_);
    entries.reserve(allEntries.size());
    for (const auto& e : allEntries) {
        if (allowEntryPredicate_ && !allowEntryPredicate_(e)) {
            continue;
        }
        if (!matchesSearch(e, tokens)) {
            continue;
        }
        entries.push_back(std::cref(e));
    }
    if (!tokens.empty()) {
        std::stable_sort(entries.begin(), entries.end(), [&](const auto& lhs, const auto& rhs) {
            return searchRank(lhs.get(), searchQuery_) < searchRank(rhs.get(), searchQuery_);
        });
    } else {
        std::stable_sort(entries.begin(), entries.end(), [](const auto& lhsRef, const auto& rhsRef) {
            const auto& lhs = lhsRef.get();
            const auto& rhs = rhsRef.get();
            const std::string lhsCategory = displayCategory(lhs);
            const std::string rhsCategory = displayCategory(rhs);
            const bool lhsScenes = lowercase(lhsCategory) == "scenes";
            const bool rhsScenes = lowercase(rhsCategory) == "scenes";
            if (lhsScenes != rhsScenes) {
                return lhsScenes;
            }
            if (lhsCategory != rhsCategory) {
                return lhsCategory < rhsCategory;
            }
            if (lhs.layerGroup != rhs.layerGroup) {
                return lhs.layerGroup < rhs.layerGroup;
            }
            const std::string lhsLabel = lhs.label.empty() ? lhs.id : lhs.label;
            const std::string rhsLabel = rhs.label.empty() ? rhs.id : rhs.label;
            return lhsLabel < rhsLabel;
        });
    }
    return entries;
}

std::vector<AssetBrowser::Row> AssetBrowser::visibleRows() const {
    std::vector<Row> rows;
    auto entries = visibleEntries();
    rows.reserve(entries.size() + 8);

    if (!searchTokens(searchQuery_).empty()) {
        for (const auto& entryRef : entries) {
            const auto& entry = entryRef.get();
            Row row;
            row.kind = Row::Asset;
            row.key = entry.id;
            row.label = entry.label.empty() ? entry.id : entry.label;
            row.description = entryDescription(entry);
            row.depth = 0;
            row.entry = &entry;
            rows.push_back(std::move(row));
        }
        return rows;
    }

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
    return expandedKeys_.find(key) != expandedKeys_.end();
}

void AssetBrowser::setExpanded(const std::string& key, bool expanded) {
    if (expanded) {
        expandedKeys_.insert(key);
    } else {
        expandedKeys_.erase(key);
    }
}
