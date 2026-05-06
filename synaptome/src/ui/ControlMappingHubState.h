#pragma once

#include "MenuController.h"
#include "MenuSkin.h"

#include "ofEvents.h"
#include "ofFileUtils.h"
#include "ofGraphics.h"
#include "ofJson.h"
#include "ofLog.h"
#include "ofUtils.h"

#include "../core/ParameterRegistry.h"
#include "../io/MidiRouter.h"
#include "../visuals/LayerFactory.h"
#include "../visuals/LayerLibrary.h"
#include "../io/ConsoleStore.h"
#include "HudFeedRegistry.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <array>
#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Aggregates parameter/key/MIDI/OSC panes into a single routing hub.
class ControlMappingHubState : public MenuController::State {
public:
    enum class HudLayoutTarget {
        Projector = 0,
        Controller = 1
    };

    struct SavedSceneInfo {
        std::string id;
        std::string label;
        std::string path;
        bool active = false;
    };

    ControlMappingHubState();
    ~ControlMappingHubState() override;

    void setPreferencesPath(const std::string& path);
    void setKeyMappingState(const std::shared_ptr<MenuController::State>& state);
    void setConsoleSlotLoadCallback(std::function<bool(int, const std::string&)> cb);
    void setConsoleSlotUnloadCallback(std::function<bool(int)> cb);
    void setConsoleSlotInventoryCallback(std::function<std::vector<ConsoleLayerInfo>()> cb);
    void setSavedSceneListCallback(std::function<std::vector<SavedSceneInfo>()> cb);
    void setSavedSceneLoadCallback(std::function<bool(const std::string&)> cb);
    void setSavedSceneSaveAsCallback(std::function<bool(const std::string&, bool)> cb);
    void setSavedSceneOverwriteCallback(std::function<bool(const std::string&)> cb);
    void markConsoleSlotsDirty() const;
    void refreshConsoleSlotBindings() const;
    void setLayoutBand(const ofRectangle& bounds);
    void clearLayoutBand();
    void setMenuSkin(const MenuSkin& skin);
    void setParameterRegistry(ParameterRegistry* registry);
    void setMidiRouter(MidiRouter* router);
    void setLayerLibrary(LayerLibrary* library);
    void setConsoleAssetResolver(std::function<const LayerLibrary::Entry*(const std::string& prefix)> resolver);
    void setDeviceMapsDirectory(const std::string& path);
    void setSlotAssignmentsPath(const std::string& path);
    void setMidiPaneStatus(const std::string& description, bool available);
    void setOscPaneStatus(const std::string& description, bool available);
    void setMidiAction(std::function<void(MenuController&)> action);
    void setOscAction(std::function<void(MenuController&)> action);
    void setRoutingRollbackAction(std::function<void()> action);
    void setFloatValueCommitCallback(std::function<void(const std::string&, float)> cb);
    // Telemetry/event callback: receives a JSON string describing a ControlHubEvent.
    void setEventCallback(std::function<void(const std::string&)> cb);
    // Publish a HUD telemetry sample so automation subscribers can mirror widget data.
    void publishHudTelemetrySample(const std::string& widgetId,
                                   const std::string& feedId,
                                   float value,
                                   const std::string& detail = std::string()) const;
    struct BioAmpMetricSample {
        float value = 0.0f;
        uint64_t timestampMs = 0;
        bool valid = false;
    };
    struct BioAmpState {
        BioAmpMetricSample raw;
        BioAmpMetricSample signal;
        BioAmpMetricSample mean;
        BioAmpMetricSample rms;
        BioAmpMetricSample domHz;
        uint16_t sampleRate = 0;
        uint16_t windowSize = 0;
        uint64_t sampleRateTimestampMs = 0;
        uint64_t windowTimestampMs = 0;
    };
    void setBioAmpMetric(const std::string& metric, float value, uint64_t timestampMs = 0);
    void setBioAmpMetadata(const std::string& field, float value, uint64_t timestampMs = 0);
    const BioAmpState& bioAmpState() const { return bioAmpState_; }
    void setHudVisible(bool visible);
    bool hudVisible() const { return hudVisible_; }
    void setHudVisibilityCallback(std::function<void(bool)> cb);
    void setHudToggleCallback(std::function<void(const std::string&, bool)> cb);
    void setHudLayoutAction(std::function<bool(MenuController&)> action);
    struct HudPlacementSnapshot {
        std::string id;
        std::string bandId;
        std::string bandLabel;
        std::string columnLabel;
        int columnIndex = -1;
        bool visible = false;
        bool collapsed = false;
        std::string target;
    };
    void setHudPlacementProvider(std::function<std::vector<HudPlacementSnapshot>()> provider);
    void setHudPlacementCallback(std::function<void(const std::string&, int)> cb);
    void setHudLayoutTarget(HudLayoutTarget target);
    HudLayoutTarget hudLayoutTarget() const { return hudLayoutTarget_; }
    bool hudColumnPickerVisible() const { return hudColumnPickerVisible_; }
    static std::string hudLayoutTargetName(HudLayoutTarget target);
    void pollHudLayoutDrift(uint64_t nowMs) const;
    void setHudFeedRegistry(HudFeedRegistry* registry);
    void notifyHudLayoutChanged() const;
    bool importLegacyHudConfig(const std::string& hudConfigPath,
                               const std::string& overlayConfigPath);
    struct HudRoutingEntry {
        std::string id;
        std::string label;
        std::string category;
        std::string target;
    };
    void emitHudRoutingManifest(const std::vector<HudRoutingEntry>& manifest) const;
    std::vector<HudPlacementSnapshot> exportHudLayoutSnapshot(HudLayoutTarget target) const;
    void emitHudLayoutSnapshot(HudLayoutTarget target,
                               const std::vector<HudPlacementSnapshot>& snapshot,
                               const std::string& reason = std::string()) const;
    void emitOverlayRouteEvent(const std::string& target,
                               const std::string& source,
                               bool followMode) const;
    void focusCategory(const std::string& category, const std::string& subcategory = std::string());
    bool debugRowIsAsset(const std::string& id) const;
    std::string debugValueForRow(const std::string& id) const;
    bool debugSetHudColumnSelection(const std::string& id, int selectionIndex);
    bool debugBeginMidiLearn(const std::string& id);
    bool debugSetGridSelection(int rowIndex, int columnIndex);
    bool debugSlotPickerVisible() const;
    void debugFlushPreferences() const { flushPreferences(); }

    const std::string& id() const override { return stateId_; }
    const std::string& label() const override { return label_; }
    const std::string& scope() const override { return scope_; }
    MenuController::StateView view() const override;
    bool handleInput(MenuController& controller, int key) override;
    bool wantsRawKeyInput() const override { return true; }
    void onEnter(MenuController& controller) override;
    void onExit(MenuController& controller) override;
    void draw() const;

    struct ViewportSnapshot {
        int treeNodeCount = 0;
        int treeScrollOffset = 0;
        int treeVisibleRows = 0;
        int gridRowCount = 0;
        int gridScrollOffset = 0;
        int gridVisibleRows = 0;
    };

    ViewportSnapshot snapshotViewport(float viewportWidth, float viewportHeight) const;

private:
    enum class Column : int {
        kName = 0,
        kValue,
        kSlot,
        kMidi,
        kMidiMin,
        kMidiMax,
        kOsc,
        kOscDeadband,
        kCount
    };
    static constexpr int kHudColumnCount = 4;
    static constexpr uint64_t kHudLayoutDriftAssertDelayMs = 750;

    struct ParameterRow {
        std::string id;
        std::string label;
        std::string category;
        std::string subcategory;
        bool isFloat = false;
        bool isString = false;
        const ParameterRegistry::FloatParam* floatParam = nullptr;
        const ParameterRegistry::BoolParam* boolParam = nullptr;
        const ParameterRegistry::StringParam* stringParam = nullptr;
        bool isAsset = false;
        std::string assetKey;
        std::string assetLabel;
        std::string familyLabel;
        bool offline = false;
        std::vector<int> consoleSlots;
        bool consoleSlotActive = false;
        bool isHudLayoutEntry = false;
        bool isSavedSceneRow = false;
        bool isSavedSceneSaveRow = false;
        bool isSavedSceneOverwriteRow = false;
        std::string savedSceneId;
        std::string savedScenePath;
    };

    struct CategorySection {
        std::string name;
        struct Subcategory {
            struct AssetGroup {
                std::string name;
                std::string assetKey;
                std::vector<int> rowIndices;
            };
            std::string name;
            std::vector<AssetGroup> assetGroups;
            std::vector<int> rowIndices;
        };
        std::vector<Subcategory> subcategories;
        std::vector<int> rowIndices;
    };

    struct TreeNode {
        std::string label;
        std::string categoryName;
        std::string subcategoryName;
        std::string assetGroupName;
        int categoryIndex = -1;
        int subcategoryIndex = -1;
        int assetGroupIndex = -1;
        int parentIndex = -1;
        int depth = 0;
        bool expandable = false;
        bool expanded = false;
        bool oscSummary = false;
    };

    struct ParameterTableModel {
        std::vector<ParameterRow> rows;
        std::vector<CategorySection> categories;
        std::vector<int> allRowIndices;
        std::vector<TreeNode> tree;
        bool dirty = true;
    };

    struct ConsoleSlotRef {
        int slotIndex = -1;
        bool active = false;
    };

    struct HudWidgetPreference {
        bool visible = true;
        int columnIndex = -1;
        std::string bandId = "hud";
        bool collapsed = false;
    };

    struct HudLayoutPlacement {
        int columnIndex = -1;
        std::string bandId = "hud";
        bool collapsed = false;
    };

    struct Preferences {
        float treeWidthRatio = 0.16f;
        std::string categoryName;
        std::string subcategoryName;
        std::string assetName;
        std::array<bool, static_cast<int>(Column::kCount)> columnVisibility{};
        std::string selectedColumnKey = "value";
        std::vector<std::string> collapsedCategories;
        bool hudVisible = true;
        std::unordered_map<std::string, HudWidgetPreference> hudWidgets;
        std::unordered_map<std::string, HudLayoutPlacement> controllerHudWidgets;
        std::string hudLayoutTarget = "projector";
        bool hudStateMigrated = false;

        Preferences() {
            columnVisibility.fill(true);
            columnVisibility[static_cast<int>(Column::kOscDeadband)] = false;
        }
    };

    struct AssetMetadata {
        std::string key;
        std::string label;
        std::string family;
        std::string familyDisplay;
        std::string subcategoryDisplay;
        std::string registryPrefix;
        std::string assetId;
    };

    struct SlotOption {
        struct ColumnBinding {
            std::string controlId;
            std::string columnId;
            std::string columnName;
            std::string bindingType;
            int channel = -1;
            int number = -1;
        };

        std::string deviceId;
        std::string deviceName;
        std::string slotId;
        std::string label;
        std::string roleType;
        bool analog = true;
        std::vector<ColumnBinding> bindings;
    };

    struct LogicalSlotBinding {
        std::string deviceId;
        std::string deviceName;
        std::string slotId;
        std::string slotLabel;
        bool analog = true;
    };

    struct VisibleRange {
        int start = 0;
        int end = -1;
        int capacity = 0;
    };

    struct LayoutContext {
        float viewportWidth = 0.0f;
        float viewportHeight = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float treeX = 0.0f;
        float treeY = 0.0f;
        float treeWidth = 0.0f;
        float treeHeight = 0.0f;
        float treeHeaderHeight = 0.0f;
        float treeBodyY = 0.0f;
        float treeBodyHeight = 0.0f;
        float gridX = 0.0f;
        float gridY = 0.0f;
        float gridWidth = 0.0f;
        float gridHeight = 0.0f;
        float headerBaseline = 0.0f;
        float treeRowHeight = 0.0f;
        float gridRowHeight = 0.0f;
        int treeVisibleRows = 0;
        int gridVisibleRows = 0;
    };

    enum class FocusPane {
        kTree,
        kGrid
    };

    static constexpr int kSlotPickerVisibleRows = 8;

    const std::string stateId_ = "ui.hub.control";
    const std::string label_ = "Browser";
    const std::string scope_ = "ControlHub";

    std::weak_ptr<MenuController::State> keyMappingState_;
    ParameterRegistry* registry_ = nullptr;
    MidiRouter* midiRouter_ = nullptr;
    LayerLibrary* layerLibrary_ = nullptr;
    std::function<const LayerLibrary::Entry*(const std::string&)> consoleAssetResolver_;
    std::function<bool(int, const std::string&)> consoleSlotLoadCallback_;
    std::function<bool(int)> consoleSlotUnloadCallback_;
    std::function<std::vector<ConsoleLayerInfo>()> consoleSlotInventoryCallback_;
    std::function<std::vector<SavedSceneInfo>()> savedSceneListCallback_;
    std::function<bool(const std::string&)> savedSceneLoadCallback_;
    std::function<bool(const std::string&, bool)> savedSceneSaveAsCallback_;
    std::function<bool(const std::string&)> savedSceneOverwriteCallback_;
    std::function<void(MenuController&)> midiAction_;
    std::function<void(MenuController&)> oscAction_;

    std::string midiStatus_ = "MIDI routing (planned)";
    std::string oscStatus_ = "OSC mapper (planned)";
    bool midiAvailable_ = false;
    bool oscAvailable_ = false;

    std::string preferencesPath_;
    std::string deviceMapsDirectory_;
    std::string slotAssignmentsPath_;
    mutable std::unordered_map<std::string, AssetMetadata> assetCatalog_;
    mutable std::unordered_map<std::string, std::string> assetKeyById_;
    mutable bool offlineHydrationDirty_ = true;
    mutable ParameterRegistry offlineRegistry_;
    mutable std::vector<std::unique_ptr<Layer>> offlineLayers_;
    mutable std::unordered_map<std::string, std::vector<int>> offlineFloatParamIndices_;
    mutable std::unordered_map<std::string, std::vector<int>> offlineBoolParamIndices_;
    mutable std::unordered_map<std::string, std::vector<int>> offlineStringParamIndices_;
    mutable std::unordered_map<std::string, std::unique_ptr<float>> offlineOpacityValues_;
    mutable bool slotCatalogDirty_ = true;
    mutable std::vector<SlotOption> slotCatalog_;
    mutable std::unordered_map<std::string, std::size_t> slotIndexById_;
    mutable std::unordered_map<std::string, std::size_t> slotIndexByLogicalKey_;
    mutable bool slotAssignmentsLoaded_ = false;
    mutable bool slotAssignmentsDirty_ = false;
    mutable std::unordered_map<std::string, LogicalSlotBinding> slotAssignments_;
    mutable bool consoleSlotInventoryDirty_ = true;
    mutable bool slotMidiAssignmentsDirty_ = false;
    mutable std::unordered_map<std::string, std::vector<ConsoleSlotRef>> consoleSlotsByAssetId_;
    mutable std::unordered_map<std::string, std::vector<ConsoleSlotRef>> consoleSlotsByAssetKey_;
    mutable std::unordered_set<int> activeConsoleSlotIndices_;
    mutable std::vector<ConsoleLayerInfo> consoleSlotInventory_;
    mutable MenuController::StateView cachedView_;
    mutable ParameterTableModel tableModel_;
    mutable const std::vector<int>* activeRowSet_ = nullptr;
    mutable int selectedRow_ = -1;
    mutable Column selectedColumn_ = Column::kValue;
    mutable int selectedTreeNodeIndex_ = 0;
    mutable FocusPane focusPane_ = FocusPane::kGrid;
    mutable Preferences preferences_;
    HudLayoutTarget hudLayoutTarget_ = HudLayoutTarget::Projector;
    mutable bool preferencesDirty_ = false;
    mutable bool treeSelectionPending_ = true;
    mutable std::string pendingCategoryPref_;
    mutable std::string pendingSubcategoryPref_;
    mutable std::string pendingAssetPref_;
    mutable bool editingValueActive_ = false;
    mutable std::string editingValueRowId_;
    mutable std::string editingValueBuffer_;
    mutable bool editingValueError_ = false;
    mutable bool editingValueOverwritePending_ = false;
    mutable bool pickingOsc_ = false;
    mutable int oscPickerSelection_ = 0;
    mutable int oscEditColumn_ = 0;
    mutable int oscPickerScrollOffset_ = 0;
    mutable bool oscValueEditActive_ = false;
    mutable std::string oscValueEditRowId_;
    mutable int oscValueEditColumn_ = 0;
    mutable std::string oscValueEditBuffer_;
    mutable bool oscValueEditOverwritePending_ = false;
    mutable bool slotPickerVisible_ = false;
    mutable std::string slotPickerRowId_;
    mutable std::vector<int> slotPickerIndices_;
    mutable int slotPickerSelection_ = -1;
    mutable int slotPickerScrollOffset_ = 0;
    mutable bool hudColumnPickerVisible_ = false;
    mutable std::string hudColumnPickerRowId_;
    mutable int hudColumnPickerSelection_ = 0;
    mutable bool hudLayoutDirty_ = false;
    mutable std::unordered_map<std::string, std::string> hudWidgetTargets_;
    mutable int hudLayoutFenceDepth_ = 0;
    mutable bool hudLayoutFenceDirty_ = false;
    mutable bool hudLayoutDriftActive_ = false;
    mutable uint64_t hudLayoutDriftSinceMs_ = 0;
    mutable std::unordered_map<std::string, float> lastValueSnapshot_;
    mutable std::unordered_map<std::string, std::string> lastStringSnapshot_;
    mutable std::unordered_map<std::string, uint64_t> lastActivityMs_;
    mutable std::unordered_map<std::string, MidiRouter::TakeoverState> takeoverByTarget_;
    mutable std::unordered_map<std::string, std::string> pendingValueBuffers_;
    mutable std::string savedSceneDraftName_ = "new-scene";
    mutable std::string editingValueErrorMessage_;
    mutable std::string bannerMessage_;
    mutable uint64_t bannerExpiryMs_ = 0;
    mutable bool routingPopoverVisible_ = false;
    mutable bool routingCommitInProgress_ = false;
    mutable bool rollbackOffered_ = false;
    std::function<void()> routingRollbackAction_;
    std::function<void(const std::string&, float)> floatValueCommitCallback_;
    bool hudVisible_ = true;
    std::function<void(bool)> hudVisibilityChanged_;
    std::function<void(const std::string&, bool)> hudToggleCallback_;
    std::function<bool(MenuController&)> hudLayoutAction_;
    std::function<std::vector<HudPlacementSnapshot>()> hudPlacementProvider_;
    std::function<void(const std::string&, int)> hudPlacementCallback_;
    HudFeedRegistry* hudFeedRegistry_ = nullptr;
    bool hudFeedListenerAttached_ = false;
    bool active_ = false;
    MenuController* controller_ = nullptr;
    mutable bool layoutBandOverride_ = false;
    mutable ofRectangle layoutBandBounds_;
    MenuSkin skin_ = MenuSkin::ConsoleHub();
    mutable int treeScrollOffset_ = 0;
    mutable int gridScrollOffset_ = 0;
    mutable std::unordered_map<std::string, bool> treeExpansionState_;
    // optional telemetry/event callback sink (mutable so it can be set from non-const callers)
    mutable std::function<void(const std::string&)> eventCallback_;
    BioAmpState bioAmpState_;

    class HudLayoutFenceScope {
    public:
        explicit HudLayoutFenceScope(const ControlMappingHubState* host)
            : host_(host) {
            if (host_) {
                host_->beginHudLayoutFence();
            }
        }
        ~HudLayoutFenceScope() {
            if (host_) {
                host_->endHudLayoutFence();
            }
        }
    private:
        const ControlMappingHubState* host_ = nullptr;
    };

    void rebuildView() const;
    void rebuildModel() const;
    std::string selectedAssetId() const;
    std::string selectedAssetLabel() const;
    void clampSelection() const;
    void invalidateRowCache() const;
    const std::vector<int>& activeRowIndices() const;
    const std::vector<int>* resolveActiveRowSet() const;
    int activeRowSlot() const;
    std::vector<std::string> columnHeaders() const;
    std::vector<int> visibleColumns() const;
    VisibleRange computeVisibleRange(int totalRows, int anchorSlot, float viewportHeight, float rowHeight) const;
    LayoutContext computeLayoutContext(float viewportWidth, float viewportHeight) const;
    void ensureTreeSelectionVisible(int visibleRows) const;
    void ensureGridSelectionVisible(int visibleRows) const;
    int firstLeafNodeIndex() const;
    void moveTreeSelection(int delta) const;
    void focusTreeParent() const;
    void focusTreeChild() const;
    void applyTreeSelection(int nodeIndex, bool userDriven) const;
    void toggleColumn(int columnIndex);
    void enforceVisibleColumnSelection() const;
    static const char* columnKey(Column column);
    static Column columnFromKey(const std::string& key);
    bool isCategoryExpanded(const std::string& name) const;
    void setCategoryExpanded(const std::string& name, bool expanded) const;
    void replayTreeSelection(const std::string& category,
                             const std::string& subcategory,
                             const std::string& asset = std::string()) const;
    std::string subcategoryExpansionKey(const std::string& category, const std::string& subcategory) const;
    void ensurePreferencesDirectory() const;
    void loadPreferences();
    void markPreferencesDirty() const;
    void flushPreferences() const;
    std::string formatValue(const ParameterRow& row) const;
    std::string hudPlacementSummary(const std::string& assetKey) const;
    std::string hudFeedSummary(const std::string& assetKey) const;
    std::string summarizeHudStatusFeed(const ofJson& payload) const;
    std::string summarizeHudSensorsFeed(const ofJson& payload) const;
    std::string summarizeHudLayersFeed(const ofJson& payload) const;
    void emitHudFeedUpdated(const HudFeedRegistry::FeedEntry& entry) const;
    void emitHudMappingEvent(const std::string& reason,
                             const std::string& widgetId = std::string(),
                             const std::string& detail = std::string()) const;
    std::string formatMidiSummary(const ParameterRow& row) const;
    std::string formatMidiMin(const ParameterRow& row) const;
    std::string formatMidiMax(const ParameterRow& row) const;
    std::string formatOscSummary(const ParameterRow& row) const;
    bool rowHasLiveParameter(const ParameterRow& row) const;
    bool rowSupportsValueEdit(const ParameterRow& row) const;
    bool isHudWidgetRow(const ParameterRow& row) const;
    bool oscSummaryActive() const;
    std::string ellipsize(const std::string& text, float maxWidth) const;
    std::vector<MidiRouter::OscSourceInfo> oscBrowserSources() const;
    std::string selectedOscSourceAddress(const std::vector<MidiRouter::OscSourceInfo>& sources) const;
    const MidiRouter::OscMap* currentOscMap(const std::string& id) const;
    const MidiRouter::OscSourceProfile* currentOscSourceProfile(const std::string& address) const;
    bool adjustCurrentOscMap(const ParameterRow& row, int column, float delta) const;
    bool adjustOscSourceProfile(const std::string& address,
                                int column,
                                float delta) const;
    bool applyOscSourcePresetRange(const std::string& address, float outMin, float outMax) const;
    bool flipOscSourceOutputRange(const std::string& address) const;
    bool adjustOscSourceSmoothing(const std::string& address, float delta) const;
    bool adjustOscSourceDeadband(const std::string& address, float delta) const;
    bool applyOscPresetRange(const ParameterRow& row, float outMin, float outMax) const;
    bool flipCurrentOscOutputRange(const ParameterRow& row) const;
    bool adjustOscSmoothing(const ParameterRow& row, float delta) const;
    bool adjustOscDeadband(const ParameterRow& row, float delta) const;
    bool beginOscValueEdit(const ParameterRow& row) const;
    bool beginOscSourceValueEdit(const std::string& address) const;
    void cancelOscValueEdit() const;
    bool commitOscValueEdit(const ParameterRow& row) const;
    bool commitOscSourceValueEdit(const std::string& address) const;
    bool handleOscValueEditCharacter(int key) const;
    void drawOscPickerPanel(float panelX,
                            float panelY,
                            float panelWidth,
                            float panelHeight,
                            const std::vector<MidiRouter::OscSourceInfo>& sources,
                            const ParameterRow* targetRow) const;
    int oscEditorColumnCount() const;
    std::string oscColumnKeyForIndex(int column) const;
    void drawSlotPickerPanel(float panelX,
                            float panelY,
                            float panelWidth,
                            float panelHeight,
                            const ParameterRow* row) const;
    void drawHudColumnPickerPanel(float panelX,
                                  float panelY,
                                  float panelWidth,
                                  float panelHeight,
                                  const ParameterRow* row) const;
    std::string formatSlotSummary(const ParameterRow& row) const;
    std::string currentSlotControlId(const ParameterRow& row) const;
    const MidiRouter::BtnMap* firstBtnMapForRow(const ParameterRow& row) const;
    bool beginSlotPicker(const ParameterRow& row);
    bool applySelectedSlot();
    void cancelSlotPicker() const;
    std::vector<std::string> candidateColumnIdsForRow(const ParameterRow& row) const;
    std::string consoleColumnIdForSlot(int slotIndex) const;
    const SlotOption::ColumnBinding* selectColumnBindingForRow(const ParameterRow& row,
                                                               const SlotOption& slot) const;
    void emitSlotBindingDiagnostics(const ParameterRow& row,
                                    const SlotOption& slot,
                                    const SlotOption::ColumnBinding& binding) const;
    bool beginHudColumnPicker(const ParameterRow& row) const;
    bool applyHudColumnSelection();
    void cancelHudColumnPicker() const;
    void updateHudVisibilityPreference(const std::string& id, bool visible) const;
    void updateHudPlacementPreference(const std::string& id, int columnIndex) const;
    void snapshotHudPreferencesFromProvider() const;
    void replayHudTogglePreferences() const;
    void replayHudPlacementPreferences() const;
    HudLayoutPlacement placementFromPreference(const HudWidgetPreference& pref) const;
    HudLayoutPlacement placementForControllerWidget(const std::string& id) const;
    HudLayoutPlacement expectedPlacementForTarget(const std::string& id, HudLayoutTarget target) const;
    void beginHudLayoutFence() const;
    void endHudLayoutFence() const;
    void applyHudLayoutSnapshot() const;
    static HudLayoutTarget hudLayoutTargetFromString(const std::string& value);
    static std::string hudLayoutTargetToString(HudLayoutTarget target);
    void seedControllerLayoutFromPrimary() const;
    std::string hudColumnLabel(int columnIndex) const;
    void applyAssetMetadata(ParameterRow& row) const;
    bool applySyntheticAssetMetadata(ParameterRow& row) const;
    void appendSceneBrowserRows() const;
    std::string sceneCurrentAssetLabel(const ConsoleLayerInfo& slot) const;
    bool isSavedSceneBrowserRow(const ParameterRow& row) const;
    bool handleSavedSceneRowActivation(const ParameterRow& row) const;
    std::string resolveAssetKey(const std::string& id) const;
    const LayerLibrary::Entry* consoleEntryForParam(const std::string& id) const;
    void rebuildAssetCatalog();
    void appendAssetPlaceholders(const std::unordered_set<std::string>& assetKeysPresent) const;
    void appendBioAmpRows() const;
    void applyConsoleSlotAnnotations(ParameterRow& row) const;
    bool isBioAmpRowId(const std::string& id) const;
    bool readBioAmpValue(const std::string& id, float& outValue, uint64_t& outTimestamp) const;
    void markOfflineHydrationDirty() const;
    void hydrateOfflineAssetsIfNeeded() const;
    void refreshConsoleSlotInventory() const;
    std::string mapFamilyLabel(const std::string& raw) const;
    void markSlotCatalogDirty() const;
    void refreshSlotCatalog() const;
    std::vector<int> slotIndicesForRow(const ParameterRow& row) const;
    const SlotOption* slotForControlId(const std::string& controlId) const;
    const SlotOption* slotForLogicalKey(const std::string& deviceId, const std::string& slotId, bool analog) const;
    std::string logicalSlotKey(const std::string& deviceId, const std::string& slotId, bool analog) const;
    std::string parameterSuffixForAssignment(const std::string& parameterId,
                                             const std::string& assetKeyHint = std::string()) const;
    std::string assignmentKeyForId(const std::string& parameterId,
                                   const std::string& assetKeyHint = std::string()) const;
    std::string assignmentKeyForRow(const ParameterRow& row) const;
    void ensureSlotAssignmentDirectory() const;
    void loadSlotAssignments() const;
    void markSlotAssignmentsDirty() const;
    void flushSlotAssignments() const;
    const LogicalSlotBinding* logicalSlotBinding(const ParameterRow& row) const;
    bool applyLogicalSlotAssignment(const ParameterRow& row, const SlotOption& slot) const;
    bool removeLogicalSlotAssignment(const std::string& parameterId) const;
    bool applySlotAssignmentToRow(const ParameterRow& row) const;
    void rebuildSlotMidiAssignments() const;
    bool rebuildSlotMidiAssignmentsFromCurrentModel() const;
    bool parameterHasModifierConflict(const ParameterRow& row, modifier::Type type) const;
    std::string midiTakeoverBadge(const ParameterRow& row) const;
    std::string modifierActivityBadge(const ParameterRow& row, modifier::Type type) const;
    std::string activityHint(const ParameterRow& row) const;
    void setBannerMessage(const std::string& message, uint64_t durationMs = 2500) const;
    void setActiveRowSlot(int slot) const;
    bool setRowBoolValue(const ParameterRow& row, bool value) const;
    bool setRowFloatValue(const ParameterRow& row, float value) const;
    bool setRowStringValue(const ParameterRow& row, const std::string& value) const;
    bool readRowBoolValue(const ParameterRow& row, bool& outValue) const;
    bool readRowFloatValue(const ParameterRow& row, float& outValue) const;
    void drawRoutingPopover(float x, float y, float width, float height, const ParameterRow* row) const;
    void emitTelemetryEvent(const std::string& action,
                            const ParameterRow* row,
                            const std::string& extra = std::string()) const;
    void persistRoutingChange() const;
    bool runControlHubValidation() const;
    std::string subcategoryForId(const std::string& id) const;
    void moveRow(int delta) const;
    void moveColumn(int delta);
    bool activateSelection(MenuController& controller);
    const ParameterRow* selectedRowData() const;
    const ParameterRow* rowForId(const std::string& id) const;
    const MidiRouter::CcMap* firstCcMapForRow(const ParameterRow& row) const;
    void beginValueEdit(const ParameterRow& row) const;
    void cancelValueEdit(bool clearPending = true) const;
    bool commitValueEdit();
    bool handleValueEditCharacter(int key);
    void refreshValueEditTarget() const;
    bool beginMidiLearn(const ParameterRow& row) const;
    bool beginOscLearn(const ParameterRow& row) const;
    bool adjustMidiRange(const ParameterRow& row, float dMin, float dMax) const;
    // (declaration above)
};

inline ControlMappingHubState::ControlMappingHubState() = default;

inline ControlMappingHubState::~ControlMappingHubState() {
    flushPreferences();
    flushSlotAssignments();
}

inline void ControlMappingHubState::setPreferencesPath(const std::string& path) {
    preferencesPath_ = path;
    ensurePreferencesDirectory();
    loadPreferences();
}

inline void ControlMappingHubState::setKeyMappingState(const std::shared_ptr<MenuController::State>& state) {
    keyMappingState_ = state;
}

inline void ControlMappingHubState::setConsoleSlotLoadCallback(std::function<bool(int, const std::string&)> cb) {
    consoleSlotLoadCallback_ = std::move(cb);
}

inline void ControlMappingHubState::setConsoleSlotUnloadCallback(std::function<bool(int)> cb) {
    consoleSlotUnloadCallback_ = std::move(cb);
}

inline void ControlMappingHubState::setConsoleSlotInventoryCallback(std::function<std::vector<ConsoleLayerInfo>()> cb) {
    consoleSlotInventoryCallback_ = std::move(cb);
    consoleSlotInventoryDirty_ = true;
}

inline void ControlMappingHubState::setSavedSceneListCallback(std::function<std::vector<SavedSceneInfo>()> cb) {
    savedSceneListCallback_ = std::move(cb);
    tableModel_.dirty = true;
}

inline void ControlMappingHubState::setSavedSceneLoadCallback(std::function<bool(const std::string&)> cb) {
    savedSceneLoadCallback_ = std::move(cb);
}

inline void ControlMappingHubState::setSavedSceneSaveAsCallback(std::function<bool(const std::string&, bool)> cb) {
    savedSceneSaveAsCallback_ = std::move(cb);
}

inline void ControlMappingHubState::setSavedSceneOverwriteCallback(std::function<bool(const std::string&)> cb) {
    savedSceneOverwriteCallback_ = std::move(cb);
}

inline void ControlMappingHubState::markConsoleSlotsDirty() const {
    consoleSlotInventoryDirty_ = true;
    slotMidiAssignmentsDirty_ = true;
    tableModel_.dirty = true;
}

inline void ControlMappingHubState::setLayoutBand(const ofRectangle& bounds) {
    layoutBandOverride_ = bounds.getWidth() > 0.0f && bounds.getHeight() > 0.0f;
    if (layoutBandOverride_) {
        layoutBandBounds_ = bounds;
    }
}

inline void ControlMappingHubState::clearLayoutBand() {
    layoutBandOverride_ = false;
    layoutBandBounds_.set(0.0f, 0.0f, 0.0f, 0.0f);
}

inline void ControlMappingHubState::setMenuSkin(const MenuSkin& skin) {
    skin_ = skin;
}

inline void ControlMappingHubState::setParameterRegistry(ParameterRegistry* registry) {
    registry_ = registry;
    tableModel_.dirty = true;
    invalidateRowCache();
    cancelValueEdit();
    cancelHudColumnPicker();
    cancelSlotPicker();
    pendingValueBuffers_.clear();
    lastValueSnapshot_.clear();
        lastStringSnapshot_.clear();
    lastActivityMs_.clear();
}

inline void ControlMappingHubState::setMidiRouter(MidiRouter* router) {
    midiRouter_ = router;
    takeoverByTarget_.clear();
    if (midiRouter_) {
        rebuildSlotMidiAssignments();
    }
}

inline void ControlMappingHubState::setLayerLibrary(LayerLibrary* library) {
    layerLibrary_ = library;
    markOfflineHydrationDirty();
    rebuildAssetCatalog();
    tableModel_.dirty = true;
}

inline void ControlMappingHubState::setDeviceMapsDirectory(const std::string& path) {
    deviceMapsDirectory_ = path;
    markSlotCatalogDirty();
}

inline void ControlMappingHubState::setMidiPaneStatus(const std::string& description, bool available) {
    midiStatus_ = description;
    midiAvailable_ = available;
}

inline void ControlMappingHubState::setSlotAssignmentsPath(const std::string& path) {
    slotAssignmentsPath_ = path;
    slotAssignmentsLoaded_ = false;
    ensureSlotAssignmentDirectory();
    loadSlotAssignments();
    if (midiRouter_) {
        rebuildSlotMidiAssignments();
    }
}

inline void ControlMappingHubState::setOscPaneStatus(const std::string& description, bool available) {
    oscStatus_ = description;
    oscAvailable_ = available;
}

inline void ControlMappingHubState::setMidiAction(std::function<void(MenuController&)> action) {
    midiAction_ = std::move(action);
    midiAvailable_ = midiAction_ != nullptr;
}

inline void ControlMappingHubState::setOscAction(std::function<void(MenuController&)> action) {
    oscAction_ = std::move(action);
    oscAvailable_ = oscAction_ != nullptr;
}

inline void ControlMappingHubState::setRoutingRollbackAction(std::function<void()> action) {
    routingRollbackAction_ = std::move(action);
}

inline void ControlMappingHubState::setFloatValueCommitCallback(std::function<void(const std::string&, float)> cb) {
    floatValueCommitCallback_ = std::move(cb);
}

inline void ControlMappingHubState::setEventCallback(std::function<void(const std::string&)> cb) {
    eventCallback_ = std::move(cb);
}

inline void ControlMappingHubState::publishHudTelemetrySample(const std::string& widgetId,
                                                              const std::string& feedId,
                                                              float value,
                                                              const std::string& detail) const {
    if (!eventCallback_) {
        return;
    }
    ofJson j;
    j["type"] = "hud.telemetry";
    j["widgetId"] = widgetId;
    j["feedId"] = feedId;
    j["value"] = value;
    if (!detail.empty()) {
        j["detail"] = detail;
    }
    j["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    try {
        eventCallback_(j.dump());
    } catch (...) {
        ofLogWarning("ControlMappingHubState") << "Event callback threw while publishing HUD telemetry";
    }
}

inline void ControlMappingHubState::setBioAmpMetric(const std::string& metric,
                                                    float value,
                                                    uint64_t timestampMs) {
    if (timestampMs == 0) {
        timestampMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    }
    auto applySample = [&](BioAmpMetricSample& sample) {
        sample.value = value;
        sample.timestampMs = timestampMs;
        sample.valid = true;
    };
    auto emitSampleTelemetry = [&](const std::string& metricId, int precision) {
        emitTelemetryEvent("sensor.bioamp", nullptr, metricId + "=" + ofToString(value, precision));
    };
    auto logSample = [&](const std::string& metricId) {
        ofLogNotice("BioAmp") << metricId << "=" << value;
    };
    if (metric == "bioamp-raw") {
        applySample(bioAmpState_.raw);
        emitSampleTelemetry(metric, 3);
        logSample(metric);
        return;
    }
    if (metric == "bioamp-signal") {
        applySample(bioAmpState_.signal);
        emitSampleTelemetry(metric, 3);
        logSample(metric);
        return;
    }
    if (metric == "bioamp-mean") {
        applySample(bioAmpState_.mean);
        emitSampleTelemetry(metric, 3);
        logSample(metric);
        return;
    }
    if (metric == "bioamp-rms") {
        applySample(bioAmpState_.rms);
        emitSampleTelemetry(metric, 3);
        logSample(metric);
        return;
    }
    if (metric == "bioamp-dom-hz") {
        applySample(bioAmpState_.domHz);
        emitSampleTelemetry(metric, 3);
        logSample(metric);
        return;
    }
    // Allow callers to route other metadata through this helper.
    setBioAmpMetadata(metric, value, timestampMs);
}

inline void ControlMappingHubState::setBioAmpMetadata(const std::string& field,
                                                      float value,
                                                      uint64_t timestampMs) {
    if (timestampMs == 0) {
        timestampMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    }
    if (field == "bioamp-sample-rate") {
        bioAmpState_.sampleRate = static_cast<uint16_t>(std::max(0.0f, value));
        bioAmpState_.sampleRateTimestampMs = timestampMs;
        emitTelemetryEvent("sensor.bioamp", nullptr, "sample_rate=" + ofToString(value, 0));
        return;
    }
    if (field == "bioamp-window") {
        bioAmpState_.windowSize = static_cast<uint16_t>(std::max(0.0f, value));
        bioAmpState_.windowTimestampMs = timestampMs;
        emitTelemetryEvent("sensor.bioamp", nullptr, "window=" + ofToString(value, 0));
        return;
    }
    setBioAmpMetric(field, value, timestampMs);
}

inline void ControlMappingHubState::setHudVisible(bool visible) {
    if (hudVisible_ == visible) {
        return;
    }
    hudVisible_ = visible;
    preferences_.hudVisible = visible;
    markPreferencesDirty();
    if (hudVisibilityChanged_) {
        hudVisibilityChanged_(hudVisible_);
    }
}

inline void ControlMappingHubState::setHudVisibilityCallback(std::function<void(bool)> cb) {
    hudVisibilityChanged_ = std::move(cb);
    if (hudVisibilityChanged_) {
        hudVisibilityChanged_(hudVisible_);
    }
}

inline void ControlMappingHubState::setHudToggleCallback(std::function<void(const std::string&, bool)> cb) {
    hudToggleCallback_ = std::move(cb);
    replayHudTogglePreferences();
}

inline void ControlMappingHubState::setHudLayoutAction(std::function<bool(MenuController&)> action) {
    hudLayoutAction_ = std::move(action);
    tableModel_.dirty = true;
}

inline void ControlMappingHubState::setHudPlacementProvider(std::function<std::vector<HudPlacementSnapshot>()> provider) {
    hudPlacementProvider_ = std::move(provider);
    if (hudPlacementProvider_) {
        hudLayoutDirty_ = true;
        snapshotHudPreferencesFromProvider();
    }
}

inline void ControlMappingHubState::setHudPlacementCallback(std::function<void(const std::string&, int)> cb) {
    hudPlacementCallback_ = std::move(cb);
    replayHudPlacementPreferences();
}

inline std::string ControlMappingHubState::hudLayoutTargetToString(HudLayoutTarget target) {
    switch (target) {
    case HudLayoutTarget::Controller:
        return "controller";
    case HudLayoutTarget::Projector:
    default:
        return "projector";
    }
}

inline std::string ControlMappingHubState::hudLayoutTargetName(HudLayoutTarget target) {
    return hudLayoutTargetToString(target);
}

inline ControlMappingHubState::HudLayoutTarget ControlMappingHubState::hudLayoutTargetFromString(const std::string& value) {
    std::string lowered = ofToLower(value);
    if (lowered == "controller" || lowered == "secondary") {
        return HudLayoutTarget::Controller;
    }
    return HudLayoutTarget::Projector;
}

inline void ControlMappingHubState::setHudLayoutTarget(HudLayoutTarget target) {
    if (target == hudLayoutTarget_) {
        return;
    }
    std::string targetId = hudLayoutTargetToString(target);
    {
        HudLayoutFenceScope fence(this);
        snapshotHudPreferencesFromProvider();
        hudLayoutTarget_ = target;
        if (hudLayoutTarget_ == HudLayoutTarget::Controller && preferences_.controllerHudWidgets.empty()) {
            seedControllerLayoutFromPrimary();
        }
        if (preferences_.hudLayoutTarget != targetId) {
            preferences_.hudLayoutTarget = targetId;
            markPreferencesDirty();
        }
        replayHudPlacementPreferences();
    }
    emitHudMappingEvent("target", std::string(), targetId);
}

inline void ControlMappingHubState::setHudFeedRegistry(HudFeedRegistry* registry) {
    hudFeedRegistry_ = registry;
    if (hudFeedRegistry_ && !hudFeedListenerAttached_) {
        hudFeedRegistry_->addListener([this](const HudFeedRegistry::FeedEntry& entry) {
            emitHudFeedUpdated(entry);
        });
        hudFeedListenerAttached_ = true;
    }
}

inline void ControlMappingHubState::notifyHudLayoutChanged() const {
    hudLayoutDirty_ = true;
    if (hudLayoutFenceDepth_ > 0) {
        hudLayoutFenceDirty_ = true;
        return;
    }
    applyHudLayoutSnapshot();
}

inline bool ControlMappingHubState::importLegacyHudConfig(const std::string& hudConfigPath,
                                                          const std::string& overlayConfigPath) {
    if (preferences_.hudStateMigrated) {
        return false;
    }
    bool imported = false;
    if (!hudConfigPath.empty() && ofFile::doesFileExist(hudConfigPath)) {
        try {
            ofJson json = ofLoadJson(hudConfigPath);
            if (json.contains("toggles") && json["toggles"].is_object()) {
                const auto& toggles = json["toggles"];
                for (auto it = toggles.begin(); it != toggles.end(); ++it) {
                    if (!it.value().is_object()) {
                        continue;
                    }
                    if (!it.value().contains("value") || !it.value()["value"].is_boolean()) {
                        continue;
                    }
                    bool state = it.value()["value"].get<bool>();
                    const std::string id = it.key();
                    if (id == "hud.visible") {
                        setHudVisible(state);
                        imported = true;
                        continue;
                    }
                    if (id.rfind("hud.", 0) != 0) {
                        continue;
                    }
                    updateHudVisibilityPreference(id, state);
                    imported = true;
                }
            }
        } catch (const std::exception& ex) {
            ofLogWarning("ControlMappingHubState") << "Failed to import legacy HUD toggles: " << ex.what();
        }
    }
    if (!overlayConfigPath.empty() && ofFile::doesFileExist(overlayConfigPath)) {
        try {
            ofJson json = ofLoadJson(overlayConfigPath);
            if (json.contains("widgets") && json["widgets"].is_array()) {
                for (const auto& widgetNode : json["widgets"]) {
                    if (!widgetNode.is_object()) {
                        continue;
                    }
                    if (!widgetNode.contains("id") || !widgetNode["id"].is_string()) {
                        continue;
                    }
                    std::string widgetId = widgetNode["id"].get<std::string>();
                    if (widgetId.empty()) {
                        continue;
                    }
                    if (widgetNode.contains("visible") && widgetNode["visible"].is_boolean()) {
                        updateHudVisibilityPreference(widgetId, widgetNode["visible"].get<bool>());
                    }
                    if (widgetNode.contains("column") && widgetNode["column"].is_number_integer()) {
                        updateHudPlacementPreference(widgetId, widgetNode["column"].get<int>());
                    }
                    auto& pref = preferences_.hudWidgets[widgetId];
                    if (widgetNode.contains("band") && widgetNode["band"].is_string()) {
                        pref.bandId = widgetNode["band"].get<std::string>();
                    }
                    if (widgetNode.contains("collapsed") && widgetNode["collapsed"].is_boolean()) {
                        pref.collapsed = widgetNode["collapsed"].get<bool>();
                    }
                    imported = true;
                }
            }
        } catch (const std::exception& ex) {
            ofLogWarning("ControlMappingHubState") << "Failed to import legacy HUD layout: " << ex.what();
        }
    }
    if (imported) {
        preferences_.hudStateMigrated = true;
        markPreferencesDirty();
        replayHudTogglePreferences();
        replayHudPlacementPreferences();
    }
    return imported;
}

inline void ControlMappingHubState::focusCategory(const std::string& category, const std::string& subcategory) {
    pendingCategoryPref_ = category;
    pendingSubcategoryPref_ = subcategory;
    pendingAssetPref_.clear();
    treeSelectionPending_ = true;
}

inline void ControlMappingHubState::emitTelemetryEvent(const std::string& action,
                                                      const ParameterRow* row,
                                                      const std::string& extra) const {
    ofJson j;
    j["type"] = action;
    if (row) {
        j["parameterId"] = row->id;
        j["label"] = row->label;
        j["category"] = row->category;
    }
    if (!extra.empty()) {
        j["detail"] = extra;
    }
    j["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    if (eventCallback_) {
        try {
            eventCallback_(j.dump());
        } catch (...) {
            ofLogWarning("ControlMappingHubState") << "Event callback threw an exception";
        }
    }
}

inline MenuController::StateView ControlMappingHubState::view() const {
    rebuildView();


    return cachedView_;
}

inline void ControlMappingHubState::draw() const {
    if (!active_ || !controller_ || !controller_->isCurrent(id())) {
        return;
    }

    rebuildView();

    const ParameterRow* selectedRow = selectedRowData();

    uint64_t nowMs = ofGetElapsedTimeMillis();
    bool bannerActive = false;
    if (!bannerMessage_.empty()) {
        if (nowMs <= bannerExpiryMs_) {
            bannerActive = true;
        } else {
            bannerMessage_.clear();
        }
    }

    const bool useLayoutBand = layoutBandOverride_ && layoutBandBounds_.getWidth() > 1.0f && layoutBandBounds_.getHeight() > 1.0f;
    if (useLayoutBand) {
        ofPushMatrix();
        ofTranslate(layoutBandBounds_.x, layoutBandBounds_.y);
    }
    const float viewportWidth = useLayoutBand ? layoutBandBounds_.getWidth() : static_cast<float>(ofGetWidth());
    const float viewportHeight = useLayoutBand ? layoutBandBounds_.getHeight() : static_cast<float>(ofGetHeight());
    if (viewportWidth <= 1.0f || viewportHeight <= 1.0f) {
        if (useLayoutBand) {
            ofPopMatrix();
        }
        return;
    }

    LayoutContext layout = computeLayoutContext(viewportWidth, viewportHeight);

    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    auto drawTextStyled = [textScale](const std::string& text, float x, float y, const ofColor& color, bool bold) {
        ofSetColor(color);
        drawBitmapStringScaled(text, x, y, textScale, bold);
    };

    ofPushStyle();
    ofSetColor(skin_.palette.background);
    ofDrawRectangle(0.0f, 0.0f, viewportWidth, viewportHeight);

    if (bannerActive) {
        drawTextStyled(bannerMessage_,
                       layout.treeX,
                       std::max(12.0f, layout.treeY - 6.0f),
                       skin_.palette.warning,
                       false);
    }

    ofSetColor(skin_.palette.surface);
    ofDrawRectRounded(layout.treeX, layout.treeY, layout.treeWidth, layout.treeHeight, skin_.metrics.borderRadius);
    ofSetColor(skin_.palette.surfaceAlternate);
    ofDrawRectRounded(layout.gridX, layout.gridY, layout.gridWidth, layout.gridHeight, skin_.metrics.borderRadius);

    ofSetColor(skin_.palette.gridDivider);
    ofDrawLine(layout.treeX + skin_.metrics.padding * 0.5f,
               layout.treeBodyY,
               layout.treeX + layout.treeWidth - skin_.metrics.padding * 0.5f,
               layout.treeBodyY);
    ofDrawLine(layout.gridX + skin_.metrics.padding * 0.5f,
               layout.gridY + layout.treeHeaderHeight,
               layout.gridX + layout.gridWidth - skin_.metrics.padding * 0.5f,
               layout.gridY + layout.treeHeaderHeight);

    drawTextStyled("Elements",
                   layout.treeX + skin_.metrics.padding,
                   layout.headerBaseline,
                   skin_.palette.headerText,
                   true);

    const auto headers = columnHeaders();
    const auto visibleCols = visibleColumns();
    std::array<float, static_cast<int>(Column::kCount)> columnWeights{{0.28f, 0.11f, 0.11f, 0.11f, 0.11f, 0.11f, 0.09f, 0.08f}};
    std::array<float, static_cast<int>(Column::kCount)> columnPositions{};
    std::array<float, static_cast<int>(Column::kCount)> columnWidths{};
    float weightSum = 0.0f;
    for (int idx : visibleCols) {
        weightSum += columnWeights[idx];
    }
    if (weightSum <= 0.0f) {
        weightSum = 1.0f;
    }
    float columnCursor = layout.gridX + skin_.metrics.padding;
    float availableWidth = layout.gridWidth - skin_.metrics.padding * 2.0f;
    for (int idx : visibleCols) {
        float share = columnWeights[idx] / weightSum;
        float colWidth = std::max(60.0f, availableWidth * share);
        columnPositions[idx] = columnCursor;
        columnWidths[idx] = colWidth;
        columnCursor += colWidth;
    }
    for (int idx : visibleCols) {
        std::string title = ellipsize(headers[idx], columnWidths[idx] - 6.0f);
        drawTextStyled(title,
                       columnPositions[idx],
                       layout.headerBaseline,
                       skin_.palette.headerText,
                       true);
    }

    int treeCount = static_cast<int>(tableModel_.tree.size());
    int treeStart = std::min(treeScrollOffset_, std::max(0, treeCount - layout.treeVisibleRows));
    for (int row = 0; row < layout.treeVisibleRows && treeStart + row < treeCount; ++row) {
        int nodeIndex = treeStart + row;
        const auto& node = tableModel_.tree[nodeIndex];
        float rowTop = layout.treeBodyY + row * layout.treeRowHeight;
        bool selected = (nodeIndex == selectedTreeNodeIndex_);
        if (selected) {
            ofSetColor(skin_.palette.gridSelectionFill);
            ofDrawRectRounded(layout.treeX + 4.0f,
                              rowTop + 1.0f,
                              layout.treeWidth - 8.0f,
                              layout.treeRowHeight - 2.0f,
                              skin_.metrics.borderRadius * 0.5f);
        }
        float indent = skin_.metrics.treeIndent * static_cast<float>(node.depth);
        float labelX = layout.treeX + skin_.metrics.padding + indent;
        if (node.expandable) {
            drawTextStyled(node.expanded ? "-" : "+",
                           layout.treeX + skin_.metrics.padding + indent - 12.0f,
                           rowTop + layout.treeRowHeight * 0.65f,
                           skin_.palette.headerText,
                           false);
        }
        float available = layout.treeWidth - (labelX - layout.treeX) - skin_.metrics.padding;
        std::string label = ellipsize(node.label, available);
        ofColor textColor = selected
            ? (focusPane_ == FocusPane::kTree ? skin_.palette.treeFocus : skin_.palette.bodyText)
            : skin_.palette.bodyText;
        drawTextStyled(label,
                       labelX,
                       rowTop + layout.treeRowHeight * 0.7f,
                       textColor,
                       selected && focusPane_ == FocusPane::kTree);
    }
    if (treeCount == 0) {
        drawTextStyled("No parameter groups registered",
                       layout.treeX + skin_.metrics.padding,
                       layout.treeBodyY + 20.0f,
                       skin_.palette.mutedText,
                       false);
    }

    const auto& activeRows = activeRowIndices();
    auto oscSources = oscSummaryActive() ? oscBrowserSources() : std::vector<MidiRouter::OscSourceInfo>{};
    int rowCount = oscSummaryActive() ? static_cast<int>(oscSources.size())
                                      : static_cast<int>(activeRows.size());
    float gridRowTop = layout.gridY + layout.treeHeaderHeight;
    if (!oscSummaryActive()) {
        int gridStart = std::min(gridScrollOffset_, std::max(0, rowCount - layout.gridVisibleRows));
        for (int i = 0; i < layout.gridVisibleRows && gridStart + i < rowCount; ++i) {
            int rowIndex = activeRows[gridStart + i];
            const auto& row = tableModel_.rows[rowIndex];
            bool rowSelected = (rowIndex == selectedRow_);
            float cellTop = gridRowTop + i * layout.gridRowHeight;
            if (rowSelected) {
                ofSetColor(skin_.palette.gridSelectionFill);
                ofDrawRectRounded(layout.gridX + 4.0f,
                                  cellTop + 1.0f,
                                  layout.gridWidth - 8.0f,
                                  layout.gridRowHeight - 2.0f,
                                  skin_.metrics.borderRadius * 0.5f);
            }
            for (int idx : visibleCols) {
                Column column = static_cast<Column>(idx);
                std::string cell;
                switch (column) {
                case Column::kName: cell = row.label; break;
                case Column::kValue:
                    if (editingValueActive_ && row.id == editingValueRowId_) {
                        cell = editingValueBuffer_.empty() ? std::string("?") : editingValueBuffer_;
                        if (editingValueError_) {
                            cell += " !";
                        }
                        cell = "{" + cell + "}";
                    } else {
                        cell = formatValue(row);
                    }
                    break;
                case Column::kSlot: cell = formatSlotSummary(row); break;
                case Column::kMidi: cell = formatMidiSummary(row); break;
                case Column::kMidiMin: cell = formatMidiMin(row); break;
                case Column::kMidiMax: cell = formatMidiMax(row); break;
                case Column::kOsc: cell = formatOscSummary(row); break;
                default: break;
                }
                bool cellSelected = rowSelected && (static_cast<int>(selectedColumn_) == idx);
                bool highlightActive = cellSelected && focusPane_ == FocusPane::kGrid;
                bool editingCell = (column == Column::kValue && editingValueActive_ && row.id == editingValueRowId_);
                bool editError = editingCell && editingValueError_;
                ofColor textColor = row.offline ? skin_.palette.mutedText : skin_.palette.bodyText;
                if (highlightActive) {
                    textColor = skin_.palette.gridSelection;
                } else if (editError) {
                    textColor = skin_.palette.warning;
                }
                std::string display = ellipsize(cell, columnWidths[idx] - 6.0f);
                drawTextStyled(display,
                               columnPositions[idx],
                               cellTop + layout.gridRowHeight * 0.7f,
                               textColor,
                               highlightActive);
                if (cellSelected && highlightActive) {
                    ofNoFill();
                    ofSetColor(skin_.palette.gridSelection);
                    ofSetLineWidth(skin_.metrics.focusStroke);
                    ofDrawRectRounded(columnPositions[idx] - 2.0f,
                                      cellTop + 2.0f,
                                      columnWidths[idx] - 4.0f,
                                      layout.gridRowHeight - 4.0f,
                                      skin_.metrics.borderRadius * 0.5f);
                    ofSetLineWidth(1.0f);
                    ofFill();
                }
            }
        }

        if (rowCount == 0) {
            drawTextStyled("Select an asset to view parameters",
                           layout.gridX + skin_.metrics.padding,
                           layout.gridY + layout.treeHeaderHeight + 20.0f,
                           skin_.palette.mutedText,
                           false);
        }
    } else {
        int gridStart = std::min(gridScrollOffset_, std::max(0, rowCount - layout.gridVisibleRows));
        for (int i = 0; i < layout.gridVisibleRows && gridStart + i < rowCount; ++i) {
            int sourceIndex = gridStart + i;
            const auto& src = oscSources[sourceIndex];
            const auto* profile = currentOscSourceProfile(src.address);
            bool rowSelected = (sourceIndex == oscPickerSelection_);
            float cellTop = gridRowTop + i * layout.gridRowHeight;
            if (rowSelected) {
                ofSetColor(skin_.palette.gridSelectionFill);
                ofDrawRectRounded(layout.gridX + 4.0f,
                                  cellTop + 1.0f,
                                  layout.gridWidth - 8.0f,
                                  layout.gridRowHeight - 2.0f,
                                  skin_.metrics.borderRadius * 0.5f);
            }
            for (int idx : visibleCols) {
                Column column = static_cast<Column>(idx);
                std::string cell;
                switch (column) {
                case Column::kName:
                    cell = src.address.empty() ? "(hint)" : src.address;
                    break;
                case Column::kValue:
                    cell = src.seen ? ofToString(src.lastValue, 3) : "-";
                    break;
                case Column::kSlot:
                    cell = profile ? ofToString(profile->inMin, 2) : "-";
                    break;
                case Column::kMidi:
                    cell = profile ? ofToString(profile->inMax, 2) : "-";
                    break;
                case Column::kMidiMin:
                    cell = profile ? ofToString(profile->outMin, 3) : "-";
                    break;
                case Column::kMidiMax:
                    cell = profile ? ofToString(profile->outMax, 3) : "-";
                    break;
                case Column::kOsc:
                    cell = profile ? ofToString(profile->smooth, 2) : "-";
                    break;
                case Column::kOscDeadband:
                    cell = profile ? ofToString(profile->deadband, 2) : "-";
                    break;
                default:
                    break;
                }
                bool cellSelected = rowSelected && (static_cast<int>(selectedColumn_) == idx);
                bool highlightActive = cellSelected && focusPane_ == FocusPane::kGrid;
                bool editingCell = oscValueEditActive_ && src.address == oscValueEditRowId_ && static_cast<int>(selectedColumn_) == idx;
                bool outOfRange = false;
                if (column == Column::kValue && profile && src.seen) {
                    float lo = std::min(profile->inMin, profile->inMax);
                    float hi = std::max(profile->inMin, profile->inMax);
                    outOfRange = (src.lastValue < lo || src.lastValue > hi);
                }
                if (editingCell) {
                    cell = oscValueEditBuffer_.empty() ? std::string("0") : oscValueEditBuffer_;
                }
                ofColor textColor = skin_.palette.bodyText;
                if (highlightActive) {
                    textColor = skin_.palette.gridSelection;
                } else if (outOfRange) {
                    textColor = skin_.palette.warning;
                }
                std::string display = ellipsize(cell, columnWidths[idx] - 6.0f);
                drawTextStyled(display,
                               columnPositions[idx],
                               cellTop + layout.gridRowHeight * 0.7f,
                               textColor,
                               highlightActive);
                if (cellSelected && highlightActive) {
                    ofNoFill();
                    ofSetColor(skin_.palette.gridSelection);
                    ofSetLineWidth(skin_.metrics.focusStroke);
                    ofDrawRectRounded(columnPositions[idx] - 2.0f,
                                      cellTop + 2.0f,
                                      columnWidths[idx] - 4.0f,
                                      layout.gridRowHeight - 4.0f,
                                      skin_.metrics.borderRadius * 0.5f);
                    ofSetLineWidth(1.0f);
                    ofFill();
                }
            }
        }

        if (rowCount == 0) {
            drawTextStyled("No OSC sources observed yet",
                           layout.gridX + skin_.metrics.padding,
                           layout.gridY + layout.treeHeaderHeight + 20.0f,
                           skin_.palette.mutedText,
                           false);
        }
    }

    if (hudColumnPickerVisible_) {
        float panelWidth = std::min(360.0f, layout.gridWidth * 0.5f);
        float panelHeight = 200.0f;
        float panelX = layout.gridX + layout.gridWidth - panelWidth - skin_.metrics.padding;
        float panelY = layout.gridY + layout.treeHeaderHeight + skin_.metrics.padding;
        drawHudColumnPickerPanel(panelX, panelY, panelWidth, panelHeight, selectedRow);
    } else if (slotPickerVisible_) {
        float panelWidth = std::min(420.0f, layout.gridWidth * 0.65f);
        float panelHeight = 220.0f;
        float panelX = layout.gridX + layout.gridWidth - panelWidth - skin_.metrics.padding;
        float panelY = layout.gridY + layout.treeHeaderHeight + skin_.metrics.padding;
        drawSlotPickerPanel(panelX, panelY, panelWidth, panelHeight, selectedRow);
    } else if (pickingOsc_ && midiRouter_) {
        const auto sources = oscBrowserSources();
        float panelX = layout.gridX + skin_.metrics.padding;
        float panelWidth = layout.gridWidth - skin_.metrics.padding * 2.0f;
        float panelHeight = std::min(layout.gridHeight * 0.6f, 240.0f);
        drawOscPickerPanel(panelX,
                           layout.gridY + layout.treeHeaderHeight + skin_.metrics.padding,
                           panelWidth,
                           panelHeight,
                           sources,
                           selectedRow);
    } else if (routingPopoverVisible_ && selectedRow) {
        float popoverWidth = std::min(360.0f, layout.gridWidth * 0.5f);
        float popoverHeight = 180.0f;
        float popoverX = layout.gridX + layout.gridWidth - popoverWidth - skin_.metrics.padding;
        float popoverY = layout.gridY + layout.treeHeaderHeight + skin_.metrics.padding;
        drawRoutingPopover(popoverX, popoverY, popoverWidth, popoverHeight, selectedRow);
    }

    ofPopStyle();
    if (useLayoutBand) {
        ofPopMatrix();
    }
}

inline bool ControlMappingHubState::handleInput(MenuController& controller, int key) {
    int baseKey = key & 0xFFFF;
    bool ctrlDown = (key & MenuController::HOTKEY_MOD_CTRL) != 0;
    bool shiftDown = (key & MenuController::HOTKEY_MOD_SHIFT) != 0;

    if (!active_) {

        return false;

    }

    if (ctrlDown && baseKey >= '1' && baseKey <= '8') {
        int slotIndex = baseKey - '0';
        if (slotIndex < 1 || slotIndex > 8) {
            return true;
        }
        if (shiftDown) {
            if (!consoleSlotUnloadCallback_) {
                setBannerMessage("Console unload unavailable", 2200);
            } else if (consoleSlotUnloadCallback_(slotIndex)) {
                setBannerMessage("Slot " + ofToString(slotIndex) + " cleared", 2000);
                markConsoleSlotsDirty();
                emitTelemetryEvent("console.slot.unload", nullptr, "slot=" + ofToString(slotIndex));
                controller.requestViewModelRefresh();
            } else {
                setBannerMessage("Slot " + ofToString(slotIndex) + " unchanged", 2000);
            }
        } else {
            std::string assetId = selectedAssetId();
            if (assetId.empty()) {
                setBannerMessage("Select an asset to load into the console", 2400);
            } else if (!consoleSlotLoadCallback_) {
                setBannerMessage("Console load unavailable", 2200);
            } else if (consoleSlotLoadCallback_(slotIndex, assetId)) {
                std::string label = selectedAssetLabel();
                if (label.empty()) {
                    label = assetId;
                }
                setBannerMessage("Loaded " + label + " -> slot " + ofToString(slotIndex), 2200);
                markConsoleSlotsDirty();
                emitTelemetryEvent("console.slot.load", selectedRowData(), "slot=" + ofToString(slotIndex));
                controller.requestViewModelRefresh();
            } else {
                setBannerMessage("Failed to load asset into slot " + ofToString(slotIndex), 2400);
            }
        }
        return true;
    }

    if (hudColumnPickerVisible_) {
        auto adjustSelection = [&](int delta) {
            const int optionCount = kHudColumnCount + 1;
            hudColumnPickerSelection_ = (hudColumnPickerSelection_ + delta + optionCount) % optionCount;
        };
        switch (baseKey) {
        case OF_KEY_ESC:
            cancelHudColumnPicker();
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_UP:
            adjustSelection(-1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_DOWN:
            adjustSelection(1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_RETURN:
        case ' ':
            if (applyHudColumnSelection()) {
                controller.requestViewModelRefresh();
            } else {
                controller.requestViewModelRefresh();
            }
            return true;
        default:
            return true;
        }
    }

    if (slotPickerVisible_) {
        if (slotPickerIndices_.empty()) {
            cancelHudColumnPicker();
            cancelSlotPicker();
            controller.requestViewModelRefresh();
            return true;
        }
        auto ensureSelectionVisible = [&](int nextSelection) {
            if (slotPickerIndices_.empty()) {
                slotPickerSelection_ = -1;
                slotPickerScrollOffset_ = 0;
                return;
            }
            int count = static_cast<int>(slotPickerIndices_.size());
            slotPickerSelection_ = ofClamp(nextSelection, 0, count - 1);
            int visibleRows = std::min(count, kSlotPickerVisibleRows);
            if (slotPickerSelection_ < slotPickerScrollOffset_) {
                slotPickerScrollOffset_ = slotPickerSelection_;
            } else if (slotPickerSelection_ >= slotPickerScrollOffset_ + visibleRows) {
                slotPickerScrollOffset_ = slotPickerSelection_ - visibleRows + 1;
            }
            int maxOffset = std::max(0, count - visibleRows);
            slotPickerScrollOffset_ = ofClamp(slotPickerScrollOffset_, 0, maxOffset);
        };
        switch (baseKey) {
        case OF_KEY_ESC:
            cancelHudColumnPicker();
            cancelSlotPicker();
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_UP:
            ensureSelectionVisible(slotPickerSelection_ - 1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_DOWN:
            ensureSelectionVisible(slotPickerSelection_ + 1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_PAGE_UP:
            ensureSelectionVisible(slotPickerSelection_ - kSlotPickerVisibleRows);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_PAGE_DOWN:
            ensureSelectionVisible(slotPickerSelection_ + kSlotPickerVisibleRows);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_HOME:
            ensureSelectionVisible(0);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_END:
            ensureSelectionVisible(static_cast<int>(slotPickerIndices_.size()) - 1);
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_RETURN:
            if (applySelectedSlot()) {
                controller.requestViewModelRefresh();
            } else {
                controller.requestViewModelRefresh();
            }
            return true;
        default:
            return true;
        }
    }

    if (!editingValueActive_ && !pickingOsc_ && !oscSummaryActive() && key == OF_KEY_BACKSPACE) {
        // swallow Backspace so it never bubbles up to MenuController pop-state handling
        return true;
    }

    rebuildView();

    const ParameterRow* selectedRow = selectedRowData();

    bool oscSummaryNavigationKey =
        key == OF_KEY_TAB ||
        key == OF_KEY_UP ||
        key == OF_KEY_DOWN ||
        key == OF_KEY_LEFT ||
        key == OF_KEY_RIGHT ||
        key == OF_KEY_PAGE_UP ||
        key == OF_KEY_PAGE_DOWN ||
        key == OF_KEY_HOME ||
        key == OF_KEY_END;
    if (pickingOsc_ || (oscSummaryActive() && !oscSummaryNavigationKey)) {
        if (!midiRouter_) {
            pickingOsc_ = false;
            cancelOscValueEdit();
            controller.requestViewModelRefresh();
            return true;
        }
        const auto sources = oscBrowserSources();
        if (sources.empty()) {
            pickingOsc_ = false;
            cancelOscValueEdit();
            controller.requestViewModelRefresh();
            return true;
        }
        bool sourceEditorMode = oscSummaryActive() && !pickingOsc_;
        std::string targetId = selectedRow ? selectedRow->id : std::string();
        std::string selectedAddress = selectedOscSourceAddress(sources);
        if (oscValueEditActive_) {
            if (key == OF_KEY_ESC) {
                cancelOscValueEdit();
                controller.requestViewModelRefresh();
                return true;
            }
            if (key == OF_KEY_RETURN) {
                bool committed = sourceEditorMode
                    ? commitOscSourceValueEdit(selectedAddress)
                    : (selectedRow && commitOscValueEdit(*selectedRow));
                if (committed) {
                    controller.requestViewModelRefresh();
                }
                return true;
            }
            if (handleOscValueEditCharacter(key)) {
                controller.requestViewModelRefresh();
                return true;
            }
        }
        switch (key) {
        case OF_KEY_ESC:
            pickingOsc_ = false;
            cancelOscValueEdit();
            controller.requestViewModelRefresh();
            return true;
        case OF_KEY_DOWN:
            if (!sourceEditorMode) {
                oscPickerSelection_ = (oscPickerSelection_ + 1) % static_cast<int>(sources.size());
                controller.requestViewModelRefresh();
                return true;
            }
            break;
        case OF_KEY_UP:
            if (!sourceEditorMode) {
                oscPickerSelection_ = (oscPickerSelection_ - 1 + static_cast<int>(sources.size())) % static_cast<int>(sources.size());
                controller.requestViewModelRefresh();
                return true;
            }
            break;
        case OF_KEY_LEFT:
            if (!sourceEditorMode) {
                if (int count = oscEditorColumnCount(); count > 0) {
                    oscEditColumn_ = (oscEditColumn_ - 1 + count) % count;
                }
                controller.requestViewModelRefresh();
                return true;
            }
            break;
        case OF_KEY_RIGHT:
            if (!sourceEditorMode) {
                if (int count = oscEditorColumnCount(); count > 0) {
                    oscEditColumn_ = (oscEditColumn_ + 1) % count;
                }
                controller.requestViewModelRefresh();
                return true;
            }
            break;
        case ',':
        case '.':
        case '<':
        case '>': {
            if (sourceEditorMode ? !selectedAddress.empty() : !targetId.empty()) {
                float coarse = 0.05f;
                float fine = 0.01f;
                float delta = 0.0f;
                if (key == ',') delta = -coarse;
                if (key == '.') delta = coarse;
                if (key == '<') delta = -fine;
                if (key == '>') delta = fine;
                bool adjusted = delta != 0.0f &&
                    (sourceEditorMode
                        ? adjustOscSourceProfile(selectedAddress, static_cast<int>(selectedColumn_), delta)
                        : (selectedRow && adjustCurrentOscMap(*selectedRow, oscEditColumn_, delta)));
                if (adjusted) {
                    if (sourceEditorMode) {
                        persistRoutingChange();
                    }
                    controller.requestViewModelRefresh();
                }
            }
            return true;
        }
        case '1':
        case '2':
        case '3': {
            if (sourceEditorMode ? !selectedAddress.empty() : static_cast<bool>(selectedRow)) {
                float outMin = 0.9f;
                float outMax = 1.1f;
                if (key == '1') {
                    outMin = 0.5f;
                    outMax = 1.5f;
                } else if (key == '3') {
                    outMin = 1.0f;
                    outMax = 1.0f;
                }
                bool applied = sourceEditorMode
                    ? applyOscSourcePresetRange(selectedAddress, outMin, outMax)
                    : applyOscPresetRange(*selectedRow, outMin, outMax);
                if (applied) {
                    if (sourceEditorMode) {
                        persistRoutingChange();
                    }
                    controller.requestViewModelRefresh();
                }
            }
            return true;
        }
        case 'f':
        case 'F':
            if ((sourceEditorMode && flipOscSourceOutputRange(selectedAddress)) ||
                (!sourceEditorMode && selectedRow && flipCurrentOscOutputRange(*selectedRow))) {
                if (sourceEditorMode) {
                    persistRoutingChange();
                }
                controller.requestViewModelRefresh();
            }
            return true;
        case 'l':
        case 'L':
            if (oscValueEditActive_) {
                cancelOscValueEdit();
            } else if (sourceEditorMode && !selectedAddress.empty()) {
                beginOscSourceValueEdit(selectedAddress);
            } else if (selectedRow) {
                beginOscValueEdit(*selectedRow);
            }
            controller.requestViewModelRefresh();
            return true;
        case 's':
        case 'S':
            if ((sourceEditorMode && adjustOscSourceSmoothing(selectedAddress, key == 'S' ? 0.05f : -0.05f)) ||
                (!sourceEditorMode && selectedRow && adjustOscSmoothing(*selectedRow, key == 'S' ? 0.05f : -0.05f))) {
                if (sourceEditorMode) {
                    persistRoutingChange();
                }
                controller.requestViewModelRefresh();
            }
            return true;
        case 'd':
        case 'D':
            if ((sourceEditorMode && adjustOscSourceDeadband(selectedAddress, key == 'D' ? 0.01f : -0.01f)) ||
                (!sourceEditorMode && selectedRow && adjustOscDeadband(*selectedRow, key == 'D' ? 0.01f : -0.01f))) {
                if (sourceEditorMode) {
                    persistRoutingChange();
                }
                controller.requestViewModelRefresh();
            }
            return true;
        case OF_KEY_RETURN:
            if (!sourceEditorMode && selectedRow && oscPickerSelection_ >= 0 && oscPickerSelection_ < static_cast<int>(sources.size())) {
                const auto& src = sources[oscPickerSelection_];
                if (!src.address.empty()) {
                    if (midiRouter_->setOscMapFromAddress(selectedRow->id, src.address)) {
                        ofLogNotice("ControlMappingHub") << "Mapped OSC " << src.address << " -> " << selectedRow->id;
                        emitTelemetryEvent("osc.bind", selectedRow, src.address);
                        persistRoutingChange();
                    } else {
                        ofLogWarning("ControlMappingHub") << "Failed to map OSC " << src.address << " -> " << selectedRow->id;
                    }
                }
            }
            if (!sourceEditorMode) {
                pickingOsc_ = false;
            }
            cancelOscValueEdit();
            controller.requestViewModelRefresh();
            return true;
        case 'u':
        case 'U':
            if (!sourceEditorMode && selectedRow && midiRouter_) {
                if (midiRouter_->removeOscMappingsForTarget(selectedRow->id)) {
                    ofLogNotice("ControlMappingHub") << "Removed OSC mappings for " << selectedRow->id;
                    setBannerMessage("OSC mappings removed");
                    emitTelemetryEvent("osc.unmap", selectedRow);
                    persistRoutingChange();
                } else {
                    setBannerMessage("No OSC mappings found", 1800);
                }
            }
            if (!sourceEditorMode) {
                pickingOsc_ = false;
            }
            cancelOscValueEdit();
            controller.requestViewModelRefresh();
            return true;
        default:
            return true;
        }
    }

    if (focusPane_ == FocusPane::kGrid && editingValueActive_) {
        if (key == OF_KEY_ESC) {
            cancelValueEdit();
            controller.requestViewModelRefresh();
            return true;
        }
        if (key == OF_KEY_RETURN) {
            commitValueEdit();
            controller.requestViewModelRefresh();
            return true;
        }
        if (handleValueEditCharacter(key)) {
            controller.requestViewModelRefresh();
            return true;
        }
    }




    if (key == '[' || key == '{') {

        cancelValueEdit();

        moveTreeSelection(-1);

        controller.requestViewModelRefresh();

        return true;

    }



    if (key == ']' || key == '}') {

        cancelValueEdit();

        moveTreeSelection(1);

        controller.requestViewModelRefresh();

        return true;

    }



    if ((key == ',' || key == '.' || key == '<' || key == '>') && focusPane_ == FocusPane::kGrid) {

        if (selectedRow && (selectedColumn_ == Column::kMidi ||
                            selectedColumn_ == Column::kMidiMin ||
                            selectedColumn_ == Column::kMidiMax)) {
            constexpr float kCoarseStep = 10.0f;
            constexpr float kFineStep = 1.0f;
            float dMin = 0.0f;
            float dMax = 0.0f;
            if (key == ',') dMin = -kCoarseStep;
            if (key == '.') dMax = kCoarseStep;
            if (key == '<') dMin = -kFineStep;
            if (key == '>') dMax = kFineStep;
            if (adjustMidiRange(*selectedRow, dMin, dMax)) {
                controller.requestViewModelRefresh();
            }
        }
        return true;

    }



    if ((key == 'n' || key == 'N') && focusPane_ == FocusPane::kGrid && selectedRow && selectedColumn_ == Column::kOsc) {
        if (beginOscLearn(*selectedRow)) {
            controller.requestViewModelRefresh();
        }
        return true;
    }



    if (key == OF_KEY_TAB) {

        cancelValueEdit();
        cancelOscValueEdit();
        focusPane_ = (focusPane_ == FocusPane::kGrid) ? FocusPane::kTree : FocusPane::kGrid;

        controller.requestViewModelRefresh();

        return true;

    }



    if (focusPane_ == FocusPane::kTree) {

        switch (key) {

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

            focusPane_ = FocusPane::kGrid;

            controller.requestViewModelRefresh();

            return true;

        default:

            break;

        }

        return false;

    }



    switch (key) {

    case OF_KEY_UP:

        moveRow(-1);

        controller.requestViewModelRefresh();

        return true;

    case OF_KEY_DOWN:

        moveRow(1);

        controller.requestViewModelRefresh();

        return true;

    case OF_KEY_PAGE_UP: {
        const auto& rows = activeRowIndices();
        if (!rows.empty()) {
            int slot = activeRowSlot();
            if (slot < 0) slot = 0;
            setActiveRowSlot(slot - 10);
            controller.requestViewModelRefresh();
        }
        return true;
    }

    case OF_KEY_PAGE_DOWN: {
        const auto& rows = activeRowIndices();
        if (!rows.empty()) {
            int slot = activeRowSlot();
            if (slot < 0) slot = 0;
            setActiveRowSlot(slot + 10);
            controller.requestViewModelRefresh();
        }
        return true;
    }

    case OF_KEY_HOME:
        setActiveRowSlot(0);
        controller.requestViewModelRefresh();
        return true;

    case OF_KEY_END: {
        const auto& rows = activeRowIndices();
        if (!rows.empty()) {
            setActiveRowSlot(static_cast<int>(rows.size()) - 1);
            controller.requestViewModelRefresh();
        }
        return true;
    }

    case OF_KEY_LEFT:

        moveColumn(-1);

        controller.requestViewModelRefresh();

        return true;

    case OF_KEY_RIGHT:

        moveColumn(1);

        controller.requestViewModelRefresh();

        return true;

    case OF_KEY_RETURN:
    case ' ':
        if (oscSummaryActive() && focusPane_ == FocusPane::kGrid) {
            const auto sources = oscBrowserSources();
            std::string selectedAddress = selectedOscSourceAddress(sources);
            bool editableOscColumn =
                selectedColumn_ == Column::kSlot ||
                selectedColumn_ == Column::kMidi ||
                selectedColumn_ == Column::kMidiMin ||
                selectedColumn_ == Column::kMidiMax ||
                selectedColumn_ == Column::kOsc ||
                selectedColumn_ == Column::kOscDeadband;
            if (!selectedAddress.empty() && editableOscColumn) {
                if (oscValueEditActive_ && oscValueEditRowId_ == selectedAddress) {
                    commitOscSourceValueEdit(selectedAddress);
                } else {
                    beginOscSourceValueEdit(selectedAddress);
                }
                controller.requestViewModelRefresh();
                return true;
            }
        }
        if (focusPane_ == FocusPane::kGrid && selectedRow) {
            if (selectedColumn_ == Column::kValue) {
                if (isSavedSceneBrowserRow(*selectedRow)) {
                    handleSavedSceneRowActivation(*selectedRow);
                    controller.requestViewModelRefresh();
                    return true;
                }
                if (isHudWidgetRow(*selectedRow) && hudPlacementCallback_ && baseKey == OF_KEY_RETURN) {
                    if (hudColumnPickerVisible_) {
                        applyHudColumnSelection();
                    } else {
                        beginHudColumnPicker(*selectedRow);
                    }
                    controller.requestViewModelRefresh();
                    return true;
                }
                if (!rowSupportsValueEdit(*selectedRow)) {
                    setBannerMessage("No editable value for this row", 2200);
                    return true;
                }
                if (!selectedRow->isFloat) {
                    if (selectedRow->isString) {
                        if (editingValueActive_ && selectedRow->id == editingValueRowId_) {
                            commitValueEdit();
                        } else {
                            beginValueEdit(*selectedRow);
                        }
                        controller.requestViewModelRefresh();
                        return true;
                    } else {
                        bool current = false;
                        if (!readRowBoolValue(*selectedRow, current)) {
                            setBannerMessage("No toggleable value", 2200);
                            return true;
                        }
                        if (setRowBoolValue(*selectedRow, !current)) {
                            tableModel_.dirty = true;
                            invalidateRowCache();
                            emitTelemetryEvent("value.toggle", selectedRow, current ? "off" : "on");
                        }
                        controller.requestViewModelRefresh();
                        return true;
                    }
                } else {
                    if (editingValueActive_ && selectedRow->id == editingValueRowId_) {
                        commitValueEdit();
                    } else {
                        beginValueEdit(*selectedRow);
                    }
                    controller.requestViewModelRefresh();
                    return true;
                }
            } else if (selectedColumn_ == Column::kSlot) {
                if (!rowHasLiveParameter(*selectedRow)) {
                    setBannerMessage("Select an asset parameter before assigning slots", 2400);
                    return true;
                }
                if (beginSlotPicker(*selectedRow)) {
                    controller.requestViewModelRefresh();
                }
                return true;
            } else if (selectedColumn_ == Column::kMidi) {
                if (!rowHasLiveParameter(*selectedRow)) {
                    setBannerMessage("Select an asset parameter before routing MIDI", 2400);
                    return true;
                }
                if (beginMidiLearn(*selectedRow)) {
                    controller.requestViewModelRefresh();
                }
                return true;
            } else if (selectedColumn_ == Column::kOsc) {
                if (!rowHasLiveParameter(*selectedRow)) {
                    setBannerMessage("Select an asset parameter before routing OSC", 2400);
                    return true;
                }
                if (midiRouter_) {
                    const auto& sources = midiRouter_->getOscSources();
                    if (!sources.empty()) {
                        pickingOsc_ = true;
                        if (oscPickerSelection_ < 0 || oscPickerSelection_ >= static_cast<int>(sources.size())) {
                            oscPickerSelection_ = 0;
                        }
                        oscEditColumn_ = 0;
                        controller.requestViewModelRefresh();
                    } else if (beginOscLearn(*selectedRow)) {
                        controller.requestViewModelRefresh();
                    }
                } else {
                    beginOscLearn(*selectedRow);
                }
                return true;
            }
        }

        if (activateSelection(controller)) {
            controller.requestViewModelRefresh();
        }

        return true;

    case 'e':

    case 'E':

        if (focusPane_ == FocusPane::kGrid && selectedColumn_ == Column::kValue && selectedRow && selectedRow->isFloat) {
            if (!rowSupportsValueEdit(*selectedRow)) {
                setBannerMessage("No editable value for this row", 2400);
                return true;
            }
            beginValueEdit(*selectedRow);
            controller.requestViewModelRefresh();
            return true;
        }
        break;

    case 'u':

    case 'U':

        if (focusPane_ == FocusPane::kGrid && selectedRow) {
            if (!rowHasLiveParameter(*selectedRow)) {
                setBannerMessage("Select an asset parameter before unmapping", 2200);
                return true;
            }
            bool removed = false;
            if (selectedColumn_ == Column::kSlot) {
                bool logicalRemoved = removeLogicalSlotAssignment(selectedRow->id);
                bool midiRemoved = midiRouter_ ? midiRouter_->removeMidiMappingsForTarget(selectedRow->id) : false;
                removed = logicalRemoved || midiRemoved;
                if (removed) {
                    emitTelemetryEvent("midi.unmap", selectedRow);
                }
            } else if (selectedColumn_ == Column::kMidi && midiRouter_) {
                removed = midiRouter_->removeMidiMappingsForTarget(selectedRow->id);
                if (removed) emitTelemetryEvent("midi.unmap", selectedRow);
            } else if (selectedColumn_ == Column::kOsc && midiRouter_) {
                removed = midiRouter_->removeOscMappingsForTarget(selectedRow->id);
                pickingOsc_ = false;
                if (removed) emitTelemetryEvent("osc.unmap", selectedRow);
            }
            if (removed) {
                tableModel_.dirty = true;
                invalidateRowCache();
                setBannerMessage("Mappings removed");
                if (midiRouter_ && (selectedColumn_ == Column::kMidi || selectedColumn_ == Column::kOsc || selectedColumn_ == Column::kSlot)) {
                    persistRoutingChange();
                }
                controller.requestViewModelRefresh();
            } else {
                setBannerMessage("No mappings to remove", 1800);
            }
            return true;
        }
        break;
    case 'r':

    case 'R':

        if (rollbackOffered_) {
            if (routingRollbackAction_) {
                routingRollbackAction_();
                rollbackOffered_ = false;
                setBannerMessage("Routing changes reverted", 2600);
                controller.requestViewModelRefresh();
            } else {
                setBannerMessage("No rollback action available", 2200);
            }
            return true;
        }
        break;

    case 'p':

    case 'P':

        if (focusPane_ == FocusPane::kGrid) {
            routingPopoverVisible_ = !routingPopoverVisible_;
            controller.requestViewModelRefresh();
            return true;
        }
        break;

    default:

        break;

    }

    return false;

}

inline void ControlMappingHubState::onEnter(MenuController& controller) {
    controller_ = &controller;
    active_ = true;
    focusPane_ = FocusPane::kGrid;
    tableModel_.dirty = true;
    invalidateRowCache();
    clampSelection();
    controller.requestViewModelRefresh();
}

inline void ControlMappingHubState::onExit(MenuController& controller) {
    (void)controller;
    flushPreferences();
    controller_ = nullptr;
    active_ = false;
    cancelValueEdit();
    cancelHudColumnPicker();
    cancelSlotPicker();
    pickingOsc_ = false;
}

inline void ControlMappingHubState::rebuildView() const {

    rebuildModel();

    if (midiRouter_) {
        takeoverByTarget_.clear();
        for (const auto& state : midiRouter_->pendingTakeovers()) {
            takeoverByTarget_[state.targetId] = state;
        }
    } else {
        takeoverByTarget_.clear();
    }

    cachedView_ = MenuController::StateView{};



    if (oscSummaryActive()) {
        MenuController::EntryView entry;
        entry.id = "osc.summary";
        entry.label = "OSC Channels";
        entry.description = "Browse and edit live OSC sources";
        entry.selectable = true;
        entry.selected = true;
        cachedView_.entries.push_back(entry);
        cachedView_.selectedIndex = std::max(0, oscPickerSelection_);
    } else {
        const auto& activeRows = activeRowIndices();

        cachedView_.entries.reserve(activeRows.size());

        for (int rowIndex : activeRows) {

            const auto& row = tableModel_.rows[rowIndex];

            MenuController::EntryView entry;

            entry.id = row.id;

            entry.label = row.label;

            std::ostringstream desc;

            desc << row.category;

            if (!row.subcategory.empty()) {

                desc << " / " << row.subcategory;

            }

            desc << "  |  Value: " << formatValue(row);
            if (!row.consoleSlots.empty()) {
                desc << "  |  Console slot";
                if (row.consoleSlots.size() > 1) {
                    desc << "s";
                }
                desc << ": ";
                for (std::size_t i = 0; i < row.consoleSlots.size(); ++i) {
                    if (i > 0) {
                        desc << ",";
                    }
                    desc << row.consoleSlots[i];
                }
            }

            entry.description = desc.str();

            entry.selectable = true;

            entry.selected = (rowIndex == selectedRow_);

            cachedView_.entries.push_back(std::move(entry));

        }

        cachedView_.selectedIndex = activeRowSlot();
    }



    cachedView_.hotkeys.clear();

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_UP, "Up", "Previous row"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_DOWN, "Down", "Next row"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_PAGE_UP, "PgUp", "Page up"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_PAGE_DOWN, "PgDn", "Page down"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_HOME, "Home", "First row"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_END, "End", "Last row"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_LEFT, "Left", "Previous column"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_RIGHT, "Right", "Next column"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_TAB, "Tab", "Swap tree/grid focus"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_RETURN, "Enter", "Edit value / assign slot / arm learn"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{'N', "N", "OSC learn (next message)"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{'U', "U", "Unmap MIDI/OSC"});

    cachedView_.hotkeys.push_back(MenuController::KeyHint{',', ", . < >", "Adjust MIDI/OSC ranges"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{MenuController::HOTKEY_MOD_CTRL | '1', "Ctrl+1..8", "Load selected asset into a console slot"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | '1', "Ctrl+Shift+1..8", "Unload a console slot"});

}

inline void ControlMappingHubState::rebuildModel() const {
    if (!tableModel_.dirty) {
        return;
    }

    tableModel_.rows.clear();
    tableModel_.categories.clear();
    tableModel_.allRowIndices.clear();
    tableModel_.tree.clear();
    activeRowSet_ = nullptr;

    if (!registry_) {
        tableModel_.dirty = false;
        selectedRow_ = -1;
        return;
    }

    std::unordered_set<std::string> assetKeysPresent;
    assetKeysPresent.reserve(assetCatalog_.size());
    refreshConsoleSlotInventory();

    auto finalizeRow = [&](ParameterRow& row) {
        applyAssetMetadata(row);
        applySyntheticAssetMetadata(row);
        if (row.isAsset && !row.assetKey.empty()) {
            assetKeysPresent.insert(row.assetKey);
        }
        applyConsoleSlotAnnotations(row);
        return true;
    };

    auto appendFloatRow = [&](const ParameterRegistry::FloatParam& param) {
        ParameterRow row;
        row.id = param.meta.id;
        row.label = param.meta.label.empty() ? param.meta.id : param.meta.label;
        row.category = param.meta.group.empty() ? std::string("General") : param.meta.group;
        row.subcategory = subcategoryForId(row.id);
        row.isFloat = true;
        row.floatParam = &param;
        if (!finalizeRow(row)) {
            return;
        }
        tableModel_.rows.push_back(std::move(row));
    };

    auto appendBoolRow = [&](const ParameterRegistry::BoolParam& param) {
        ParameterRow row;
        row.id = param.meta.id;
        row.label = param.meta.label.empty() ? param.meta.id : param.meta.label;
        row.category = param.meta.group.empty() ? std::string("General") : param.meta.group;
        row.subcategory = subcategoryForId(row.id);
        row.isFloat = false;
        row.boolParam = &param;
        if (!finalizeRow(row)) {
            return;
        }
        tableModel_.rows.push_back(std::move(row));
    };

    auto appendStringRow = [&](const ParameterRegistry::StringParam& param) {
        ParameterRow row;
        row.id = param.meta.id;
        row.label = param.meta.label.empty() ? param.meta.id : param.meta.label;
        row.category = param.meta.group.empty() ? std::string("General") : param.meta.group;
        row.subcategory = subcategoryForId(row.id);
        row.isFloat = false;
        row.isString = true;
        row.stringParam = &param;
        if (!finalizeRow(row)) {
            return;
        }
        tableModel_.rows.push_back(std::move(row));
    };

    for (const auto& fp : registry_->floats()) {
        appendFloatRow(fp);
    }
    for (const auto& bp : registry_->bools()) {
        appendBoolRow(bp);
    }
    for (const auto& sp : registry_->strings()) {
        appendStringRow(sp);
    }

    appendSceneBrowserRows();
    appendBioAmpRows();
    appendAssetPlaceholders(assetKeysPresent);

    std::sort(tableModel_.rows.begin(), tableModel_.rows.end(), [](const ParameterRow& a, const ParameterRow& b) {
        if (a.category != b.category) return a.category < b.category;
        if (a.subcategory != b.subcategory) return a.subcategory < b.subcategory;
        if (a.assetLabel != b.assetLabel) return a.assetLabel < b.assetLabel;
        return a.label < b.label;
    });

    tableModel_.allRowIndices.reserve(tableModel_.rows.size());
    for (std::size_t i = 0; i < tableModel_.rows.size(); ++i) {
        tableModel_.allRowIndices.push_back(static_cast<int>(i));
    }

    std::map<std::string, std::map<std::string, std::vector<int>>> sectionMap;
    std::map<std::string, std::map<std::string, std::map<std::string, std::vector<int>>>> assetSectionMap;
    for (std::size_t i = 0; i < tableModel_.rows.size(); ++i) {
        const auto& row = tableModel_.rows[i];
        sectionMap[row.category][row.subcategory].push_back(static_cast<int>(i));
        if (row.isAsset && !row.assetLabel.empty()) {
            assetSectionMap[row.category][row.subcategory][row.assetLabel].push_back(static_cast<int>(i));
        }
    }

    for (const auto& categoryPair : sectionMap) {
        CategorySection category;
        category.name = categoryPair.first;
        for (const auto& subPair : categoryPair.second) {
            CategorySection::Subcategory sub;
            sub.name = subPair.first;
            sub.rowIndices = subPair.second;
            auto categoryAssetIt = assetSectionMap.find(category.name);
            if (categoryAssetIt != assetSectionMap.end()) {
                auto subAssetIt = categoryAssetIt->second.find(sub.name);
                if (subAssetIt != categoryAssetIt->second.end()) {
                    for (const auto& assetPair : subAssetIt->second) {
                        if (assetPair.first.empty()) {
                            continue;
                        }
                        CategorySection::Subcategory::AssetGroup assetGroup;
                        assetGroup.name = assetPair.first;
                        const int firstIndex = assetPair.second.empty() ? -1 : assetPair.second.front();
                        if (firstIndex >= 0 && firstIndex < static_cast<int>(tableModel_.rows.size())) {
                            assetGroup.assetKey = tableModel_.rows[static_cast<std::size_t>(firstIndex)].assetKey;
                        }
                        assetGroup.rowIndices = assetPair.second;
                        sub.assetGroups.push_back(std::move(assetGroup));
                    }
                }
            }
            category.rowIndices.insert(category.rowIndices.end(), sub.rowIndices.begin(), sub.rowIndices.end());
            category.subcategories.push_back(std::move(sub));
        }
        tableModel_.categories.push_back(std::move(category));
    }

    std::unordered_set<std::string> categoryNames;
    categoryNames.reserve(tableModel_.categories.size() * 2);
    for (const auto& cat : tableModel_.categories) {
        categoryNames.insert(cat.name);
        for (const auto& sub : cat.subcategories) {
            if (!sub.assetGroups.empty()) {
                categoryNames.insert(subcategoryExpansionKey(cat.name, sub.name));
            }
        }
    }
    for (auto it = treeExpansionState_.begin(); it != treeExpansionState_.end();) {
        if (categoryNames.find(it->first) == categoryNames.end()) {
            it = treeExpansionState_.erase(it);
        } else {
            ++it;
        }
    }

    tableModel_.tree.clear();
    for (std::size_t i = 0; i < tableModel_.categories.size(); ++i) {
        const auto& category = tableModel_.categories[i];
        TreeNode catNode;
        catNode.label = category.name.empty() ? std::string("(Ungrouped)") : category.name;
        catNode.categoryName = category.name;
        catNode.categoryIndex = static_cast<int>(i);
        catNode.depth = 0;
        catNode.parentIndex = -1;
        catNode.expandable = !category.subcategories.empty();
        bool expanded = catNode.expandable ? isCategoryExpanded(category.name) : false;
        catNode.expanded = expanded;
        treeExpansionState_[category.name] = expanded;
        tableModel_.tree.push_back(catNode);
        int catNodeIndex = static_cast<int>(tableModel_.tree.size() - 1);
        if (!catNode.expandable || !catNode.expanded) {
            continue;
        }
        for (std::size_t j = 0; j < category.subcategories.size(); ++j) {
            TreeNode subNode;
            subNode.label = category.subcategories[j].name.empty() ? std::string("(General)") : category.subcategories[j].name;
            subNode.categoryName = category.name;
            subNode.subcategoryName = category.subcategories[j].name;
            subNode.categoryIndex = static_cast<int>(i);
            subNode.subcategoryIndex = static_cast<int>(j);
            subNode.parentIndex = catNodeIndex;
            subNode.depth = 1;
            bool hasDistinctAssetChildren = category.subcategories[j].assetGroups.size() > 1;
            if (!hasDistinctAssetChildren && category.subcategories[j].assetGroups.size() == 1) {
                hasDistinctAssetChildren = category.subcategories[j].assetGroups.front().name != category.subcategories[j].name;
            }
            subNode.expandable = hasDistinctAssetChildren;
            subNode.expanded = hasDistinctAssetChildren
                ? isCategoryExpanded(subcategoryExpansionKey(category.name, category.subcategories[j].name))
                : false;
            tableModel_.tree.push_back(subNode);
            int subNodeIndex = static_cast<int>(tableModel_.tree.size() - 1);
            if (!subNode.expandable || !subNode.expanded) {
                continue;
            }
            for (std::size_t k = 0; k < category.subcategories[j].assetGroups.size(); ++k) {
                TreeNode assetNode;
                assetNode.label = category.subcategories[j].assetGroups[k].name.empty()
                    ? std::string("(Asset)")
                    : category.subcategories[j].assetGroups[k].name;
                assetNode.categoryName = category.name;
                assetNode.subcategoryName = category.subcategories[j].name;
                assetNode.assetGroupName = category.subcategories[j].assetGroups[k].name;
                assetNode.categoryIndex = static_cast<int>(i);
                assetNode.subcategoryIndex = static_cast<int>(j);
                assetNode.assetGroupIndex = static_cast<int>(k);
                assetNode.parentIndex = subNodeIndex;
                assetNode.depth = 2;
                assetNode.expandable = false;
                assetNode.expanded = false;
                tableModel_.tree.push_back(std::move(assetNode));
            }
        }
    }

    TreeNode oscNode;
    oscNode.label = "OSC Channels";
    oscNode.depth = 0;
    oscNode.parentIndex = -1;
    oscNode.expandable = false;
    oscNode.expanded = false;
    oscNode.oscSummary = true;
    tableModel_.tree.push_back(std::move(oscNode));

    auto firstLeafIndex = [&]() -> int {
        int fallback = -1;
        for (std::size_t i = 0; i < tableModel_.tree.size(); ++i) {
            const auto& node = tableModel_.tree[i];
            bool isLeaf = node.categoryIndex >= 0 && !node.expandable &&
                (node.subcategoryIndex >= 0 || node.assetGroupIndex >= 0);
            if (isLeaf) {
                if (fallback < 0) {
                    fallback = static_cast<int>(i);
                }
                if (node.categoryName != "Sensors") {
                    return static_cast<int>(i);
                }
            }
        }
        if (fallback >= 0) {
            return fallback;
        }
        if (tableModel_.tree.empty()) {
            return -1;
        }
        return 0;
    };

    if (treeSelectionPending_) {
        bool applied = false;
        if (pendingCategoryPref_.empty() && pendingSubcategoryPref_.empty() && pendingAssetPref_.empty()) {
            int leaf = firstLeafIndex();
            if (leaf >= 0) {
                selectedTreeNodeIndex_ = leaf;
                applied = true;
            }
        } else {
            for (std::size_t i = 0; i < tableModel_.tree.size(); ++i) {
                const auto& node = tableModel_.tree[i];
                if (node.categoryIndex < 0 || node.categoryIndex >= static_cast<int>(tableModel_.categories.size())) {
                    continue;
                }
                const auto& category = tableModel_.categories[node.categoryIndex];
                std::string categoryName = category.name;
                std::string subcategoryName;
                if (node.subcategoryIndex >= 0 &&
                    node.subcategoryIndex < static_cast<int>(category.subcategories.size())) {
                    subcategoryName = category.subcategories[node.subcategoryIndex].name;
                }
                if (categoryName == pendingCategoryPref_) {
                    if (pendingSubcategoryPref_.empty() && node.subcategoryIndex < 0) {
                        selectedTreeNodeIndex_ = static_cast<int>(i);
                        applied = true;
                        break;
                    }
                    if (!pendingSubcategoryPref_.empty() && subcategoryName == pendingSubcategoryPref_) {
                        if (!pendingAssetPref_.empty()) {
                            if (node.assetGroupName == pendingAssetPref_) {
                                selectedTreeNodeIndex_ = static_cast<int>(i);
                                applied = true;
                                break;
                            }
                            continue;
                        }
                        if (node.expandable) {
                            continue;
                        }
                        if (node.assetGroupIndex >= 0) {
                            selectedTreeNodeIndex_ = static_cast<int>(i);
                            applied = true;
                            break;
                        }
                        selectedTreeNodeIndex_ = static_cast<int>(i);
                        applied = true;
                        break;
                    }
                }
            }
        }
        if (!applied) {
            int fallback = firstLeafIndex();
            if (fallback >= 0) {
                selectedTreeNodeIndex_ = fallback;
            } else {
                selectedTreeNodeIndex_ = tableModel_.tree.empty() ? -1 : 0;
            }
        }
        pendingCategoryPref_.clear();
        pendingSubcategoryPref_.clear();
        pendingAssetPref_.clear();
        treeSelectionPending_ = false;
    } else {
        if (tableModel_.tree.empty()) {
            selectedTreeNodeIndex_ = -1;
        } else if (selectedTreeNodeIndex_ < 0 ||
                   selectedTreeNodeIndex_ >= static_cast<int>(tableModel_.tree.size())) {
            int fallback = firstLeafIndex();
            selectedTreeNodeIndex_ = fallback >= 0 ? fallback : 0;
        }
    }

    if (tableModel_.tree.empty()) {
        treeScrollOffset_ = 0;
    } else {
        int maxOffset = std::max(0, static_cast<int>(tableModel_.tree.size()) - 1);
        treeScrollOffset_ = std::clamp(treeScrollOffset_, 0, maxOffset);
    }

    refreshValueEditTarget();
    enforceVisibleColumnSelection();
    clampSelection();
    if (slotMidiAssignmentsDirty_) {
        slotMidiAssignmentsDirty_ = false;
        rebuildSlotMidiAssignmentsFromCurrentModel();
    }
    tableModel_.dirty = false;
}

inline void ControlMappingHubState::clampSelection() const {
    if (oscSummaryActive()) {
        const auto sources = oscBrowserSources();
        if (sources.empty()) {
            oscPickerSelection_ = -1;
            cancelOscValueEdit();
            return;
        }
        if (oscPickerSelection_ < 0 || oscPickerSelection_ >= static_cast<int>(sources.size())) {
            oscPickerSelection_ = 0;
        }
        if (oscValueEditActive_ && oscPickerSelection_ >= 0 &&
            oscPickerSelection_ < static_cast<int>(sources.size()) &&
            sources[oscPickerSelection_].address != oscValueEditRowId_) {
            cancelOscValueEdit();
        }
        return;
    }
    const auto& rows = activeRowIndices();
    if (rows.empty()) {
        selectedRow_ = -1;
        cancelValueEdit();
        return;
    }
    if (selectedRow_ < 0) {
        selectedRow_ = rows.front();
    }
    if (std::find(rows.begin(), rows.end(), selectedRow_) == rows.end()) {
        selectedRow_ = rows.front();
    }
    if (editingValueActive_) {
        const auto& row = tableModel_.rows[selectedRow_];
        if (row.id != editingValueRowId_) {
            cancelValueEdit();
        }
    }
}

inline std::vector<std::string> ControlMappingHubState::columnHeaders() const {
    if (oscSummaryActive()) {
        return {
            "Source Address",
            "Live Value",
            "Input Min",
            "Input Max",
            "Output Min x",
            "Output Max x",
            "Smooth",
            "Deadband"
        };
    }
    return {
        "Parameter",
        "Value",
        "Slot",
        "MIDI",
        "MIDI Min",
        "MIDI Max",
        "OSC",
        ""
    };
}

inline std::string ControlMappingHubState::formatValue(const ParameterRow& row) const {
    uint64_t nowMs = ofGetElapsedTimeMillis();
    std::string value;
    if (row.isHudLayoutEntry) {
        return "Open layout editor";
    }
    if (row.isSavedSceneSaveRow) {
        return savedSceneDraftName_.empty() ? std::string("new-scene") : savedSceneDraftName_;
    }
    if (row.isSavedSceneOverwriteRow) {
        return row.savedScenePath.empty() ? std::string("Overwrite") : "Overwrite  |  " + row.savedScenePath;
    }
    if (row.isSavedSceneRow) {
        if (!row.savedScenePath.empty()) {
            return "Load  |  " + row.savedScenePath;
        }
        if (row.savedSceneId.empty()) {
            return "Save a scene to populate this list";
        }
        return "Load";
    }
    if (isBioAmpRowId(row.id)) {
        float sampleValue = 0.0f;
        uint64_t sampleTs = 0;
        if (readBioAmpValue(row.id, sampleValue, sampleTs)) {
            int precision = (row.id == "sensors.bioamp.sample_rate" || row.id == "sensors.bioamp.window") ? 0 : 3;
            value = ofToString(sampleValue, precision);
            lastValueSnapshot_[row.id] = sampleValue;
            lastActivityMs_[row.id] = sampleTs > 0 ? sampleTs : nowMs;
        } else {
            value = "No data";
        }
        return value;
    }
    if (row.isFloat && row.floatParam && row.floatParam->value) {
        float current = *row.floatParam->value;
        value = ofToString(current, 2);
        auto snapshot = lastValueSnapshot_.find(row.id);
        if (snapshot == lastValueSnapshot_.end() || std::fabs(snapshot->second - current) > 1e-3f) {
            lastValueSnapshot_[row.id] = current;
            lastActivityMs_[row.id] = nowMs;
        }
    } else if (row.isString && row.stringParam && row.stringParam->value) {
        const std::string& current = *row.stringParam->value;
        value = current.empty() ? "(empty)" : current;
        auto sit = lastStringSnapshot_.find(row.id);
        if (sit == lastStringSnapshot_.end() || sit->second != current) {
            lastStringSnapshot_[row.id] = current;
            lastActivityMs_[row.id] = nowMs;
        }
    } else if (!row.isFloat && row.boolParam && row.boolParam->value) {
        bool current = *row.boolParam->value;
        value = current ? "On" : "Off";
        auto snapshot = lastValueSnapshot_.find(row.id);
        float numeric = current ? 1.0f : 0.0f;
        if (snapshot == lastValueSnapshot_.end() || snapshot->second != numeric) {
            lastValueSnapshot_[row.id] = numeric;
            lastActivityMs_[row.id] = nowMs;
        }
    } else {
        value = "-";
    }
    bool isHudAsset = row.isAsset && row.assetKey.rfind("hud.", 0) == 0;
    if (isHudAsset && hudPlacementProvider_) {
        std::string placement = hudPlacementSummary(row.assetKey);
        if (!placement.empty()) {
            if (value == "-" || value.empty()) {
                value = placement;
            } else {
                value += "  |  " + placement;
            }
        }
    }
    if (isHudAsset) {
        std::string feed = hudFeedSummary(row.assetKey);
        if (!feed.empty()) {
            if (value == "-" || value.empty()) {
                value = feed;
            } else {
                value += "  |  " + feed;
            }
        }
    }
    value += activityHint(row);
    return value;
}

inline std::string ControlMappingHubState::hudPlacementSummary(const std::string& assetKey) const {
    if (!hudPlacementProvider_) {
        return std::string();
    }
    const auto placements = hudPlacementProvider_();
    for (const auto& placement : placements) {
        if (placement.id != assetKey) {
            continue;
        }
        std::string bandLabel = placement.bandLabel.empty() ? std::string("HUD") : placement.bandLabel;
        std::string columnLabel = placement.columnLabel;
        if (columnLabel.empty() && placement.columnIndex >= 0) {
            columnLabel = hudColumnLabel(placement.columnIndex);
        }
        if (columnLabel.empty()) {
            columnLabel = std::string("Column 1");
        }
        std::string summary = "Band: " + bandLabel + "  Column: " + columnLabel;
        if (!placement.target.empty()) {
            std::string routeLabel = placement.target;
            if (!routeLabel.empty()) {
                routeLabel[0] = static_cast<char>(std::toupper(routeLabel[0]));
            }
            summary += "  Route: " + routeLabel;
        }
        return summary;
    }
    return std::string();
}

inline std::string ControlMappingHubState::hudColumnLabel(int columnIndex) const {
    int clamped = ofClamp(columnIndex, 0, kHudColumnCount - 1);
    return "Column " + ofToString(clamped + 1);
}

inline std::string ControlMappingHubState::hudFeedSummary(const std::string& assetKey) const {
    if (!hudFeedRegistry_) {
        return std::string();
    }
    auto entry = hudFeedRegistry_->latest(assetKey);
    if (!entry) {
        return std::string();
    }
    const auto& payload = entry->payload;
    if (assetKey == "hud.status") {
        return summarizeHudStatusFeed(payload);
    }
    if (assetKey == "hud.sensors") {
        return summarizeHudSensorsFeed(payload);
    }
    if (assetKey == "hud.layers") {
        return summarizeHudLayersFeed(payload);
    }
    return std::string();
}

inline std::string ControlMappingHubState::summarizeHudStatusFeed(const ofJson& payload) const {
    std::vector<std::string> parts;
    if (payload.contains("slots") && payload["slots"].is_object()) {
        const auto& slots = payload["slots"];
        int active = slots.value("active", 0);
        int assigned = slots.value("assigned", 0);
        int capacity = std::max(slots.value("capacity", 0), 1);
        parts.push_back("Slots " + ofToString(active) + "/" + ofToString(capacity) + " (" + ofToString(assigned) + " assigned)");
    }
    if (payload.contains("connections") && payload["connections"].is_object()) {
        const auto& connections = payload["connections"];
        if (connections.contains("midi")) {
            const auto& midi = connections["midi"];
            std::string line = "MIDI: " + std::string(midi.value("connected", false) ? "connected" : "offline");
            std::string label = midi.value("label", std::string());
            if (!label.empty()) {
                line += " (" + label + ")";
            }
            parts.push_back(line);
        }
        if (connections.contains("collector")) {
            const auto& collector = connections["collector"];
            std::string line = "Sensor bus: " + std::string(collector.value("connected", false) ? "connected" : "searching");
            std::string label = collector.value("label", std::string());
            if (!label.empty()) {
                line += " (" + label + ")";
            }
            parts.push_back(line);
        }
    }
    if (payload.contains("controller") && payload["controller"].is_object()) {
        const auto& controller = payload["controller"];
        bool enabled = controller.value("enabled", false);
        bool active = controller.value("active", false);
        bool follow = controller.value("follow", true);
        std::string focus = controller.value("focusOwner", std::string("console"));
        bool needsAttention = controller.value("needsAttention", false);
        std::ostringstream line;
        line << "Controller: ";
        if (!enabled) {
            line << "disabled";
        } else if (!active) {
            line << "spawning";
        } else {
            line << (follow ? "follow" : "freeform");
        }
        line << " | focus " << (focus.empty() ? "console" : focus);
        if (needsAttention) {
            line << "  ATTENTION";
        }
        parts.push_back(line.str());
    }
    if (payload.contains("activeBank")) {
        parts.push_back("Bank: " + payload["activeBank"].get<std::string>());
    }
    if (payload.contains("takeovers") && payload["takeovers"].is_array()) {
        std::size_t pending = payload["takeovers"].size();
        if (pending > 0) {
            parts.push_back("Takeovers: " + ofToString(static_cast<int>(pending)));
        }
    }
    if (payload.contains("oscSources") && payload["oscSources"].is_array()) {
        parts.push_back("OSC sources: " + ofToString(static_cast<int>(payload["oscSources"].size())));
    }
    if (parts.empty()) {
        return std::string();
    }
    return ofJoinString(parts, "  |  ");
}

inline std::string ControlMappingHubState::summarizeHudSensorsFeed(const ofJson& payload) const {
    auto indicatorLabel = [](const ofJson& node) -> std::string {
        if (node.contains("indicator")) {
            return node["indicator"].get<std::string>();
        }
        return "[ ]";
    };
    std::vector<std::string> parts;
    if (payload.contains("deck") && payload["deck"].is_object()) {
        parts.push_back("Deck " + indicatorLabel(payload["deck"]));
    }
    if (payload.contains("matrix") && payload["matrix"].is_object()) {
        parts.push_back("Matrix " + indicatorLabel(payload["matrix"]));
    }
    if (payload.contains("oscHistory") && payload["oscHistory"].is_array()) {
        parts.push_back("Recent OSC: " + ofToString(static_cast<int>(payload["oscHistory"].size())));
    }
    if (parts.empty()) {
        return std::string();
    }
    return ofJoinString(parts, "  |  ");
}

inline std::string ControlMappingHubState::summarizeHudLayersFeed(const ofJson& payload) const {
    std::vector<std::string> parts;
    if (payload.contains("summary") && payload["summary"].is_object()) {
        const auto& summary = payload["summary"];
        if (summary.contains("grid")) {
            const auto& grid = summary["grid"];
            std::string gridSummary = "Grid seg " + ofToString(grid.value("segments", 0));
            const std::string modes = grid.value("deformationSummary", std::string("flat"));
            if (modes != "flat") {
                gridSummary += " " + modes;
            }
            parts.push_back(std::move(gridSummary));
        }
        if (summary.contains("geodesic")) {
            const auto& sphere = summary["geodesic"];
            std::string sphereSummary = "Sphere r=" + ofToString(sphere.value("radius", 0.0f), 1);
            if (sphere.value("deform", false)) {
                sphereSummary += " deform";
            }
            parts.push_back(std::move(sphereSummary));
        }
    }
    if (payload.contains("slots") && payload["slots"].is_array()) {
        int active = 0;
        std::vector<std::string> labels;
        for (const auto& slot : payload["slots"]) {
            if (!slot.is_object()) continue;
            if (slot.value("active", false)) {
                ++active;
                std::string label = slot.value("label", std::string());
                if (!label.empty()) {
                    labels.push_back(label);
                }
            }
        }
        if (!labels.empty()) {
            std::string joined;
            std::size_t limit = std::min<std::size_t>(labels.size(), 3);
            for (std::size_t i = 0; i < limit; ++i) {
                if (i > 0) {
                    joined += ", ";
                }
                joined += labels[i];
            }
            if (labels.size() > limit) {
                joined += ", ...";
            }
            parts.push_back("Active slots: " + ofToString(active) + " (" + joined + ")");
        } else if (active > 0) {
            parts.push_back("Active slots: " + ofToString(active));
        }
    }
    if (parts.empty()) {
        return std::string();
    }
    return ofJoinString(parts, "  |  ");
}

inline void ControlMappingHubState::emitHudFeedUpdated(const HudFeedRegistry::FeedEntry& entry) const {
    if (!eventCallback_) {
        return;
    }
    ofJson j;
    j["type"] = "hud.feed.updated";
    j["widgetId"] = entry.widgetId;
    j["timestampMs"] = entry.timestampMs;
    j["payload"] = entry.payload;
    try {
        eventCallback_(j.dump());
    } catch (...) {
        ofLogWarning("ControlMappingHubState") << "Event callback threw while publishing HUD feed update";
    }
}

inline void ControlMappingHubState::emitHudMappingEvent(const std::string& reason,
                                                        const std::string& widgetId,
                                                        const std::string& detail) const {
    if (!eventCallback_) {
        return;
    }
    ofJson j;
    j["type"] = "hud.mapping.changed";
    j["reason"] = reason;
    if (!widgetId.empty()) {
        j["widgetId"] = widgetId;
    }
    if (!detail.empty()) {
        j["detail"] = detail;
    }
    j["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    try {
        eventCallback_(j.dump());
    } catch (...) {
        ofLogWarning("ControlMappingHubState") << "Event callback threw while publishing HUD mapping event";
    }
}

inline bool ControlMappingHubState::rowHasLiveParameter(const ParameterRow& row) const {
    if (row.isHudLayoutEntry) {
        return false;
    }
    if (isBioAmpRowId(row.id)) {
        return true;
    }
    if (row.isFloat && row.floatParam) {
        return true;
    }
    if (row.boolParam) {
        return true;
    }
    if (row.isString && row.stringParam) {
        return true;
    }
    return false;
}

inline bool ControlMappingHubState::rowSupportsValueEdit(const ParameterRow& row) const {
    if (row.isSavedSceneSaveRow) {
        return true;
    }
    if (row.isHudLayoutEntry) {
        return false;
    }
    if (isBioAmpRowId(row.id)) {
        return false;
    }
    if (row.isFloat && row.floatParam) {
        return true;
    }
    if (row.isString && row.stringParam) {
        return true;
    }
    if (row.boolParam) {
        return true;
    }
    return false;
}

inline bool ControlMappingHubState::isHudWidgetRow(const ParameterRow& row) const {
    return row.isAsset && row.assetKey.rfind("hud.", 0) == 0;
}

inline bool ControlMappingHubState::oscSummaryActive() const {
    if (selectedTreeNodeIndex_ < 0 || selectedTreeNodeIndex_ >= static_cast<int>(tableModel_.tree.size())) {
        return false;
    }
    return tableModel_.tree[selectedTreeNodeIndex_].oscSummary;
}

inline std::vector<MidiRouter::OscSourceInfo> ControlMappingHubState::oscBrowserSources() const {
    std::vector<MidiRouter::OscSourceInfo> sources;
    if (!midiRouter_) {
        return sources;
    }
    sources = midiRouter_->getOscSources();
    std::unordered_set<std::string> seen;
    seen.reserve(sources.size());
    for (const auto& src : sources) {
        if (!src.address.empty()) {
            seen.insert(src.address);
        }
    }
    for (const auto& profile : midiRouter_->getOscSourceProfiles()) {
        if (profile.pattern.empty() || seen.count(profile.pattern) > 0) {
            continue;
        }
        MidiRouter::OscSourceInfo info;
        info.address = profile.pattern;
        sources.push_back(info);
        seen.insert(profile.pattern);
    }
    return sources;
}

inline std::string ControlMappingHubState::selectedOscSourceAddress(const std::vector<MidiRouter::OscSourceInfo>& sources) const {
    if (oscPickerSelection_ < 0 || oscPickerSelection_ >= static_cast<int>(sources.size())) {
        return std::string();
    }
    return sources[oscPickerSelection_].address;
}

inline std::string ControlMappingHubState::formatMidiSummary(const ParameterRow& row) const {
    if (row.isHudLayoutEntry) {
        return "-";
    }
    if (!midiRouter_) {
        return midiAvailable_ ? midiStatus_ : "Unavailable";
    }

    const MidiRouter::CcMap* cc = firstCcMapForRow(row);
    if (cc) {
        std::ostringstream oss;
        oss << "CC";
        if (cc->cc >= 0) {
            oss << cc->cc;
        } else {
            oss << "?";
        }
        if (!cc->bankId.empty()) {
            oss << "@" << cc->bankId;
        }
        std::string summary = oss.str();
        if (parameterHasModifierConflict(row, modifier::Type::kMidiCc) || parameterHasModifierConflict(row, modifier::Type::kMidiNote)) {
            summary += " !";
        }
        summary += midiTakeoverBadge(row);
        return summary;
    }

    for (const auto& map : midiRouter_->getBtnMaps()) {
        if (map.target == row.id) {
            std::ostringstream oss;
            oss << "BTN";
            if (map.num >= 0) {
                oss << map.num;
            } else {
                oss << "?";
            }
            if (!map.bankId.empty()) {
                oss << "@" << map.bankId;
            }
            std::string summary = oss.str();
            if (parameterHasModifierConflict(row, modifier::Type::kMidiNote)) {
                summary += " !";
            }
            summary += midiTakeoverBadge(row);
            return summary;
        }
    }

    return "Unmapped";
}

inline std::string ControlMappingHubState::formatMidiMin(const ParameterRow& row) const {
    if (row.isHudLayoutEntry) {
        return "-";
    }
    if (const auto* map = firstCcMapForRow(row)) {
        return ofToString(map->outMin, 2);
    }
    return "-";
}

inline std::string ControlMappingHubState::formatMidiMax(const ParameterRow& row) const {
    if (row.isHudLayoutEntry) {
        return "-";
    }
    if (const auto* map = firstCcMapForRow(row)) {
        return ofToString(map->outMax, 2);
    }
    return "-";
}

inline std::string ControlMappingHubState::formatOscSummary(const ParameterRow& row) const {
    if (row.isHudLayoutEntry) {
        return "-";
    }
    if (!midiRouter_) {
        return oscAvailable_ ? oscStatus_ : "Unavailable";
    }

    std::vector<std::string> matches;
    matches.reserve(2);

    for (const auto& map : midiRouter_->getOscMaps()) {
        if (map.target != row.id) {
            continue;
        }
        std::ostringstream oss;
        oss << map.pattern;
        if (!map.bankId.empty()) {
            oss << "@" << map.bankId;
        }
        if (!map.controlId.empty() && map.controlId != row.id) {
            oss << ":" << map.controlId;
        }
        matches.push_back(oss.str());
    }

    if (matches.empty()) {
        return "Unmapped";
    }

    std::string summary = matches.front();
    if (matches.size() > 1) {
        summary += " +" + ofToString(static_cast<int>(matches.size() - 1));
    }
    const std::string badge = modifierActivityBadge(row, modifier::Type::kOsc);
    if (!badge.empty()) {
        summary = badge.substr(1) + " " + summary;
    }
    if (parameterHasModifierConflict(row, modifier::Type::kOsc)) {
        summary += " !";
    }
    return summary;
}

inline std::string ControlMappingHubState::ellipsize(const std::string& text, float maxWidth) const {
    if (maxWidth <= 0.0f) {
        return text;
    }
    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    static ofBitmapFont bitmapFont;
    auto bbox = bitmapFont.getBoundingBox(text, 0.0f, 0.0f);
    if (bbox.getWidth() * textScale <= maxWidth) {
        return text;
    }
    std::string trimmed = text;
    while (!trimmed.empty()) {
        trimmed.pop_back();
        std::string candidate = trimmed + "...";
        bbox = bitmapFont.getBoundingBox(candidate, 0.0f, 0.0f);
        if (bbox.getWidth() * textScale <= maxWidth) {
            return candidate;
        }
    }
    return std::string("...");
}


inline void ControlMappingHubState::drawSlotPickerPanel(float panelX,
                                                        float panelY,
                                                        float panelWidth,
                                                        float panelHeight,
                                                        const ParameterRow* row) const {
    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    auto drawTextStyled = [textScale](const std::string& text, float x, float y, const ofColor& color, bool bold) {
        ofSetColor(color);
        drawBitmapStringScaled(text, x, y, textScale, bold);
    };

    ofPushStyle();
    ofSetColor(skin_.palette.surface);
    ofDrawRectRounded(panelX, panelY, panelWidth, panelHeight, skin_.metrics.borderRadius);

    std::string title = row ? ("Assign slot for " + row->label) : std::string("Assign slot");
    drawTextStyled(title, panelX + 12.0f, panelY + 20.0f, skin_.palette.headerText, true);

    float startX = panelX + 12.0f;
    float rowY = panelY + 44.0f * textScale;
    float lineHeight = 18.0f * textScale;

    if (slotPickerIndices_.empty()) {
        drawTextStyled("No controller slots found", startX, rowY + 4.0f, skin_.palette.mutedText, false);
    } else {
        int total = static_cast<int>(slotPickerIndices_.size());
        int visibleRows = std::min(total, kSlotPickerVisibleRows);
        slotPickerScrollOffset_ = std::max(0, std::min(slotPickerScrollOffset_, total - visibleRows));
        int endIndex = std::min(total, slotPickerScrollOffset_ + visibleRows);
        std::string currentDeviceId;
        for (int i = slotPickerScrollOffset_; i < endIndex; ++i) {
            int slotIndex = slotPickerIndices_[i];
            if (slotIndex < 0 || slotIndex >= static_cast<int>(slotCatalog_.size())) {
                continue;
            }
            const auto& slot = slotCatalog_[static_cast<std::size_t>(slotIndex)];
            if (slot.deviceId != currentDeviceId) {
                currentDeviceId = slot.deviceId;
                std::string deviceLabel = slot.deviceName.empty() ? slot.deviceId : slot.deviceName;
                drawTextStyled(deviceLabel, startX, rowY, skin_.palette.bodyText, true);
                rowY += lineHeight;
            }
            bool selected = (i == slotPickerSelection_);
            ofColor textColor = selected ? skin_.palette.gridSelection : skin_.palette.bodyText;
            std::string slotLabel = slot.label.empty() ? slot.slotId : slot.label;
            if (slotLabel.empty()) {
                slotLabel = slot.slotId.empty() ? std::string("Unnamed Slot") : slot.slotId;
            }
            drawTextStyled(slotLabel, startX + 12.0f, rowY, textColor, selected);
            if (!slot.bindings.empty()) {
                const auto& binding = slot.bindings.front();
                std::string bindingHint;
                if (!binding.bindingType.empty()) {
                    bindingHint = ofToUpper(binding.bindingType) + " ";
                }
                bindingHint += ofToString(binding.number);
                if (slot.bindings.size() > 1) {
                    bindingHint += "  (" + ofToString(static_cast<int>(slot.bindings.size())) + " columns)";
                }
                drawTextStyled(bindingHint, startX + 220.0f, rowY, textColor, false);
            }
            rowY += lineHeight;
        }
        if (endIndex < total) {
            drawTextStyled("...", startX, rowY, skin_.palette.mutedText, false);
            rowY += lineHeight;
        }
    }

    drawTextStyled("Enter: assign slot    Esc: cancel",
                   startX,
                   panelY + panelHeight - 12.0f,
                   skin_.palette.mutedText,
                   false);
    ofPopStyle();
}

inline void ControlMappingHubState::drawHudColumnPickerPanel(float panelX,
                                                             float panelY,
                                                             float panelWidth,
                                                             float panelHeight,
                                                             const ParameterRow* row) const {
    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    auto drawTextStyled = [textScale](const std::string& text, float x, float y, const ofColor& color, bool bold) {
        ofSetColor(color);
        drawBitmapStringScaled(text, x, y, textScale, bold);
    };
    ofPushStyle();
    ofSetColor(skin_.palette.surface);
    ofDrawRectRounded(panelX, panelY, panelWidth, panelHeight, skin_.metrics.borderRadius);
    std::string title = row ? ("HUD column for " + row->label) : std::string("HUD column");
    drawTextStyled(title, panelX + 12.0f, panelY + 20.0f, skin_.palette.headerText, true);
    float startX = panelX + 12.0f;
    float rowY = panelY + 48.0f * textScale;
    const float lineHeight = 20.0f * textScale;
    std::vector<std::string> options;
    options.emplace_back("Inactive");
    for (int i = 0; i < kHudColumnCount; ++i) {
        options.push_back(hudColumnLabel(i));
    }
    for (std::size_t i = 0; i < options.size(); ++i) {
        bool selected = static_cast<int>(i) == hudColumnPickerSelection_;
        drawTextStyled(options[i],
                       startX,
                       rowY,
                       selected ? skin_.palette.gridSelection : skin_.palette.bodyText,
                       selected);
        rowY += lineHeight;
    }
    drawTextStyled("Enter: apply    Esc: cancel",
                   startX,
                   panelY + panelHeight - 12.0f,
                   skin_.palette.mutedText,
                   false);
    ofPopStyle();
}

inline std::string ControlMappingHubState::formatSlotSummary(const ParameterRow& row) const {
    if (row.isHudLayoutEntry) {
        return "-";
    }
    if (const auto* logical = logicalSlotBinding(row)) {
        std::string label = logical->slotLabel.empty() ? logical->slotId : logical->slotLabel;
        std::string device = logical->deviceName.empty() ? logical->deviceId : logical->deviceName;
        if (!device.empty()) {
            return label + " - " + device;
        }
        return label;
    }
    if (!midiRouter_) {
        return midiAvailable_ ? midiStatus_ : std::string("Unavailable");
    }
    std::string controlId = currentSlotControlId(row);
    if (controlId.empty()) {
        return "Unassigned";
    }
    if (const auto* slot = slotForControlId(controlId)) {
        std::string label = slot->slotId.empty() ? slot->label : slot->slotId;
        if (label.empty()) {
            label = controlId;
        }
        std::string context;
        if (!slot->bindings.empty()) {
            context = slot->bindings.front().columnName.empty() ? slot->deviceName : slot->bindings.front().columnName;
        } else {
            context = slot->deviceName;
        }
        if (!context.empty()) {
            return label + " - " + context;
        }
        return label;
    }
    return controlId;
}

inline std::string ControlMappingHubState::currentSlotControlId(const ParameterRow& row) const {
    if (!midiRouter_) {
        return std::string();
    }
    if (const auto* cc = firstCcMapForRow(row)) {
        return cc->controlId;
    }
    if (const auto* btn = firstBtnMapForRow(row)) {
        return btn->controlId;
    }
    return std::string();
}

inline const MidiRouter::BtnMap* ControlMappingHubState::firstBtnMapForRow(const ParameterRow& row) const {
    if (!midiRouter_) {
        return nullptr;
    }
    for (const auto& map : midiRouter_->getBtnMaps()) {
        if (map.target == row.id) {
            return &map;
        }
    }
    return nullptr;
}

inline bool ControlMappingHubState::beginSlotPicker(const ParameterRow& row) {
    if (!row.isFloat && !row.boolParam) {
        setBannerMessage("Slot assignment not supported", 2000);
        return false;
    }
    auto indices = slotIndicesForRow(row);
    if (indices.empty()) {
        setBannerMessage("No compatible slots", 2000);
        return false;
    }
    slotPickerIndices_ = std::move(indices);
    slotPickerVisible_ = true;
    slotPickerRowId_ = row.id;
    slotPickerSelection_ = 0;
    slotPickerScrollOffset_ = 0;
    auto assignSelection = [&](int slotIndex) {
        if (slotIndex < 0) {
            return false;
        }
        for (std::size_t i = 0; i < slotPickerIndices_.size(); ++i) {
            if (slotPickerIndices_[i] == slotIndex) {
                slotPickerSelection_ = static_cast<int>(i);
                return true;
            }
        }
        return false;
    };
    bool assigned = false;
    if (const auto* logical = logicalSlotBinding(row)) {
        auto keyIt = slotIndexByLogicalKey_.find(logicalSlotKey(logical->deviceId, logical->slotId, logical->analog));
        if (keyIt != slotIndexByLogicalKey_.end()) {
            assigned = assignSelection(static_cast<int>(keyIt->second));
        }
    }
    if (!assigned) {
        std::string currentId = currentSlotControlId(row);
        if (!currentId.empty()) {
            auto sit = slotIndexById_.find(currentId);
            if (sit != slotIndexById_.end()) {
                assignSelection(static_cast<int>(sit->second));
            }
        }
    }
    pickingOsc_ = false;
    routingPopoverVisible_ = false;
    return true;
}

inline bool ControlMappingHubState::applySelectedSlot() {
    if (!slotPickerVisible_) {
        return false;
    }
    if (slotPickerSelection_ < 0 || slotPickerSelection_ >= static_cast<int>(slotPickerIndices_.size())) {
        cancelHudColumnPicker();
        cancelSlotPicker();
        return false;
    }
    const auto* row = rowForId(slotPickerRowId_);
    if (!row) {
        cancelHudColumnPicker();
        cancelSlotPicker();
        return false;
    }
    int slotIndex = slotPickerIndices_[slotPickerSelection_];
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slotCatalog_.size())) {
        return false;
    }
    const auto& slot = slotCatalog_[static_cast<std::size_t>(slotIndex)];
    if (!applyLogicalSlotAssignment(*row, slot)) {
        setBannerMessage("Failed to assign slot", 2200);
        return false;
    }
    bool removedMidi = false;
    if (midiRouter_) {
        removedMidi = midiRouter_->removeMidiMappingsForTarget(row->id);
    }
    bool appliedMidi = applySlotAssignmentToRow(*row);
    if (removedMidi || appliedMidi) {
        persistRoutingChange();
    }
    emitTelemetryEvent("midi.slot.assign", row, slot.deviceId + "." + slot.slotId);
    std::string label = slot.label.empty() ? slot.slotId : slot.label;
    setBannerMessage("Mapped " + (label.empty() ? slot.slotId : label) + " -> " + row->label, 2400);
    tableModel_.dirty = true;
    invalidateRowCache();
    cancelHudColumnPicker();
    cancelSlotPicker();
    return true;
}

inline std::string ControlMappingHubState::consoleColumnIdForSlot(int slotIndex) const {
    if (slotIndex <= 0) {
        return std::string();
    }
    return "column" + ofToString(slotIndex);
}

inline std::vector<std::string> ControlMappingHubState::candidateColumnIdsForRow(const ParameterRow& row) const {
    std::vector<std::string> ids;
    auto addUnique = [&](const std::string& value) {
        if (value.empty()) {
            return;
        }
        if (std::find(ids.begin(), ids.end(), value) == ids.end()) {
            ids.push_back(value);
        }
    };
    static const std::string kConsolePrefix = "console.layer";
    if (row.id.rfind(kConsolePrefix, 0) == 0) {
        std::size_t pos = kConsolePrefix.size();
        std::string digits;
        while (pos < row.id.size() && std::isdigit(static_cast<unsigned char>(row.id[pos]))) {
            digits.push_back(row.id[pos]);
            ++pos;
        }
        if (!digits.empty()) {
            addUnique("column" + digits);
        }
    }
    for (int slotIndex : row.consoleSlots) {
        addUnique(consoleColumnIdForSlot(slotIndex));
    }
    return ids;
}

inline const ControlMappingHubState::SlotOption::ColumnBinding*
ControlMappingHubState::selectColumnBindingForRow(const ParameterRow& row, const SlotOption& slot) const {
    if (slot.bindings.empty()) {
        return nullptr;
    }
    auto candidates = candidateColumnIdsForRow(row);
    if (!candidates.empty()) {
        for (const auto& candidate : candidates) {
            for (const auto& binding : slot.bindings) {
                if (binding.columnId == candidate) {
                    return &binding;
                }
            }
        }
    }
    return &slot.bindings.front();
}

inline void ControlMappingHubState::emitSlotBindingDiagnostics(const ParameterRow& row,
                                                               const SlotOption& slot,
                                                               const SlotOption::ColumnBinding& binding) const {
    std::ostringstream detail;
    detail << slot.deviceId;
    if (!binding.columnId.empty()) {
        detail << "." << binding.columnId;
    }
    detail << "." << slot.slotId;
    std::string bindingType = binding.bindingType.empty()
                                  ? (slot.analog ? std::string("cc") : std::string("note"))
                                  : binding.bindingType;
    detail << " type=" << bindingType << " num=" << binding.number << " ch=" << binding.channel;
    ofLogNotice("ControlMappingHub") << "Applied slot binding for " << row.id << " -> " << detail.str();
    emitTelemetryEvent("midi.slot.cc_bind", &row, detail.str());
}

inline void ControlMappingHubState::cancelSlotPicker() const {
    slotPickerVisible_ = false;
    slotPickerRowId_.clear();
    slotPickerIndices_.clear();
    slotPickerSelection_ = -1;
    slotPickerScrollOffset_ = 0;
}

inline bool ControlMappingHubState::beginHudColumnPicker(const ParameterRow& row) const {
    if (!isHudWidgetRow(row)) {
        return false;
    }
    cancelHudColumnPicker();
    cancelSlotPicker();
    hudColumnPickerVisible_ = true;
    hudColumnPickerRowId_ = row.id;
    hudColumnPickerSelection_ = 0;
    if (!hudPlacementProvider_) {
        return true;
    }
    const auto placements = hudPlacementProvider_();
    for (const auto& placement : placements) {
        if (placement.id != row.assetKey && placement.id != row.id) {
            continue;
        }
        if (placement.visible && placement.columnIndex >= 0) {
            hudColumnPickerSelection_ = ofClamp(placement.columnIndex, 0, kHudColumnCount - 1) + 1;
        } else {
            hudColumnPickerSelection_ = 0;
        }
        break;
    }
    return true;
}

inline bool ControlMappingHubState::applyHudColumnSelection() {
    if (!hudColumnPickerVisible_) {
        return false;
    }
    const auto* row = rowForId(hudColumnPickerRowId_);
    if (!row) {
        cancelHudColumnPicker();
        return false;
    }
    bool changed = false;
    std::string targetId = row->assetKey.empty() ? row->id : row->assetKey;
    if (hudColumnPickerSelection_ <= 0) {
        changed = setRowBoolValue(*row, false);
    } else {
        int columnIndex = hudColumnPickerSelection_ - 1;
        changed = setRowBoolValue(*row, true) || changed;
        if (hudPlacementCallback_) {
            hudPlacementCallback_(targetId, columnIndex);
            changed = true;
        }
        updateHudPlacementPreference(targetId, columnIndex);
        emitHudMappingEvent("placement", targetId);
    }
    cancelHudColumnPicker();
    tableModel_.dirty = true;
    invalidateRowCache();
    return changed;
}

inline void ControlMappingHubState::cancelHudColumnPicker() const {
    hudColumnPickerVisible_ = false;
    hudColumnPickerRowId_.clear();
    hudColumnPickerSelection_ = 0;
}

inline void ControlMappingHubState::updateHudVisibilityPreference(const std::string& id, bool visible) const {
    if (id.empty()) {
        return;
    }
    auto it = preferences_.hudWidgets.find(id);
    if (it != preferences_.hudWidgets.end() && it->second.visible == visible) {
        return;
    }
    preferences_.hudWidgets[id].visible = visible;
    markPreferencesDirty();
}

inline void ControlMappingHubState::updateHudPlacementPreference(const std::string& id, int columnIndex) const {
    if (id.empty()) {
        return;
    }
    if (hudLayoutTarget_ == HudLayoutTarget::Controller) {
        auto it = preferences_.controllerHudWidgets.find(id);
        if (it != preferences_.controllerHudWidgets.end() && it->second.columnIndex == columnIndex) {
            return;
        }
        preferences_.controllerHudWidgets[id].columnIndex = columnIndex;
    } else {
        auto it = preferences_.hudWidgets.find(id);
        if (it != preferences_.hudWidgets.end() && it->second.columnIndex == columnIndex) {
            return;
        }
        preferences_.hudWidgets[id].columnIndex = columnIndex;
    }
    markPreferencesDirty();
}

inline void ControlMappingHubState::snapshotHudPreferencesFromProvider() const {
    if (!hudPlacementProvider_ || !hudLayoutDirty_) {
        return;
    }
    const auto placements = hudPlacementProvider_();
    hudWidgetTargets_.clear();
    for (const auto& placement : placements) {
        if (placement.id.empty()) {
            continue;
        }
        if (!placement.target.empty()) {
            hudWidgetTargets_[placement.id] = placement.target;
        }
        auto& pref = preferences_.hudWidgets[placement.id];
        pref.visible = placement.visible;
        if (hudLayoutTarget_ == HudLayoutTarget::Controller) {
            auto& controllerPref = preferences_.controllerHudWidgets[placement.id];
            controllerPref.columnIndex = placement.columnIndex;
            if (!placement.bandId.empty()) {
                controllerPref.bandId = placement.bandId;
            }
            controllerPref.collapsed = placement.collapsed;
        } else {
            pref.columnIndex = placement.columnIndex;
            if (!placement.bandId.empty()) {
                pref.bandId = placement.bandId;
            }
            pref.collapsed = placement.collapsed;
        }
    }
    hudLayoutDirty_ = false;
}

inline void ControlMappingHubState::applyHudLayoutSnapshot() const {
    snapshotHudPreferencesFromProvider();
    markPreferencesDirty();
    emitHudMappingEvent("layout");
}

inline void ControlMappingHubState::beginHudLayoutFence() const {
    ++hudLayoutFenceDepth_;
}

inline void ControlMappingHubState::endHudLayoutFence() const {
    if (hudLayoutFenceDepth_ == 0) {
        return;
    }
    --hudLayoutFenceDepth_;
    if (hudLayoutFenceDepth_ == 0 && hudLayoutFenceDirty_) {
        hudLayoutFenceDirty_ = false;
        applyHudLayoutSnapshot();
    }
}

inline ControlMappingHubState::HudLayoutPlacement ControlMappingHubState::placementFromPreference(const HudWidgetPreference& pref) const {
    HudLayoutPlacement placement;
    placement.columnIndex = pref.columnIndex;
    placement.bandId = pref.bandId;
    placement.collapsed = pref.collapsed;
    return placement;
}

inline ControlMappingHubState::HudLayoutPlacement ControlMappingHubState::placementForControllerWidget(const std::string& id) const {
    HudLayoutPlacement placement;
    auto it = preferences_.controllerHudWidgets.find(id);
    if (it != preferences_.controllerHudWidgets.end()) {
        placement = it->second;
    } else {
        auto prefIt = preferences_.hudWidgets.find(id);
        if (prefIt != preferences_.hudWidgets.end()) {
            placement = placementFromPreference(prefIt->second);
        }
    }
    return placement;
}

inline ControlMappingHubState::HudLayoutPlacement ControlMappingHubState::expectedPlacementForTarget(const std::string& id,
                                                                                                     HudLayoutTarget target) const {
    if (target == HudLayoutTarget::Controller) {
        return placementForControllerWidget(id);
    }
    HudLayoutPlacement placement;
    auto it = preferences_.hudWidgets.find(id);
    if (it != preferences_.hudWidgets.end()) {
        placement = placementFromPreference(it->second);
    }
    return placement;
}

inline void ControlMappingHubState::seedControllerLayoutFromPrimary() const {
    bool wrote = false;
    for (const auto& entry : preferences_.hudWidgets) {
        if (entry.first.rfind("hud.", 0) != 0) {
            continue;
        }
        preferences_.controllerHudWidgets[entry.first] = placementFromPreference(entry.second);
        wrote = true;
    }
    if (wrote) {
        markPreferencesDirty();
    }
}

inline void ControlMappingHubState::replayHudTogglePreferences() const {
    if (!hudToggleCallback_) {
        return;
    }
    for (const auto& entry : preferences_.hudWidgets) {
        if (entry.first.rfind("hud.", 0) != 0) {
            continue;
        }
        hudToggleCallback_(entry.first, entry.second.visible);
    }
}

inline void ControlMappingHubState::replayHudPlacementPreferences() const {
    if (!hudPlacementCallback_) {
        return;
    }
    if (hudLayoutTarget_ == HudLayoutTarget::Controller) {
        for (const auto& entry : preferences_.hudWidgets) {
            if (entry.first.rfind("hud.", 0) != 0) {
                continue;
            }
            HudLayoutPlacement placement = placementForControllerWidget(entry.first);
            if (placement.columnIndex < 0) {
                continue;
            }
            hudPlacementCallback_(entry.first, placement.columnIndex);
        }
    } else {
        for (const auto& entry : preferences_.hudWidgets) {
            if (entry.first.rfind("hud.", 0) != 0) {
                continue;
            }
            if (entry.second.columnIndex < 0) {
                continue;
            }
            hudPlacementCallback_(entry.first, entry.second.columnIndex);
        }
    }
}

inline void ControlMappingHubState::pollHudLayoutDrift(uint64_t nowMs) const {
    if (!hudPlacementProvider_) {
        hudLayoutDriftActive_ = false;
        hudLayoutDriftSinceMs_ = 0;
        return;
    }
    if (hudLayoutDirty_) {
        hudLayoutDriftSinceMs_ = 0;
        return;
    }
    const auto placements = hudPlacementProvider_();
    if (placements.empty()) {
        hudLayoutDriftActive_ = false;
        hudLayoutDriftSinceMs_ = 0;
        return;
    }
    std::unordered_map<std::string, HudPlacementSnapshot> actual;
    actual.reserve(placements.size());
    for (const auto& placement : placements) {
        actual[placement.id] = placement;
    }
    struct DriftEntry {
        std::string id;
        HudLayoutPlacement expected;
        HudPlacementSnapshot snapshot;
    };
    std::vector<DriftEntry> drift;
    drift.reserve(preferences_.hudWidgets.size());
    for (const auto& entry : preferences_.hudWidgets) {
        if (entry.first.rfind("hud.", 0) != 0) {
            continue;
        }
        HudLayoutPlacement expected = expectedPlacementForTarget(entry.first, hudLayoutTarget_);
        if (expected.columnIndex < 0 && expected.bandId.empty()) {
            continue;
        }
        auto it = actual.find(entry.first);
        if (it == actual.end()) {
            continue;
        }
        const auto& snapshot = it->second;
        bool mismatch = false;
        if (expected.columnIndex >= 0 && snapshot.columnIndex >= 0 && expected.columnIndex != snapshot.columnIndex) {
            mismatch = true;
        }
        if (!mismatch && !expected.bandId.empty() && !snapshot.bandId.empty() && expected.bandId != snapshot.bandId) {
            mismatch = true;
        }
        if (!mismatch && expected.collapsed != snapshot.collapsed) {
            mismatch = true;
        }
        if (mismatch) {
            drift.push_back({entry.first, expected, snapshot});
        }
    }
    if (drift.empty()) {
        if (hudLayoutDriftActive_) {
            emitHudMappingEvent("drift_cleared", std::string(), hudLayoutTargetName(hudLayoutTarget_));
            hudLayoutDriftActive_ = false;
        }
        hudLayoutDriftSinceMs_ = 0;
        return;
    }
    if (hudLayoutDriftSinceMs_ == 0) {
        hudLayoutDriftSinceMs_ = nowMs;
    }
    std::vector<std::string> summary;
    summary.reserve(drift.size());
    for (const auto& entry : drift) {
        std::string note = entry.id;
        if (entry.expected.columnIndex >= 0 && entry.snapshot.columnIndex >= 0 &&
            entry.expected.columnIndex != entry.snapshot.columnIndex) {
            note += ":col " + ofToString(entry.snapshot.columnIndex) + "->" + ofToString(entry.expected.columnIndex);
        } else if (!entry.expected.bandId.empty() && !entry.snapshot.bandId.empty() &&
                   entry.expected.bandId != entry.snapshot.bandId) {
            note += ":band " + entry.snapshot.bandId + "->" + entry.expected.bandId;
        } else if (entry.expected.collapsed != entry.snapshot.collapsed) {
            note += ":collapsed";
        }
        summary.push_back(std::move(note));
    }
    if (!hudLayoutDriftActive_) {
        ofLogWarning("ControlMappingHubState") << "HUD layout drift (" << hudLayoutTargetName(hudLayoutTarget_)
                                               << ") detected for " << drift.size() << " widget(s): "
                                               << ofJoinString(summary, ", ");
        emitHudMappingEvent("drift", drift.front().id, hudLayoutTargetName(hudLayoutTarget_));
        hudLayoutDriftActive_ = true;
    }
    if (nowMs - hudLayoutDriftSinceMs_ >= kHudLayoutDriftAssertDelayMs && hudPlacementCallback_) {
        HudLayoutFenceScope fence(this);
        for (const auto& entry : drift) {
            if (entry.expected.columnIndex >= 0 && entry.snapshot.columnIndex >= 0 &&
                entry.expected.columnIndex != entry.snapshot.columnIndex) {
                hudPlacementCallback_(entry.id, entry.expected.columnIndex);
            }
        }
        hudLayoutDriftSinceMs_ = nowMs;
    }
}

inline std::vector<ControlMappingHubState::HudPlacementSnapshot>
ControlMappingHubState::exportHudLayoutSnapshot(HudLayoutTarget target) const {
    snapshotHudPreferencesFromProvider();
    std::vector<HudPlacementSnapshot> snapshot;
    snapshot.reserve(preferences_.hudWidgets.size());
    for (const auto& entry : preferences_.hudWidgets) {
        if (entry.first.rfind("hud.", 0) != 0) {
            continue;
        }
        HudPlacementSnapshot meta;
        meta.id = entry.first;
        meta.visible = entry.second.visible;
        auto targetIt = hudWidgetTargets_.find(entry.first);
        if (targetIt != hudWidgetTargets_.end() && !targetIt->second.empty()) {
            meta.target = targetIt->second;
        } else {
            meta.target = "projector";
        }
        HudLayoutPlacement placement = expectedPlacementForTarget(entry.first, target);
        meta.columnIndex = placement.columnIndex;
        meta.bandId = placement.bandId.empty() ? "hud" : placement.bandId;
        meta.bandLabel = meta.bandId;
        if (placement.columnIndex >= 0) {
            meta.columnLabel = "Column " + ofToString(placement.columnIndex + 1);
        }
        meta.collapsed = placement.collapsed;
        snapshot.push_back(std::move(meta));
    }
    return snapshot;
}

inline void ControlMappingHubState::emitHudLayoutSnapshot(HudLayoutTarget target,
                                                          const std::vector<HudPlacementSnapshot>& snapshot,
                                                          const std::string& reason) const {
    if (!eventCallback_) {
        return;
    }
    ofJson j;
    j["type"] = "hud.layout.snapshot";
    j["target"] = hudLayoutTargetToString(target);
    if (!reason.empty()) {
        j["reason"] = reason;
    }
    j["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    ofJson widgets = ofJson::array();
    for (const auto& entry : snapshot) {
        if (entry.id.empty()) {
            continue;
        }
        ofJson node;
        node["id"] = entry.id;
        node["column"] = entry.columnIndex;
        node["visible"] = entry.visible;
        node["collapsed"] = entry.collapsed;
        if (!entry.bandId.empty()) {
            node["band"] = entry.bandId;
        }
        if (!entry.target.empty()) {
            node["target"] = entry.target;
        }
        widgets.push_back(std::move(node));
    }
    j["widgets"] = std::move(widgets);
    try {
        eventCallback_(j.dump());
    } catch (...) {
        ofLogWarning("ControlMappingHubState") << "Event callback threw while publishing HUD layout snapshot";
    }
}

inline void ControlMappingHubState::emitOverlayRouteEvent(const std::string& target,
                                                          const std::string& source,
                                                          bool followMode) const {
    if (!eventCallback_) {
        return;
    }
    ofJson j;
    j["type"] = "overlay.route.changed";
    if (!target.empty()) {
        j["target"] = target;
    }
    if (!source.empty()) {
        j["source"] = source;
    }
    j["followMode"] = followMode;
    j["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    try {
        eventCallback_(j.dump());
    } catch (...) {
        ofLogWarning("ControlMappingHubState") << "Event callback threw while publishing overlay route event";
    }
}

inline void ControlMappingHubState::emitHudRoutingManifest(const std::vector<HudRoutingEntry>& manifest) const {
    if (!eventCallback_) {
        return;
    }
    ofJson j;
    j["type"] = "overlay.routing.manifest";
    j["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    ofJson widgets = ofJson::array();
    for (const auto& entry : manifest) {
        if (entry.id.empty()) {
            continue;
        }
        ofJson node;
        node["id"] = entry.id;
        if (!entry.label.empty()) {
            node["label"] = entry.label;
        }
        if (!entry.category.empty()) {
            node["category"] = entry.category;
        }
        if (!entry.target.empty()) {
            node["target"] = entry.target;
        }
        widgets.push_back(std::move(node));
    }
    if (!widgets.empty()) {
        j["widgets"] = std::move(widgets);
    }
    try {
        eventCallback_(j.dump());
    } catch (...) {
        ofLogWarning("ControlMappingHubState") << "Event callback threw while publishing overlay routing manifest";
    }
}

inline void ControlMappingHubState::applyAssetMetadata(ParameterRow& row) const {
    row.isAsset = false;
    row.assetKey.clear();
    row.assetLabel.clear();
    row.familyLabel.clear();
    std::string key = resolveAssetKey(row.id);
    if (key.empty()) {
        if (row.category.empty()) {
            row.category = "General";
        }
        return;
    }
    auto it = assetCatalog_.find(key);
    if (it == assetCatalog_.end()) {
        if (const auto* entry = consoleEntryForParam(row.id)) {
            row.isAsset = true;
            row.assetKey = key;
            row.assetLabel = entry->label.empty() ? entry->id : entry->label;
            std::string family = entry->category;
            row.familyLabel = mapFamilyLabel(family);
            row.category = row.familyLabel.empty() ? std::string("Assets") : row.familyLabel;
            row.subcategory = row.assetLabel.empty() ? row.assetKey : row.assetLabel;
        }
        return;
    }
    row.isAsset = true;
    row.assetKey = key;
    row.assetLabel = it->second.label;
    row.familyLabel = it->second.familyDisplay.empty() ? it->second.family : it->second.familyDisplay;
    row.category = row.familyLabel.empty() ? std::string("Assets") : row.familyLabel;
    row.subcategory = it->second.subcategoryDisplay.empty()
                          ? (row.assetLabel.empty() ? row.assetKey : row.assetLabel)
                          : it->second.subcategoryDisplay;
}

inline bool ControlMappingHubState::applySyntheticAssetMetadata(ParameterRow& row) const {
    if (row.isAsset) {
        return false;
    }
    struct SyntheticBinding {
        const char* paramId;
        const char* assetKey;
        const char* label;
        const char* family;
    };
    static constexpr std::array<SyntheticBinding, 16> kSyntheticBindings = {{
        { "transport.bpm", "session.controls", "Session Controls", "Session Controls" },
        { "globals.speed", "session.controls", "Session Controls", "Session Controls" },
        { "camera.dist", "session.controls", "Session Controls", "Session Controls" },
        { "fx.master", "fx.master", "Master FX", "Post FX" },
        { "ui.hud", "ui.visibility", "Visibility", "UI" },
        { "ui.console.visible", "ui.visibility", "Visibility", "UI" },
        { "ui.hub.visible", "ui.visibility", "Visibility", "UI" },
        { "ui.menu.visible", "ui.visibility", "Visibility", "UI" },
        { "ui.menu_text_size", "ui.visibility", "Visibility", "UI" },
        { "console.dual_display.mode", "ui.dual_display", "Dual Display", "UI" },
        { "console.secondary_display.enabled", "ui.dual_display", "Dual Display", "UI" },
        { "console.secondary_display.follow_primary", "ui.dual_display", "Dual Display", "UI" },
        { "console.secondary_display.monitor", "ui.dual_display", "Dual Display", "UI" },
        { "console.secondary_display.layout_watchdog", "ui.dual_display", "Dual Display", "UI" },
        { "console.secondary_display.force_resync", "ui.dual_display", "Dual Display", "UI" },
        { "console.controller.focus_console", "ui.dual_display", "Dual Display", "UI" }
    }};
    for (const auto& binding : kSyntheticBindings) {
        if (row.id == binding.paramId) {
            row.isAsset = true;
            row.assetKey = binding.assetKey;
            row.assetLabel = binding.label;
            row.familyLabel = binding.family;
            row.category = row.familyLabel;
            row.subcategory = row.assetLabel;
            return true;
        }
    }
    return false;
}

inline std::string ControlMappingHubState::sceneCurrentAssetLabel(const ConsoleLayerInfo& slot) const {
    std::string label;
    if (!slot.label.empty()) {
        label = slot.label;
    } else if (!slot.assetId.empty()) {
        if (auto it = assetKeyById_.find(slot.assetId); it != assetKeyById_.end()) {
            auto metaIt = assetCatalog_.find(it->second);
            if (metaIt != assetCatalog_.end() && !metaIt->second.label.empty()) {
                label = metaIt->second.label;
            }
        }
        if (label.empty()) {
            label = slot.assetId;
        }
    } else {
        label = "(empty)";
    }
    return ofToString(slot.index) + ": " + label;
}

inline void ControlMappingHubState::appendSceneBrowserRows() const {
    if (!consoleSlotInventory_.empty()) {
        const std::size_t baseRowCount = tableModel_.rows.size();
        std::vector<bool> slotHasRows(consoleSlotInventory_.size() + 1, false);

        for (std::size_t i = 0; i < baseRowCount; ++i) {
            const auto& source = tableModel_.rows[i];
            if (source.consoleSlots.empty()) {
                continue;
            }
            for (int slotIndex : source.consoleSlots) {
                if (slotIndex <= 0 || slotIndex >= static_cast<int>(slotHasRows.size())) {
                    continue;
                }
                const auto& slot = consoleSlotInventory_[static_cast<std::size_t>(slotIndex - 1)];
                ParameterRow mirrored = source;
                mirrored.category = "Scenes";
                mirrored.subcategory = "Current";
                mirrored.familyLabel = "Scenes";
                mirrored.assetLabel = sceneCurrentAssetLabel(slot);
                tableModel_.rows.push_back(std::move(mirrored));
                slotHasRows[slotIndex] = true;
            }
        }

        for (const auto& slot : consoleSlotInventory_) {
            if (slot.index <= 0 || slot.index >= static_cast<int>(slotHasRows.size()) || slotHasRows[slot.index]) {
                continue;
            }
            ParameterRow placeholder;
            placeholder.id = "scene.current.slot." + ofToString(slot.index);
            placeholder.label = slot.assetId.empty() ? "Empty slot" : "No parameter shortcuts available";
            placeholder.category = "Scenes";
            placeholder.subcategory = "Current";
            placeholder.isAsset = true;
            placeholder.assetLabel = sceneCurrentAssetLabel(slot);
            placeholder.familyLabel = "Scenes";
            placeholder.consoleSlots = { slot.index };
            placeholder.consoleSlotActive = slot.active;
            if (!slot.assetId.empty()) {
                if (auto it = assetKeyById_.find(slot.assetId); it != assetKeyById_.end()) {
                    placeholder.assetKey = it->second;
                }
            }
            tableModel_.rows.push_back(std::move(placeholder));
        }
    }

    ParameterRow saveAsRow;
    saveAsRow.id = "scene.saved.save_as";
    saveAsRow.label = "+ Save Scene As";
    saveAsRow.category = "Scenes";
    saveAsRow.subcategory = "Saved";
    saveAsRow.isString = true;
    saveAsRow.isSavedSceneRow = true;
    saveAsRow.isSavedSceneSaveRow = true;
    saveAsRow.familyLabel = "Scenes";
    tableModel_.rows.push_back(std::move(saveAsRow));

    std::vector<SavedSceneInfo> savedScenes;
    if (savedSceneListCallback_) {
        savedScenes = savedSceneListCallback_();
    }
    auto activeIt = std::find_if(savedScenes.begin(), savedScenes.end(), [](const SavedSceneInfo& scene) {
        return scene.active;
    });
    if (activeIt != savedScenes.end()) {
        ParameterRow overwriteRow;
        overwriteRow.id = "scene.saved.overwrite";
        overwriteRow.label = "Overwrite Current Scene";
        overwriteRow.category = "Scenes";
        overwriteRow.subcategory = "Saved";
        overwriteRow.isSavedSceneRow = true;
        overwriteRow.isSavedSceneOverwriteRow = true;
        overwriteRow.savedSceneId = activeIt->id;
        overwriteRow.savedScenePath = activeIt->path;
        overwriteRow.familyLabel = "Scenes";
        tableModel_.rows.push_back(std::move(overwriteRow));
    }
    if (savedScenes.empty()) {
        ParameterRow emptyRow;
        emptyRow.id = "scene.saved.empty";
        emptyRow.label = "No saved scenes found";
        emptyRow.category = "Scenes";
        emptyRow.subcategory = "Saved";
        emptyRow.isSavedSceneRow = true;
        emptyRow.familyLabel = "Scenes";
        emptyRow.offline = true;
        tableModel_.rows.push_back(std::move(emptyRow));
        return;
    }

    for (const auto& scene : savedScenes) {
        ParameterRow row;
        row.id = "scene.saved.entry." + scene.id;
        row.label = scene.label.empty() ? scene.id : scene.label;
        if (scene.active) {
            row.label += " (current)";
        }
        row.category = "Scenes";
        row.subcategory = "Saved";
        row.isSavedSceneRow = true;
        row.savedSceneId = scene.id;
        row.savedScenePath = scene.path;
        row.familyLabel = "Scenes";
        tableModel_.rows.push_back(std::move(row));
    }
}

inline bool ControlMappingHubState::isSavedSceneBrowserRow(const ParameterRow& row) const {
    return row.isSavedSceneRow || row.id == "scene.saved.empty";
}

inline bool ControlMappingHubState::handleSavedSceneRowActivation(const ParameterRow& row) const {
    if (!isSavedSceneBrowserRow(row)) {
        return false;
    }
    if (row.isSavedSceneSaveRow) {
        if (editingValueActive_ && editingValueRowId_ == row.id) {
            return const_cast<ControlMappingHubState*>(this)->commitValueEdit();
        }
        beginValueEdit(row);
        setBannerMessage("Type a scene name, then press Enter again to save", 2600);
        return true;
    }
    if (row.isSavedSceneOverwriteRow) {
        cancelValueEdit();
        if (row.savedSceneId.empty()) {
            setBannerMessage("No active named scene to overwrite", 2200);
            return true;
        }
        if (!savedSceneOverwriteCallback_) {
            setBannerMessage("Scene overwrite unavailable", 2200);
            return true;
        }
        if (savedSceneOverwriteCallback_(row.savedSceneId)) {
            tableModel_.dirty = true;
            invalidateRowCache();
            markConsoleSlotsDirty();
            setBannerMessage("Overwrote current scene", 2200);
            return true;
        }
        setBannerMessage("Failed to overwrite current scene", 2600);
        return true;
    }
    if (row.savedSceneId.empty()) {
        setBannerMessage("No saved scenes available yet", 2200);
        return true;
    }
    if (!savedSceneLoadCallback_) {
        setBannerMessage("Saved scene load unavailable", 2200);
        return true;
    }
    if (savedSceneLoadCallback_(row.savedSceneId)) {
        cancelValueEdit();
        tableModel_.dirty = true;
        invalidateRowCache();
        savedSceneDraftName_ = row.label;
        markConsoleSlotsDirty();
        setBannerMessage("Loaded scene " + row.label, 2200);
    } else {
        setBannerMessage("Failed to load scene " + row.label, 2600);
    }
    return true;
}

inline const LayerLibrary::Entry* ControlMappingHubState::consoleEntryForParam(const std::string& id) const {
    if (!consoleAssetResolver_) {
        return nullptr;
    }
    static const std::string kConsolePrefix = "console.layer";
    if (id.rfind(kConsolePrefix, 0) != 0) {
        return nullptr;
    }
    std::size_t dot = id.find('.', kConsolePrefix.size());
    std::string prefix = dot == std::string::npos ? id : id.substr(0, dot);
    return consoleAssetResolver_(prefix);
}

inline std::string ControlMappingHubState::resolveAssetKey(const std::string& id) const {
    auto direct = assetCatalog_.find(id);
    if (direct != assetCatalog_.end()) {
        return direct->first;
    }
    if (const auto* entry = consoleEntryForParam(id)) {
        return entry->registryPrefix.empty() ? entry->id : entry->registryPrefix;
    }
    if (consoleAssetResolver_) {
        static const std::string kConsolePrefix = "console.layer";
        if (id.rfind(kConsolePrefix, 0) == 0) {
            std::size_t dot = id.find('.', kConsolePrefix.size());
            std::string prefix = dot == std::string::npos ? id : id.substr(0, dot);
            if (const auto* resolved = consoleAssetResolver_(prefix)) {
                return resolved->registryPrefix.empty() ? resolved->id : resolved->registryPrefix;
            }
        }
    }
    auto dot = id.find_last_of('.');
    if (dot == std::string::npos) {
        return std::string();
    }
    std::string scope = id.substr(0, dot);
    auto scopeDirect = assetCatalog_.find(scope);
    if (scopeDirect != assetCatalog_.end()) {
        return scopeDirect->first;
    }
    auto tokens = ofSplitString(scope, ".", true, true);
    if (tokens.empty()) {
        return std::string();
    }
    std::size_t start = 0;
    std::string firstLower = ofToLower(tokens[0]);
    if (firstLower.rfind("deck", 0) == 0 && tokens.size() > 1) {
        start = 1;
    }
    std::string candidate;
    std::string resolved;
    for (std::size_t i = start; i < tokens.size(); ++i) {
        if (!candidate.empty()) {
            candidate += ".";
        }
        candidate += tokens[i];
        if (assetCatalog_.count(candidate)) {
            resolved = candidate;
        }
    }
    return resolved;
}

inline void ControlMappingHubState::rebuildAssetCatalog() {
    assetCatalog_.clear();
    assetKeyById_.clear();
    markOfflineHydrationDirty();
    if (!layerLibrary_) {
        return;
    }
    for (const auto& entry : layerLibrary_->entries()) {
        std::string key = entry.registryPrefix.empty() ? entry.id : entry.registryPrefix;
        if (key.empty()) {
            continue;
        }
        AssetMetadata meta;
        meta.key = key;
        meta.label = entry.label.empty() ? entry.id : entry.label;
        meta.family = entry.category;
        meta.familyDisplay = mapFamilyLabel(entry.category);
        if (meta.familyDisplay == "Generative") {
            meta.family = "2D";
            meta.familyDisplay = "2D";
            meta.subcategoryDisplay = "Generative";
        }
        if (key == "overlay.text" || entry.id == "text.layer") {
            meta.family = "2D";
            meta.familyDisplay = "2D";
            meta.subcategoryDisplay = "Text Layer";
        }
        if (key.rfind("hud.", 0) == 0) {
            meta.family = "UI";
            meta.familyDisplay = "UI";
            meta.subcategoryDisplay = "HUD Widgets";
        }
        meta.registryPrefix = key;
        meta.assetId = entry.id;
        assetCatalog_[meta.key] = meta;
        if (!meta.assetId.empty()) {
            assetKeyById_[meta.assetId] = meta.key;
        }
    }
}

inline void ControlMappingHubState::appendAssetPlaceholders(const std::unordered_set<std::string>& assetKeysPresent) const {
    if (assetCatalog_.empty()) {
        return;
    }
    hydrateOfflineAssetsIfNeeded();
    const auto& offlineFloats = offlineRegistry_.floats();
    const auto& offlineBools = offlineRegistry_.bools();
    const auto& offlineStrings = offlineRegistry_.strings();
    for (const auto& kv : assetCatalog_) {
        if (assetKeysPresent.count(kv.first)) {
            continue;
        }
        const auto& meta = kv.second;
        std::string baseLabel = meta.label.empty() ? meta.key : meta.label;
        std::string family = meta.familyDisplay.empty() ? meta.family : meta.familyDisplay;
        std::string subcategory = meta.subcategoryDisplay.empty() ? baseLabel : meta.subcategoryDisplay;
        if (family.empty()) {
            family = "Assets";
        }
        std::string prefix = meta.registryPrefix.empty() ? meta.key : meta.registryPrefix;
        bool appended = false;
        auto appendCommon = [&](ParameterRow& row) {
            row.category = family;
            row.subcategory = subcategory;
            row.isAsset = true;
            row.assetKey = meta.key;
            row.assetLabel = baseLabel;
            row.familyLabel = family;
            row.offline = true;
            appended = true;
            applyConsoleSlotAnnotations(row);
            tableModel_.rows.push_back(std::move(row));
        };
        auto fit = offlineFloatParamIndices_.find(prefix);
        if (fit != offlineFloatParamIndices_.end()) {
            for (int idx : fit->second) {
                if (idx < 0 || idx >= static_cast<int>(offlineFloats.size())) {
                    continue;
                }
                const auto& param = offlineFloats[static_cast<std::size_t>(idx)];
                ParameterRow row;
                row.id = param.meta.id;
                row.label = param.meta.label.empty() ? param.meta.id : param.meta.label;
                row.isFloat = true;
                row.floatParam = &param;
                appendCommon(row);
            }
        }
        auto bit = offlineBoolParamIndices_.find(prefix);
        if (bit != offlineBoolParamIndices_.end()) {
            for (int idx : bit->second) {
                if (idx < 0 || idx >= static_cast<int>(offlineBools.size())) {
                    continue;
                }
                const auto& param = offlineBools[static_cast<std::size_t>(idx)];
                ParameterRow row;
                row.id = param.meta.id;
                row.label = param.meta.label.empty() ? param.meta.id : param.meta.label;
                row.boolParam = &param;
                appendCommon(row);
            }
        }
        auto sit = offlineStringParamIndices_.find(prefix);
        if (sit != offlineStringParamIndices_.end()) {
            for (int idx : sit->second) {
                if (idx < 0 || idx >= static_cast<int>(offlineStrings.size())) {
                    continue;
                }
                const auto& param = offlineStrings[static_cast<std::size_t>(idx)];
                ParameterRow row;
                row.id = param.meta.id;
                row.label = param.meta.label.empty() ? param.meta.id : param.meta.label;
                row.isString = true;
                row.stringParam = &param;
                appendCommon(row);
            }
        }
        if (!appended) {
            ParameterRow row;
            row.id = "asset.unhydrated." + meta.key;
            row.label = baseLabel + " (unhydrated)";
            row.category = family;
            row.subcategory = subcategory;
            row.isAsset = true;
            row.assetKey = meta.key;
            row.assetLabel = baseLabel;
            row.familyLabel = family;
            row.offline = true;
            applyConsoleSlotAnnotations(row);
            tableModel_.rows.push_back(std::move(row));
        }
    }
}

inline void ControlMappingHubState::appendBioAmpRows() const {
    static const struct {
        const char* id;
        const char* label;
    } kBioRows[] = {
        {"sensors.bioamp.raw", "BioAmp Raw"},
        {"sensors.bioamp.signal", "BioAmp Filtered"},
        {"sensors.bioamp.mean", "BioAmp Mean"},
        {"sensors.bioamp.rms", "BioAmp RMS"},
        {"sensors.bioamp.dom_hz", "BioAmp Dominant Hz"},
        {"sensors.bioamp.sample_rate", "BioAmp Sample Rate"},
        {"sensors.bioamp.window", "BioAmp Window"}
    };
    for (const auto& def : kBioRows) {
        ParameterRow row;
        row.id = def.id;
        row.label = def.label;
        row.category = "Sensors";
        row.subcategory = "BioAmp EXG Pill";
        row.isFloat = true;
        row.offline = false;
        applyAssetMetadata(row);
        applySyntheticAssetMetadata(row);
        tableModel_.rows.push_back(std::move(row));
    }
}

inline bool ControlMappingHubState::isBioAmpRowId(const std::string& id) const {
    return id == "sensors.bioamp.raw" ||
           id == "sensors.bioamp.signal" ||
           id == "sensors.bioamp.mean" ||
           id == "sensors.bioamp.rms" ||
           id == "sensors.bioamp.dom_hz" ||
           id == "sensors.bioamp.sample_rate" ||
           id == "sensors.bioamp.window";
}

inline bool ControlMappingHubState::readBioAmpValue(const std::string& id,
                                                    float& outValue,
                                                    uint64_t& outTimestamp) const {
    const auto convertSample = [&](const BioAmpMetricSample& sample) -> bool {
        if (!sample.valid) {
            return false;
        }
        outValue = sample.value;
        outTimestamp = sample.timestampMs;
        return true;
    };
    if (id == "sensors.bioamp.raw") {
        return convertSample(bioAmpState_.raw);
    }
    if (id == "sensors.bioamp.signal") {
        return convertSample(bioAmpState_.signal);
    }
    if (id == "sensors.bioamp.mean") {
        return convertSample(bioAmpState_.mean);
    }
    if (id == "sensors.bioamp.rms") {
        return convertSample(bioAmpState_.rms);
    }
    if (id == "sensors.bioamp.dom_hz") {
        return convertSample(bioAmpState_.domHz);
    }
    if (id == "sensors.bioamp.sample_rate") {
        if (bioAmpState_.sampleRate == 0) {
            return false;
        }
        outValue = static_cast<float>(bioAmpState_.sampleRate);
        outTimestamp = bioAmpState_.sampleRateTimestampMs;
        return true;
    }
    if (id == "sensors.bioamp.window") {
        if (bioAmpState_.windowSize == 0) {
            return false;
        }
        outValue = static_cast<float>(bioAmpState_.windowSize);
        outTimestamp = bioAmpState_.windowTimestampMs;
        return true;
    }
    return false;
}

inline void ControlMappingHubState::markOfflineHydrationDirty() const {
    offlineHydrationDirty_ = true;
}

inline void ControlMappingHubState::hydrateOfflineAssetsIfNeeded() const {
    if (!offlineHydrationDirty_) {
        return;
    }
    offlineHydrationDirty_ = false;
    offlineRegistry_ = ParameterRegistry();
    offlineLayers_.clear();
    offlineFloatParamIndices_.clear();
    offlineBoolParamIndices_.clear();
    offlineStringParamIndices_.clear();
    offlineOpacityValues_.clear();
    if (!layerLibrary_) {
        return;
    }
    auto& factory = LayerFactory::instance();
    for (const auto& entry : layerLibrary_->entries()) {
        auto layer = factory.create(entry.type);
        if (!layer) {
            continue;
        }
        std::string prefix = entry.registryPrefix.empty() ? entry.id : entry.registryPrefix;
        if (prefix.empty()) {
            continue;
        }
        layer->setRegistryPrefix(prefix);
        layer->setInstanceId(entry.id);
        layer->configure(entry.config);
        std::size_t prevFloat = offlineRegistry_.floats().size();
        std::size_t prevBool = offlineRegistry_.bools().size();
        std::size_t prevString = offlineRegistry_.strings().size();
        layer->setup(offlineRegistry_);
        float entryOpacity = std::clamp(entry.opacity, 0.0f, 1.0f);
        if (!prefix.empty()) {
            auto opacityStore = std::make_unique<float>(entryOpacity);
            ParameterRegistry::Descriptor opacityMeta;
            opacityMeta.label = "Layer Opacity";
            opacityMeta.group = "Visibility";
            opacityMeta.description = "Base opacity for this layer before FX or modifiers";
            opacityMeta.range.min = 0.0f;
            opacityMeta.range.max = 1.0f;
            opacityMeta.range.step = 0.01f;
            opacityMeta.quickAccess = true;
            opacityMeta.quickAccessOrder = 0;
            offlineRegistry_.addFloat(prefix + ".opacity", opacityStore.get(), entryOpacity, opacityMeta);
            offlineOpacityValues_[prefix] = std::move(opacityStore);
        }
        const auto& floats = offlineRegistry_.floats();
        for (std::size_t i = prevFloat; i < floats.size(); ++i) {
            offlineFloatParamIndices_[prefix].push_back(static_cast<int>(i));
        }
        const auto& bools = offlineRegistry_.bools();
        for (std::size_t i = prevBool; i < bools.size(); ++i) {
            offlineBoolParamIndices_[prefix].push_back(static_cast<int>(i));
        }
        const auto& strings = offlineRegistry_.strings();
        for (std::size_t i = prevString; i < strings.size(); ++i) {
            offlineStringParamIndices_[prefix].push_back(static_cast<int>(i));
        }
        offlineLayers_.push_back(std::move(layer));
    }
}

inline void ControlMappingHubState::refreshConsoleSlotInventory() const {
    if (!consoleSlotInventoryDirty_) {
        return;
    }
    consoleSlotInventoryDirty_ = false;
    consoleSlotsByAssetId_.clear();
    consoleSlotsByAssetKey_.clear();
    activeConsoleSlotIndices_.clear();
    consoleSlotInventory_.clear();
    if (!consoleSlotInventoryCallback_) {
        return;
    }
    consoleSlotInventory_ = consoleSlotInventoryCallback_();
    for (const auto& info : consoleSlotInventory_) {
        if (info.assetId.empty()) {
            continue;
        }
        ConsoleSlotRef ref;
        ref.slotIndex = info.index;
        ref.active = info.active;
        if (!info.assetId.empty()) {
            consoleSlotsByAssetId_[info.assetId].push_back(ref);
            activeConsoleSlotIndices_.insert(info.index);
        }
        auto keyIt = assetKeyById_.find(info.assetId);
        if (keyIt != assetKeyById_.end()) {
            consoleSlotsByAssetKey_[keyIt->second].push_back(ref);
        }
    }
}

inline void ControlMappingHubState::setConsoleAssetResolver(std::function<const LayerLibrary::Entry*(const std::string& prefix)> resolver) {
    consoleAssetResolver_ = std::move(resolver);
}

inline std::string ControlMappingHubState::mapFamilyLabel(const std::string& raw) const {
    if (raw.empty()) {
        return std::string("Assets");
    }
    std::string lowered = ofToLower(raw);
    if (lowered == "geometry") {
        return "3D / Mesh";
    }
    if (lowered == "fx" || lowered == "effects") {
        return "Post FX";
    }
    if (lowered == "generative") {
        return "Generative";
    }
    if (lowered == "media") {
        return "Media";
    }
    return raw;
}

inline void ControlMappingHubState::markSlotCatalogDirty() const {
    slotCatalogDirty_ = true;
    slotCatalog_.clear();
    slotIndexById_.clear();
    slotIndexByLogicalKey_.clear();
}

inline void ControlMappingHubState::refreshSlotCatalog() const {
    if (!slotCatalogDirty_) {
        return;
    }
    slotCatalogDirty_ = false;
    slotCatalog_.clear();
    slotIndexById_.clear();
    slotIndexByLogicalKey_.clear();
    if (deviceMapsDirectory_.empty()) {
        return;
    }
    ofDirectory dir(deviceMapsDirectory_);
    if (!dir.exists()) {
        return;
    }
    dir.allowExt("json");
    dir.listDir();
    for (auto& file : dir.getFiles()) {
        try {
            ofJson doc = ofLoadJson(file.getAbsolutePath());
            std::string deviceId = doc.value("deviceId", file.getBaseName());
            std::string deviceName = doc.value("name", deviceId);
            if (!doc.contains("columns") || !doc["columns"].is_object()) {
                continue;
            }
            for (const auto& column : doc["columns"].items()) {
                std::string columnId = column.key();
                const auto& columnNode = column.value();
                std::string columnName = columnNode.value("name", columnId);
                if (!columnNode.contains("slots") || !columnNode["slots"].is_array()) {
                    continue;
                }
                for (const auto& slotNode : columnNode["slots"]) {
                    if (!slotNode.is_object()) {
                        continue;
                    }
                    if (!slotNode.contains("binding") || !slotNode["binding"].is_object()) {
                        continue;
                    }
                    const auto& binding = slotNode["binding"];
                    SlotOption::ColumnBinding columnBinding;
                    columnBinding.columnId = columnId;
                    columnBinding.columnName = columnName.empty() ? columnId : columnName;
                    columnBinding.bindingType = ofToLower(binding.value("type", std::string()));
                    columnBinding.channel = binding.value("channel", binding.value("ch", -1));
                    columnBinding.number = -1;
                    if (binding.contains("number")) {
                        columnBinding.number = binding["number"].get<int>();
                    } else if (binding.contains("num")) {
                        columnBinding.number = binding["num"].get<int>();
                    } else if (binding.contains("cc")) {
                        columnBinding.number = binding["cc"].get<int>();
                    } else if (binding.contains("note")) {
                        columnBinding.number = binding["note"].get<int>();
                    }
                    std::string slotId = slotNode.value("id", std::string());
                    std::string slotLabel = slotNode.value("label", slotId);
                    std::string roleType = slotNode.value("role", std::string());
                    if (slotId.empty() || columnBinding.number < 0) {
                        continue;
                    }
                    bool analog = true;
                    std::string bindingType = ofToLower(columnBinding.bindingType);
                    if (bindingType.empty()) {
                        bindingType = roleType == "button" ? "note" : "cc";
                    }
                    analog = bindingType != "note";
                    columnBinding.bindingType = bindingType;
                    columnBinding.controlId = deviceId + "." + columnId + "." + slotId;
                    std::string logicalKey = logicalSlotKey(deviceId, slotId, analog);
                    SlotOption* slot = nullptr;
                    auto keyIt = slotIndexByLogicalKey_.find(logicalKey);
                    if (keyIt == slotIndexByLogicalKey_.end()) {
                        SlotOption option;
                        option.deviceId = deviceId;
                        option.deviceName = deviceName;
                        option.slotId = slotId;
                        option.label = slotLabel.empty() ? slotId : slotLabel;
                        option.roleType = roleType;
                        option.analog = analog;
                        slotCatalog_.push_back(std::move(option));
                        std::size_t newIndex = slotCatalog_.size() - 1;
                        slotIndexByLogicalKey_[logicalKey] = newIndex;
                        slot = &slotCatalog_.back();
                    } else {
                        slot = &slotCatalog_[keyIt->second];
                    }
                    if (slot) {
                        slot->bindings.push_back(columnBinding);
                        slotIndexById_[columnBinding.controlId] = static_cast<std::size_t>(slot - &slotCatalog_.front());
                    }
                }
            }
        } catch (const std::exception& ex) {
            ofLogWarning("ControlMappingHub") << "Failed to parse device map " << file.getFileName() << ": " << ex.what();
        }
    }
}

inline std::vector<int> ControlMappingHubState::slotIndicesForRow(const ParameterRow& row) const {
    refreshSlotCatalog();
    std::vector<int> indices;
    bool wantsAnalog = row.isFloat;
    bool wantsButton = (!row.isFloat && row.boolParam != nullptr);
    if (!wantsAnalog && !wantsButton) {
        return indices;
    }
    for (std::size_t i = 0; i < slotCatalog_.size(); ++i) {
        const auto& slot = slotCatalog_[i];
        if (wantsAnalog && !slot.analog) {
            continue;
        }
        if (wantsButton && slot.analog) {
            continue;
        }
        indices.push_back(static_cast<int>(i));
    }
    return indices;
}

inline const ControlMappingHubState::SlotOption* ControlMappingHubState::slotForControlId(const std::string& controlId) const {
    refreshSlotCatalog();
    auto it = slotIndexById_.find(controlId);
    if (it == slotIndexById_.end()) {
        return nullptr;
    }
    std::size_t index = it->second;
    if (index >= slotCatalog_.size()) {
        return nullptr;
    }
    return &slotCatalog_[index];
}

inline const ControlMappingHubState::SlotOption* ControlMappingHubState::slotForLogicalKey(const std::string& deviceId,
                                                                                           const std::string& slotId,
                                                                                           bool analog) const {
    refreshSlotCatalog();
    auto keyIt = slotIndexByLogicalKey_.find(logicalSlotKey(deviceId, slotId, analog));
    if (keyIt == slotIndexByLogicalKey_.end()) {
        return nullptr;
    }
    std::size_t index = keyIt->second;
    if (index >= slotCatalog_.size()) {
        return nullptr;
    }
    return &slotCatalog_[index];
}

inline std::string ControlMappingHubState::logicalSlotKey(const std::string& deviceId,
                                                          const std::string& slotId,
                                                          bool analog) const {
    return deviceId + "::" + slotId + "::" + (analog ? "analog" : "digital");
}

inline std::string ControlMappingHubState::parameterSuffixForAssignment(const std::string& parameterId,
                                                                        const std::string& assetKeyHint) const {
    if (parameterId.empty()) {
        return std::string();
    }
    static const std::string kConsolePrefix = "console.layer";
    if (parameterId.rfind(kConsolePrefix, 0) == 0) {
        std::size_t pos = kConsolePrefix.size();
        while (pos < parameterId.size() && std::isdigit(static_cast<unsigned char>(parameterId[pos]))) {
            ++pos;
        }
        if (pos < parameterId.size() && parameterId[pos] == '.') {
            ++pos;
        }
        return parameterId.substr(pos);
    }
    if (!assetKeyHint.empty()) {
        std::string prefix = assetKeyHint + ".";
        if (parameterId.rfind(prefix, 0) == 0 && parameterId.size() > prefix.size()) {
            return parameterId.substr(prefix.size());
        }
    }
    auto dot = parameterId.find('.');
    if (dot != std::string::npos && dot + 1 < parameterId.size()) {
        return parameterId.substr(dot + 1);
    }
    return parameterId;
}

inline std::string ControlMappingHubState::assignmentKeyForId(const std::string& parameterId,
                                                              const std::string& assetKeyHint) const {
    if (parameterId.empty()) {
        return std::string();
    }
    std::string assetKey = assetKeyHint;
    if (assetKey.empty()) {
        assetKey = resolveAssetKey(parameterId);
    }
    if (assetKey.empty()) {
        return parameterId;
    }
    std::string suffix = parameterSuffixForAssignment(parameterId, assetKey);
    if (suffix.empty()) {
        suffix = parameterId;
    }
    return assetKey + "::" + suffix;
}

inline std::string ControlMappingHubState::assignmentKeyForRow(const ParameterRow& row) const {
    std::string assetKey = row.assetKey;
    if (assetKey.empty()) {
        assetKey = resolveAssetKey(row.id);
    }
    if (assetKey.empty()) {
        return row.id;
    }
    std::string suffix = parameterSuffixForAssignment(row.id, assetKey);
    if (suffix.empty()) {
        suffix = row.id;
    }
    return assetKey + "::" + suffix;
}

inline void ControlMappingHubState::ensureSlotAssignmentDirectory() const {
    if (slotAssignmentsPath_.empty()) {
        return;
    }
    auto dir = ofFilePath::getEnclosingDirectory(slotAssignmentsPath_, false);
    if (!dir.empty()) {
        ofDirectory::createDirectory(dir, true, true);
    }
}

inline void ControlMappingHubState::loadSlotAssignments() const {
    if (slotAssignmentsLoaded_) {
        return;
    }
    slotAssignmentsLoaded_ = true;
    slotAssignments_.clear();
    slotAssignmentsDirty_ = false;
    if (slotAssignmentsPath_.empty() || !ofFile::doesFileExist(slotAssignmentsPath_)) {
        return;
    }
    try {
        ofJson doc = ofLoadJson(slotAssignmentsPath_);
        if (doc.contains("assignments") && doc["assignments"].is_array()) {
            for (const auto& entry : doc["assignments"]) {
                if (!entry.is_object()) {
                    continue;
                }
                std::string parameterId = entry.value("parameterId", std::string());
                if (parameterId.empty()) {
                    continue;
                }
                std::string assignmentKey = assignmentKeyForId(parameterId);
                if (assignmentKey.empty()) {
                    assignmentKey = parameterId;
                }
                LogicalSlotBinding binding;
                binding.deviceId = entry.value("deviceId", std::string());
                binding.deviceName = entry.value("deviceName", binding.deviceId);
                binding.slotId = entry.value("slotId", std::string());
                binding.slotLabel = entry.value("slotLabel", binding.slotId);
                binding.analog = entry.value("analog", true);
                slotAssignments_[assignmentKey] = std::move(binding);
            }
        }
    } catch (const std::exception& ex) {
        ofLogWarning("ControlMappingHub") << "Failed to load slot assignments: " << ex.what();
    }
}

inline void ControlMappingHubState::markSlotAssignmentsDirty() const {
    slotAssignmentsDirty_ = true;
}

inline void ControlMappingHubState::flushSlotAssignments() const {
    if (!slotAssignmentsDirty_ || slotAssignmentsPath_.empty()) {
        return;
    }
    ensureSlotAssignmentDirectory();
    ofJson doc;
    ofJson assignments = ofJson::array();
    for (const auto& pair : slotAssignments_) {
        ofJson entry;
        entry["parameterId"] = pair.first;
        entry["deviceId"] = pair.second.deviceId;
        entry["deviceName"] = pair.second.deviceName;
        entry["slotId"] = pair.second.slotId;
        entry["slotLabel"] = pair.second.slotLabel;
        entry["analog"] = pair.second.analog;
        assignments.push_back(entry);
    }
    doc["assignments"] = assignments;
    std::string tmpPath = slotAssignmentsPath_ + ".tmp";
    try {
        if (!ofSavePrettyJson(tmpPath, doc)) {
            ofLogWarning("ControlMappingHub") << "Failed to write slot assignments";
            return;
        }
        if (!ofFile::moveFromTo(tmpPath, slotAssignmentsPath_, true, true)) {
            ofLogWarning("ControlMappingHub") << "Failed to commit slot assignments";
            return;
        }
        slotAssignmentsDirty_ = false;
    } catch (const std::exception& ex) {
        ofLogWarning("ControlMappingHub") << "Failed to save slot assignments: " << ex.what();
    }
}

inline const ControlMappingHubState::LogicalSlotBinding* ControlMappingHubState::logicalSlotBinding(const ParameterRow& row) const {
    loadSlotAssignments();
    std::string key = assignmentKeyForRow(row);
    if (key.empty()) {
        key = row.id;
    }
    auto it = slotAssignments_.find(key);
    if (it == slotAssignments_.end() && key != row.id) {
        auto legacyIt = slotAssignments_.find(row.id);
        if (legacyIt != slotAssignments_.end()) {
            slotAssignments_[key] = legacyIt->second;
            slotAssignments_.erase(legacyIt);
            markSlotAssignmentsDirty();
            flushSlotAssignments();
            it = slotAssignments_.find(key);
        }
    }
    if (it == slotAssignments_.end()) {
        return nullptr;
    }
    return &it->second;
}

inline bool ControlMappingHubState::applyLogicalSlotAssignment(const ParameterRow& row, const SlotOption& slot) const {
    loadSlotAssignments();
    LogicalSlotBinding binding;
    binding.deviceId = slot.deviceId;
    binding.deviceName = slot.deviceName;
    binding.slotId = slot.slotId;
    binding.slotLabel = slot.label.empty() ? slot.slotId : slot.label;
    binding.analog = slot.analog;
    std::string key = assignmentKeyForRow(row);
    if (key.empty()) {
        key = row.id;
    }
    slotAssignments_[key] = std::move(binding);
    markSlotAssignmentsDirty();
    flushSlotAssignments();
    return true;
}

inline bool ControlMappingHubState::removeLogicalSlotAssignment(const std::string& parameterId) const {
    loadSlotAssignments();
    auto eraseKey = [&](const std::string& key) -> bool {
        auto it = slotAssignments_.find(key);
        if (it == slotAssignments_.end()) {
            return false;
        }
        slotAssignments_.erase(it);
        return true;
    };
    std::string canonical = assignmentKeyForId(parameterId);
    bool erased = false;
    if (!canonical.empty()) {
        erased = eraseKey(canonical);
    }
    if (!erased) {
        erased = eraseKey(parameterId);
    }
    if (!erased) {
        return false;
    }
    markSlotAssignmentsDirty();
    flushSlotAssignments();
    return true;
}

inline bool ControlMappingHubState::applySlotAssignmentToRow(const ParameterRow& row) const {
    if (!midiRouter_) {
        return false;
    }
    const auto* logical = logicalSlotBinding(row);
    if (!logical) {
        return false;
    }
    const auto* slot = slotForLogicalKey(logical->deviceId, logical->slotId, logical->analog);
    if (!slot || slot->bindings.empty()) {
        return false;
    }
    const auto* binding = selectColumnBindingForRow(row, *slot);
    if (!binding) {
        ofLogWarning("ControlMappingHub") << "Unable to resolve binding for slot " << slot->slotId << " (" << row.id << ")";
        return false;
    }
    MidiRouter::BindingMetadata meta;
    meta.channel = binding->channel;
    meta.controlId = binding->controlId;
    meta.deviceId = slot->deviceId;
    meta.columnId = binding->columnId;
    meta.slotId = slot->slotId;

    if (logical->analog) {
        if (!row.isFloat || !row.floatParam) {
            return false;
        }
        float* valuePtr = row.floatParam->value;
        if (valuePtr) {
            float bindMin = row.floatParam->meta.range.min;
            float bindMax = row.floatParam->meta.range.max;
            if (!std::isfinite(bindMin) || !std::isfinite(bindMax) || std::fabs(bindMax - bindMin) < 1e-6f) {
                bindMin = 0.0f;
                bindMax = 1.0f;
            }
            float bindStep = row.floatParam->meta.range.step;
            bool bindSnap = bindStep > 0.0f && std::fabs(std::round(bindStep) - bindStep) < 1e-4f;
            midiRouter_->bindFloat(row.id, valuePtr, bindMin, bindMax, bindSnap, bindStep);
        }
        float outMin = row.floatParam->meta.range.min;
        float outMax = row.floatParam->meta.range.max;
        if (!std::isfinite(outMin) || !std::isfinite(outMax) || std::fabs(outMax - outMin) < 1e-6f) {
            outMin = 0.0f;
            outMax = 1.0f;
        }
        float step = row.floatParam->meta.range.step;
        bool snapInt = step > 0.0f && std::fabs(std::round(step) - step) < 1e-4f;
        midiRouter_->setOrUpdateCc(row.id, binding->number, outMin, outMax, snapInt, step, meta);
        emitSlotBindingDiagnostics(row, *slot, *binding);
        return true;
    }

    if (row.boolParam) {
        if (row.boolParam->value) {
            midiRouter_->bindBool(row.id, row.boolParam->value, MidiRouter::BoolMode::Toggle);
        }
        float setValue = row.boolParam->baseValue ? 1.0f : 0.0f;
        midiRouter_->setOrUpdateBtn(row.id, binding->number, "toggle", setValue, meta);
        emitSlotBindingDiagnostics(row, *slot, *binding);
        return true;
    }
    return false;
}

inline void ControlMappingHubState::rebuildSlotMidiAssignments() const {
    if (!midiRouter_) {
        return;
    }
    slotMidiAssignmentsDirty_ = false;
    rebuildView();
    rebuildSlotMidiAssignmentsFromCurrentModel();
}

inline bool ControlMappingHubState::rebuildSlotMidiAssignmentsFromCurrentModel() const {
    if (!midiRouter_) {
        return false;
    }
    loadSlotAssignments();
    refreshSlotCatalog();
    bool applied = false;
    for (const auto& row : tableModel_.rows) {
        const auto* logical = logicalSlotBinding(row);
        if (!logical) {
            continue;
        }
        midiRouter_->removeMidiMappingsForTarget(row.id);
        applied = applySlotAssignmentToRow(row) || applied;
    }
    if (applied) {
        persistRoutingChange();
    }
    return applied;
}

inline void ControlMappingHubState::refreshConsoleSlotBindings() const {
    markConsoleSlotsDirty();
    rebuildSlotMidiAssignments();
}


inline const MidiRouter::OscMap* ControlMappingHubState::currentOscMap(const std::string& id) const {
    if (!midiRouter_ || id.empty()) {
        return nullptr;
    }
    return midiRouter_->findOscMap(id);
}

inline const MidiRouter::OscSourceProfile* ControlMappingHubState::currentOscSourceProfile(const std::string& address) const {
    if (!midiRouter_ || address.empty()) {
        return nullptr;
    }
    return midiRouter_->findOscSourceProfile(address);
}

inline std::string ControlMappingHubState::oscColumnKeyForIndex(int column) const {
    if (column >= 0 && column < static_cast<int>(skin_.oscPicker.editorColumns.size())) {
        return skin_.oscPicker.editorColumns[column].id;
    }
    if (column == 0) return "inMin";
    if (column == 1) return "inMax";
    if (column == 2) return "outMinFactor";
    if (column == 3) return "outMaxFactor";
    return std::string();
}

inline bool ControlMappingHubState::adjustCurrentOscMap(const ParameterRow& row, int column, float delta) const {
    if (!midiRouter_ || row.id.empty()) {
        return false;
    }
    const auto* map = currentOscMap(row.id);
    if (!map) {
        return false;
    }
    return adjustOscSourceProfile(map->pattern, column, delta);
}

inline bool ControlMappingHubState::adjustOscSourceProfile(const std::string& address, int column, float delta) const {
    if (!midiRouter_ || address.empty()) {
        return false;
    }
    if (std::fabs(delta) < 1e-6f) {
        return false;
    }
    float dInMin = 0.0f;
    float dInMax = 0.0f;
    float dOutMin = 0.0f;
    float dOutMax = 0.0f;
    float dSmooth = 0.0f;
    float dDeadband = 0.0f;
    std::string columnId = oscColumnKeyForIndex(column);
    if (column == static_cast<int>(Column::kSlot) || columnId == "inMin") {
        dInMin = delta;
    } else if (column == static_cast<int>(Column::kMidi) || columnId == "inMax") {
        dInMax = delta;
    } else if (column == static_cast<int>(Column::kMidiMin) || columnId == "outMinFactor") {
        dOutMin = delta;
    } else if (column == static_cast<int>(Column::kMidiMax) || columnId == "outMaxFactor") {
        dOutMax = delta;
    } else if (column == static_cast<int>(Column::kOsc)) {
        dSmooth = delta;
    } else if (column == static_cast<int>(Column::kOscDeadband)) {
        dDeadband = delta;
    } else {
        return false;
    }
    return midiRouter_->adjustOscSourceProfile(address, dInMin, dInMax, dOutMin, dOutMax, dSmooth, dDeadband);
}

inline bool ControlMappingHubState::applyOscPresetRange(const ParameterRow& row, float outMin, float outMax) const {
    if (!midiRouter_ || row.id.empty()) {
        return false;
    }
    const auto* map = currentOscMap(row.id);
    if (!map) {
        return false;
    }
    if (applyOscSourcePresetRange(map->pattern, outMin, outMax)) {
        emitTelemetryEvent("osc.range.preset", &row);
        persistRoutingChange();
        return true;
    }
    return false;
}

inline bool ControlMappingHubState::applyOscSourcePresetRange(const std::string& address, float outMin, float outMax) const {
    const auto* profile = currentOscSourceProfile(address);
    if (!profile) {
        return false;
    }
    float dOutMin = outMin - profile->outMin;
    float dOutMax = outMax - profile->outMax;
    if (std::fabs(dOutMin) < 1e-6f && std::fabs(dOutMax) < 1e-6f) {
        return false;
    }
    return midiRouter_->adjustOscSourceProfile(address, 0.0f, 0.0f, dOutMin, dOutMax, 0.0f, 0.0f);
}

inline bool ControlMappingHubState::flipCurrentOscOutputRange(const ParameterRow& row) const {
    if (!midiRouter_ || row.id.empty()) {
        return false;
    }
    const auto* map = currentOscMap(row.id);
    if (!map) {
        return false;
    }
    if (flipOscSourceOutputRange(map->pattern)) {
        emitTelemetryEvent("osc.range.flip", &row);
        persistRoutingChange();
        return true;
    }
    return false;
}

inline bool ControlMappingHubState::flipOscSourceOutputRange(const std::string& address) const {
    const auto* profile = currentOscSourceProfile(address);
    if (!profile) {
        return false;
    }
    float dOutMin = profile->outMax - profile->outMin;
    float dOutMax = profile->outMin - profile->outMax;
    if (std::fabs(dOutMin) < 1e-6f && std::fabs(dOutMax) < 1e-6f) {
        return false;
    }
    return midiRouter_->adjustOscSourceProfile(address, 0.0f, 0.0f, dOutMin, dOutMax, 0.0f, 0.0f);
}

inline bool ControlMappingHubState::adjustOscSmoothing(const ParameterRow& row, float delta) const {
    if (!midiRouter_ || row.id.empty() || std::fabs(delta) < 1e-6f) {
        return false;
    }
    const auto* map = currentOscMap(row.id);
    if (!map) {
        return false;
    }
    if (adjustOscSourceSmoothing(map->pattern, delta)) {
        emitTelemetryEvent("osc.range.smooth", &row);
        persistRoutingChange();
        return true;
    }
    return false;
}

inline bool ControlMappingHubState::adjustOscDeadband(const ParameterRow& row, float delta) const {
    if (!midiRouter_ || row.id.empty() || std::fabs(delta) < 1e-6f) {
        return false;
    }
    const auto* map = currentOscMap(row.id);
    if (!map) {
        return false;
    }
    if (adjustOscSourceDeadband(map->pattern, delta)) {
        emitTelemetryEvent("osc.range.deadband", &row);
        persistRoutingChange();
        return true;
    }
    return false;
}

inline bool ControlMappingHubState::adjustOscSourceSmoothing(const std::string& address, float delta) const {
    if (!midiRouter_ || address.empty() || std::fabs(delta) < 1e-6f) {
        return false;
    }
    return midiRouter_->adjustOscSourceProfile(address, 0.0f, 0.0f, 0.0f, 0.0f, delta, 0.0f);
}

inline bool ControlMappingHubState::adjustOscSourceDeadband(const std::string& address, float delta) const {
    if (!midiRouter_ || address.empty() || std::fabs(delta) < 1e-6f) {
        return false;
    }
    return midiRouter_->adjustOscSourceProfile(address, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, delta);
}

inline int ControlMappingHubState::oscEditorColumnCount() const {
    int count = static_cast<int>(skin_.oscPicker.editorColumns.size());
    return count > 0 ? count : 4;
}

inline bool ControlMappingHubState::beginOscValueEdit(const ParameterRow& row) const {
    if (!midiRouter_ || row.id.empty()) {
        return false;
    }
    const auto* map = currentOscMap(row.id);
    if (!map) {
        setBannerMessage("Bind OSC before editing ranges", 2000);
        return false;
    }
    std::string columnId = oscColumnKeyForIndex(oscEditColumn_);
    if (columnId != "inMin" && columnId != "inMax" && columnId != "outMinFactor" && columnId != "outMaxFactor") {
        setBannerMessage("Select an editable OSC column", 2000);
        return false;
    }
    oscValueEditActive_ = true;
    oscValueEditRowId_ = row.id;
    oscValueEditColumn_ = oscEditColumn_;
    float value = 0.0f;
    if (columnId == "inMin") value = map->inMin;
    else if (columnId == "inMax") value = map->inMax;
    else if (columnId == "outMinFactor") value = map->outMin;
    else if (columnId == "outMaxFactor") value = map->outMax;
    oscValueEditBuffer_ = ofToString(value, 4);
    oscValueEditOverwritePending_ = true;
    setBannerMessage("Type value, Enter=commit, Esc=cancel", 2200);
    return true;
}

inline bool ControlMappingHubState::beginOscSourceValueEdit(const std::string& address) const {
    const auto* profile = currentOscSourceProfile(address);
    if (!profile) {
        setBannerMessage("Select a configured OSC source", 2000);
        return false;
    }
    if (selectedColumn_ != Column::kSlot &&
        selectedColumn_ != Column::kMidi &&
        selectedColumn_ != Column::kMidiMin &&
        selectedColumn_ != Column::kMidiMax &&
        selectedColumn_ != Column::kOsc &&
        selectedColumn_ != Column::kOscDeadband) {
        setBannerMessage("Select an editable OSC column", 2000);
        return false;
    }
    oscValueEditActive_ = true;
    oscValueEditRowId_ = address;
    oscValueEditColumn_ = static_cast<int>(selectedColumn_);
    float value = 0.0f;
    if (selectedColumn_ == Column::kSlot) value = profile->inMin;
    else if (selectedColumn_ == Column::kMidi) value = profile->inMax;
    else if (selectedColumn_ == Column::kMidiMin) value = profile->outMin;
    else if (selectedColumn_ == Column::kMidiMax) value = profile->outMax;
    else if (selectedColumn_ == Column::kOsc) value = profile->smooth;
    else if (selectedColumn_ == Column::kOscDeadband) value = profile->deadband;
    oscValueEditBuffer_ = ofToString(value, 4);
    oscValueEditOverwritePending_ = true;
    setBannerMessage("Type value, Enter=commit, Esc=cancel", 2200);
    return true;
}

inline bool ControlMappingHubState::commitOscValueEdit(const ParameterRow& row) const {
    if (!oscValueEditActive_) {
        return false;
    }
    const auto* map = currentOscMap(row.id);
    if (!map) {
        cancelOscValueEdit();
        setBannerMessage("OSC mapping missing", 2000);
        return false;
    }
    std::string trimmed = ofTrim(oscValueEditBuffer_);
    if (trimmed.empty()) {
        setBannerMessage("Enter a numeric value", 2000);
        return false;
    }
    char* endPtr = nullptr;
    double parsed = std::strtod(trimmed.c_str(), &endPtr);
    if (!endPtr || *endPtr != '\0' || !std::isfinite(parsed)) {
        setBannerMessage("Invalid number format", 2000);
        return false;
    }
    float target = static_cast<float>(parsed);
    float dInMin = 0.0f;
    float dInMax = 0.0f;
    float dOutMin = 0.0f;
    float dOutMax = 0.0f;
    std::string columnId = oscColumnKeyForIndex(oscValueEditColumn_);
    if (columnId == "inMin") {
        dInMin = target - map->inMin;
    } else if (columnId == "inMax") {
        dInMax = target - map->inMax;
    } else if (columnId == "outMinFactor") {
        dOutMin = target - map->outMin;
    } else if (columnId == "outMaxFactor") {
        dOutMax = target - map->outMax;
    } else {
        return false;
    }
    if (std::fabs(dInMin) < 1e-6f && std::fabs(dInMax) < 1e-6f && std::fabs(dOutMin) < 1e-6f && std::fabs(dOutMax) < 1e-6f) {
        cancelOscValueEdit();
        return true;
    }
    if (midiRouter_->adjustOscMap(row.id, dInMin, dInMax, dOutMin, dOutMax, 0.0f, 0.0f)) {
        emitTelemetryEvent("osc.range.commit", &row);
        persistRoutingChange();
        cancelOscValueEdit();
        return true;
    }
    setBannerMessage("Failed to update OSC range", 2000);
    return false;
}

inline void ControlMappingHubState::cancelOscValueEdit() const {
    oscValueEditActive_ = false;
    oscValueEditRowId_.clear();
    oscValueEditBuffer_.clear();
    oscValueEditOverwritePending_ = false;
}

inline bool ControlMappingHubState::handleOscValueEditCharacter(int key) const {
    if (!oscValueEditActive_) {
        return false;
    }
    if (key == OF_KEY_BACKSPACE || key == OF_KEY_DEL) {
        if (!oscValueEditBuffer_.empty()) {
            oscValueEditBuffer_.pop_back();
        }
        if (oscValueEditBuffer_.empty()) {
            oscValueEditOverwritePending_ = true;
        }
        return true;
    }
    if ((key >= '0' && key <= '9') || key == '.' || key == '-' || key == '+') {
        if (oscValueEditOverwritePending_) {
            oscValueEditBuffer_.clear();
            oscValueEditOverwritePending_ = false;
        }
        oscValueEditBuffer_.push_back(static_cast<char>(key));
        return true;
    }
    return false;
}

inline bool ControlMappingHubState::commitOscSourceValueEdit(const std::string& address) const {
    if (!oscValueEditActive_) {
        return false;
    }
    const auto* profile = currentOscSourceProfile(address);
    if (!profile) {
        cancelOscValueEdit();
        setBannerMessage("OSC source profile missing", 2000);
        return false;
    }
    std::string trimmed = ofTrim(oscValueEditBuffer_);
    if (trimmed.empty()) {
        setBannerMessage("Enter a numeric value", 2000);
        return false;
    }
    char* endPtr = nullptr;
    double parsed = std::strtod(trimmed.c_str(), &endPtr);
    if (!endPtr || *endPtr != '\0' || !std::isfinite(parsed)) {
        setBannerMessage("Invalid number format", 2000);
        return false;
    }
    float target = static_cast<float>(parsed);
    float dInMin = 0.0f;
    float dInMax = 0.0f;
    float dOutMin = 0.0f;
    float dOutMax = 0.0f;
    float dSmooth = 0.0f;
    float dDeadband = 0.0f;
    if (oscValueEditColumn_ == static_cast<int>(Column::kSlot)) {
        dInMin = target - profile->inMin;
    } else if (oscValueEditColumn_ == static_cast<int>(Column::kMidi)) {
        dInMax = target - profile->inMax;
    } else if (oscValueEditColumn_ == static_cast<int>(Column::kMidiMin)) {
        dOutMin = target - profile->outMin;
    } else if (oscValueEditColumn_ == static_cast<int>(Column::kMidiMax)) {
        dOutMax = target - profile->outMax;
    } else if (oscValueEditColumn_ == static_cast<int>(Column::kOsc)) {
        dSmooth = target - profile->smooth;
    } else if (oscValueEditColumn_ == static_cast<int>(Column::kOscDeadband)) {
        dDeadband = target - profile->deadband;
    } else {
        return false;
    }
    if (std::fabs(dInMin) < 1e-6f && std::fabs(dInMax) < 1e-6f && std::fabs(dOutMin) < 1e-6f &&
        std::fabs(dOutMax) < 1e-6f && std::fabs(dSmooth) < 1e-6f && std::fabs(dDeadband) < 1e-6f) {
        cancelOscValueEdit();
        return true;
    }
    if (midiRouter_->adjustOscSourceProfile(address, dInMin, dInMax, dOutMin, dOutMax, dSmooth, dDeadband)) {
        persistRoutingChange();
        cancelOscValueEdit();
        return true;
    }
    setBannerMessage("Failed to update OSC range", 2000);
    return false;
}

inline void ControlMappingHubState::drawOscPickerPanel(float panelX,
                                                       float panelY,
                                                       float panelWidth,
                                                       float panelHeight,
                                                       const std::vector<MidiRouter::OscSourceInfo>& sources,
                                                       const ParameterRow* targetRow) const {
    ofPushStyle();
    ofSetColor(skin_.palette.surfaceAlternate);
    ofDrawRectRounded(panelX, panelY, panelWidth, panelHeight, skin_.metrics.borderRadius);

    std::string selectedAddress = selectedOscSourceAddress(sources);
    const auto* activeMap = (targetRow && !targetRow->id.empty()) ? currentOscMap(targetRow->id) : nullptr;
    const auto* activeProfile = !selectedAddress.empty() ? currentOscSourceProfile(selectedAddress)
                                                         : (activeMap ? currentOscSourceProfile(activeMap->pattern) : nullptr);
    int editorColumnCount = oscEditorColumnCount();
    if (editorColumnCount > 0) {
        oscEditColumn_ = (oscEditColumn_ % editorColumnCount + editorColumnCount) % editorColumnCount;
    }

    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    auto drawTextStyled = [textScale](const std::string& text, float x, float y, const ofColor& color, bool bold) {
        ofSetColor(color);
        drawBitmapStringScaled(text, x, y, textScale, bold);
    };

    auto computeColumns = [](const auto& descriptors,
                             float startX,
                             float width,
                             float minWidth,
                             auto& positions,
                             auto& widths) {
        float totalWeight = 0.0f;
        for (const auto& desc : descriptors) {
            totalWeight += std::max(0.01f, desc.weight);
        }
        float cursor = startX;
        for (std::size_t i = 0; i < descriptors.size(); ++i) {
            float normalized = std::max(0.01f, descriptors[i].weight) / totalWeight;
            float w = std::max(minWidth, width * normalized);
            positions[i] = cursor;
            widths[i] = w;
            cursor += w;
        }
    };

    std::vector<float> sourcePos(skin_.oscPicker.sourceColumns.size());
    std::vector<float> sourceWidths(sourcePos.size());
    computeColumns(skin_.oscPicker.sourceColumns,
                   panelX + 16.0f,
                   std::max(80.0f, panelWidth - 32.0f),
                   90.0f,
                   sourcePos,
                   sourceWidths);
    for (std::size_t i = 0; i < skin_.oscPicker.sourceColumns.size(); ++i) {
        drawTextStyled(skin_.oscPicker.sourceColumns[i].label,
                       sourcePos[i],
                       panelY + 20.0f,
                       skin_.palette.headerText,
                       true);
    }

    float rowY = panelY + 40.0f;
    float rowHeight = 18.0f * textScale;
    float availableListHeight = panelHeight - (rowY - panelY) - 8.0f;
    int maxRows = std::max(1, static_cast<int>(std::floor(availableListHeight / rowHeight)));
    uint64_t nowMs = ofGetElapsedTimeMillis();
    if (sources.empty()) {
        drawTextStyled("No OSC sources observed yet", panelX + 12.0f, rowY, skin_.palette.mutedText, false);
        rowY += rowHeight;
    } else {
        int total = static_cast<int>(sources.size());
        int maxStart = std::max(0, total - maxRows);
        if (oscPickerSelection_ < oscPickerScrollOffset_) {
            oscPickerScrollOffset_ = oscPickerSelection_;
        } else if (oscPickerSelection_ >= oscPickerScrollOffset_ + maxRows) {
            oscPickerScrollOffset_ = oscPickerSelection_ - (maxRows - 1);
        }
        oscPickerScrollOffset_ = std::clamp(oscPickerScrollOffset_, 0, maxStart);

        int startIndex = oscPickerScrollOffset_;
        int rowsToRender = std::min<int>(std::max(0, total - startIndex), maxRows);
        for (int idx = 0; idx < rowsToRender; ++idx) {
            int i = startIndex + idx;
            const auto& src = sources[i];
            bool selected = (static_cast<int>(i) == oscPickerSelection_);
            if (selected) {
                ofPushStyle();
                ofNoFill();
                ofSetColor(skin_.palette.gridSelection);
                ofDrawRectangle(panelX + 8.0f,
                                rowY - rowHeight + 4.0f,
                                panelWidth - 16.0f,
                                rowHeight);
                ofPopStyle();
            }
            float addrWidth = !sourceWidths.empty() ? sourceWidths[0] : std::max(80.0f, panelWidth * 0.5f);
            float addrX = !sourcePos.empty() ? sourcePos[0] : (panelX + 16.0f);
            float valueX = sourcePos.size() > 1 ? sourcePos[1] : (panelX + panelWidth * 0.5f);
            float valueWidth = sourceWidths.size() > 1 ? sourceWidths[1] : std::max(80.0f, panelWidth * 0.3f);
            std::string addr = ellipsize(src.address.empty() ? "(hint)" : src.address, addrWidth - 8.0f);
            bool outOfRange = false;
            if (activeProfile && src.seen) {
                float lo = std::min(activeProfile->inMin, activeProfile->inMax);
                float hi = std::max(activeProfile->inMin, activeProfile->inMax);
                outOfRange = (src.lastValue < lo || src.lastValue > hi);
            }
            std::string value = src.seen ? ofToString(src.lastValue, 3) : "-";
            if (src.seen && src.lastSeenMs) {
                uint64_t ageMs = (nowMs >= src.lastSeenMs) ? nowMs - src.lastSeenMs : 0;
                if (ageMs > 4000) {
                    value += " (" + ofToString(static_cast<float>(ageMs) / 1000.0f, 1) + "s)";
                }
            }
            drawTextStyled(addr,
                           addrX,
                           rowY,
                           selected ? skin_.palette.gridSelection : skin_.palette.bodyText,
                           selected);
            drawTextStyled(value,
                           valueX,
                           rowY,
                           outOfRange ? skin_.palette.warning : skin_.palette.bodyText,
                           false);
            rowY += rowHeight;
        }
        if (total > maxRows) {
            drawTextStyled("... more ...", sourcePos.empty() ? panelX + 16.0f : sourcePos[0], rowY, skin_.palette.mutedText, false);
            rowY += rowHeight;
        }
    }

    float editorY = panelY + panelHeight + 12.0f * textScale;
    float editorHeight = 110.0f * textScale;
    ofSetColor(skin_.palette.surface);
    ofDrawRectRounded(panelX, editorY, panelWidth, editorHeight, skin_.metrics.borderRadius);

    std::vector<float> mapPos(skin_.oscPicker.editorColumns.size());
    std::vector<float> mapWidths(mapPos.size());
    computeColumns(skin_.oscPicker.editorColumns,
                   panelX + 16.0f,
                   std::max(80.0f, panelWidth - 32.0f),
                   100.0f,
                   mapPos,
                   mapWidths);

    for (std::size_t i = 0; i < skin_.oscPicker.editorColumns.size(); ++i) {
        drawTextStyled(skin_.oscPicker.editorColumns[i].label,
                       mapPos[i],
                       editorY + 18.0f,
                       skin_.palette.headerText,
                       true);
    }

    std::vector<std::string> mapValues(mapPos.size(), "-");
    if (activeProfile) {
        for (std::size_t i = 0; i < skin_.oscPicker.editorColumns.size(); ++i) {
            const auto& column = skin_.oscPicker.editorColumns[i];
            if (column.id == "inMin") {
                mapValues[i] = ofToString(activeProfile->inMin, 2);
            } else if (column.id == "inMax") {
                mapValues[i] = ofToString(activeProfile->inMax, 2);
            } else if (column.id == "outMinFactor") {
                mapValues[i] = ofToString(activeProfile->outMin, 3);
            } else if (column.id == "outMaxFactor") {
                mapValues[i] = ofToString(activeProfile->outMax, 3);
            }
        }
    }

    std::string editId = targetRow ? targetRow->id : selectedAddress;
    if (oscValueEditActive_ && !editId.empty() && editId == oscValueEditRowId_) {
        if (oscValueEditColumn_ >= 0 && oscValueEditColumn_ < static_cast<int>(mapValues.size())) {
            mapValues[oscValueEditColumn_] = oscValueEditBuffer_.empty() ? std::string("0") : oscValueEditBuffer_;
        }
    }

    float mapRowY = editorY + 38.0f;
    for (std::size_t i = 0; i < mapValues.size(); ++i) {
        bool selected = (static_cast<int>(i) == oscEditColumn_);
        drawTextStyled(mapValues[i],
                       mapPos[i],
                       mapRowY,
                       selected ? skin_.palette.gridSelection : skin_.palette.bodyText,
                       selected);
    }

    std::string smoothing = activeProfile ? ofToString(activeProfile->smooth, 2) : "-";
    std::string deadband = activeProfile ? ofToString(activeProfile->deadband, 2) : "-";
    drawTextStyled("Smooth " + smoothing + " (s/S)    Deadband " + deadband + " (d/D)",
                   panelX + 16.0f,
                   editorY + editorHeight - 28.0f,
                   skin_.palette.mutedText,
                   false);

    std::string hint = targetRow
        ? "OSC: Up/Down source  Left/Right column  ,/. coarse  </> fine  1=wide  2=tight  3=unity  F=flip  s/S smooth  d/D deadband  L=edit  Enter=bind  Esc=close"
        : "OSC: Up/Down source  Left/Right column  ,/. coarse  </> fine  1=wide  2=tight  3=unity  F=flip  s/S smooth  d/D deadband  L=edit  Esc=close";
    drawTextStyled(hint, panelX, editorY + editorHeight + 18.0f, skin_.palette.mutedText, false);
    ofPopStyle();
}

inline bool ControlMappingHubState::parameterHasModifierConflict(const ParameterRow& row, modifier::Type type) const {
    const auto* param = row.isFloat ? static_cast<const void*>(row.floatParam) : static_cast<const void*>(row.boolParam);
    if (!param) {
        return false;
    }
    const auto* modifiers = row.isFloat ? &row.floatParam->modifiers : &row.boolParam->modifiers;
    for (const auto& runtime : *modifiers) {
        if (runtime.descriptor.type == type && runtime.conflict) {
            return true;
        }
    }
    return false;
}

inline std::string ControlMappingHubState::midiTakeoverBadge(const ParameterRow& row) const {
    if (row.id.empty()) {
        return std::string();
    }
    auto it = takeoverByTarget_.find(row.id);
    if (it == takeoverByTarget_.end()) {
        return std::string();
    }
    const auto& state = it->second;
    float delta = std::fabs(state.delta);
    if (delta > 0.02f) {
        return " [Pending]";
    }
    return " [Matched]";
}

inline std::string ControlMappingHubState::modifierActivityBadge(const ParameterRow& row, modifier::Type type) const {
    const auto* param = row.isFloat ? static_cast<const void*>(row.floatParam) : static_cast<const void*>(row.boolParam);
    if (!param) {
        return std::string();
    }
    const auto* modifiers = row.isFloat ? &row.floatParam->modifiers : &row.boolParam->modifiers;
    for (const auto& runtime : *modifiers) {
        if (runtime.descriptor.type != type) {
            continue;
        }
        if (!runtime.descriptor.enabled) {
            return " [Disabled]";
        }
        if (runtime.active && runtime.applied) {
            return " [Live]";
        }
        if (runtime.active) {
            return " [Input]";
        }
        return " [Idle]";
    }
    return std::string();
}

inline std::string ControlMappingHubState::activityHint(const ParameterRow& row) const {
    auto it = lastActivityMs_.find(row.id);
    if (it == lastActivityMs_.end()) {
        return std::string();
    }
    uint64_t now = ofGetElapsedTimeMillis();
    uint64_t ageMs = (now >= it->second) ? now - it->second : 0;
    if (ageMs > 5000) {
        return std::string();
    }
    float seconds = static_cast<float>(ageMs) / 1000.0f;
    return " (" + ofToString(seconds, 1) + "s)";
}

inline void ControlMappingHubState::setBannerMessage(const std::string& message, uint64_t durationMs) const {
    bannerMessage_ = message;
    bannerExpiryMs_ = ofGetElapsedTimeMillis() + durationMs;
}

inline void ControlMappingHubState::setActiveRowSlot(int slot) const {
    cancelHudColumnPicker();
    const auto& rows = activeRowIndices();
    if (oscSummaryActive()) {
        const auto sources = oscBrowserSources();
        if (sources.empty()) {
            oscPickerSelection_ = -1;
            cancelOscValueEdit();
            return;
        }
        slot = std::clamp(slot, 0, static_cast<int>(sources.size()) - 1);
        oscPickerSelection_ = slot;
        if (oscValueEditActive_ && sources[slot].address != oscValueEditRowId_) {
            cancelOscValueEdit();
        }
        return;
    }
    if (rows.empty()) {
        selectedRow_ = -1;
        cancelValueEdit();
        return;
    }
    slot = std::clamp(slot, 0, static_cast<int>(rows.size()) - 1);
    int newRowIndex = rows[slot];
    if (newRowIndex != selectedRow_) {
        cancelValueEdit();
        selectedRow_ = newRowIndex;
        routingPopoverVisible_ = false;
    }
}

inline bool ControlMappingHubState::readRowBoolValue(const ParameterRow& row, bool& outValue) const {
    if (row.boolParam && row.boolParam->value) {
        outValue = *row.boolParam->value;
        return true;
    }
    if (registry_) {
        if (const auto* param = registry_->findBool(row.id)) {
            outValue = param->value ? *param->value : param->baseValue;
            return true;
        }
    }
    return false;
}

inline void ControlMappingHubState::applyConsoleSlotAnnotations(ParameterRow& row) const {
    loadSlotAssignments();
    if (!row.isAsset || row.assetKey.empty()) {
        row.consoleSlots.clear();
        row.consoleSlotActive = false;
        static const std::string kConsolePrefix = "console.layer";
        if (row.id.rfind(kConsolePrefix, 0) == 0) {
            std::size_t pos = kConsolePrefix.size();
            std::string digits;
            while (pos < row.id.size() && std::isdigit(static_cast<unsigned char>(row.id[pos]))) {
                digits.push_back(row.id[pos]);
                ++pos;
            }
            if (!digits.empty()) {
                int columnIndex = ofToInt(digits);
                if (columnIndex > 0 && activeConsoleSlotIndices_.count(columnIndex) > 0) {
                    row.consoleSlots.push_back(columnIndex);
                    row.consoleSlotActive = true;
                }
            }
        }
        return;
    }
    row.consoleSlots.clear();
    row.consoleSlotActive = false;
    auto sit = consoleSlotsByAssetKey_.find(row.assetKey);
    if (sit == consoleSlotsByAssetKey_.end()) {
        return;
    }
    std::vector<int> slots;
    slots.reserve(sit->second.size());
    for (const auto& ref : sit->second) {
        if (ref.slotIndex > 0) {
            slots.push_back(ref.slotIndex);
        }
        if (ref.active) {
            row.consoleSlotActive = true;
        }
    }
    std::sort(slots.begin(), slots.end());
    slots.erase(std::unique(slots.begin(), slots.end()), slots.end());
    row.consoleSlots = std::move(slots);
}

inline bool ControlMappingHubState::readRowFloatValue(const ParameterRow& row, float& outValue) const {
    if (row.floatParam && row.floatParam->value) {
        outValue = *row.floatParam->value;
        return true;
    }
    if (registry_) {
        if (const auto* param = registry_->findFloat(row.id)) {
            outValue = param->value ? *param->value : param->baseValue;
            return true;
        }
    }
    if (isBioAmpRowId(row.id)) {
        uint64_t dummyTs = 0;
        if (readBioAmpValue(row.id, outValue, dummyTs)) {
            return true;
        }
    }
    return false;
}

inline bool ControlMappingHubState::setRowBoolValue(const ParameterRow& row, bool value) const {
    if (row.isHudLayoutEntry) {
        return false;
    }
    bool changed = false;
    if (registry_) {
        if (registry_->findBool(row.id)) {
            bool current = registry_->getBoolBase(row.id);
            if (current != value) {
                registry_->setBoolBase(row.id, value, true);
                changed = true;
            }
        }
    }
    if (!changed && row.boolParam && row.boolParam->value) {
        if (*row.boolParam->value != value || row.boolParam->baseValue != value) {
            *row.boolParam->value = value;
            const_cast<ParameterRegistry::BoolParam*>(row.boolParam)->baseValue = value;
            changed = true;
        }
    }
    if (row.id.rfind("hud.", 0) == 0 && changed) {
        updateHudVisibilityPreference(row.id, value);
        if (hudToggleCallback_) {
            hudToggleCallback_(row.id, value);
        }
        emitHudMappingEvent("toggle", row.id);
    }
    return changed;
}

inline bool ControlMappingHubState::setRowFloatValue(const ParameterRow& row, float value) const {
    if (row.isHudLayoutEntry) {
        return false;
    }
    if (registry_) {
        if (registry_->findFloat(row.id)) {
            float current = registry_->getFloatBase(row.id);
            if (current != value) {
                registry_->setFloatBase(row.id, value, true);
                if (floatValueCommitCallback_) {
                    floatValueCommitCallback_(row.id, value);
                }
                return true;
            }
            return false;
        }
    }
    if (row.floatParam && row.floatParam->value) {
        if (*row.floatParam->value != value || row.floatParam->baseValue != value) {
            *row.floatParam->value = value;
            const_cast<ParameterRegistry::FloatParam*>(row.floatParam)->baseValue = value;
            return true;
        }
    }
    return false;
}

inline bool ControlMappingHubState::setRowStringValue(const ParameterRow& row, const std::string& value) const {
    if (row.isHudLayoutEntry) {
        return false;
    }
    if (row.isSavedSceneSaveRow) {
        if (!savedSceneSaveAsCallback_) {
            return false;
        }
        if (!savedSceneSaveAsCallback_(value, false)) {
            return false;
        }
        savedSceneDraftName_ = value;
        tableModel_.dirty = true;
        return true;
    }
    if (registry_) {
        if (registry_->findString(row.id)) {
            registry_->setStringBase(row.id, value, true);
            return true;
        }
    }
    if (row.stringParam && row.stringParam->value) {
        if (*row.stringParam->value != value || row.stringParam->baseValue != value) {
            *row.stringParam->value = value;
            const_cast<ParameterRegistry::StringParam*>(row.stringParam)->baseValue = value;
            return true;
        }
    }
    return false;
}

inline void ControlMappingHubState::drawRoutingPopover(float x,
                                                       float y,
                                                       float width,
                                                       float height,
                                                       const ParameterRow* row) const {
    ofPushStyle();
    ofSetColor(skin_.palette.surface);
    ofDrawRectRounded(x, y, width, height, skin_.metrics.borderRadius);
    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    float padding = 12.0f;
    float lineHeight = 18.0f * textScale;
    float cursorY = y + padding + lineHeight;
    auto drawText = [textScale](const std::string& text, float px, float py, const ofColor& color, bool bold) {
        ofSetColor(color);
        drawBitmapStringScaled(text, px, py, textScale, bold);
    };
    drawText("Parameter", x + padding, cursorY, skin_.palette.headerText, true);
    cursorY += lineHeight;
    drawText(row->label + " (" + row->id + ")", x + padding, cursorY, skin_.palette.bodyText, false);
    cursorY += lineHeight * 1.5f;
    drawText("Mappings", x + padding, cursorY, skin_.palette.headerText, true);
    cursorY += lineHeight;
    std::string midi = formatMidiSummary(*row);
    std::string osc = formatOscSummary(*row);
    drawText("MIDI: " + midi, x + padding, cursorY, skin_.palette.bodyText, false);
    cursorY += lineHeight;
    drawText("OSC:  " + osc, x + padding, cursorY, skin_.palette.bodyText, false);
    cursorY += lineHeight * 1.5f;
    drawText("Conflicts", x + padding, cursorY, skin_.palette.headerText, true);
    cursorY += lineHeight;
    std::string conflicts;
    if (parameterHasModifierConflict(*row, modifier::Type::kMidiCc) ||
        parameterHasModifierConflict(*row, modifier::Type::kMidiNote) ||
        parameterHasModifierConflict(*row, modifier::Type::kOsc)) {
        conflicts = "Present";
    } else {
        conflicts = "None";
    }
    drawText(conflicts,
             x + padding,
             cursorY,
             conflicts == "None" ? skin_.palette.bodyText : skin_.palette.warning,
             false);
    cursorY += lineHeight * 1.5f;
    drawText("Hints", x + padding, cursorY, skin_.palette.headerText, true);
    cursorY += lineHeight;
    drawText("Enter: edit/learn", x + padding, cursorY, skin_.palette.mutedText, false);
    cursorY += lineHeight;
    drawText("U: unmap  P: close", x + padding, cursorY, skin_.palette.mutedText, false);
    ofPopStyle();
}

// emitTelemetryEvent is implemented earlier in the header (single canonical definition).

inline void ControlMappingHubState::persistRoutingChange() const {
    if (!midiRouter_) {
        return;
    }
    if (routingCommitInProgress_) {
        return;
    }
    routingCommitInProgress_ = true;
    bool saved = midiRouter_->save("");
    if (!saved) {
        ofLogWarning("ControlMappingHub") << "Failed to persist MIDI/OSC map";
        rollbackOffered_ = routingRollbackAction_ != nullptr;
        setBannerMessage("Failed to write routing changes", 3200);
        routingCommitInProgress_ = false;
        return;
    }
    // emit telemetry that routing was persisted to disk
    emitTelemetryEvent("routing.persist", nullptr, "saved");
    bool valid = runControlHubValidation();
    if (!valid) {
        rollbackOffered_ = routingRollbackAction_ != nullptr;
        if (rollbackOffered_) {
            setBannerMessage("Validation failed. Press R to rollback.", 4200);
        } else {
            setBannerMessage("Validation failed. Check logs.", 4200);
        }
    } else {
        rollbackOffered_ = false;
    }
    if (!valid) {
        // emit a telemetry event indicating validation failure
        emitTelemetryEvent("routing.validation.failed", nullptr, "validation_failed");
    }
    routingCommitInProgress_ = false;
}

inline bool ControlMappingHubState::runControlHubValidation() const {
    auto script = ofFilePath::getAbsolutePath("tools/validate_configs.py");
    if (!ofFile::doesFileExist(script)) {
        return true;
    }
    const std::string validationCommand = "python \"" + script + "\" --control-hub";
    const int validationExitCode = std::system(validationCommand.c_str());
    if (validationExitCode != 0) {
        ofLogWarning("ControlMappingHub") << "browser validation failed (exit " << validationExitCode << ")";
        return false;
    }
    return true;
}



inline const ControlMappingHubState::ParameterRow* ControlMappingHubState::selectedRowData() const {
    if (selectedRow_ < 0 || selectedRow_ >= static_cast<int>(tableModel_.rows.size())) {
        return nullptr;
    }
    return &tableModel_.rows[selectedRow_];
}

inline std::string ControlMappingHubState::selectedAssetId() const {
    const auto* row = selectedRowData();
    if (!row || row->assetKey.empty()) {
        return std::string();
    }
    auto it = assetCatalog_.find(row->assetKey);
    if (it != assetCatalog_.end() && !it->second.assetId.empty()) {
        return it->second.assetId;
    }
    return row->assetKey;
}

inline std::string ControlMappingHubState::selectedAssetLabel() const {
    const auto* row = selectedRowData();
    if (!row) {
        return std::string();
    }
    if (!row->assetLabel.empty()) {
        return row->assetLabel;
    }
    if (!row->subcategory.empty()) {
        return row->subcategory;
    }
    return row->label;
}

inline const ControlMappingHubState::ParameterRow* ControlMappingHubState::rowForId(const std::string& id) const {
    auto it = std::find_if(tableModel_.rows.begin(), tableModel_.rows.end(), [&](const ParameterRow& candidate) {
        return candidate.id == id;
    });
    if (it == tableModel_.rows.end()) {
        return nullptr;
    }
    return &(*it);
}

inline bool ControlMappingHubState::debugRowIsAsset(const std::string& id) const {
    rebuildView();
    const auto* row = rowForId(id);
    return row ? row->isAsset : false;
}

inline std::string ControlMappingHubState::debugValueForRow(const std::string& id) const {
    rebuildView();
    const auto* row = rowForId(id);
    if (!row) {
        return std::string();
    }
    return formatValue(*row);
}

inline bool ControlMappingHubState::debugSetHudColumnSelection(const std::string& id, int selectionIndex) {
    rebuildView();
    const auto* row = rowForId(id);
    if (!row) {
        return false;
    }
    if (!beginHudColumnPicker(*row)) {
        return false;
    }
    hudColumnPickerSelection_ = selectionIndex;
    return applyHudColumnSelection();
}

inline bool ControlMappingHubState::debugBeginMidiLearn(const std::string& id) {
    rebuildView();
    const auto* row = rowForId(id);
    if (!row) {
        return false;
    }
    return beginMidiLearn(*row);
}

inline bool ControlMappingHubState::debugSetGridSelection(int rowIndex, int columnIndex) {
    rebuildView();
    const auto& rows = activeRowIndices();
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) {
        return false;
    }
    focusPane_ = FocusPane::kGrid;
    selectedRow_ = rows[rowIndex];
    if (columnIndex < 0) {
        columnIndex = 0;
    }
    if (columnIndex >= static_cast<int>(Column::kCount)) {
        columnIndex = static_cast<int>(Column::kCount) - 1;
    }
    selectedColumn_ = static_cast<Column>(columnIndex);
    return true;
}

inline bool ControlMappingHubState::debugSlotPickerVisible() const {
    return slotPickerVisible_;
}

inline const MidiRouter::CcMap* ControlMappingHubState::firstCcMapForRow(const ParameterRow& row) const {
    if (!midiRouter_) {
        return nullptr;
    }
    const auto& maps = midiRouter_->getCcMaps();
    auto it = std::find_if(maps.begin(), maps.end(), [&](const MidiRouter::CcMap& map) {
        return map.target == row.id;
    });
    return it != maps.end() ? &(*it) : nullptr;
}

inline void ControlMappingHubState::beginValueEdit(const ParameterRow& row) const {
    if (!row.isFloat && !row.isString) {
        cancelValueEdit();
        return;
    }
    routingPopoverVisible_ = false;
    editingValueActive_ = true;
    editingValueRowId_ = row.id;
    editingValueError_ = false;
    editingValueErrorMessage_.clear();
    auto pending = pendingValueBuffers_.find(row.id);
    if (pending != pendingValueBuffers_.end()) {
        editingValueBuffer_ = pending->second;
        editingValueOverwritePending_ = editingValueBuffer_.empty();
    } else {
        if (row.isFloat && row.floatParam && row.floatParam->value) {
            editingValueBuffer_ = ofToString(*row.floatParam->value, 2);
        } else if (row.isSavedSceneSaveRow) {
            editingValueBuffer_ = savedSceneDraftName_;
        } else if (row.isString && row.stringParam && row.stringParam->value) {
            editingValueBuffer_ = *row.stringParam->value;
        } else {
            editingValueBuffer_.clear();
        }
        editingValueOverwritePending_ = true;
    }
}

inline void ControlMappingHubState::cancelValueEdit(bool clearPending) const {
    if (editingValueActive_ && !editingValueRowId_.empty()) {
        if (clearPending) {
            pendingValueBuffers_.erase(editingValueRowId_);
        } else if (!editingValueBuffer_.empty()) {
            pendingValueBuffers_[editingValueRowId_] = editingValueBuffer_;
        }
    }
    editingValueActive_ = false;
    editingValueRowId_.clear();
    editingValueBuffer_.clear();
    editingValueError_ = false;
    editingValueErrorMessage_.clear();
    editingValueOverwritePending_ = false;
}

inline bool ControlMappingHubState::commitValueEdit() {
    if (!editingValueActive_) {
        return false;
    }
    const auto* row = rowForId(editingValueRowId_);
    if (!row || (!row->isFloat && !row->isString)) {
        cancelValueEdit();
        return false;
    }
    auto trimWhitespace = [](std::string value) {
        auto notSpaceFront = std::find_if(value.begin(), value.end(), [](unsigned char ch) {
            return !std::isspace(static_cast<unsigned char>(ch));
        });
        value.erase(value.begin(), notSpaceFront);
        auto notSpaceBack = std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
            return !std::isspace(static_cast<unsigned char>(ch));
        });
        value.erase(notSpaceBack.base(), value.end());
        return value;
    };
    std::string trimmed = trimWhitespace(editingValueBuffer_);
    if (row->isString) {
        if (row->isSavedSceneSaveRow && trimmed.empty()) {
            editingValueError_ = true;
            editingValueErrorMessage_ = "Scene name required";
            setBannerMessage("Enter a scene name");
            return false;
        }
        // allow empty strings for text overlay
        if (!setRowStringValue(*row, trimmed)) {
            editingValueError_ = true;
            editingValueErrorMessage_ = row->isSavedSceneSaveRow
                ? "Unable to save scene"
                : "Unable to update value";
            setBannerMessage(row->isSavedSceneSaveRow
                ? "Scene save failed"
                : "Unable to update value");
            return false;
        }
        if (row->isSavedSceneSaveRow) {
            markConsoleSlotsDirty();
            setBannerMessage("Saved scene as " + trimmed, 2200);
        }
        emitTelemetryEvent("value.commit", row, trimmed);
    } else {
        if (trimmed.empty()) {
            editingValueError_ = true;
            editingValueErrorMessage_ = "Value required";
            setBannerMessage("Enter a numeric value");
            return false;
        }
        char* endPtr = nullptr;
        double parsed = std::strtod(trimmed.c_str(), &endPtr);
        if (!endPtr || *endPtr != '\0' || !std::isfinite(parsed)) {
            editingValueError_ = true;
            editingValueErrorMessage_ = "Invalid number";
            setBannerMessage("Invalid number format");
            return false;
        }
        float numeric = static_cast<float>(parsed);
        if (!setRowFloatValue(*row, numeric)) {
            editingValueError_ = true;
            editingValueErrorMessage_ = "Unable to update value";
            setBannerMessage("Unable to update value");
            return false;
        }
        // emit telemetry for committed value edits so host/HUD can react
        emitTelemetryEvent("value.commit", row, ofToString(numeric, 2));
    }
    editingValueActive_ = false;
    editingValueRowId_.clear();
    editingValueBuffer_.clear();
    editingValueError_ = false;
    editingValueOverwritePending_ = false;
    editingValueErrorMessage_.clear();
    pendingValueBuffers_.erase(row->id);
    tableModel_.dirty = true;
    invalidateRowCache();
    return true;
}

inline bool ControlMappingHubState::handleValueEditCharacter(int key) {
    if (!editingValueActive_) {
        return false;
    }
    if (key == OF_KEY_BACKSPACE || key == OF_KEY_DEL) {
        if (!editingValueBuffer_.empty()) {
            editingValueBuffer_.pop_back();
        }
        editingValueError_ = false;
        editingValueErrorMessage_.clear();
        if (editingValueBuffer_.empty()) {
            editingValueOverwritePending_ = true;
        }
        if (!editingValueRowId_.empty()) {
            pendingValueBuffers_[editingValueRowId_] = editingValueBuffer_;
        }
        return true;
    }
    // If editing a string parameter, accept printable characters (ASCII range)
    const auto* row = rowForId(editingValueRowId_);
    if (row && row->isString) {
        if (key >= 32 && key <= 126) {
            if (editingValueOverwritePending_) {
                editingValueBuffer_.clear();
                editingValueOverwritePending_ = false;
            }
            editingValueBuffer_.push_back(static_cast<char>(key));
            editingValueError_ = false;
            editingValueErrorMessage_.clear();
            if (!editingValueRowId_.empty()) {
                pendingValueBuffers_[editingValueRowId_] = editingValueBuffer_;
            }
            return true;
        }
        return false;
    }

    if ((key >= '0' && key <= '9') || key == '.' || key == '-' || key == '+') {
        if (editingValueOverwritePending_) {
            editingValueBuffer_.clear();
            editingValueOverwritePending_ = false;
        }
        editingValueBuffer_.push_back(static_cast<char>(key));
        editingValueError_ = false;
        editingValueErrorMessage_.clear();
        if (!editingValueRowId_.empty()) {
            pendingValueBuffers_[editingValueRowId_] = editingValueBuffer_;
        }
        return true;
    }
    return false;
}

inline void ControlMappingHubState::refreshValueEditTarget() const {
    if (!editingValueActive_) {
        return;
    }
    const auto* row = rowForId(editingValueRowId_);
    if (!row || (!row->isFloat && !row->isString)) {
        cancelValueEdit();
    }
}

inline bool ControlMappingHubState::beginMidiLearn(const ParameterRow& row) const {
    if (!rowHasLiveParameter(row)) {
        setBannerMessage("Select an asset parameter before routing MIDI", 2400);
        return false;
    }
    if (!midiRouter_) {
        ofLogWarning("ControlMappingHub") << "Cannot start MIDI learn without an active MidiRouter";
        return false;
    }
    midiRouter_->beginLearn(row.id);
    ofLogNotice("ControlMappingHub") << "MIDI learn armed for " << row.id;
    // notify telemetry that a MIDI learn was armed
    emitTelemetryEvent("midi.learn.arm", &row);
    return true;
}

inline bool ControlMappingHubState::beginOscLearn(const ParameterRow& row) const {
    if (!rowHasLiveParameter(row)) {
        setBannerMessage("Select an asset parameter before routing OSC", 2400);
        return false;
    }
    if (!midiRouter_) {
        ofLogWarning("ControlMappingHub") << "Cannot start OSC learn without an active MidiRouter";
        return false;
    }
    midiRouter_->beginLearn(row.id, true);
    ofLogNotice("ControlMappingHub") << "OSC learn armed for " << row.id;
    return true;
}

inline bool ControlMappingHubState::adjustMidiRange(const ParameterRow& row, float dMin, float dMax) const {
    if (!rowHasLiveParameter(row)) {
        setBannerMessage("Select an asset parameter before adjusting ranges", 2200);
        return false;
    }
    if (!midiRouter_) {
        return false;
    }
    if (!firstCcMapForRow(row)) {
        return false;
    }
    midiRouter_->adjustCcRange(row.id, dMin, dMax);
    emitTelemetryEvent("midi.range.adjust",
                       &row,
                       "dMin=" + ofToString(dMin, 2) + ",dMax=" + ofToString(dMax, 2));
    persistRoutingChange();
    return true;
}

inline std::string ControlMappingHubState::subcategoryForId(const std::string& id) const {
    auto pos = id.find_last_of('.');
    if (pos == std::string::npos) {
        return std::string();
    }
    std::string scope = id.substr(0, pos);
    auto tokens = ofSplitString(scope, ".", true, true);
    if (tokens.empty()) {
        return scope;
    }
    std::size_t start = 0;
    std::string firstLower = ofToLower(tokens[0]);
    if (firstLower.rfind("deck", 0) == 0 && tokens.size() > 1) {
        start = 1;
    }
    std::string rebuilt;
    for (std::size_t i = start; i < tokens.size(); ++i) {
        if (!rebuilt.empty()) {
            rebuilt += ".";
        }
        rebuilt += tokens[i];
    }
    return rebuilt.empty() ? scope : rebuilt;
}

inline void ControlMappingHubState::moveRow(int delta) const {
    cancelHudColumnPicker();
    cancelSlotPicker();
    if (oscSummaryActive()) {
        const auto sources = oscBrowserSources();
        if (sources.empty()) {
            oscPickerSelection_ = -1;
            cancelOscValueEdit();
            return;
        }
        int slot = activeRowSlot();
        if (slot < 0) {
            slot = 0;
        }
        int count = static_cast<int>(sources.size());
        slot = (slot + delta + count) % count;
        oscPickerSelection_ = slot;
        if (oscValueEditActive_ && sources[slot].address != oscValueEditRowId_) {
            cancelOscValueEdit();
        }
        return;
    }
    const auto& rows = activeRowIndices();
    if (rows.empty()) {
        selectedRow_ = -1;
        cancelValueEdit();
        return;
    }
    int slot = activeRowSlot();
    if (slot < 0) {
        slot = 0;
    }
    int count = static_cast<int>(rows.size());
    slot = (slot + delta + count) % count;
    selectedRow_ = rows[slot];
    if (editingValueActive_) {
        const auto& row = tableModel_.rows[selectedRow_];
        if (row.id != editingValueRowId_) {
            cancelValueEdit();
        }
    }
}

inline void ControlMappingHubState::moveColumn(int delta) {
    cancelHudColumnPicker();
    cancelSlotPicker();
    auto visible = visibleColumns();
    if (visible.empty()) {
        selectedColumn_ = Column::kName;
        cancelValueEdit();
        return;
    }
    int idx = static_cast<int>(selectedColumn_);
    auto it = std::find(visible.begin(), visible.end(), idx);
    int pos = (it == visible.end()) ? 0 : static_cast<int>(std::distance(visible.begin(), it));
    int count = static_cast<int>(visible.size());
    pos = (pos + delta + count) % count;
    selectedColumn_ = static_cast<Column>(visible[pos]);
    if (selectedColumn_ != Column::kValue) {
        cancelValueEdit(false);
    }
}

inline bool ControlMappingHubState::activateSelection(MenuController& controller) {
    const auto& rows = activeRowIndices();
    if (rows.empty()) {
        return false;
    }
    const ParameterRow* row = selectedRowData();
    switch (selectedColumn_) {
    case Column::kMidi:
        if (midiAction_) {
            midiAction_(controller);
            return true;
        }
        break;
    case Column::kOsc:
        if (oscAction_) {
            oscAction_(controller);
            return true;
        }
        break;
    default:
        break;
    }
    ofLogNotice("ControlMappingHub") << "Cell editing not yet implemented for column " << static_cast<int>(selectedColumn_);
    return false;
}





inline void ControlMappingHubState::invalidateRowCache() const {
    activeRowSet_ = nullptr;
}

inline const std::vector<int>& ControlMappingHubState::activeRowIndices() const {
    const auto* rows = resolveActiveRowSet();
    if (rows) {
        return *rows;
    }
    static const std::vector<int> kEmpty;
    return kEmpty;
}

inline const std::vector<int>* ControlMappingHubState::resolveActiveRowSet() const {
    if (activeRowSet_) {
        return activeRowSet_;
    }
    if (oscSummaryActive()) {
        static const std::vector<int> kEmpty;
        activeRowSet_ = &kEmpty;
        return activeRowSet_;
    }
    if (tableModel_.tree.empty()) {
        activeRowSet_ = &tableModel_.allRowIndices;
        return activeRowSet_;
    }
    if (selectedTreeNodeIndex_ < 0 || selectedTreeNodeIndex_ >= static_cast<int>(tableModel_.tree.size())) {
        activeRowSet_ = &tableModel_.allRowIndices;
        return activeRowSet_;
    }
    const auto& node = tableModel_.tree[selectedTreeNodeIndex_];
    if (node.categoryIndex < 0) {
        activeRowSet_ = &tableModel_.allRowIndices;
        return activeRowSet_;
    }
    if (node.categoryIndex >= static_cast<int>(tableModel_.categories.size())) {
        activeRowSet_ = &tableModel_.allRowIndices;
        return activeRowSet_;
    }
    if (node.subcategoryIndex >= 0 && node.assetGroupIndex >= 0) {
        const auto& subcats = tableModel_.categories[node.categoryIndex].subcategories;
        if (node.subcategoryIndex >= static_cast<int>(subcats.size())) {
            activeRowSet_ = &tableModel_.categories[node.categoryIndex].rowIndices;
            return activeRowSet_;
        }
        const auto& assetGroups = subcats[node.subcategoryIndex].assetGroups;
        if (node.assetGroupIndex >= 0 && node.assetGroupIndex < static_cast<int>(assetGroups.size())) {
            activeRowSet_ = &assetGroups[node.assetGroupIndex].rowIndices;
            return activeRowSet_;
        }
    }
    if (node.subcategoryIndex < 0) {
        const auto& category = tableModel_.categories[node.categoryIndex];
        if (category.subcategories.empty()) {
            activeRowSet_ = &category.rowIndices;
        } else {
            static const std::vector<int> kEmpty;
            activeRowSet_ = &kEmpty;
        }
        return activeRowSet_;
    }
    const auto& subcats = tableModel_.categories[node.categoryIndex].subcategories;
    if (node.subcategoryIndex >= static_cast<int>(subcats.size())) {
        activeRowSet_ = &tableModel_.categories[node.categoryIndex].rowIndices;
        return activeRowSet_;
    }
    activeRowSet_ = &subcats[node.subcategoryIndex].rowIndices;
    return activeRowSet_;
}

inline int ControlMappingHubState::activeRowSlot() const {
    if (oscSummaryActive()) {
        const auto sources = oscBrowserSources();
        if (sources.empty() || oscPickerSelection_ < 0) {
            return -1;
        }
        return std::clamp(oscPickerSelection_, 0, static_cast<int>(sources.size()) - 1);
    }
    const auto& rows = activeRowIndices();
    if (rows.empty() || selectedRow_ < 0) {
        return -1;
    }
    auto it = std::find(rows.begin(), rows.end(), selectedRow_);
    if (it == rows.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(rows.begin(), it));
}

inline std::vector<int> ControlMappingHubState::visibleColumns() const {
    if (oscSummaryActive()) {
        return {
            static_cast<int>(Column::kName),
            static_cast<int>(Column::kValue),
            static_cast<int>(Column::kSlot),
            static_cast<int>(Column::kMidi),
            static_cast<int>(Column::kMidiMin),
            static_cast<int>(Column::kMidiMax),
            static_cast<int>(Column::kOsc),
            static_cast<int>(Column::kOscDeadband)
        };
    }
    std::vector<int> visible;
    for (int i = 0; i < static_cast<int>(Column::kCount); ++i) {
        if (preferences_.columnVisibility[i]) {
            visible.push_back(i);
        }
    }
    if (visible.empty()) {
        visible.push_back(static_cast<int>(Column::kName));
    }
    return visible;
}

inline ControlMappingHubState::VisibleRange ControlMappingHubState::computeVisibleRange(int totalRows,
                                                                                       int anchorSlot,
                                                                                       float viewportHeight,
                                                                                       float rowHeight) const {
    VisibleRange range;
    if (totalRows <= 0) {
        range.start = 0;
        range.end = -1;
        range.capacity = 0;
        return range;
    }
    if (viewportHeight <= 0.0f || rowHeight <= 0.0f) {
        range.capacity = std::max(1, totalRows);
        range.start = 0;
        range.end = totalRows - 1;
        return range;
    }
    int capacity = std::max(1, static_cast<int>(viewportHeight / rowHeight));
    range.capacity = capacity;
    if (anchorSlot < 0) {
        anchorSlot = 0;
    }
    int maxStart = std::max(0, totalRows - capacity);
    int start = std::clamp(anchorSlot - capacity / 2, 0, maxStart);
    range.start = start;
    range.end = std::min(totalRows - 1, start + capacity + 2);
    return range;
}

inline ControlMappingHubState::LayoutContext ControlMappingHubState::computeLayoutContext(float viewportWidth,
                                                                                          float viewportHeight) const {
    LayoutContext ctx;
    ctx.viewportWidth = viewportWidth;
    ctx.viewportHeight = viewportHeight;
    const float margin = skin_.metrics.margin;
    const float width = std::max(60.0f, viewportWidth - margin * 2.0f);
    const float height = std::max(60.0f, viewportHeight - margin * 2.0f);
    ctx.width = width;
    ctx.height = height;
    ctx.treeX = margin;
    ctx.treeY = margin;
    float maxTreeWidth = width - skin_.metrics.gridMinWidth - skin_.metrics.panelSpacing;
    if (maxTreeWidth < skin_.metrics.treeMinWidth) {
        maxTreeWidth = std::max(skin_.metrics.treeMinWidth, width * 0.4f);
    }
    float desiredTreeWidth = width * preferences_.treeWidthRatio;
    ctx.treeWidth = std::clamp(desiredTreeWidth, skin_.metrics.treeMinWidth, maxTreeWidth);
    ctx.treeHeight = height;
    ctx.gridX = ctx.treeX + ctx.treeWidth + skin_.metrics.panelSpacing;
    ctx.gridY = ctx.treeY;
    ctx.gridWidth = std::max(40.0f, width - ctx.treeWidth - skin_.metrics.panelSpacing);
    ctx.gridHeight = height;
    ctx.treeHeaderHeight = skin_.metrics.columnHeaderHeight;
    ctx.headerBaseline = ctx.treeY + ctx.treeHeaderHeight - 6.0f;
    ctx.treeBodyY = ctx.treeY + ctx.treeHeaderHeight;
    ctx.treeBodyHeight = std::max(20.0f, ctx.treeHeight - ctx.treeHeaderHeight);
    ctx.treeRowHeight = std::max(14.0f, skin_.metrics.treeRowHeight);
    ctx.gridRowHeight = std::max(14.0f, skin_.metrics.rowHeight);
    ctx.treeVisibleRows = std::max(1, static_cast<int>(ctx.treeBodyHeight / ctx.treeRowHeight));
    float gridBodyHeight = ctx.gridHeight - ctx.treeHeaderHeight;
    ctx.gridVisibleRows = std::max(1, static_cast<int>(gridBodyHeight / ctx.gridRowHeight));
    ensureTreeSelectionVisible(ctx.treeVisibleRows);
    ensureGridSelectionVisible(ctx.gridVisibleRows);
    return ctx;
}

inline void ControlMappingHubState::ensureTreeSelectionVisible(int visibleRows) const {
    int count = static_cast<int>(tableModel_.tree.size());
    if (count <= 0) {
        treeScrollOffset_ = 0;
        return;
    }
    visibleRows = std::max(1, visibleRows);
    int selection = selectedTreeNodeIndex_;
    if (selection < 0) {
        selection = 0;
    }
    if (selection < treeScrollOffset_) {
        treeScrollOffset_ = selection;
    } else if (selection >= treeScrollOffset_ + visibleRows) {
        treeScrollOffset_ = selection - visibleRows + 1;
    }
    int maxOffset = std::max(0, count - visibleRows);
    treeScrollOffset_ = std::clamp(treeScrollOffset_, 0, maxOffset);
}

inline void ControlMappingHubState::ensureGridSelectionVisible(int visibleRows) const {
    if (oscSummaryActive()) {
        const auto sources = oscBrowserSources();
        if (sources.empty()) {
            gridScrollOffset_ = 0;
            return;
        }
        visibleRows = std::max(1, visibleRows);
        int slot = activeRowSlot();
        if (slot < 0) {
            slot = 0;
        }
        if (slot < gridScrollOffset_) {
            gridScrollOffset_ = slot;
        } else if (slot >= gridScrollOffset_ + visibleRows) {
            gridScrollOffset_ = slot - visibleRows + 1;
        }
        int maxOffset = std::max(0, static_cast<int>(sources.size()) - visibleRows);
        gridScrollOffset_ = std::clamp(gridScrollOffset_, 0, maxOffset);
        return;
    }
    const auto& rows = activeRowIndices();
    if (rows.empty()) {
        gridScrollOffset_ = 0;
        return;
    }
    visibleRows = std::max(1, visibleRows);
    int slot = activeRowSlot();
    if (slot < 0) {
        slot = 0;
    }
    if (slot < gridScrollOffset_) {
        gridScrollOffset_ = slot;
    } else if (slot >= gridScrollOffset_ + visibleRows) {
        gridScrollOffset_ = slot - visibleRows + 1;
    }
    int maxOffset = std::max(0, static_cast<int>(rows.size()) - visibleRows);
    gridScrollOffset_ = std::clamp(gridScrollOffset_, 0, maxOffset);
}

inline int ControlMappingHubState::firstLeafNodeIndex() const {
    for (std::size_t i = 0; i < tableModel_.tree.size(); ++i) {
        if (tableModel_.tree[i].categoryIndex >= 0 && !tableModel_.tree[i].expandable &&
            (tableModel_.tree[i].subcategoryIndex >= 0 || tableModel_.tree[i].assetGroupIndex >= 0)) {
            return static_cast<int>(i);
        }
    }
    return tableModel_.tree.empty() ? -1 : 0;
}

inline bool ControlMappingHubState::isCategoryExpanded(const std::string& name) const {
    if (name.empty()) {
        return true;
    }
    auto it = treeExpansionState_.find(name);
    if (it != treeExpansionState_.end()) {
        return it->second;
    }
    return true;
}

inline void ControlMappingHubState::setCategoryExpanded(const std::string& name, bool expanded) const {
    if (name.empty()) {
        return;
    }
    treeExpansionState_[name] = expanded;
    auto& collapsed = preferences_.collapsedCategories;
    auto it = std::find(collapsed.begin(), collapsed.end(), name);
    if (!expanded) {
        if (it == collapsed.end()) {
            collapsed.push_back(name);
        }
    } else if (it != collapsed.end()) {
        collapsed.erase(it);
    }
    markPreferencesDirty();
}

inline std::string ControlMappingHubState::subcategoryExpansionKey(const std::string& category,
                                                                   const std::string& subcategory) const {
    if (category.empty() || subcategory.empty()) {
        return category;
    }
    return category + "::" + subcategory;
}

inline void ControlMappingHubState::replayTreeSelection(const std::string& category,
                                                        const std::string& subcategory,
                                                        const std::string& asset) const {
    pendingCategoryPref_ = category;
    pendingSubcategoryPref_ = subcategory;
    pendingAssetPref_ = asset;
    treeSelectionPending_ = true;
    tableModel_.dirty = true;
}

inline ControlMappingHubState::ViewportSnapshot ControlMappingHubState::snapshotViewport(float viewportWidth,
                                                                                         float viewportHeight) const {
    rebuildView();
    clampSelection();
    LayoutContext ctx = computeLayoutContext(viewportWidth, viewportHeight);
    ViewportSnapshot snapshot;
    snapshot.treeNodeCount = static_cast<int>(tableModel_.tree.size());
    snapshot.treeScrollOffset = treeScrollOffset_;
    snapshot.treeVisibleRows = ctx.treeVisibleRows;
    if (oscSummaryActive()) {
        snapshot.gridRowCount = static_cast<int>(oscBrowserSources().size());
    } else {
        const auto& rows = activeRowIndices();
        snapshot.gridRowCount = static_cast<int>(rows.size());
    }
    snapshot.gridScrollOffset = gridScrollOffset_;
    snapshot.gridVisibleRows = ctx.gridVisibleRows;
    return snapshot;
}

inline void ControlMappingHubState::moveTreeSelection(int delta) const {
    cancelHudColumnPicker();
    cancelSlotPicker();
    if (tableModel_.tree.empty()) {
        selectedTreeNodeIndex_ = -1;
        return;
    }
    int count = static_cast<int>(tableModel_.tree.size());
    int current = selectedTreeNodeIndex_;
    if (current < 0 || current >= count) {
        current = 0;
    }
    int idx = (current + delta + count) % count;
    applyTreeSelection(idx, true);
}

inline void ControlMappingHubState::focusTreeParent() const {
    if (tableModel_.tree.empty()) {
        return;
    }
    if (selectedTreeNodeIndex_ < 0 || selectedTreeNodeIndex_ >= static_cast<int>(tableModel_.tree.size())) {
        applyTreeSelection(0, true);
        return;
    }
    const auto& node = tableModel_.tree[selectedTreeNodeIndex_];
    if (node.expandable && node.expanded) {
        const std::string expansionKey = node.depth == 1
            ? subcategoryExpansionKey(node.categoryName, node.subcategoryName)
            : node.categoryName;
        setCategoryExpanded(expansionKey, false);
        if (node.depth == 1) {
            replayTreeSelection(node.categoryName, node.subcategoryName);
        } else {
            replayTreeSelection(node.categoryName, std::string());
        }
        return;
    }
    int parent = node.parentIndex;
    if (parent >= 0 && parent < static_cast<int>(tableModel_.tree.size())) {
        applyTreeSelection(parent, true);
    }
}

inline void ControlMappingHubState::focusTreeChild() const {
    if (tableModel_.tree.empty()) {
        return;
    }
    if (selectedTreeNodeIndex_ < 0 || selectedTreeNodeIndex_ >= static_cast<int>(tableModel_.tree.size())) {
        applyTreeSelection(0, true);
        return;
    }
    const auto& node = tableModel_.tree[selectedTreeNodeIndex_];
    if (!node.expandable) {
        return;
    }
    if (!node.expanded) {
        const std::string expansionKey = node.depth == 1
            ? subcategoryExpansionKey(node.categoryName, node.subcategoryName)
            : node.categoryName;
        setCategoryExpanded(expansionKey, true);
        if (node.depth == 1) {
            replayTreeSelection(node.categoryName, node.subcategoryName, preferences_.assetName);
        } else {
            replayTreeSelection(node.categoryName, preferences_.subcategoryName, preferences_.assetName);
        }
        return;
    }
    for (std::size_t i = selectedTreeNodeIndex_ + 1; i < tableModel_.tree.size(); ++i) {
        if (tableModel_.tree[i].parentIndex == selectedTreeNodeIndex_) {
            applyTreeSelection(static_cast<int>(i), true);
            return;
        }
        if (tableModel_.tree[i].depth <= node.depth) {
            break;
        }
    }
}

inline void ControlMappingHubState::applyTreeSelection(int nodeIndex, bool userDriven) const {
    cancelHudColumnPicker();
    cancelSlotPicker();
    int previousIndex = selectedTreeNodeIndex_;
    if (tableModel_.tree.empty()) {
        selectedTreeNodeIndex_ = -1;
    } else {
        nodeIndex = std::clamp(nodeIndex, 0, static_cast<int>(tableModel_.tree.size()) - 1);
        selectedTreeNodeIndex_ = nodeIndex;
    }
    if (selectedTreeNodeIndex_ != previousIndex) {
        gridScrollOffset_ = 0;
    }
    invalidateRowCache();
    clampSelection();
    if (!userDriven) {
        return;
    }
    if (tableModel_.tree.empty() ||
        selectedTreeNodeIndex_ < 0 ||
        selectedTreeNodeIndex_ >= static_cast<int>(tableModel_.tree.size())) {
        preferences_.categoryName.clear();
        preferences_.subcategoryName.clear();
        preferences_.assetName.clear();
    } else {
        const auto& node = tableModel_.tree[selectedTreeNodeIndex_];
        if (node.categoryIndex >= 0 && node.categoryIndex < static_cast<int>(tableModel_.categories.size())) {
            preferences_.categoryName = tableModel_.categories[node.categoryIndex].name;
            if (node.subcategoryIndex >= 0 && node.subcategoryIndex < static_cast<int>(tableModel_.categories[node.categoryIndex].subcategories.size())) {
                preferences_.subcategoryName = tableModel_.categories[node.categoryIndex].subcategories[node.subcategoryIndex].name;
                if (node.assetGroupIndex >= 0) {
                    const auto& assetGroups = tableModel_.categories[node.categoryIndex].subcategories[node.subcategoryIndex].assetGroups;
                    if (node.assetGroupIndex < static_cast<int>(assetGroups.size())) {
                        preferences_.assetName = assetGroups[node.assetGroupIndex].name;
                    } else {
                        preferences_.assetName.clear();
                    }
                } else {
                    preferences_.assetName.clear();
                }
            } else {
                preferences_.subcategoryName.clear();
                preferences_.assetName.clear();
            }
        } else {
            preferences_.categoryName.clear();
            preferences_.subcategoryName.clear();
            preferences_.assetName.clear();
        }
    }
    pendingCategoryPref_.clear();
    pendingSubcategoryPref_.clear();
    pendingAssetPref_.clear();
    treeSelectionPending_ = false;
    markPreferencesDirty();
}

inline void ControlMappingHubState::toggleColumn(int columnIndex) {
    if (columnIndex < 0 || columnIndex >= static_cast<int>(Column::kCount)) {
        return;
    }
    cancelValueEdit();
    int visibleCount = 0;
    for (bool visible : preferences_.columnVisibility) {
        if (visible) {
            ++visibleCount;
        }
    }
    bool currentlyVisible = preferences_.columnVisibility[columnIndex];
    if (visibleCount <= 1 && currentlyVisible) {
        return;
    }
    preferences_.columnVisibility[columnIndex] = !currentlyVisible;
    if (!preferences_.columnVisibility[columnIndex]) {
        enforceVisibleColumnSelection();
    }
    markPreferencesDirty();
}

inline void ControlMappingHubState::enforceVisibleColumnSelection() const {
    auto visible = visibleColumns();
    if (visible.empty()) {
        preferences_.columnVisibility[static_cast<int>(Column::kName)] = true;
        selectedColumn_ = Column::kName;
        cancelValueEdit();
        return;
    }
    int idx = static_cast<int>(selectedColumn_);
    if (std::find(visible.begin(), visible.end(), idx) == visible.end()) {
        selectedColumn_ = static_cast<Column>(visible.front());
        cancelValueEdit();
    }
}

inline const char* ControlMappingHubState::columnKey(Column column) {
    switch (column) {
    case Column::kName: return "name";
    case Column::kValue: return "value";
    case Column::kSlot: return "slot";
    case Column::kMidi: return "midi";
    case Column::kMidiMin: return "midi_min";
    case Column::kMidiMax: return "midi_max";
    case Column::kOsc: return "osc";
    case Column::kOscDeadband: return "osc_deadband";
    default: return "value";
    }
}

inline ControlMappingHubState::Column ControlMappingHubState::columnFromKey(const std::string& key) {
    if (key == "name") return Column::kName;
    if (key == "value") return Column::kValue;
    if (key == "slot") return Column::kSlot;
    if (key == "midi") return Column::kMidi;
    if (key == "midi_min") return Column::kMidiMin;
    if (key == "midi_max") return Column::kMidiMax;
    if (key == "osc") return Column::kOsc;
    if (key == "osc_deadband") return Column::kOscDeadband;
    return Column::kValue;
}

inline void ControlMappingHubState::ensurePreferencesDirectory() const {
    if (preferencesPath_.empty()) {
        return;
    }
    auto dir = ofFilePath::getEnclosingDirectory(preferencesPath_, false);
    if (!dir.empty()) {
        ofDirectory::createDirectory(dir, true, true);
    }
}

inline void ControlMappingHubState::loadPreferences() {
    if (preferencesPath_.empty()) {
        treeSelectionPending_ = true;
        pendingCategoryPref_.clear();
        pendingSubcategoryPref_.clear();
        pendingAssetPref_.clear();
        return;
    }
    if (!ofFile::doesFileExist(preferencesPath_)) {
        treeSelectionPending_ = true;
        pendingCategoryPref_.clear();
        pendingSubcategoryPref_.clear();
        pendingAssetPref_.clear();
        return;
    }
    try {
        auto buffer = ofBufferFromFile(preferencesPath_);
        ofJson json = ofJson::parse(buffer.getText());
        preferences_.treeWidthRatio = json.value("treeWidthRatio", preferences_.treeWidthRatio);
        preferences_.categoryName = json.value("selectedCategory", std::string());
        preferences_.subcategoryName = json.value("selectedSubcategory", std::string());
        preferences_.assetName = json.value("selectedAsset", std::string());
        preferences_.hudVisible = json.value("hudVisible", preferences_.hudVisible);
        std::string selectedColumnKey = json.value("selectedColumn", std::string());
        if (!selectedColumnKey.empty()) {
            selectedColumn_ = columnFromKey(selectedColumnKey);
        }
        if (json.contains("visibleColumns") && json["visibleColumns"].is_object()) {
            const auto& visible = json["visibleColumns"];
            for (int i = 0; i < static_cast<int>(Column::kCount); ++i) {
                const char* key = columnKey(static_cast<Column>(i));
                bool defaultVisible = static_cast<Column>(i) != Column::kOscDeadband;
                preferences_.columnVisibility[i] = visible.value(key, defaultVisible);
            }
        }
        preferences_.collapsedCategories.clear();
        if (json.contains("collapsedCategories") && json["collapsedCategories"].is_array()) {
            for (const auto& entry : json["collapsedCategories"]) {
                if (entry.is_string()) {
                    preferences_.collapsedCategories.push_back(entry.get<std::string>());
                }
            }
        }
        preferences_.hudWidgets.clear();
        if (json.contains("hudWidgets") && json["hudWidgets"].is_array()) {
            for (const auto& widgetEntry : json["hudWidgets"]) {
                if (!widgetEntry.is_object()) {
                    continue;
                }
                std::string widgetId = widgetEntry.value("id", std::string());
                if (widgetId.empty()) {
                    continue;
                }
                HudWidgetPreference pref;
                pref.visible = widgetEntry.value("visible", pref.visible);
                pref.columnIndex = widgetEntry.value("column", pref.columnIndex);
                pref.bandId = widgetEntry.value("band", pref.bandId);
                pref.collapsed = widgetEntry.value("collapsed", pref.collapsed);
                preferences_.hudWidgets[widgetId] = std::move(pref);
            }
        }
        preferences_.controllerHudWidgets.clear();
        if (json.contains("hudControllerWidgets") && json["hudControllerWidgets"].is_array()) {
            for (const auto& widgetEntry : json["hudControllerWidgets"]) {
                if (!widgetEntry.is_object()) {
                    continue;
                }
                std::string widgetId = widgetEntry.value("id", std::string());
                if (widgetId.empty()) {
                    continue;
                }
                HudLayoutPlacement placement;
                placement.columnIndex = widgetEntry.value("column", placement.columnIndex);
                placement.bandId = widgetEntry.value("band", placement.bandId);
                placement.collapsed = widgetEntry.value("collapsed", placement.collapsed);
                preferences_.controllerHudWidgets[widgetId] = std::move(placement);
            }
        }
        preferences_.hudLayoutTarget = json.value("hudLayoutTarget", preferences_.hudLayoutTarget);
        hudLayoutTarget_ = hudLayoutTargetFromString(preferences_.hudLayoutTarget);
        if (hudLayoutTarget_ == HudLayoutTarget::Controller && preferences_.controllerHudWidgets.empty()) {
            seedControllerLayoutFromPrimary();
        }
        preferences_.hudStateMigrated = json.value("hudStateMigrated", preferences_.hudStateMigrated);
        treeExpansionState_.clear();
        for (const auto& name : preferences_.collapsedCategories) {
            treeExpansionState_[name] = false;
        }
        pendingCategoryPref_ = preferences_.categoryName;
        pendingSubcategoryPref_ = preferences_.subcategoryName;
        pendingAssetPref_ = preferences_.assetName;
        treeSelectionPending_ = true;
        enforceVisibleColumnSelection();
    } catch (const std::exception& ex) {
        ofLogWarning("ControlMappingHub") << "Failed to load browser preferences: " << ex.what();
        treeSelectionPending_ = true;
        pendingCategoryPref_.clear();
        pendingSubcategoryPref_.clear();
        pendingAssetPref_.clear();
    }
    hudVisible_ = preferences_.hudVisible;
    if (hudVisibilityChanged_) {
        hudVisibilityChanged_(hudVisible_);
    }
    replayHudTogglePreferences();
    replayHudPlacementPreferences();
    preferencesDirty_ = false;
}

inline void ControlMappingHubState::markPreferencesDirty() const {
    preferencesDirty_ = true;
}

inline void ControlMappingHubState::flushPreferences() const {
    if (!preferencesDirty_ || preferencesPath_.empty()) {
        return;
    }
    snapshotHudPreferencesFromProvider();
    ensurePreferencesDirectory();
    ofJson json;
    json["treeWidthRatio"] = preferences_.treeWidthRatio;
    json["selectedCategory"] = preferences_.categoryName;
    json["selectedSubcategory"] = preferences_.subcategoryName;
    json["selectedAsset"] = preferences_.assetName;
    json["hudVisible"] = preferences_.hudVisible;
    json["selectedColumn"] = columnKey(selectedColumn_);
    json["collapsedCategories"] = preferences_.collapsedCategories;
    ofJson visible = ofJson::object();
    for (int i = 0; i < static_cast<int>(Column::kCount); ++i) {
        visible[columnKey(static_cast<Column>(i))] = preferences_.columnVisibility[i];
    }
    json["visibleColumns"] = visible;
    ofJson hudWidgets = ofJson::array();
    for (const auto& entry : preferences_.hudWidgets) {
        if (entry.first.empty()) {
            continue;
        }
        ofJson widget;
        widget["id"] = entry.first;
        widget["visible"] = entry.second.visible;
        widget["column"] = entry.second.columnIndex;
        if (!entry.second.bandId.empty()) {
            widget["band"] = entry.second.bandId;
        }
        widget["collapsed"] = entry.second.collapsed;
        hudWidgets.push_back(std::move(widget));
    }
    if (!hudWidgets.empty()) {
        json["hudWidgets"] = hudWidgets;
    }
    if (!preferences_.controllerHudWidgets.empty()) {
        ofJson controllerWidgets = ofJson::array();
        for (const auto& entry : preferences_.controllerHudWidgets) {
            if (entry.first.empty()) {
                continue;
            }
            ofJson widget;
            widget["id"] = entry.first;
            widget["column"] = entry.second.columnIndex;
            if (!entry.second.bandId.empty()) {
                widget["band"] = entry.second.bandId;
            }
            widget["collapsed"] = entry.second.collapsed;
            controllerWidgets.push_back(std::move(widget));
        }
        if (!controllerWidgets.empty()) {
            json["hudControllerWidgets"] = controllerWidgets;
        }
    }
    json["hudLayoutTarget"] = preferences_.hudLayoutTarget;
    json["hudStateMigrated"] = preferences_.hudStateMigrated;
    std::string tmpPath = preferencesPath_ + ".tmp";
    try {
        if (!ofSavePrettyJson(tmpPath, json)) {
            ofLogWarning("ControlMappingHub") << "Failed to write browser preferences";
            return;
        }
        if (!ofFile::moveFromTo(tmpPath, preferencesPath_, true, true)) {
            ofLogWarning("ControlMappingHub") << "Failed to commit browser preferences";
            return;
        }
        preferencesDirty_ = false;
    } catch (const std::exception& ex) {
        ofLogWarning("ControlMappingHub") << "Failed to save preferences: " << ex.what();
    }
}
