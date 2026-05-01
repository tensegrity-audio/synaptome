// Canonical implementation of ControlHubEventBridge kept here to satisfy
// Visual Studio project layouts that compile this file. This implementation
// forwards events to the HudRegistry and restores the toggle state after a
// short debounce window.

#include "ControlHubEventBridge.h"
#include "HudRegistry.h"
#include "ofMain.h"
#include "ofLog.h"
#include "ofUtils.h"

#include <vector>
#include <sstream>
#include <iostream>
#include <cctype>

namespace {
std::string canonicalizeBioAmpParameterId(const std::string& metric) {
	if (metric == "bioamp-raw") return "sensors.bioamp.raw";
	if (metric == "bioamp-signal") return "sensors.bioamp.signal";
	if (metric == "bioamp-mean") return "sensors.bioamp.mean";
	if (metric == "bioamp-rms") return "sensors.bioamp.rms";
	if (metric == "bioamp-dom-hz") return "sensors.bioamp.dom_hz";
	if (metric == "bioamp-sample-rate" || metric == "sample_rate") return "sensors.bioamp.sample_rate";
	if (metric == "bioamp-window" || metric == "window") return "sensors.bioamp.window";
	return std::string();
}

bool parseBioAmpDetail(const std::string& detail, std::string& outId, float& outValue) {
	auto pos = detail.find('=');
	if (pos == std::string::npos) {
		return false;
	}
	std::string metric = detail.substr(0, pos);
	std::string valueStr = detail.substr(pos + 1);
	auto trim = [](std::string& s) {
		auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](unsigned char c) { return !isSpace(c); }));
		s.erase(std::find_if(s.rbegin(), s.rend(), [&](unsigned char c) { return !isSpace(c); }).base(), s.end());
	};
	trim(metric);
	trim(valueStr);
	std::string canonical = canonicalizeBioAmpParameterId(metric);
	if (canonical.empty()) {
		return false;
	}
	outId = canonical;
	outValue = ofToFloat(valueStr);
	return true;
}
} // namespace

ControlHubEventBridge::ControlHubEventBridge(HudRegistry* hudRegistry, uint64_t debounceMs)
	: hud_(hudRegistry), debounceMs_(debounceMs) {}

ControlHubEventBridge::~ControlHubEventBridge() = default;

void ControlHubEventBridge::onEvent(const std::string& type,
									const std::string& parameterId,
									const std::string& source,
									float value) {
	(void)parameterId; (void)source; (void)value;
	ofLogNotice("ControlHubEventBridge") << "Received event: " << type;

	if (!hud_) return;

	const std::string toggleId = "hud.controls";
	bool prev = hud_->isEnabled(toggleId);

	uint64_t now = static_cast<uint64_t>(ofGetElapsedTimeMillis());
	PendingRestore p;
	p.untilMs = now + debounceMs_;
	p.prevValue = prev;
	pending_[toggleId] = p;

	hud_->setValue(toggleId, true);
}

void ControlHubEventBridge::onEventJson(const std::string& jsonPayload) {
	ofLogVerbose("ControlHubEventBridge") << "JSON event: " << jsonPayload;
	try {
		auto json = ofJson::parse(jsonPayload);
		const std::string type = json.value("type", std::string("hub.event"));
		if (type == "hud.telemetry") {
			if (telemetrySink_) {
				const std::string widgetId = json.value("widgetId", json.value("widget", std::string()));
				const std::string feedId = json.value("feedId", json.value("feed", std::string()));
				const std::string detail = json.value("detail", std::string());
				float value = json.value("value", 0.0f);
				telemetrySink_(widgetId, feedId, value, detail);
			}
			return;
		}
		if (type == "sensor.bioamp") {
			if (sensorSink_) {
				const std::string detail = json.value("detail", std::string());
				std::string parameterId;
				float parsedValue = 0.0f;
				if (parseBioAmpDetail(detail, parameterId, parsedValue)) {
					uint64_t ts = json.value("timestampMs", static_cast<uint64_t>(ofGetElapsedTimeMillis()));
					sensorSink_(parameterId, parsedValue, ts);
				}
			}
			return;
		}
		const std::string parameterId = json.value("parameterId", std::string());
		const std::string source = json.value("source", std::string());
		float value = json.value("value", 0.0f);
		onEvent(type, parameterId, source, value);
	} catch (const std::exception& e) {
		ofLogWarning("ControlHubEventBridge") << "Failed to parse event JSON: " << e.what();
		onEvent("hub.event", "", "", 0.0f);
	}
}

void ControlHubEventBridge::setTelemetrySink(std::function<void(const std::string&,
																const std::string&,
																float,
																const std::string&)> sink) {
	telemetrySink_ = std::move(sink);
}

void ControlHubEventBridge::setSensorTelemetrySink(std::function<void(const std::string&,
																	  float,
																	  uint64_t)> sink) {
	sensorSink_ = std::move(sink);
}

void ControlHubEventBridge::update(uint64_t nowMs) {
	if (pending_.empty()) return;
	std::vector<std::string> toRestore;
	for (auto& kv : pending_) {
		if (nowMs >= kv.second.untilMs) {
			toRestore.push_back(kv.first);
		}
	}
	for (const auto& id : toRestore) {
		auto it = pending_.find(id);
		if (it != pending_.end()) {
			hud_->setValue(id, it->second.prevValue);
			pending_.erase(it);
		}
	}
}
