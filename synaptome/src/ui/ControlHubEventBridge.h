#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

class HudRegistry;

// Lightweight bridge that consumes Control Hub events and fans out to the HudRegistry.
// Behavior: when an event arrives it briefly (debounce window) flashes the `hud.controls`
// toggle for operator feedback and restores the previous visibility state afterwards.
class ControlHubEventBridge {
public:
    explicit ControlHubEventBridge(HudRegistry* hudRegistry, uint64_t debounceMs = 200);
    ~ControlHubEventBridge();

    // Consume a simple event. The JSON payload format is optional; this overload is convenient
    // for code that already has parsed fields.
    void onEvent(const std::string& type, const std::string& parameterId, const std::string& source, float value);

    // Accept a raw JSON string (optional). Implementations may parse type/parameterId.
    void onEventJson(const std::string& jsonPayload);

    void setTelemetrySink(std::function<void(const std::string& widgetId,
                                             const std::string& feedId,
                                             float value,
                                             const std::string& detail)> sink);
    void setSensorTelemetrySink(std::function<void(const std::string& sensorId,
                                                   float value,
                                                   uint64_t timestampMs)> sink);

    // Called regularly from the host update loop to expire flashes and restore state.
    void update(uint64_t nowMs);

private:
    HudRegistry* hud_ = nullptr;
    uint64_t debounceMs_ = 200;

    struct PendingRestore {
        uint64_t untilMs = 0;
        bool prevValue = false;
    };

    std::unordered_map<std::string, PendingRestore> pending_;
    std::function<void(const std::string&, const std::string&, float, const std::string&)> telemetrySink_;
    std::function<void(const std::string&, float, uint64_t)> sensorSink_;
};
