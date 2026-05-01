#pragma once

#include "MenuController.h"
#include "ColumnControls.h"
#include "ofJson.h"
#include "../io/MidiRouter.h"

#include <string>
#include <vector>
#include <cstdint>


// DevicesPanel renders the Device Mapper UI (Devices tab) inside the Control Hub.
class DevicesPanel : public MenuController::State {
public:
    DevicesPanel();
    ~DevicesPanel() override;

    void setMidiRouter(MidiRouter* router);
    void setDeviceMapsDirectory(const std::string& path);
    void markRosterDirty() const;
    bool isBindingCaptureActive() const { return pendingLearn_.active; }

    struct RoleSlot {
        ColumnRowDropdown roleDropdown;
        ColumnNumericCell sensitivity;
        ColumnTextCell labelCell;
        std::string midiBinding;
        std::string slotId;
        std::string bindingType;
        std::string bindingDisplayOverride;
        int bindingChannel = -1;
        int bindingNumber = -1;
        bool bindingEdited = false;
        bool bindingPending = false;
        ofJson originalBinding;
        bool hasOriginalBinding = false;
        RoleSlot(const std::string& baseId);
    };

    struct GroupRow {
        std::string id;
        std::string name;
        std::vector<RoleSlot> roles;
    };

    struct DeviceRow {
        std::string id;
        std::string name;
        bool connected = false;
        bool hasMapping = false;
        bool dirty = false;
        std::string model;
        std::string sourcePath;
        std::vector<std::string> portHints;
        std::vector<std::string> ports;
        std::string portLabel;
        ofJson originalDoc;
        std::vector<GroupRow> groups;
    };

    const std::string& id() const override { return stateId_; }
    const std::string& label() const override { return label_; }
    const std::string& scope() const override { return scope_; }
    MenuController::StateView view() const override;
    bool handleInput(MenuController& controller, int key) override;
    void onEnter(MenuController& controller) override;
    void onExit(MenuController& controller) override;
    void draw() const;

private:
    enum class Mode {
        DeviceList,
        GroupDetail
    };

    enum class FocusPane {

        Tree,

        Grid

    };



    struct TreeNode {

        int deviceIndex = -1;

        int groupIndex = -1;

        int depth = 0;

    };

    struct EntryRef {
        enum class Kind {
            DeviceRow,
            GroupHeader,
            RoleDropdown,
            RoleSensitivity,
            RoleLabel,
            RoleBinding,
            Action
        };
        enum class ActionId {
            None,
            CreateDefaultGroup,
            SaveDevice
        };
        Kind kind;
        int deviceIndex = -1;
        int groupIndex = -1;
        int roleIndex = -1;
        bool selectable = true;
        ActionId action = ActionId::None;
    };

    MenuController::StateView buildDeviceListView(MenuController::StateViewBuilder& builder) const;
    MenuController::StateView buildGroupDetailView(MenuController::StateViewBuilder& builder) const;
    void moveSelection(int delta) const;
    bool activateSelection(MenuController& controller, int key);
    bool handleRoleInput(const EntryRef& ref, int key);
    void refreshDeviceRoster() const;
    bool deviceMatchesPort(const DeviceRow& device, const std::string& portName) const;
    bool beginBindingLearn(const EntryRef& ref);
    bool clearBinding(const EntryRef& ref);
    bool saveSelectedDevice();
    void cancelPendingLearn();
    void markDeviceDirty(int deviceIndex);
    void updateBindingDescription(RoleSlot& role) const;
    bool applyBindingCapture(int deviceIndex, int groupIndex, int roleIndex, const MidiRouter::CapturedMidiControl& capture);
    bool createDefaultGroup(int deviceIndex);

    void rebuildTreeNodes() const;
    void ensureTreeSelectionValid() const;
    void moveTreeSelection(int delta) const;
    void focusTreeParent() const;
    void focusTreeChild() const;
    void applyTreeSelection(int nodeIndex) const;
    void ensureGridSelectionValid() const;
    void moveGridRow(int delta) const;
    void moveGridColumn(int delta) const;
    void syncEntrySelectionFromGrid() const;
    int roleCountForSelection() const;
    bool dropdownCapturesNavigation(int baseKey) const;


    mutable std::vector<EntryRef> entryRefs_;
    mutable std::vector<DeviceRow> devices_;
    MidiRouter* midiRouter_ = nullptr;
    std::string deviceMapsDirectory_;
    mutable bool rosterDirty_ = true;
    mutable uint64_t lastRosterRefreshMs_ = 0;
    uint64_t rosterRefreshIntervalMs_ = 1000;
    mutable Mode mode_ = Mode::DeviceList;
    mutable int selectedDeviceIndex_ = 0;
    mutable int selectedGroupIndex_ = 0;
    mutable int selectedEntryIndex_ = 0;
    bool detailDirty_ = true;
    bool active_ = false;
    MenuController* controller_ = nullptr;
    struct PendingLearn {
        bool active = false;
        int deviceIndex = -1;
        int groupIndex = -1;
        int roleIndex = -1;
    };
    mutable PendingLearn pendingLearn_;

    mutable std::vector<TreeNode> treeNodes_;

    mutable int selectedTreeNodeIndex_ = 0;

    mutable int activeTreeNodeIndex_ = -1;

    mutable FocusPane focusPane_ = FocusPane::Tree;

    mutable int gridRowIndex_ = 0;

    mutable int gridColumnIndex_ = 1;

    static constexpr int kGridColumnCount = 5;

    const std::string stateId_ = "ui.devices";
    const std::string label_ = "Devices";
    const std::string scope_ = "Devices";
};
