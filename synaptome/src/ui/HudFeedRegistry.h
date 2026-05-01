#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ofJson.h"

// Central registry that captures structured HUD feed payloads keyed by widget id.
// Each publish stores the latest payload plus timestamp and optionally notifies listeners.
class HudFeedRegistry {
public:
    struct FeedEntry {
        std::string widgetId;
        ofJson payload;
        uint64_t timestampMs = 0;
    };

    using Listener = std::function<void(const FeedEntry& entry)>;

    void setClock(std::function<uint64_t()> clock);
    void setTelemetryEmitter(std::function<void(const FeedEntry& entry)> emitter);

    void publish(const std::string& widgetId, ofJson payload);
    std::optional<FeedEntry> latest(const std::string& widgetId) const;
    std::vector<FeedEntry> feeds() const;

    void addListener(Listener listener);

private:
    uint64_t now() const;

    std::function<uint64_t()> clock_;
    std::function<void(const FeedEntry& entry)> telemetryEmitter_;
    std::unordered_map<std::string, FeedEntry> feeds_;
    std::vector<Listener> listeners_;
};
