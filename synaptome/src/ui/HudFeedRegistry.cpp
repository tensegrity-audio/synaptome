#include "HudFeedRegistry.h"

#include "ofMain.h"

void HudFeedRegistry::setClock(std::function<uint64_t()> clock) {
    clock_ = std::move(clock);
}

void HudFeedRegistry::setTelemetryEmitter(std::function<void(const FeedEntry& entry)> emitter) {
    telemetryEmitter_ = std::move(emitter);
}

void HudFeedRegistry::publish(const std::string& widgetId, ofJson payload) {
    if (widgetId.empty()) {
        return;
    }
    FeedEntry entry;
    entry.widgetId = widgetId;
    entry.timestampMs = now();
    entry.payload = std::move(payload);

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

std::optional<HudFeedRegistry::FeedEntry> HudFeedRegistry::latest(const std::string& widgetId) const {
    auto it = feeds_.find(widgetId);
    if (it == feeds_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<HudFeedRegistry::FeedEntry> HudFeedRegistry::feeds() const {
    std::vector<FeedEntry> entries;
    entries.reserve(feeds_.size());
    for (const auto& kv : feeds_) {
        entries.push_back(kv.second);
    }
    return entries;
}

void HudFeedRegistry::addListener(Listener listener) {
    listeners_.push_back(std::move(listener));
}

uint64_t HudFeedRegistry::now() const {
    if (clock_) {
        return clock_();
    }
    return static_cast<uint64_t>(ofGetElapsedTimeMillis());
}
