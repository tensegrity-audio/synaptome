#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ofJson.h"

// Lightweight stand-in for the runtime HudFeedRegistry so tests can link
// without bringing in the full openFrameworks stack.
class HudFeedRegistry {
public:
    struct FeedEntry {
        std::string widgetId;
        ofJson payload;
        uint64_t timestampMs = 0;
    };

    using Listener = std::function<void(const FeedEntry& entry)>;

    void setClock(std::function<uint64_t()> clock) { clock_ = std::move(clock); }
    void setTelemetryEmitter(std::function<void(const FeedEntry& entry)> emitter) {
        telemetryEmitter_ = std::move(emitter);
    }

    void publish(const std::string& widgetId, ofJson payload) {
        if (widgetId.empty()) {
            return;
        }
        FeedEntry entry;
        entry.widgetId = widgetId;
        entry.payload = std::move(payload);
        entry.timestampMs = now();
        feeds_[widgetId] = entry;
        if (telemetryEmitter_) {
            telemetryEmitter_(entry);
        }
        for (const auto& listener : listeners_) {
            if (listener) {
                listener(entry);
            }
        }
    }

    std::optional<FeedEntry> latest(const std::string& widgetId) const {
        auto it = feeds_.find(widgetId);
        if (it == feeds_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<FeedEntry> feeds() const {
        std::vector<FeedEntry> entries;
        entries.reserve(feeds_.size());
        for (const auto& kv : feeds_) {
            entries.push_back(kv.second);
        }
        return entries;
    }

    void addListener(Listener listener) { listeners_.push_back(std::move(listener)); }

private:
    uint64_t now() const {
        if (clock_) {
            return clock_();
        }
        return ofGetElapsedTimeMillis();
    }

    std::function<uint64_t()> clock_;
    std::function<void(const FeedEntry& entry)> telemetryEmitter_;
    std::unordered_map<std::string, FeedEntry> feeds_;
    std::vector<Listener> listeners_;
};
