#pragma once
#include "ofMain.h"
#include "io/MidiRouter.h"
#include "io/SerialSlipOsc.h"
#include "io/AudioInputBridge.h"
#include "io/OscParameterRouter.h"
#include "io/ConsoleStore.h"
#include "ui/MenuController.h"
#include "ui/MenuSkin.h"
#include "ui/AssetBrowser.h"
#include "ui/HotkeyManager.h"
#include "ui/KeyMappingUI.h"
#include "ui/HudRegistry.h"
#include "ui/HudFeedRegistry.h"
#include "ui/ControlMappingHubState.h"
#include "ui/DevicesPanel.h"
#include "ui/HudLayoutEditor.h"
#include "ui/overlays/OverlayManager.h"
#include "core/ParameterRegistry.h"
#include "core/BankRegistry.h"
#include "visuals/GridLayer.h"
#include "visuals/GeodesicLayer.h"
#include "visuals/LayerFactory.h"
#include "visuals/LayerLibrary.h"
#include "visuals/OscilloscopeLayer.h"
#include "visuals/PerlinNoiseLayer.h"
#include "visuals/StlModelLayer.h"
#include "visuals/GameOfLifeLayer.h"
#include "visuals/AgentFieldLayer.h"
#include "visuals/FlockingLayer.h"
#include "visuals/FlowFieldLayer.h"
#include "visuals/VideoGrabberLayer.h"
#include "visuals/VideoClipLayer.h"
#include "visuals/TextLayer.h"
#include "visuals/effects/PostEffectChain.h"
#include <deque>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include "ui/ControlHubEventBridge.h"
#include "ui/ThreeBandLayout.h"
#include "ui/ConsoleState.h"

class ControlHubEventBridge;
class SecondaryDisplayView;

class ofApp : public ofBaseApp {
public:
    friend class SecondaryDisplayView;
    ofApp();
    void setup() override;
    void setLaunchArguments(int argc, char** argv);
    void update() override;
    void draw() override;

    void keyPressed(int key) override;
    void mousePressed(int x, int y, int button) override;
    void mouseDragged(int x, int y, int button) override;
    void mouseReleased(int x, int y, int button) override;
    void exit() override;

    std::string composeHudControls() const;
    std::string composeHudLayerSummary() const;
    std::string composeHudLayerDetails() const;
    std::string composeHudLayers() const;
    std::string composeHudStatus() const;
    std::string composeHudSensors() const;
    std::string composeHudMenu() const;
    std::optional<HudFeedRegistry::FeedEntry> latestHudFeed(const std::string& widgetId) const;
    ofColor secondaryDisplayBackgroundColor() const;
    std::string secondaryDisplayLabel() const;

    // orbit camera
    ofCamera cam;
    float camDist = 700.0f;
    float camTheta = glm::radians(45.0f); // yaw
    float camPhi = glm::radians(20.0f);   // pitch
    glm::vec2 lastMouse { 0, 0 };
    bool dragging = false;

    // time / ui
    bool hudShowControls = true;
    bool hudShowStatus = true;
    bool hudShowLayers = true;
    bool hudShowSensors = true;
    bool hudShowMenu = true;
    bool hudShowTelemetry = true;
    bool paused = false;
    float speed = 1.0f;
    float t = 0.0f;

    // midi + overlay
    MidiRouter midi;
    MenuController menuController;
    HudRegistry hudRegistry;
    mutable HudFeedRegistry hudFeedRegistry;
    OverlayManager overlayManager;
    HotkeyManager hotkeyManager;
    std::shared_ptr<AssetBrowser> assetBrowser;
    std::shared_ptr<KeyMappingUI> keyMappingUi;
    std::shared_ptr<ControlMappingHubState> controlMappingHub;
    ThreeBandLayoutManager threeBandLayout;
    std::shared_ptr<DevicesPanel> devicesPanel;
    MenuSkin menuSkin = MenuSkin::ConsoleHub();
    std::shared_ptr<ConsoleState> consoleState;
    struct BioAmpParameterValues {
        float raw = 0.0f;
        float signal = 0.0f;
        float mean = 0.0f;
        float rms = 0.0f;
        float domHz = 0.0f;
        float sampleRate = 0.0f;
        float window = 0.0f;
    };
    BioAmpParameterValues bioAmpParameters_;
    ConsoleBioAmpSnapshot liveBioAmpSnapshot_;
    ConsoleBioAmpSnapshot pendingBioAmpSnapshot_;
    bool pendingBioAmpSeedDefined_ = false;
    std::shared_ptr<HudLayoutEditor> hudLayoutEditor;
    ParameterRegistry paramRegistry;
    BankRegistry bankRegistry;
    std::string midiMapPath;
    std::string hotkeyMapPath;
    std::string deviceMapsDir;
    std::string hudConfigPath;
    std::string overlayLayoutPath;
    std::string controlHubPrefsPath;
    std::string activeMidiBank = "home";

    // Event bridge (connects control hub events -> HUD)
    std::unique_ptr<ControlHubEventBridge> controlHubEventBridge;

    void publishHudTelemetrySample(const std::string& widgetId,
                                   const std::string& feedId,
                                   float value,
                                   const std::string& detail = std::string()) const;
    void applyHudTelemetryOverride(const std::string& widgetId,
                                   const std::string& feedId,
                                   float value,
                                   const std::string& detail);
    float hudTelemetryValueOr(const std::string& widgetId,
                              const std::string& feedId,
                              float fallback,
                              uint64_t maxAgeMs = 2000) const;
    void handleSensorTelemetrySample(const std::string& parameterId,
                                     float value,
                                     uint64_t timestampMs);

    // Global parameters mirrored into registry
    float param_speed = 1.0f;
    float param_camDist = 700.0f;
    float param_bpm = 120.0f;
    float param_masterFx = 0.0f;
    bool param_showHud = true;
    bool param_showConsole = true;
    bool param_showControlHub = true;
    bool param_showMenus = true;
    float param_menuTextSize = 1.0f;
    bool param_layoutWatchdogEnabled = true;
    bool param_layoutResyncRequest = false;
    bool param_controllerFocusConsole = true;
    std::string param_dualDisplayMode = "single";
    bool param_secondaryDisplayEnabled = false;
    std::string param_secondaryDisplayMonitor;
    int param_secondaryDisplayX = 100;
    int param_secondaryDisplayY = 100;
    int param_secondaryDisplayWidth = 1280;
    int param_secondaryDisplayHeight = 720;
    bool param_secondaryDisplayVsync = true;
    float param_secondaryDisplayDpi = 1.0f;
    std::string param_secondaryDisplayBackground = "#000000";
    bool param_secondaryDisplayFollowPrimary = true;
    SerialSlipOsc collector;
    OscParameterRouter oscRouter;
    mutable std::unordered_map<std::string, uint64_t> oscRouteMuteUntilMs_;
    mutable std::unordered_map<std::string, uint64_t> oscModifierTelemetryMs_;
    // Local host audio capture bridge (selectable device)
    AudioInputBridge audioBridge;
    std::size_t localMicModifierIndex = static_cast<std::size_t>(-1);
    std::string localMicTargetParam = "fx.master";
    struct LocalMicSettings {
        bool enabled = false;
        bool armed = false;
        bool ingestLocally = true;
        float ingestRateLimitHz = 60.0f;
        uint64_t ingestIntervalMs = 0;
        float ingestDeadband = 0.01f;
        std::string addressPrefix = "/sensor/host/localmic";
        std::string levelAddress = "/sensor/host/localmic/mic-level";
        std::string peakAddress = "/sensor/host/localmic/mic-peak";
        int requestedDeviceIndex = -1;
        std::string deviceNameContains;
        int deviceIndex = -1;
        std::string deviceLabel;
        int sampleRate = 48000;
        int bufferSize = 256;
        int channels = 1;
        bool publishOsc = false;
        std::string oscHost = "127.0.0.1";
        int oscPort = 0;
    } localMicSettings_;
    bool localMicIngestInitialized_ = false;
    uint64_t lastLocalMicIngestMs_ = 0;
    float lastLocalMicLevel_ = 0.0f;
    float lastLocalMicPeak_ = 0.0f;
    // Diagnostic: last time we logged mic values (ms)
    uint64_t lastMicLogMs = 0;
    std::deque<std::pair<std::string, float>> oscHistory;
    std::size_t oscHistoryMax = 8;
    struct HudCategoryActivity {
        uint64_t lastSeenMs = 0;
        std::string lastMetric;
        float lastValue = 0.0f;
        bool hasValue = false;

        void mark(uint64_t nowMs, const std::string& metricName, float metricValue) {
            lastSeenMs = nowMs;
            lastMetric = metricName;
            lastValue = metricValue;
            hasValue = true;
        }
    };

    struct HudDeckActivity {
        uint64_t lastAnyMs = 0;
        HudCategoryActivity hr;
        HudCategoryActivity imu;
        HudCategoryActivity aux;
    } hudDeckActivity;

    struct HudMatrixActivity {
        uint64_t lastAnyMs = 0;
        HudCategoryActivity mic;
        HudCategoryActivity bio;
        HudCategoryActivity imu;
        HudCategoryActivity aux;
    } hudMatrixActivity;
    struct HudHostActivity {
        uint64_t lastAnyMs = 0;
        HudCategoryActivity mic;
        HudCategoryActivity aux;
    } hudHostActivity;

    struct HudTelemetrySampleOverride {
        float value = 0.0f;
        uint64_t timestampMs = 0;
        std::string detail;
    };

    struct MenuHudSnapshot {
        bool hasState = false;
        std::vector<std::string> breadcrumbs;
        std::string scope;
        std::vector<MenuController::KeyHint> hotkeys;
        std::string selectedLabel;
        std::string selectedDescription;
    } menuHudSnapshot;

    LayerLibrary layerLibrary;

    // Console container: fixed-size 8 layers managed independently of legacy decks.
    struct ConsoleSlot {
        std::string assetId;
        std::string label;
        std::string type;
        std::string paramPrefix;
        bool active = false;
        float opacity = 1.0f;
        std::unique_ptr<Layer> layer;
        ConsoleLayerCoverageInfo coverage;
        float coverageParamValue = 0.0f;
        ofFbo layerFbo;
        ofFbo upstreamFbo;
        ofFbo effectFbo;
    };
    std::vector<ConsoleSlot> consoleSlots;
    std::string consoleConfigPath;
    std::string activeScenePath_;
    std::string activeNamedScenePath_;
    struct OverlayVisibilityCache {
        bool hud = true;
        bool console = true;
        bool controlHub = true;
        bool menus = true;
    } overlayVisibility_;
    bool controllerFocusDirty_ = false;
    std::string persistedDualDisplayMode_ = "single";
    std::string lastOverlayRouteTarget_;
    struct SecondaryDisplayRuntimeState {
        bool enabled = false;
        bool active = false;
        std::string monitorId;
        int x = 100;
        int y = 100;
        int width = 1280;
        int height = 720;
        bool vsync = true;
        float dpiScale = 1.0f;
        std::string background = "#000000";
        std::string spawnReason;
        bool followPrimary = true;
    } secondaryDisplay_;
    struct SecondaryDisplayWatchdogState {
        bool tripped = false;
        std::string lastReason;
        uint64_t lastEmitMs = 0;
        uint64_t lastRecoveryAttemptMs = 0;
    } secondaryDisplayWatchdog_;
    enum class SceneLoadPhase {
        Idle,
        Requested,
        Parsing,
        Validating,
        Building,
        Applying,
        Publishing,
        Succeeded,
        Failed
    };
    struct SceneLoadUiSnapshot {
        bool active = false;
        SceneLoadPhase phase = SceneLoadPhase::Idle;
        std::string scenePath;
        std::string displayName;
        std::string status;
        std::string failure;
        uint64_t startedMs = 0;
        uint64_t updatedMs = 0;
        uint64_t phaseStartedMs = 0;
        uint64_t lastPhaseElapsedMs = 0;
        uint64_t totalElapsedMs = 0;
        bool secondaryDisplayPreserved = false;
        bool secondaryDisplayWasActive = false;
        bool controllerFocusConsole = true;
    } sceneLoadUiSnapshot_;
    SceneLoadPhase sceneLoadPhase_ = SceneLoadPhase::Idle;
    struct SceneApplyPlan {
        std::string canonicalPath;
        std::string fullPath;
        ofJson scene;
        std::string activeNamedScenePath;
        ofJson routerSnapshot;
        ofJson slotAssignmentsSnapshot;
        bool restoreSecondaryDisplay = false;
        bool restoreControllerFocusConsole = true;
        bool restorePersistenceSuspended = false;
        bool consoleLayoutDefined = false;
        bool consoleApplied = false;
    };
    struct SceneLoadRollbackSnapshot {
        std::string activeScenePath;
        std::string activeNamedScenePath;
        ofJson scene;
        ofJson routerSnapshot;
        ofJson slotAssignmentsSnapshot;
        bool secondaryDisplayEnabled = false;
        bool paramSecondaryDisplayEnabled = false;
        bool paramControllerFocusConsole = true;
        bool controllerFocusConsole = true;
        bool secondaryDisplayRenderPaused = false;
        bool consolePersistenceSuspended = false;
    };
    enum class ControllerFocusOwner {
        Console,
        Controller
    };
    struct ControllerFocusRuntimeState {
        ControllerFocusOwner owner = ControllerFocusOwner::Console;
        bool preferConsole = true;
        bool lastCommandSucceeded = false;
        bool needsAttention = false;
        std::string lastDetail;
        uint64_t lastCommandMs = 0;
        uint64_t lastTelemetryMs = 0;
        bool holdActive = false;
        ControllerFocusOwner holdOwner = ControllerFocusOwner::Console;
        std::string holdReason;
        std::string midiFocusHoldToken;
        std::string hudPickerFocusHoldToken;
        std::string devicesFocusHoldToken;
    } controllerFocus_;
    struct ControllerFocusHoldEntry {
        std::string token;
        ControllerFocusOwner owner = ControllerFocusOwner::Console;
        std::string reason;
    };
    std::vector<ControllerFocusHoldEntry> controllerFocusHolds_;
    uint64_t controllerFocusHoldCounter_ = 0;
    glm::vec2 lastPrimaryWindowScale_ = glm::vec2(1.0f, 1.0f);
    glm::vec2 lastSecondaryWindowScale_ = glm::vec2(1.0f, 1.0f);
    bool lastPrimaryIconified_ = false;
    bool lastSecondaryIconified_ = false;
    std::optional<bool> secondaryDisplayCliOverride_;
    std::optional<std::string> secondaryDisplayMonitorCliOverride_;
    std::string secondaryDisplayActiveMonitor_;
    std::shared_ptr<ofAppBaseWindow> secondaryWindow_;
    std::shared_ptr<SecondaryDisplayView> secondaryWindowApp_;
    float appliedMenuTextSize_ = -1.0f;
    bool consolePersistenceSuspended_ = false;
    bool secondaryDisplayRenderPaused_ = false;

    GridLayer* gridLayer = nullptr;
    GeodesicLayer* geodesicLayer = nullptr;
    PerlinNoiseLayer* perlinLayer = nullptr;
    GameOfLifeLayer* gameOfLifeLayer = nullptr;

    PostEffectChain postEffects;
    ofFbo compositeFbo;
    int compositeWidth = 0;
    int compositeHeight = 0;

    void setupOscRoutes();
    void setupLocalMicBridge();
    void ingestOscMessage(const std::string& address, float value);
    void noteSensorActivity(const std::string& address, float value);
    void updateLocalMicBridge(uint64_t nowMs);
    void suspendOscRoute(const std::string& paramId, uint64_t durationMs) const;
    bool oscRouteWriteAllowed(const std::string& paramId) const;
    void emitOscModifierTelemetry(const std::string& paramId, float rawValue) const;
    void registerDynamicOscRoute(const MidiRouter::OscMap& map);
    void rebuildDynamicOscRoutes();

    std::vector<std::string> loadOscChannelHints() const;

private:
    void registerCoreMidiTargets();
    void configureDefaultBanks();
    void applyMenuTextSkin();
    void ensureActiveBankValid();
    void drawTextOverlay();
    void syncTextOverlayFontSelection();
    bool ensureTextOverlayFontLoaded();
    bool toggleMenuState(MenuController& controller, const std::shared_ptr<MenuController::State>& state, bool allowStack = false);
    bool toggleConsoleAndControlHub(MenuController& controller);
    void registerPerlinMidi(PerlinNoiseLayer* layer);
    void registerGameOfLifeMidi(GameOfLifeLayer* layer);
    float* fxRouteParamForType(const std::string& type);
    const float* fxRouteParamForType(const std::string& type) const;
    void setFxRouteForType(const std::string& type, float routeValue);
    void syncActiveFxWithConsoleSlots(bool enablePresent = true);
    void refreshLayerReferences();
    bool loadScene(const std::string& path);
    void saveScene(const std::string& path);
    std::vector<ControlMappingHubState::SavedSceneInfo> listSavedScenes() const;
    bool loadSavedSceneById(const std::string& sceneId);
    bool saveNamedScene(const std::string& sceneName, bool overwrite);
    bool overwriteSavedSceneById(const std::string& sceneId);
    std::string canonicalScenePath(const std::string& path) const;
    bool isAutosaveScenePath(const std::string& path) const;
    std::string sceneDisplayNameForPath(const std::string& path) const;
    const char* sceneLoadPhaseLabel(SceneLoadPhase phase) const;
    void beginSceneLoadPhase(SceneLoadPhase phase,
                             const std::string& canonicalPath,
                             const std::string& status = std::string());
    void finishSceneLoad(bool success,
                         const std::string& canonicalPath,
                         const std::string& status = std::string());
    bool sceneLoadInProgress() const;
    bool parseSceneLoadPlan(const std::string& canonicalPath,
                            SceneApplyPlan& plan,
                            std::string& error) const;
    bool buildSceneApplyPlan(const std::string& canonicalPath,
                             const ofJson& scene,
                             SceneApplyPlan& plan,
                             std::string& error) const;
    bool validateSceneConsoleLayout(const ofJson& consoleNode,
                                    std::string& error) const;
    bool applyScenePlan(SceneApplyPlan& plan);
    bool publishScenePlan(const SceneApplyPlan& plan,
                          const SceneLoadRollbackSnapshot& rollback,
                          std::string& error);
    SceneLoadRollbackSnapshot captureSceneRollbackSnapshot(const std::string& targetCanonicalPath) const;
    bool rollbackSceneLoad(const SceneLoadRollbackSnapshot& snapshot,
                           const std::string& failedCanonicalPath,
                           const std::string& reason,
                           bool restorePersistedFiles = false);
    ofJson encodeSceneJson(const std::string& path) const;
    bool writeSceneJson(const std::string& path, const ofJson& scene) const;
    // Console-related helpers
    bool addAssetToConsoleLayer(int layerIndex,
                                const std::string& assetId,
                                bool activate,
                                std::optional<float> opacityOverride = std::nullopt);
    void openAssetBrowserForConsole(int layerIndex);
    void clearConsoleSlot(int index);
    void clearAllConsoleSlots();
    void seedConsoleDefaultsIfEmpty();
    void persistConsoleAssignments();
    bool loadConsoleLayoutFromScene(const ofJson& consoleNode);
    void writeConsoleLayoutToScene(ofJson& scene) const;
    void writeConsoleParametersToScene(ofJson& slotNode, const ConsoleSlot& slot) const;
    ConsoleSlot* consoleSlotForIndex(int layerIndex);
    const ConsoleSlot* consoleSlotForIndex(int layerIndex) const;
    int findConsoleSlotByAsset(const std::string& assetId) const;
    std::string consoleSlotPrefix(int layerIndex) const;
    void registerConsoleLayerOpacityParam(int layerIndex, ConsoleSlot& slot);
    void registerConsoleLayerCoverageParam(int layerIndex, ConsoleSlot& slot);
    void unregisterConsoleLayerCoverageParam(int layerIndex);
    void importConsoleCoverageFromInfo(int layerIndex, const ConsoleLayerCoverageInfo& coverage);
    void propagateEffectCoverageChange(const std::string& effectType, float coverage);
    void applyEffectCoverageDefaults(ConsoleSlot& slot, const std::string& effectType);
    float consoleSlotBaseOpacity(int layerIndex) const;
    void updateConsoleLayers(const LayerUpdateParams& params);
    void drawConsole(glm::ivec2 viewport, float beatPhase);
    void ensureConsoleLayerViewports(glm::ivec2 viewport);
    void ensureSlotFbo(ofFbo& fbo, glm::ivec2 viewport);
    bool applyEffectSlot(ConsoleSlot& slot, ofFbo& src, ofFbo& dst);
    void handleHudVisibilityChanged(bool visible);
    bool toggleHudTools(MenuController& controller);
    bool openHudLayoutEditor(MenuController& controller);
    void ensureHudToolStack(MenuController& controller);
    void registerHudWidgetsFromCatalog();
    void publishOverlayVisibilityTelemetry(const std::string& feedId, bool visible);
    void publishDualDisplayTelemetry(const std::string& mode);
    void handleSecondaryDisplayParamChange();
    bool requestSecondaryDisplay(bool enable, const std::string& reason);
    bool spawnSecondaryDisplayShell(const std::string& reason);
    void destroySecondaryDisplayShell(const std::string& reason);
    void publishSecondaryDisplayTelemetry() const;
    void updateSecondaryDisplayWatchdog();
    void publishSecondaryDisplayWatchdog(const std::string& detail, bool healthy);
    void handleSecondaryDisplayWatchdogTrip(const std::string& reason);
    bool isSecondaryMonitorPresent(const std::string& label) const;
    void handleControllerFocusParamChange();
    void requestControllerFocusToggle();
    bool focusPrimaryWindow(const std::string& reason);
    bool focusSecondaryWindow(const std::string& reason);
    ControllerFocusOwner currentControllerFocusOwner() const;
    void publishControllerFocusTelemetry(const std::string& detail, bool success);
    void updateControllerFocusWatchdog();
    std::string controllerFocusStatusBadge() const;
    void toggleDualDisplayMode();
    void publishSecondaryDisplayFollowTelemetry() const;
    void toggleSecondaryDisplayFollow();
    void migrateOverlaysToPrimary();
    void migrateOverlaysToController();
    void announceOverlayMigration(const std::string& target);
    void emitOverlayRouteTelemetry(const std::string& source, bool forceEvent);
    void broadcastHudLayoutSnapshots(const std::string& reason, bool guardHeld = false);
    void broadcastHudRoutingManifest();
    void handleControlHubEvent(const std::string& payload);
    void handleHudMappingChangedEvent(const ofJson& event);
    void requestHudLayoutResync(const std::string& reason);
    bool tryEnterLayoutSyncGuard(const std::string& reason);
    void leaveLayoutSyncGuard(const std::string& reason = std::string());
    void updateLayoutSyncWatchdog(uint64_t nowMs);
    class LayoutSyncGuardScope {
    public:
        LayoutSyncGuardScope(ofApp* host, const std::string& reason);
        ~LayoutSyncGuardScope();
        bool owns() const { return owns_; }
    private:
        ofApp* host_;
        std::string reason_;
        bool owns_ = false;
    };
    void drawMenuPanels(const ThreeBandLayout& layout,
                        bool showConsole,
                        bool showControlHub,
                        bool showMenus);
    void drawSceneLoadSnapshot(float width, float height) const;
    void drawSecondaryDisplayWindow(float width, float height);
    void syncHudLayoutTarget();
    void monitorWindowContentScale();
    void manageControllerFocusHolds();
    void manageFocusHold(std::string& token, bool shouldHold, ControllerFocusOwner owner, const std::string& reason);
    ControllerFocusOwner desiredControllerFocusOwner() const;
    void applyDesiredControllerFocus(const std::string& reason);
    std::string acquireControllerFocusHold(ControllerFocusOwner owner, const std::string& reason);
    void releaseControllerFocusHold(const std::string& token);
    void forceHudLayoutResync(const std::string& reason = std::string());

    std::unordered_map<std::string, HudTelemetrySampleOverride> hudTelemetryOverrides_;
    bool midiTelemetryInitialized_ = false;
    bool collectorTelemetryInitialized_ = false;
    bool lastMidiConnected_ = false;
    bool lastCollectorConnected_ = false;
    uint64_t lastCollectorPacketMs_ = 0;
    uint64_t lastCollectorWaitLogMs_ = 0;
    uint64_t collectorPacketsSeen_ = 0;
    uint64_t lastHudTimingTelemetryMs_ = 0;
    uint64_t lastHudGpuTelemetryMs_ = 0;
    std::string hudTelemetryKey(const std::string& widgetId, const std::string& feedId) const;
    std::optional<HudTelemetrySampleOverride> hudTelemetryOverrideSample(const std::string& widgetId,
                                                                         const std::string& feedId,
                                                                         uint64_t maxAgeMs = 2000) const;
    bool layoutSyncGuardActive_ = false;
    bool layoutSyncGuardStalled_ = false;
    uint64_t layoutSyncGuardSinceMs_ = 0;
    uint64_t layoutSyncGuardFrame_ = 0;
    std::string layoutSyncGuardReason_;
    bool layoutSyncResyncPending_ = false;
    std::string layoutSyncResyncReason_;
    uint64_t layoutSyncResyncLastAttemptMs_ = 0;
};
