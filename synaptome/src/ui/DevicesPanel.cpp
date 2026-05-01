#include "DevicesPanel.h"
#include "ofUtils.h"
#include "ofMath.h"

#include "ofLog.h"
#include "ofGraphics.h"
#include "../io/MidiRouter.h"
#include "ofFileUtils.h"
#include "ofJson.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>
#include <limits>
#include <array>

namespace {
    std::string indentLabel(const std::string& text, int depth) {
        return std::string(static_cast<std::size_t>(std::max(0, depth)) * 2, ' ') + text;
    }

    MenuController::EntryView makeSectionEntry(const std::string& id,
                                               const std::string& label,
                                               const std::string& description,
                                               int depth,
                                               bool selectable) {
        MenuController::EntryView entry;
        entry.id = id;
        entry.label = indentLabel(label, depth);
        entry.description = description;
        entry.selectable = selectable;
        return entry;
    }

    std::string slotSummary(const DevicesPanel::RoleSlot& role) {
        std::ostringstream summary;
        summary << "Type: " << (role.roleDropdown.selectedValue().empty() ? "(none)" : role.roleDropdown.selectedValue());
        summary << "    Label: " << role.labelCell.value();
        summary << "    Binding: " << (role.midiBinding.empty() ? "Unassigned" : role.midiBinding);
        return summary.str();
    }
    constexpr int kMaxColumns = 8;

    std::string lowerCopy(const std::string& value) {
        std::string copy = value;
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return copy;
    }

    bool equalsIgnoreCase(const std::string& a, const std::string& b) {
        return lowerCopy(a) == lowerCopy(b);
    }

    bool containsIgnoreCase(const std::string& text, const std::string& needle) {
        if (needle.empty()) {
            return false;
        }
        return lowerCopy(text).find(lowerCopy(needle)) != std::string::npos;
    }

    void appendUniqueStrings(const ofJson& node, std::vector<std::string>& dest) {
        if (!node.is_array()) {
            return;
        }
        for (const auto& entry : node) {
            if (!entry.is_string()) {
                continue;
            }
            std::string value = entry.get<std::string>();
            if (value.empty()) {
                continue;
            }
            bool exists = std::any_of(dest.begin(), dest.end(), [&](const std::string& existing) {
                return equalsIgnoreCase(existing, value);
            });
            if (!exists) {
                dest.push_back(value);
            }
        }
    }

    bool looksLikeBinding(const ofJson& node) {
        if (!node.is_object()) {
            return false;
        }
        static const char* kBindingKeys[] = { "binding", "channel", "ch", "number", "num", "cc", "note" };
        for (auto* key : kBindingKeys) {
            if (node.contains(key)) {
                return true;
            }
        }
        return false;
    }

    bool parseBindingObject(const ofJson& node, DevicesPanel::RoleSlot& role) {
        std::string type;
        if (node.contains("type") && node["type"].is_string()) {
            type = ofToLower(node["type"].get<std::string>());
        }
        int number = -1;
        if (node.contains("number")) {
            number = node["number"].get<int>();
        } else if (node.contains("num")) {
            number = node["num"].get<int>();
        } else if (node.contains("cc")) {
            number = node["cc"].get<int>();
            if (type.empty()) {
                type = "cc";
            }
        } else if (node.contains("note")) {
            number = node["note"].get<int>();
            if (type.empty()) {
                type = "note";
            }
        }
        if (type.empty()) {
            type = "cc";
        }
        if (number < 0) {
            return false;
        }
        int channel = -1;
        if (node.contains("channel")) {
            channel = node["channel"].get<int>();
        } else if (node.contains("ch")) {
            channel = node["ch"].get<int>();
        }
        role.bindingType = type;
        role.bindingNumber = number;
        role.bindingChannel = channel;
        return true;
    }

    std::string bindingLabelForSlot(const DevicesPanel::RoleSlot& role) {
        if (role.bindingPending) {
            return "Listening for MIDI...";
        }
        if (!role.bindingDisplayOverride.empty()) {
            return role.bindingDisplayOverride;
        }
        if (!role.bindingType.empty() && role.bindingNumber >= 0) {
            std::string label;
            if (role.bindingType == "cc") {
                label = "CC";
            } else if (role.bindingType == "note") {
                label = "Note";
            } else if (role.bindingType == "button") {
                label = "Button";
            } else if (role.bindingType == "fader") {
                label = "Fader";
            } else {
                label = ofToUpper(role.bindingType);
            }
            label += " " + ofToString(role.bindingNumber);
            if (role.bindingChannel > 0) {
                label += " (Ch " + ofToString(role.bindingChannel) + ")";
            }
            return label;
        }
        return "Unassigned";
    }
    void addRoleSlot(const ofJson& slotNode,
                     const std::string& slotId,
                     const std::string& baseId,
                     DevicesPanel::GroupRow& group) {
        group.roles.emplace_back(baseId + "." + slotId);
        auto& role = group.roles.back();
        role.slotId = slotId;
        std::string label = slotId;
        std::string roleType = role.roleDropdown.selectedValue();
        float sensitivity = role.sensitivity.value();
        if (slotNode.is_object()) {
            label = slotNode.value("label", label);
            roleType = slotNode.value("role", roleType);
            sensitivity = slotNode.value("sensitivity", sensitivity);
        } else if (slotNode.is_string()) {
            label = slotNode.get<std::string>();
        }
        role.labelCell.setValue(label);
        role.roleDropdown.setSelectedValue(roleType);
        role.sensitivity.setValue(sensitivity);
        role.bindingType.clear();
        role.bindingNumber = -1;
        role.bindingChannel = -1;
        role.bindingDisplayOverride.clear();
        role.bindingPending = false;
        role.bindingEdited = false;
        role.hasOriginalBinding = false;
        role.originalBinding = ofJson();
        if (slotNode.is_object()) {
            if (slotNode.contains("binding") && looksLikeBinding(slotNode["binding"])) {
                if (parseBindingObject(slotNode["binding"], role)) {
                    role.originalBinding = slotNode["binding"];
                    role.hasOriginalBinding = true;
                }
            } else if (looksLikeBinding(slotNode)) {
                if (parseBindingObject(slotNode, role)) {
                    role.originalBinding = slotNode;
                    role.hasOriginalBinding = true;
                }
            } else if (slotNode.contains("binding")) {
                const auto& bindingNode = slotNode["binding"];
                if (bindingNode.is_string()) {
                    role.bindingDisplayOverride = bindingNode.get<std::string>();
                } else if (bindingNode.is_number_integer()) {
                    role.bindingDisplayOverride = std::string("Value ") + ofToString(bindingNode.get<int>());
                } else if (bindingNode.is_array()) {
                    role.bindingDisplayOverride = bindingNode.dump();
                } else if (bindingNode.is_object() && !parseBindingObject(bindingNode, role)) {
                    role.bindingDisplayOverride = bindingNode.dump();
                }
            }
        } else if (slotNode.is_string()) {
            role.bindingDisplayOverride = slotNode.get<std::string>();
        } else if (slotNode.is_number_integer()) {
            role.bindingDisplayOverride = std::string("Value ") + ofToString(slotNode.get<int>());
        }
        role.midiBinding = bindingLabelForSlot(role);
    }

    void populateGroupFromColumn(const std::string& baseId,
                                 const ofJson& columnNode,
                                 DevicesPanel::GroupRow& group) {
        auto processArray = [&](const ofJson& arr) {
            int fallbackIndex = 1;
            for (const auto& entry : arr) {
                if (entry.is_object()) {
                    std::string slotId = entry.value("id", entry.value("slot", std::string()));
                    if (slotId.empty()) {
                        slotId = std::string("Slot ") + ofToString(fallbackIndex++);
                    }
                    addRoleSlot(entry, slotId, baseId, group);
                } else {
                    std::string slotId = std::string("Slot ") + ofToString(fallbackIndex++);
                    addRoleSlot(entry, slotId, baseId, group);
                }
            }
        };
        if (columnNode.is_object()) {
            if (columnNode.contains("slots") && columnNode["slots"].is_array()) {
                processArray(columnNode["slots"]);
                return;
            }
            if (columnNode.contains("roles") && columnNode["roles"].is_array()) {
                processArray(columnNode["roles"]);
                return;
            }
            for (auto it = columnNode.begin(); it != columnNode.end(); ++it) {
                const std::string key = it.key();
                if (key == "name" || key == "label" || key == "description" || key == "ports" || key == "portHints") {
                    continue;
                }
                addRoleSlot(it.value(), key, baseId, group);
            }
        } else if (columnNode.is_array()) {
            processArray(columnNode);
        }
    }

    std::string deviceStatusLabel(const DevicesPanel::DeviceRow& device) {
        std::string status = device.connected ? "Online" : "Offline";
        if (!device.portLabel.empty()) {
            status += " @ " + device.portLabel;
        }
        if (device.dirty) {
            status += " - Unsaved changes";
            return status;
        }
        if (device.hasMapping) {
            status += " - Mapping saved";
        } else if (device.connected) {
            status += " - No mapping";
        } else {
            status += " - Mapping missing";
        }
        return status;
    }
}

void DevicesPanel::onEnter(MenuController& controller) {
    controller_ = &controller;
    active_ = true;
    markRosterDirty();
    controller.requestViewModelRefresh();
}
void DevicesPanel::onExit(MenuController& controller) {
    (void)controller;
    active_ = false;
    cancelPendingLearn();
    controller_ = nullptr;
}

DevicesPanel::RoleSlot::RoleSlot(const std::string& baseId)
    : roleDropdown(baseId + ".role", "Role Type")
    , sensitivity(baseId + ".sensitivity", "Sensitivity", 1.0f, 0.1f, 4.0f, 0.05f, 0.25f)
    , labelCell(baseId + ".label", "Label", "Unnamed") {
    roleDropdown.setOptions({
        {"fader", "Fader"},
        {"knob", "Knob"},
        {"button", "Button"},
        {"shift", "Shift"},
        {"macro", "Macro"}
    });
    roleDropdown.setSelectedValue("fader");
    sensitivity.setUnitLabel("x");
    midiBinding = "Unassigned";
    slotId = baseId;
    bindingType.clear();
    bindingDisplayOverride.clear();
    bindingChannel = -1;
    bindingNumber = -1;
    bindingEdited = false;
    bindingPending = false;
    hasOriginalBinding = false;
    originalBinding = ofJson();
}

DevicesPanel::DevicesPanel() = default;

DevicesPanel::~DevicesPanel() {
    cancelPendingLearn();
}

void DevicesPanel::setMidiRouter(MidiRouter* router) {
    if (midiRouter_ != router) {
        cancelPendingLearn();
    }
    midiRouter_ = router;
    markRosterDirty();
}

void DevicesPanel::setDeviceMapsDirectory(const std::string& path) {
    deviceMapsDirectory_ = path;
    if (!deviceMapsDirectory_.empty()) {
        ofDirectory::createDirectory(deviceMapsDirectory_, true, true);
    }
    markRosterDirty();
}

void DevicesPanel::markRosterDirty() const {
    rosterDirty_ = true;
}
MenuController::StateView DevicesPanel::view() const {
    refreshDeviceRoster();
    MenuController::StateViewBuilder builder;
    entryRefs_.clear();

    if (mode_ == Mode::DeviceList) {
        builder.addHotkey(OF_KEY_RETURN, "Enter", "Edit mapping");
        builder.addHotkey(OF_KEY_ESC, "Esc", "Close");
        return buildDeviceListView(builder);
    }

    builder.addHotkey(OF_KEY_RETURN, "Enter", "Edit role");
    builder.addHotkey(OF_KEY_ESC, "Esc", "Back to devices");
    builder.addHotkey(OF_KEY_LEFT, "Left", "Prev group");
    builder.addHotkey(OF_KEY_RIGHT, "Right", "Next group");
    builder.addHotkey('S', "S", "Save mapping");
    builder.addHotkey('A', "A", "Apply to rig");
    return buildGroupDetailView(builder);
}

MenuController::StateView DevicesPanel::buildDeviceListView(MenuController::StateViewBuilder& builder) const {
    MenuController::EntryView header;
    header.id = "devices.header";
    header.label = "Devices";
    header.selectable = false;
    builder.addEntry(header);
    entryRefs_.push_back(EntryRef{ EntryRef::Kind::GroupHeader, -1, -1, -1, false });

    for (std::size_t i = 0; i < devices_.size(); ++i) {
        const auto& device = devices_[i];
        MenuController::EntryView entry;
        entry.id = "device." + device.id;
        std::string label = device.name + (device.connected ? " (online)" : " (offline)");
        if (device.dirty) {
            label += " *";
        }
        entry.label = label;
        entry.description = deviceStatusLabel(device);
        entry.selectable = true;
        builder.addEntry(entry);
        EntryRef ref;
        ref.kind = EntryRef::Kind::DeviceRow;
        ref.deviceIndex = static_cast<int>(i);
        ref.selectable = true;
        entryRefs_.push_back(ref);
    }

    if (entryRefs_.size() <= 1) {
        builder.setSelectedIndex(-1);
    } else {
        selectedEntryIndex_ = ofClamp(selectedEntryIndex_, 1, static_cast<int>(entryRefs_.size()) - 1);
        builder.setSelectedIndex(selectedEntryIndex_);
    }

    return builder.build();
}

MenuController::StateView DevicesPanel::buildGroupDetailView(MenuController::StateViewBuilder& builder) const {
    if (devices_.empty()) {
        return buildDeviceListView(builder);
    }

    selectedDeviceIndex_ = ofClamp(selectedDeviceIndex_, 0, static_cast<int>(devices_.size()) - 1);
    const auto& device = devices_[selectedDeviceIndex_];

    MenuController::EntryView header;
    header.id = "detail.header";
    std::string headerLabel = device.name;
    if (device.dirty) {
        headerLabel += " *";
    }
    header.label = headerLabel + " - Mappings";
    header.description = deviceStatusLabel(device);
    header.selectable = false;
    builder.addEntry(header);
    entryRefs_.push_back(EntryRef{ EntryRef::Kind::GroupHeader, selectedDeviceIndex_, -1, -1, false });

    if (device.dirty) {
        auto dirty = makeSectionEntry("detail.dirty", "Unsaved changes", "Press S or select Save mapping", 0, false);
        builder.addEntry(dirty);
        entryRefs_.push_back(EntryRef{ EntryRef::Kind::GroupHeader, selectedDeviceIndex_, -1, -1, false });

        auto save = makeSectionEntry("detail.save", "Save mapping", "Write changes to disk", 1, true);
        builder.addEntry(save);
        EntryRef saveRef;
        saveRef.kind = EntryRef::Kind::Action;
        saveRef.action = EntryRef::ActionId::SaveDevice;
        saveRef.deviceIndex = selectedDeviceIndex_;
        saveRef.selectable = true;
        entryRefs_.push_back(saveRef);
    }

    const bool hasGroups = !device.groups.empty();
    const bool atColumnLimit = static_cast<int>(device.groups.size()) >= kMaxColumns;
    if (hasGroups && !atColumnLimit) {
        auto addColumn = makeSectionEntry("detail.addColumn", "Add column", "Create another column with default slots", 1, true);
        builder.addEntry(addColumn);
        EntryRef addRef;
        addRef.kind = EntryRef::Kind::Action;
        addRef.action = EntryRef::ActionId::CreateDefaultGroup;
        addRef.deviceIndex = selectedDeviceIndex_;
        addRef.selectable = true;
        entryRefs_.push_back(addRef);
    } else if (hasGroups && atColumnLimit) {
        auto limit = makeSectionEntry("detail.columnLimit", "Column limit reached", "All 8 columns configured", 1, false);
        builder.addEntry(limit);
        entryRefs_.push_back(EntryRef{ EntryRef::Kind::GroupHeader, selectedDeviceIndex_, -1, -1, false });
    }

    if (device.groups.empty()) {
        auto empty = makeSectionEntry("detail.empty", "No groups defined", "Press Enter to create a default column", 0, false);
        builder.addEntry(empty);
        entryRefs_.push_back(EntryRef{ EntryRef::Kind::GroupHeader, selectedDeviceIndex_, -1, -1, false });

        auto action = makeSectionEntry("detail.create", "Create default mapping", "Generate a starter column for this device", 1, true);
        builder.addEntry(action);
        EntryRef ref;
        ref.kind = EntryRef::Kind::Action;
        ref.action = EntryRef::ActionId::CreateDefaultGroup;
        ref.deviceIndex = selectedDeviceIndex_;
        ref.selectable = true;
        entryRefs_.push_back(ref);
        selectedEntryIndex_ = static_cast<int>(entryRefs_.size()) - 1;
        builder.setSelectedIndex(selectedEntryIndex_);
        return builder.build();
    }

    selectedGroupIndex_ = ofClamp(selectedGroupIndex_, 0, static_cast<int>(device.groups.size()) - 1);
    const auto& group = device.groups[selectedGroupIndex_];

    auto groupHeader = makeSectionEntry("detail.group." + group.id, "Column: " + group.name, "Slots: " + ofToString(group.roles.size()), 0, false);
    builder.addEntry(groupHeader);
    entryRefs_.push_back(EntryRef{ EntryRef::Kind::GroupHeader, selectedDeviceIndex_, selectedGroupIndex_, -1, false });

    for (std::size_t i = 0; i < group.roles.size(); ++i) {
        const auto& role = group.roles[i];
        const int slotDepth = 1;
        const int fieldDepth = 2;

        MenuController::EntryView slotHeader = makeSectionEntry(
            "detail.slot." + group.id + "." + ofToString(i),
            "Slot " + role.slotId + " - " + role.labelCell.value(),
            slotSummary(role),
            slotDepth,
            false);
        builder.addEntry(slotHeader);
        entryRefs_.push_back(EntryRef{ EntryRef::Kind::GroupHeader, selectedDeviceIndex_, selectedGroupIndex_, static_cast<int>(i), false });

        EntryRef dropdownRef{ EntryRef::Kind::RoleDropdown, selectedDeviceIndex_, selectedGroupIndex_, static_cast<int>(i), true };
        auto dropdownEntry = role.roleDropdown.toEntryView(false);
        dropdownEntry.label = indentLabel("Type", fieldDepth);
        builder.addEntry(dropdownEntry);
        entryRefs_.push_back(dropdownRef);

        EntryRef numericRef{ EntryRef::Kind::RoleSensitivity, selectedDeviceIndex_, selectedGroupIndex_, static_cast<int>(i), true };
        auto numericEntry = role.sensitivity.toEntryView(false);
        numericEntry.label = indentLabel("Sensitivity", fieldDepth);
        builder.addEntry(numericEntry);
        entryRefs_.push_back(numericRef);

        EntryRef labelRef{ EntryRef::Kind::RoleLabel, selectedDeviceIndex_, selectedGroupIndex_, static_cast<int>(i), true };
        auto labelEntry = role.labelCell.toEntryView(false);
        labelEntry.label = indentLabel("Label", fieldDepth);
        builder.addEntry(labelEntry);
        entryRefs_.push_back(labelRef);

        MenuController::EntryView binding = makeSectionEntry(
            "detail.binding." + group.id + "." + ofToString(i),
            "Binding",
            role.midiBinding,
            fieldDepth,
            true);
        binding.pendingChanges = role.bindingPending;
        builder.addEntry(binding);
        entryRefs_.push_back(EntryRef{ EntryRef::Kind::RoleBinding, selectedDeviceIndex_, selectedGroupIndex_, static_cast<int>(i), true });
    }

    if (entryRefs_.empty()) {
        selectedEntryIndex_ = -1;
        builder.setSelectedIndex(-1);
    } else {
        int lastIndex = static_cast<int>(entryRefs_.size()) - 1;
        if (selectedEntryIndex_ < 0 || selectedEntryIndex_ > lastIndex) {
            selectedEntryIndex_ = -1;
            for (std::size_t i = 0; i < entryRefs_.size(); ++i) {
                if (entryRefs_[i].selectable) {
                    selectedEntryIndex_ = static_cast<int>(i);
                    break;
                }
            }
        }
        if (selectedEntryIndex_ >= 0 && selectedEntryIndex_ <= lastIndex && !entryRefs_[selectedEntryIndex_].selectable) {
            int previous = selectedEntryIndex_;
            moveSelection(1);
            if (selectedEntryIndex_ == previous || selectedEntryIndex_ < 0 || !entryRefs_[selectedEntryIndex_].selectable) {
                selectedEntryIndex_ = -1;
            }
        }
        builder.setSelectedIndex(selectedEntryIndex_);
    }

    return builder.build();
}
void DevicesPanel::moveSelection(int delta) const {
    if (entryRefs_.empty()) {
        selectedEntryIndex_ = -1;
        return;
    }
    int count = static_cast<int>(entryRefs_.size());
    int next = selectedEntryIndex_;
    for (int i = 0; i < count; ++i) {
        next = (next + delta + count) % count;
        if (entryRefs_[next].selectable) {
            selectedEntryIndex_ = next;
            return;
        }
    }
}

bool DevicesPanel::handleInput(MenuController& controller, int key) {
    int baseKey = key & 0xFFFF;

    if (mode_ == Mode::DeviceList) {
        switch (baseKey) {
        case OF_KEY_UP:
            moveSelection(-1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_DOWN:
            moveSelection(1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_RETURN:
            if (activateSelection(controller, key)) {
                controller.requestViewModelRefresh();
                return true;
            }
            break;
        default:
            break;
        }
        return false;
    }

    rebuildTreeNodes();
    ensureTreeSelectionValid();
    ensureGridSelectionValid();
    if (mode_ == Mode::GroupDetail && focusPane_ == FocusPane::Grid) {
        // Keep the entryRefs selection aligned with the highlighted grid cell so
        // Enter immediately targets the visible role dropdown.
        syncEntrySelectionFromGrid();
    }

    if (baseKey == OF_KEY_ESC) {
        cancelPendingLearn();
        mode_ = Mode::DeviceList;
        focusPane_ = FocusPane::Tree;
        selectedEntryIndex_ = 1;
        controller.requestViewModelRefresh();
        return true;
    }

    if (baseKey == 'S' || baseKey == 's') {
        cancelPendingLearn();
        if (saveSelectedDevice()) {
            controller.requestViewModelRefresh();
        }
        return true;
    }

    if (baseKey == OF_KEY_TAB) {
        cancelPendingLearn();
        focusPane_ = (focusPane_ == FocusPane::Tree) ? FocusPane::Grid : FocusPane::Tree;
        if (focusPane_ == FocusPane::Grid) {
            ensureGridSelectionValid();
            syncEntrySelectionFromGrid();
        }
        controller.requestViewModelRefresh();
        return true;
    }

    auto applyTreeDelta = [&](int delta) {
        if (treeNodes_.empty()) {
            return;
        }
        moveTreeSelection(delta);
        applyTreeSelection(selectedTreeNodeIndex_);
        focusPane_ = FocusPane::Grid;
    };

    if (baseKey == '[' || baseKey == '{') {
        cancelPendingLearn();
        applyTreeDelta(-1);
        controller.requestViewModelRefresh();
        return true;
    }

    if (baseKey == ']' || baseKey == '}') {
        cancelPendingLearn();
        applyTreeDelta(1);
        controller.requestViewModelRefresh();
        return true;
    }

    if (focusPane_ == FocusPane::Tree) {
        switch (baseKey) {
        case OF_KEY_UP:
            moveTreeSelection(-1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_DOWN:
            moveTreeSelection(1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_LEFT:
            focusTreeParent();
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_RIGHT:
            focusTreeChild();
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_RETURN:
        case ' ':
            cancelPendingLearn();
            if (selectedTreeNodeIndex_ >= 0) {
                applyTreeSelection(selectedTreeNodeIndex_);
                focusPane_ = FocusPane::Grid;
                controller.requestViewModelRefresh();
            }
            return true;
        default:
            break;
        }
        return false;
    }

    if (dropdownCapturesNavigation(baseKey)) {
        if (selectedEntryIndex_ >= 0 && selectedEntryIndex_ < static_cast<int>(entryRefs_.size())) {
            const EntryRef& ref = entryRefs_[selectedEntryIndex_];
            if (handleRoleInput(ref, key)) {
                controller.requestViewModelRefresh();
            }
        }
        return true;
    }

    switch (baseKey) {
    case OF_KEY_UP:
        moveGridRow(-1);
        controller.requestViewModelRefresh();
        return true;
    case OF_KEY_DOWN:
        moveGridRow(1);
        controller.requestViewModelRefresh();
        return true;
    case OF_KEY_LEFT:
        moveGridColumn(-1);
        controller.requestViewModelRefresh();
        return true;
    case OF_KEY_RIGHT:
        moveGridColumn(1);
        controller.requestViewModelRefresh();
        return true;
    default:
        break;
    }

    if (baseKey == OF_KEY_RETURN || baseKey == ' ') {
        if (activateSelection(controller, key)) {
            controller.requestViewModelRefresh();
            return true;
        }
    }

    if (selectedEntryIndex_ < 0 || selectedEntryIndex_ >= static_cast<int>(entryRefs_.size())) {
        return false;
    }

    const EntryRef& ref = entryRefs_[selectedEntryIndex_];
    if (!ref.selectable) {
        return false;
    }

    bool handled = handleRoleInput(ref, key);
    if (handled) {
        controller.requestViewModelRefresh();
    }
    return handled;
}
bool DevicesPanel::activateSelection(MenuController& controller, int /*key*/) {
    if (selectedEntryIndex_ < 0 || selectedEntryIndex_ >= static_cast<int>(entryRefs_.size())) {
        return false;
    }
    if (mode_ == Mode::GroupDetail && focusPane_ == FocusPane::Grid) {
        // If the grid highlight moved (Tab/arrow) without the entry index staying
        // in sync, realign it before resolving the selected reference.
        syncEntrySelectionFromGrid();
        if (selectedEntryIndex_ < 0 || selectedEntryIndex_ >= static_cast<int>(entryRefs_.size())) {
            return false;
        }
    }
    const EntryRef& ref = entryRefs_[selectedEntryIndex_];
    if (!ref.selectable) {
        return false;
    }

    if (mode_ == Mode::DeviceList && ref.kind == EntryRef::Kind::DeviceRow) {
        selectedDeviceIndex_ = ref.deviceIndex;
        selectedGroupIndex_ = 0;
        mode_ = Mode::GroupDetail;
        selectedEntryIndex_ = 2;
        focusPane_ = FocusPane::Tree;
        gridRowIndex_ = 0;
        gridColumnIndex_ = 1;
        activeTreeNodeIndex_ = -1;
        rebuildTreeNodes();
        ensureTreeSelectionValid();
        if (activeTreeNodeIndex_ >= 0) {
            selectedTreeNodeIndex_ = activeTreeNodeIndex_;
        }
        syncEntrySelectionFromGrid();
        return true;
    }

    if (mode_ == Mode::GroupDetail) {
        if (ref.kind == EntryRef::Kind::Action) {
            switch (ref.action) {
            case EntryRef::ActionId::CreateDefaultGroup:
                if (createDefaultGroup(ref.deviceIndex)) {
                    selectedDeviceIndex_ = ref.deviceIndex;
                    selectedGroupIndex_ = ofClamp(static_cast<int>(devices_[ref.deviceIndex].groups.size()) - 1, 0, static_cast<int>(devices_[ref.deviceIndex].groups.size()) - 1);
                    selectedEntryIndex_ = 2;
                    markDeviceDirty(ref.deviceIndex);
                    return true;
                }
                return false;
            case EntryRef::ActionId::SaveDevice:
                selectedDeviceIndex_ = ref.deviceIndex;
                cancelPendingLearn();
                if (saveSelectedDevice()) {
                    return true;
                }
                return false;
            case EntryRef::ActionId::None:
            default:
                break;
            }
            return false;
        }
        return handleRoleInput(ref, OF_KEY_RETURN);
    }

    return false;
}

bool DevicesPanel::handleRoleInput(const EntryRef& ref, int key) {
    if (ref.deviceIndex < 0 || ref.deviceIndex >= static_cast<int>(devices_.size())) {
        return false;
    }
    auto& device = devices_[ref.deviceIndex];
    if (ref.groupIndex < 0 || ref.groupIndex >= static_cast<int>(device.groups.size())) {
        return false;
    }
    auto& group = device.groups[ref.groupIndex];
    if (ref.roleIndex < 0 || ref.roleIndex >= static_cast<int>(group.roles.size())) {
        return false;
    }
    auto& role = group.roles[ref.roleIndex];
    int baseKey = key & 0xFFFF;
    bool handled = false;

    switch (ref.kind) {
    case EntryRef::Kind::RoleDropdown:
        handled = role.roleDropdown.handleKey(key);
        if ((key & 0xFFFF) == OF_KEY_RETURN) {
            ofLogNotice("DevicesPanel") << "RoleDropdown Enter device=" << device.name
                                        << " group=" << group.id << " slot=" << role.slotId
                                        << " handled=" << (handled ? "true" : "false");
        }
        break;
    case EntryRef::Kind::RoleSensitivity:
        handled = role.sensitivity.handleKey(key);
        break;
    case EntryRef::Kind::RoleLabel:
        handled = role.labelCell.handleKey(key);
        break;
    case EntryRef::Kind::RoleBinding:
        if (baseKey == OF_KEY_RETURN) {
            return beginBindingLearn(ref);
        }
        if (baseKey == OF_KEY_BACKSPACE || baseKey == OF_KEY_DEL) {
            return clearBinding(ref);
        }
        break;
    default:
        break;
    }

    if (handled) {
        markDeviceDirty(ref.deviceIndex);
        return true;
    }
    return false;
}
void DevicesPanel::refreshDeviceRoster() const {
    if (pendingLearn_.active) {
        return;
    }
    for (const auto& device : devices_) {
        if (device.dirty) {
            return;
        }
    }

    uint64_t now = ofGetElapsedTimeMillis();
    if (!rosterDirty_ && now - lastRosterRefreshMs_ < rosterRefreshIntervalMs_) {
        return;
    }
    lastRosterRefreshMs_ = now;
    rosterDirty_ = false;

    std::string previousDeviceId;
    if (!devices_.empty() && selectedDeviceIndex_ >= 0 && selectedDeviceIndex_ < static_cast<int>(devices_.size())) {
        previousDeviceId = devices_[selectedDeviceIndex_].id;
    }

    std::vector<DeviceRow> refreshed;

    if (!deviceMapsDirectory_.empty()) {
        ofDirectory dir(deviceMapsDirectory_);
        if (dir.exists()) {
            dir.allowExt("json");
            dir.listDir();
            for (std::size_t i = 0; i < dir.size(); ++i) {
                const auto& file = dir.getFile(i);
                try {
                    ofJson doc = ofLoadJson(file.getAbsolutePath());
                    if (!doc.is_object()) {
                        doc = ofJson::object();
                    }
                    DeviceRow device;
                    device.hasMapping = true;
                    device.connected = false;
                    device.dirty = false;
                    device.sourcePath = file.getAbsolutePath();
                    device.originalDoc = doc;
                    device.id = doc.value("deviceId", file.getBaseName());
                    if (device.id.empty()) {
                        device.id = file.getBaseName();
                    }
                    device.name = doc.value("name", std::string());
                    device.model = doc.value("model", device.name);
                    if (device.name.empty()) {
                        device.name = device.model.empty() ? device.id : device.model;
                    }
                    if (doc.contains("portHints")) {
                        appendUniqueStrings(doc["portHints"], device.portHints);
                    }
                    if (doc.contains("ports")) {
                        appendUniqueStrings(doc["ports"], device.ports);
                    }
                    if (device.portHints.empty() && !device.model.empty()) {
                        device.portHints.push_back(device.model);
                    }
                    if (doc.contains("columns") && doc["columns"].is_object()) {
                        for (const auto& column : doc["columns"].items()) {
                            GroupRow group;
                            group.id = column.key();
                            const auto& columnNode = column.value();
                            std::string name = columnNode.value("name", std::string());
                            if (name.empty()) {
                                name = std::string("Column ") + column.key();
                            }
                            group.name = name;
                            populateGroupFromColumn(device.id + "." + group.id, columnNode, group);
                            device.groups.push_back(std::move(group));
                        }
                    }
                    refreshed.push_back(std::move(device));
                } catch (const std::exception& ex) {
                    ofLogWarning("DevicesPanel") << "Failed to parse device map " << file.getAbsolutePath() << ": " << ex.what();
                }
            }
        }
    }

    std::vector<std::string> ports;
    if (midiRouter_) {
        ports = midiRouter_->availableInputPorts();
    }

    for (const auto& portName : ports) {
        bool matched = false;
        for (auto& device : refreshed) {
            if (deviceMatchesPort(device, portName)) {
                device.connected = true;
                device.portLabel = portName;
                matched = true;
                break;
            }
        }
        if (!matched) {
            DeviceRow device;
            device.id = lowerCopy(portName);
            device.name = portName;
            device.connected = true;
            device.portLabel = portName;
            device.hasMapping = false;
            refreshed.push_back(std::move(device));
        }
    }

    if (!previousDeviceId.empty()) {
        for (std::size_t i = 0; i < refreshed.size(); ++i) {
            if (refreshed[i].id == previousDeviceId) {
                selectedDeviceIndex_ = static_cast<int>(i);
                break;
            }
        }
    } else {
        selectedDeviceIndex_ = 0;
    }

    devices_ = std::move(refreshed);
}

bool DevicesPanel::deviceMatchesPort(const DeviceRow& device, const std::string& portName) const {
    if (portName.empty()) {
        return false;
    }
    if (equalsIgnoreCase(device.portLabel, portName)) {
        return true;
    }
    for (const auto& hint : device.portHints) {
        if (containsIgnoreCase(portName, hint)) {
            return true;
        }
    }
    for (const auto& port : device.ports) {
        if (equalsIgnoreCase(port, portName)) {
            return true;
        }
    }
    return false;
}
bool DevicesPanel::beginBindingLearn(const EntryRef& ref) {
    if (!midiRouter_) {
        return false;
    }
    if (ref.deviceIndex < 0 || ref.deviceIndex >= static_cast<int>(devices_.size())) {
        return false;
    }
    auto& device = devices_[ref.deviceIndex];
    if (ref.groupIndex < 0 || ref.groupIndex >= static_cast<int>(device.groups.size())) {
        return false;
    }
    auto& group = device.groups[ref.groupIndex];
    if (ref.roleIndex < 0 || ref.roleIndex >= static_cast<int>(group.roles.size())) {
        return false;
    }
    cancelPendingLearn();
    auto& role = group.roles[ref.roleIndex];
    role.bindingPending = true;
    updateBindingDescription(role);
    pendingLearn_.active = true;
    pendingLearn_.deviceIndex = ref.deviceIndex;
    pendingLearn_.groupIndex = ref.groupIndex;
    pendingLearn_.roleIndex = ref.roleIndex;
    midiRouter_->captureNextMidiControl([this, deviceIndex = ref.deviceIndex, groupIndex = ref.groupIndex, roleIndex = ref.roleIndex](const MidiRouter::CapturedMidiControl& capture) {
        applyBindingCapture(deviceIndex, groupIndex, roleIndex, capture);
        if (controller_) {
            controller_->requestViewModelRefresh();
        }
    });
    return true;
}

bool DevicesPanel::clearBinding(const EntryRef& ref) {
    if (ref.deviceIndex < 0 || ref.deviceIndex >= static_cast<int>(devices_.size())) {
        return false;
    }
    auto& device = devices_[ref.deviceIndex];
    if (ref.groupIndex < 0 || ref.groupIndex >= static_cast<int>(device.groups.size())) {
        return false;
    }
    auto& group = device.groups[ref.groupIndex];
    if (ref.roleIndex < 0 || ref.roleIndex >= static_cast<int>(group.roles.size())) {
        return false;
    }
    auto& role = group.roles[ref.roleIndex];
    role.bindingPending = false;
    role.bindingType.clear();
    role.bindingNumber = -1;
    role.bindingChannel = -1;
    role.bindingDisplayOverride.clear();
    role.bindingEdited = true;
    role.hasOriginalBinding = false;
    role.originalBinding = ofJson();
    updateBindingDescription(role);
    markDeviceDirty(ref.deviceIndex);
    if (controller_) {
        controller_->requestViewModelRefresh();
    }
    return true;
}

bool DevicesPanel::saveSelectedDevice() {
    if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= static_cast<int>(devices_.size())) {
        return false;
    }
    auto& device = devices_[selectedDeviceIndex_];
    if (!device.dirty) {
        return true;
    }
    if (device.id.empty()) {
        ofLogWarning("DevicesPanel") << "Cannot save device with empty id";
        return false;
    }

    ofJson doc = device.originalDoc.is_object() ? device.originalDoc : ofJson::object();
    doc["deviceId"] = device.id;
    doc["name"] = device.name;
    if (!device.model.empty()) {
        doc["model"] = device.model;
    }
    if (!device.portHints.empty()) {
        doc["portHints"] = device.portHints;
    }
    if (!device.ports.empty()) {
        doc["ports"] = device.ports;
    }

    ofJson columns = ofJson::object();
    for (const auto& group : device.groups) {
        ofJson columnNode;
        columnNode["name"] = group.name;
        ofJson slots = ofJson::array();
        for (const auto& role : group.roles) {
            ofJson slotNode;
            slotNode["id"] = role.slotId;
            slotNode["label"] = role.labelCell.value();
            slotNode["role"] = role.roleDropdown.selectedValue();
            slotNode["sensitivity"] = role.sensitivity.value();
            if (!role.bindingType.empty() && role.bindingNumber >= 0) {
                ofJson binding;
                binding["type"] = role.bindingType;
                binding["number"] = role.bindingNumber;
                if (role.bindingChannel > 0) {
                    binding["channel"] = role.bindingChannel;
                }
                slotNode["binding"] = binding;
            } else if (!role.bindingDisplayOverride.empty()) {
                slotNode["binding"] = role.bindingDisplayOverride;
            }
            slots.push_back(slotNode);
        }
        columnNode["slots"] = slots;
        columns[group.id] = columnNode;
    }
    doc["columns"] = columns;

    if (device.sourcePath.empty()) {
        if (deviceMapsDirectory_.empty()) {
            ofLogWarning("DevicesPanel") << "Device maps directory not set";
            return false;
        }
        device.sourcePath = ofFilePath::join(deviceMapsDirectory_, device.id + ".json");
    }
    if (!ofSavePrettyJson(device.sourcePath, doc)) {
        ofLogWarning("DevicesPanel") << "Failed to save device map" << device.sourcePath;
        return false;
    }
    device.originalDoc = doc;
    device.hasMapping = true;
    device.dirty = false;
    markRosterDirty();
    ofLogNotice("DevicesPanel") << "Saved mapping for " << device.name << " -> " << device.sourcePath;
    return true;
}

void DevicesPanel::cancelPendingLearn() {
    if (!pendingLearn_.active) {
        return;
    }
    if (midiRouter_) {
        midiRouter_->cancelMidiControlCapture();
    }
    if (pendingLearn_.deviceIndex >= 0 && pendingLearn_.deviceIndex < static_cast<int>(devices_.size())) {
        auto& device = devices_[pendingLearn_.deviceIndex];
        if (pendingLearn_.groupIndex >= 0 && pendingLearn_.groupIndex < static_cast<int>(device.groups.size())) {
            auto& group = device.groups[pendingLearn_.groupIndex];
            if (pendingLearn_.roleIndex >= 0 && pendingLearn_.roleIndex < static_cast<int>(group.roles.size())) {
                auto& role = group.roles[pendingLearn_.roleIndex];
                role.bindingPending = false;
                updateBindingDescription(role);
            }
        }
    }
    pendingLearn_ = PendingLearn{};
}
void DevicesPanel::markDeviceDirty(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        return;
    }
    devices_[deviceIndex].dirty = true;
}

void DevicesPanel::updateBindingDescription(RoleSlot& role) const {
    role.midiBinding = bindingLabelForSlot(role);
}

bool DevicesPanel::applyBindingCapture(int deviceIndex, int groupIndex, int roleIndex, const MidiRouter::CapturedMidiControl& capture) {
    pendingLearn_ = PendingLearn{};
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        return false;
    }
    auto& device = devices_[deviceIndex];
    if (groupIndex < 0 || groupIndex >= static_cast<int>(device.groups.size())) {
        return false;
    }
    auto& group = device.groups[groupIndex];
    if (roleIndex < 0 || roleIndex >= static_cast<int>(group.roles.size())) {
        return false;
    }
    auto& role = group.roles[roleIndex];
    role.bindingPending = false;
    role.bindingType = ofToLower(capture.type.empty() ? std::string("cc") : capture.type);
    role.bindingChannel = capture.channel;
    role.bindingNumber = capture.number;
    role.bindingDisplayOverride.clear();
    role.bindingEdited = true;
    role.originalBinding = ofJson();
    role.hasOriginalBinding = false;
    updateBindingDescription(role);
    markDeviceDirty(deviceIndex);
    return true;
}

bool DevicesPanel::createDefaultGroup(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        return false;
    }
    auto& device = devices_[deviceIndex];
    if (static_cast<int>(device.groups.size()) >= kMaxColumns) {
        ofLogWarning("DevicesPanel") << "Column limit reached for " << device.name;
        return false;
    }
    GroupRow group;
    int groupNumber = static_cast<int>(device.groups.size()) + 1;
    group.id = "column" + ofToString(groupNumber);
    group.name = "Column " + ofToString(groupNumber);
    auto addRole = [&](const std::string& slotId, const std::string& label, const std::string& type) {
        std::string baseId = device.id + "." + group.id + "." + slotId;
        group.roles.emplace_back(baseId);
        auto& role = group.roles.back();
        role.slotId = slotId;
        role.labelCell.setValue(label);
        role.roleDropdown.setSelectedValue(type);
        role.bindingType.clear();
        role.bindingChannel = -1;
        role.bindingNumber = -1;
        role.bindingDisplayOverride.clear();
        role.bindingPending = false;
        role.bindingEdited = true;
        role.hasOriginalBinding = false;
        role.originalBinding = ofJson();
        updateBindingDescription(role);
    };
    addRole("K1", "Knob 1", "knob");
    addRole("K2", "Knob 2", "knob");
    addRole("K3", "Knob 3", "knob");
    addRole("F", "Fader", "fader");
    addRole("B1", "Button 1", "button");
    addRole("B2", "Button 2", "button");
    device.groups.push_back(std::move(group));
    return true;
}

void DevicesPanel::rebuildTreeNodes() const {
    treeNodes_.clear();
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        TreeNode deviceNode;
        deviceNode.deviceIndex = static_cast<int>(i);
        deviceNode.groupIndex = -1;
        deviceNode.depth = 0;
        treeNodes_.push_back(deviceNode);
        const auto& device = devices_[i];
        for (std::size_t g = 0; g < device.groups.size(); ++g) {
            TreeNode groupNode;
            groupNode.deviceIndex = static_cast<int>(i);
            groupNode.groupIndex = static_cast<int>(g);
            groupNode.depth = 1;
            treeNodes_.push_back(groupNode);
        }
    }
    if (activeTreeNodeIndex_ >= static_cast<int>(treeNodes_.size())) {
        activeTreeNodeIndex_ = treeNodes_.empty() ? -1 : 0;
    }
    if (activeTreeNodeIndex_ < 0 && !treeNodes_.empty()) {
        // try to match current selection
        for (std::size_t i = 0; i < treeNodes_.size(); ++i) {
            const auto& node = treeNodes_[i];
            if (node.deviceIndex == selectedDeviceIndex_ && node.groupIndex == selectedGroupIndex_) {
                activeTreeNodeIndex_ = static_cast<int>(i);
                break;
            }
        }
        if (activeTreeNodeIndex_ < 0) {
            activeTreeNodeIndex_ = 0;
        }
    }
}

void DevicesPanel::ensureTreeSelectionValid() const {
    if (treeNodes_.empty()) {
        selectedTreeNodeIndex_ = -1;
        return;
    }
    selectedTreeNodeIndex_ = ofClamp(selectedTreeNodeIndex_, 0, static_cast<int>(treeNodes_.size()) - 1);
}

void DevicesPanel::moveTreeSelection(int delta) const {
    if (treeNodes_.empty()) {
        selectedTreeNodeIndex_ = -1;
        return;
    }
    int count = static_cast<int>(treeNodes_.size());
    int next = selectedTreeNodeIndex_;
    if (next < 0) {
        next = 0;
    }
    next = (next + delta + count) % count;
    selectedTreeNodeIndex_ = next;
}

void DevicesPanel::focusTreeParent() const {
    if (selectedTreeNodeIndex_ < 0 || selectedTreeNodeIndex_ >= static_cast<int>(treeNodes_.size())) {
        return;
    }
    const auto& node = treeNodes_[selectedTreeNodeIndex_];
    if (node.groupIndex < 0) {
        return;
    }
    for (int i = selectedTreeNodeIndex_; i >= 0; --i) {
        const auto& candidate = treeNodes_[i];
        if (candidate.deviceIndex == node.deviceIndex && candidate.groupIndex < 0) {
            selectedTreeNodeIndex_ = i;
            break;
        }
    }
}

void DevicesPanel::focusTreeChild() const {
    if (selectedTreeNodeIndex_ < 0 || selectedTreeNodeIndex_ >= static_cast<int>(treeNodes_.size())) {
        return;
    }
    const auto& node = treeNodes_[selectedTreeNodeIndex_];
    if (node.groupIndex >= 0) {
        return;
    }
    for (int i = selectedTreeNodeIndex_ + 1; i < static_cast<int>(treeNodes_.size()); ++i) {
        const auto& candidate = treeNodes_[i];
        if (candidate.deviceIndex != node.deviceIndex) {
            break;
        }
        if (candidate.groupIndex >= 0) {
            selectedTreeNodeIndex_ = i;
            break;
        }
    }
}

void DevicesPanel::applyTreeSelection(int nodeIndex) const {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(treeNodes_.size())) {
        return;
    }
    const auto& node = treeNodes_[nodeIndex];
    if (node.deviceIndex < 0 || node.deviceIndex >= static_cast<int>(devices_.size())) {
        return;
    }
    selectedDeviceIndex_ = node.deviceIndex;
    const auto& device = devices_[selectedDeviceIndex_];
    if (node.groupIndex >= 0 && node.groupIndex < static_cast<int>(device.groups.size())) {
        selectedGroupIndex_ = node.groupIndex;
    } else {
        selectedGroupIndex_ = 0;
    }
    mode_ = Mode::GroupDetail;
    gridRowIndex_ = 0;
    gridColumnIndex_ = ofClamp(gridColumnIndex_, 1, kGridColumnCount - 1);
    activeTreeNodeIndex_ = nodeIndex;
    ensureGridSelectionValid();
    syncEntrySelectionFromGrid();
}

void DevicesPanel::moveGridRow(int delta) const {
    int roleCount = roleCountForSelection();
    if (roleCount <= 0) {
        gridRowIndex_ = 0;
        selectedEntryIndex_ = -1;
        return;
    }
    gridRowIndex_ = ofClamp(gridRowIndex_ + delta, 0, roleCount - 1);
    syncEntrySelectionFromGrid();
}

void DevicesPanel::moveGridColumn(int delta) const {
    int roleCount = roleCountForSelection();
    if (roleCount <= 0) {
        selectedEntryIndex_ = -1;
        return;
    }
    int next = ofClamp(gridColumnIndex_ + delta, 1, kGridColumnCount - 1);
    if (next != gridColumnIndex_) {
        gridColumnIndex_ = next;
        syncEntrySelectionFromGrid();
    }
}

void DevicesPanel::ensureGridSelectionValid() const {
    gridColumnIndex_ = ofClamp(gridColumnIndex_, 1, kGridColumnCount - 1);
    int roleCount = roleCountForSelection();
    if (roleCount <= 0) {
        gridRowIndex_ = 0;
        return;
    }
    gridRowIndex_ = ofClamp(gridRowIndex_, 0, roleCount - 1);
}

int DevicesPanel::roleCountForSelection() const {
    if (mode_ != Mode::GroupDetail) {
        return 0;
    }
    if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= static_cast<int>(devices_.size())) {
        return 0;
    }
    const auto& device = devices_[selectedDeviceIndex_];
    if (selectedGroupIndex_ < 0 || selectedGroupIndex_ >= static_cast<int>(device.groups.size())) {
        return 0;
    }
    return static_cast<int>(device.groups[selectedGroupIndex_].roles.size());
}

void DevicesPanel::syncEntrySelectionFromGrid() const {
    if (mode_ != Mode::GroupDetail) {
        selectedEntryIndex_ = -1;
        return;
    }
    if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= static_cast<int>(devices_.size())) {
        selectedEntryIndex_ = -1;
        return;
    }
    const auto& device = devices_[selectedDeviceIndex_];
    if (selectedGroupIndex_ < 0 || selectedGroupIndex_ >= static_cast<int>(device.groups.size())) {
        selectedEntryIndex_ = -1;
        return;
    }
    const auto& group = device.groups[selectedGroupIndex_];
    if (group.roles.empty()) {
        selectedEntryIndex_ = -1;
        return;
    }
    int roleIndex = ofClamp(gridRowIndex_, 0, static_cast<int>(group.roles.size()) - 1);
    gridRowIndex_ = roleIndex;
    if (gridColumnIndex_ <= 0 || gridColumnIndex_ >= kGridColumnCount) {
        selectedEntryIndex_ = -1;
        return;
    }
    EntryRef::Kind desiredKind;
    switch (gridColumnIndex_) {
    case 1:
        desiredKind = EntryRef::Kind::RoleDropdown;
        break;
    case 2:
        desiredKind = EntryRef::Kind::RoleSensitivity;
        break;
    case 3:
        desiredKind = EntryRef::Kind::RoleLabel;
        break;
    case 4:
        desiredKind = EntryRef::Kind::RoleBinding;
        break;
    default:
        selectedEntryIndex_ = -1;
        return;
    }
    for (std::size_t i = 0; i < entryRefs_.size(); ++i) {
        const auto& ref = entryRefs_[i];
        if (ref.deviceIndex == selectedDeviceIndex_ &&
            ref.groupIndex == selectedGroupIndex_ &&
            ref.roleIndex == roleIndex &&
            ref.kind == desiredKind) {
            selectedEntryIndex_ = static_cast<int>(i);
            return;
        }
    }
    selectedEntryIndex_ = -1;
    if (roleCountForSelection() > 0 && gridColumnIndex_ >= 1 && gridColumnIndex_ < kGridColumnCount) {
        ofLogWarning("DevicesPanel") << "syncEntrySelectionFromGrid: no entry for deviceIdx=" << selectedDeviceIndex_
                                     << " groupIdx=" << selectedGroupIndex_
                                     << " roleIdx=" << roleIndex
                                     << " column=" << gridColumnIndex_;
    }
}

bool DevicesPanel::dropdownCapturesNavigation(int baseKey) const {
    if (selectedEntryIndex_ < 0 || selectedEntryIndex_ >= static_cast<int>(entryRefs_.size())) {
        return false;
    }
    const auto& ref = entryRefs_[selectedEntryIndex_];
    if (ref.kind != EntryRef::Kind::RoleDropdown) {
        return false;
    }
    if (ref.deviceIndex < 0 || ref.deviceIndex >= static_cast<int>(devices_.size())) {
        return false;
    }
    const auto& device = devices_[ref.deviceIndex];
    if (ref.groupIndex < 0 || ref.groupIndex >= static_cast<int>(device.groups.size())) {
        return false;
    }
    const auto& group = device.groups[ref.groupIndex];
    if (ref.roleIndex < 0 || ref.roleIndex >= static_cast<int>(group.roles.size())) {
        return false;
    }
    const auto& role = group.roles[ref.roleIndex];
    if (!role.roleDropdown.isOpen()) {
        return false;
    }
    switch (baseKey) {
    case OF_KEY_UP:
    case OF_KEY_DOWN:
    case OF_KEY_PAGE_UP:
    case OF_KEY_PAGE_DOWN:
    case OF_KEY_RETURN:
    case OF_KEY_ESC:
        return true;
    default:
        return false;
    }
}

void DevicesPanel::draw() const {
    if (!active_ || !controller_ || !controller_->isCurrent(id())) {
        return;
    }

    rebuildTreeNodes();
    ensureTreeSelectionValid();
    ensureGridSelectionValid();

    const float margin = 24.0f;
    const float headerHeight = 30.0f;
    const float treeMinWidth = 160.0f;
    const float treeMaxWidth = 360.0f;
    const float treePadding = 14.0f;
    const float rowHeight = 20.0f;

    const float screenW = static_cast<float>(ofGetWidth());
    const float screenH = static_cast<float>(ofGetHeight());
    const float usableW = screenW - margin * 2.0f;
    const float usableH = screenH - margin * 2.0f;

    float treeWidth = ofClamp(usableW * 0.32f, treeMinWidth, treeMaxWidth);
    if (treeWidth > usableW - 260.0f) {
        treeWidth = usableW - 260.0f;
    }
    treeWidth = std::max(treeWidth, treeMinWidth);

    const float treeX = margin;
    const float treeY = margin + headerHeight;
    const float treeH = usableH - headerHeight - 8.0f;

    const float gridX = treeX + treeWidth + 16.0f;
    const float gridY = treeY;
    const float gridW = usableW - treeWidth - 16.0f;
    const float gridH = treeH;

    ofPushStyle();
    ofSetColor(30, 30, 36, 235);
    ofDrawRectangle(treeX, treeY, treeWidth, treeH);
    ofSetColor(20, 20, 26, 235);
    ofDrawRectangle(gridX, gridY, gridW, gridH);

    auto drawTextStyled = [](const std::string& text, float x, float y, const ofColor& color, bool bold) {
        ofSetColor(color);
        if (bold) {
            ofDrawBitmapString(text, x, y);
            ofDrawBitmapString(text, x + 1.0f, y);
        } else {
            ofDrawBitmapString(text, x, y);
        }
    };

    float treeCursor = treeY + rowHeight + treePadding * 0.5f;
    const float indentStep = 16.0f;
    for (std::size_t i = 0; i < treeNodes_.size(); ++i) {
        const auto& node = treeNodes_[i];
        std::string label;
        if (node.deviceIndex >= 0 && node.deviceIndex < static_cast<int>(devices_.size())) {
            const auto& device = devices_[node.deviceIndex];
            if (node.groupIndex < 0) {
                label = device.name + (device.connected ? " (online)" : " (offline)");
                if (device.dirty) {
                    label += " *";
                }
            } else if (node.groupIndex < static_cast<int>(device.groups.size())) {
                label = "Column " + ofToString(node.groupIndex + 1) + ": " + device.groups[node.groupIndex].name;
            }
        }
        if (label.empty()) {
            label = "(unassigned)";
        }
        bool selected = (static_cast<int>(i) == selectedTreeNodeIndex_);
        bool active = (static_cast<int>(i) == activeTreeNodeIndex_);
        bool editingNode = (mode_ == Mode::GroupDetail &&
                            node.deviceIndex == selectedDeviceIndex_ &&
                            node.groupIndex == selectedGroupIndex_ &&
                            node.groupIndex >= 0);
        float indent = treePadding + node.depth * indentStep;

        ofColor color;
        if (selected && focusPane_ == FocusPane::Tree) {
            color = ofColor(90, 255, 140);
        } else if (editingNode) {
            color = ofColor(90, 255, 140);
        } else if (selected) {
            color = ofColor(190, 220, 190);
        } else if (active) {
            color = ofColor(255, 210, 160);
        } else {
            color = ofColor(200);
        }

        bool bold = (focusPane_ == FocusPane::Tree && selected) ||
                    (focusPane_ == FocusPane::Grid && editingNode);
        drawTextStyled(label, treeX + indent, treeCursor, color, bold);
        treeCursor += rowHeight;
    }
    if (treeNodes_.empty()) {
        drawTextStyled("No devices detected", treeX + treePadding, treeCursor, ofColor(200), false);
    }

    std::ostringstream header;
    header << "Device Mapper  |  Devices: " << devices_.size();
    if (mode_ == Mode::GroupDetail && selectedDeviceIndex_ >= 0 && selectedDeviceIndex_ < static_cast<int>(devices_.size())) {
        const auto& device = devices_[selectedDeviceIndex_];
        header << "  |  " << device.name;
        if (selectedGroupIndex_ >= 0 && selectedGroupIndex_ < static_cast<int>(device.groups.size())) {
            header << " / Column " << selectedGroupIndex_ + 1;
        }
    }
    header << "   Tab: focus  |  Enter: edit  |  S: save  |  Del: clear binding";
    ofDrawBitmapStringHighlight(header.str(), margin, margin + 16.0f);

    std::array<std::string, kGridColumnCount> gridHeaders{{ "Slot", "Type", "Sensitivity", "Label", "Binding" }};
    std::array<float, kGridColumnCount> columnWeights{{ 0.24f, 0.18f, 0.18f, 0.20f, 0.20f }};
    std::array<float, kGridColumnCount> columnPositions{};
    std::array<float, kGridColumnCount> columnWidths{};
    float totalWeight = 0.0f;
    for (float w : columnWeights) totalWeight += w;
    float cursor = gridX + treePadding;
    for (std::size_t i = 0; i < gridHeaders.size(); ++i) {
        columnWidths[i] = (columnWeights[i] / totalWeight) * (gridW - treePadding * 2.0f);
        columnPositions[i] = cursor;
        cursor += columnWidths[i];
    }

    auto drawCell = [&](std::size_t colIndex,
                        float rowY,
                        const std::string& text,
                        bool rowSelected,
                        bool columnSelected) {
        ofColor color = rowSelected ? ofColor(235) : ofColor(180);
        bool bold = false;
        if (columnSelected && focusPane_ == FocusPane::Grid) {
            color = ofColor(90, 255, 140);
            bold = true;
        }
        ofSetColor(color);
        if (bold) {
            ofDrawBitmapString(text, columnPositions[colIndex] + 6.0f, rowY);
            ofDrawBitmapString(text, columnPositions[colIndex] + 7.0f, rowY);
        } else {
            ofDrawBitmapString(text, columnPositions[colIndex] + 6.0f, rowY);
        }
    };

    auto drawDropdownOverlay = [&](const ColumnRowDropdown& dropdown,
                                   float cellX,
                                   float cellY,
                                   float cellWidth) {
        const auto& options = dropdown.options();
        if (options.empty()) {
            return;
        }
        const float padding = 6.0f;
        const float itemHeight = rowHeight - 2.0f;
        float boxWidth = std::max(cellWidth, 140.0f);
        float boxX = cellX;
        float boxY = cellY - rowHeight + 2.0f;
        if (boxY < gridY) {
            boxY = gridY + 2.0f;
        }
        float boxHeight = itemHeight * static_cast<float>(options.size()) + padding * 2.0f;
        if (boxY + boxHeight > gridY + gridH - 4.0f) {
            boxY = gridY + gridH - boxHeight - 4.0f;
        }

        ofPushStyle();
        ofFill();
        ofSetColor(12, 16, 24, 255);
        ofDrawRectangle(boxX, boxY, boxWidth, boxHeight);
        ofNoFill();
        ofSetColor(90, 255, 140, 200);
        ofSetLineWidth(1.5f);
        ofDrawRectangle(boxX, boxY, boxWidth, boxHeight);
        ofFill();
        int highlighted = dropdown.highlightedIndex();
        for (std::size_t i = 0; i < options.size(); ++i) {
            float textY = boxY + padding + itemHeight * (static_cast<float>(i) + 0.7f);
            bool isHighlighted = static_cast<int>(i) == highlighted;
            ofColor color = isHighlighted ? ofColor(90, 255, 140) : ofColor(220);
            ofSetColor(color);
            if (isHighlighted) {
                ofDrawRectangle(boxX + 2.0f,
                                boxY + padding + itemHeight * static_cast<float>(i),
                                boxWidth - 4.0f,
                                itemHeight);
                ofSetColor(12, 16, 24);
            }
            ofDrawBitmapString(options[i].label, boxX + padding + 2.0f, textY);
        }
        ofPopStyle();
    };

    ofSetColor(255);
    float headerY = gridY + rowHeight + treePadding;
    for (std::size_t h = 0; h < gridHeaders.size(); ++h) {
        drawCell(h, headerY, gridHeaders[h], false, false);
    }

    float gridCursor = headerY + rowHeight;
    if (mode_ != Mode::GroupDetail || selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= static_cast<int>(devices_.size())) {
        ofSetColor(210);
        ofDrawBitmapString("Select a device to edit its mapping", gridX + treePadding, gridCursor);
    } else {
        const auto& device = devices_[selectedDeviceIndex_];
        if (device.groups.empty() || selectedGroupIndex_ < 0 || selectedGroupIndex_ >= static_cast<int>(device.groups.size())) {
            ofSetColor(210);
            ofDrawBitmapString("No columns defined. Use the tree to add a default mapping.", gridX + treePadding, gridCursor);
        } else {
            const auto& group = device.groups[selectedGroupIndex_];
            for (std::size_t r = 0; r < group.roles.size(); ++r) {
                const auto& role = group.roles[r];
                bool rowSelected = (gridRowIndex_ == static_cast<int>(r));
                for (std::size_t c = 0; c < gridHeaders.size(); ++c) {
                    bool cellSelected = rowSelected && (static_cast<int>(c) == gridColumnIndex_);
                    std::string text;
                    switch (c) {
                    case 0:
                        text = role.slotId + " (" + role.labelCell.value() + ")";
                        break;
                    case 1:
                        text = role.roleDropdown.selectedValue();
                        break;
                    case 2:
                        text = ofToString(role.sensitivity.value(), 2) + "x";
                        break;
                    case 3:
                        text = role.labelCell.value();
                        break;
                    case 4:
                        text = role.bindingPending ? "Listening..." : (role.midiBinding.empty() ? "Unassigned" : role.midiBinding);
                        break;
                    default:
                        break;
                    }
                    if (c == 1 && role.roleDropdown.isOpen()) {
                        ofPushStyle();
                        ofFill();
                        ofSetColor(12, 16, 24, 255);
                        ofDrawRectangle(columnPositions[c],
                                        gridCursor - rowHeight + 4.0f,
                                        columnWidths[c],
                                        rowHeight);
                        ofPopStyle();
                    }
                    drawCell(c, gridCursor, text, rowSelected, cellSelected);
                    if (c == 1 && role.roleDropdown.isOpen()) {
                        drawDropdownOverlay(role.roleDropdown, columnPositions[c], gridCursor, columnWidths[c]);
                    }
                }
                gridCursor += rowHeight;
            }
        }
    }

    ofPopStyle();
}
