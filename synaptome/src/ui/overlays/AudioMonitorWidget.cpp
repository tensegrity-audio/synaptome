#include "AudioMonitorWidget.h"

#include "../../io/AudioAnalysisBus.h"
#include "HudThemeUtils.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace {
    constexpr float kWaveformGain = 2.75f;
    constexpr float kWaveformFollow = 0.58f;
    constexpr uint64_t kFreshWindowMs = 1500;

    ofColor withAlpha(ofColor color, int alpha) {
        color.a = static_cast<unsigned char>(ofClamp(alpha, 0, 255));
        return color;
    }

    std::string displaySourceLabel(const std::string& sourceLabel) {
        return sourceLabel.empty() ? std::string("Audio input") : sourceLabel;
    }

    void drawMeter(const std::string& label,
                   float value,
                   float x,
                   float y,
                   float width,
                   float height,
                   const HudSkin* hud,
                   const ofColor& fillColor) {
        const float clamped = ofClamp(value, 0.0f, 1.0f);
        const float labelWidth = 42.0f * hudTypographyScale(hud);
        const float barX = x + labelWidth;
        const float barW = std::max(1.0f, width - labelWidth);
        ofSetColor(hudMutedColor(hud));
        drawBitmapStringScaled(label, x, y + height, hudTypographyScale(hud));
        ofSetColor(withAlpha(hudBadgeFill(hud), 210));
        ofDrawRectangle(barX, y, barW, height);
        ofSetColor(fillColor);
        ofDrawRectangle(barX, y, barW * clamped, height);
        ofSetColor(withAlpha(hudChromeColor(hud), 110));
        ofNoFill();
        ofDrawRectangle(barX, y, barW, height);
        ofFill();
    }
}

AudioMonitorWidget::AudioMonitorWidget() {
    metadata_.id = "hud.audio.waveform";
    metadata_.label = "Audio Waveform";
    metadata_.category = "HUD";
    metadata_.description = "Shows the current audio input waveform.";
    metadata_.defaultColumn = 1;
    metadata_.defaultHeight = 190.0f;
    metadata_.minHeight = 150.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
}

void AudioMonitorWidget::setup(const SetupParams& params) {
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void AudioMonitorWidget::update(const UpdateParams& params) {
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }

    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    if (!snapshot.valid || snapshot.waveform.empty() || snapshot.frame == lastFrame_) {
        return;
    }

    if (!hasSample_ || waveform_.size() != snapshot.waveform.size()) {
        waveform_ = snapshot.waveform;
        level_ = snapshot.level;
        peak_ = snapshot.peak;
        bass_ = snapshot.bass;
        mids_ = snapshot.mids;
        highs_ = snapshot.highs;
        hasSample_ = true;
    } else {
        for (std::size_t i = 0; i < waveform_.size(); ++i) {
            waveform_[i] = ofLerp(waveform_[i], snapshot.waveform[i], kWaveformFollow);
        }
        level_ = ofLerp(level_, snapshot.level, kWaveformFollow);
        peak_ = ofLerp(peak_, snapshot.peak, kWaveformFollow);
        bass_ = ofLerp(bass_, snapshot.bass, kWaveformFollow);
        mids_ = ofLerp(mids_, snapshot.mids, kWaveformFollow);
        highs_ = ofLerp(highs_, snapshot.highs, kWaveformFollow);
    }

    sourceLabel_ = displaySourceLabel(snapshot.sourceLabel);
    lastFrame_ = snapshot.frame;
    lastSampleMs_ = static_cast<uint64_t>(ofGetElapsedTimeMillis());
}

void AudioMonitorWidget::draw(const DrawParams& params) {
    const float padding = hudBlockPadding(hudSkin_);
    const float lineH = hudLineHeight(hudSkin_);
    const float scale = hudTypographyScale(hudSkin_);
    const float x = params.bounds.x + padding;
    const float y = params.bounds.y + padding;
    const float contentW = std::max(1.0f, params.bounds.width - padding * 2.0f);
    const uint64_t nowMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    const bool fresh = hasSample_ && lastSampleMs_ != 0 && (nowMs - lastSampleMs_) <= kFreshWindowMs;

    ofPushStyle();
    drawHudPanelBackground(params.bounds, hudSkin_);

    ofSetColor(fresh ? hudAccentColor(hudSkin_) : hudMutedColor(hudSkin_));
    drawBitmapStringScaled("Audio Waveform", x, y + lineH, scale, true);

    std::string status;
    if (hasSample_) {
        std::ostringstream out;
        out << sourceLabel_ << "  RMS " << ofToString(level_, 3)
            << "  Peak " << ofToString(peak_, 3);
        if (!fresh) {
            out << "  stale";
        }
        status = out.str();
    } else {
        status = "No audio input";
    }
    status = hudEllipsizeText(status, contentW + padding * 2.0f, hudSkin_, 1);
    ofSetColor(hasSample_ ? hudTextColor(hudSkin_) : hudMutedColor(hudSkin_));
    drawBitmapStringScaled(status, x, y + lineH * 2.0f, scale);

    const float scopeTop = y + lineH * 2.45f;
    const float meterH = 8.0f;
    const float meterTop = params.bounds.y + params.bounds.height - padding - lineH - meterH;
    const float scopeH = std::max(34.0f, meterTop - scopeTop - padding * 0.55f);
    const float centerY = scopeTop + scopeH * 0.5f;

    ofSetColor(withAlpha(hudBadgeFill(hudSkin_), 190));
    ofDrawRectangle(x, scopeTop, contentW, scopeH);
    ofSetColor(withAlpha(hudChromeColor(hudSkin_), fresh ? 90 : 50));
    ofDrawLine(x, centerY, x + contentW, centerY);

    if (hasSample_ && waveform_.size() > 1) {
        ofSetLineWidth(1.4f);
        ofSetColor(withAlpha(fresh ? hudAccentColor(hudSkin_) : hudMutedColor(hudSkin_), fresh ? 230 : 155));
        const float denom = static_cast<float>(std::max<std::size_t>(1, waveform_.size() - 1));
        float prevX = x;
        float prevY = centerY - ofClamp(waveform_.front() * kWaveformGain, -1.0f, 1.0f) * scopeH * 0.45f;
        for (std::size_t i = 1; i < waveform_.size(); ++i) {
            const float px = x + (static_cast<float>(i) / denom) * contentW;
            const float sample = ofClamp(waveform_[i] * kWaveformGain, -1.0f, 1.0f);
            const float py = centerY - sample * scopeH * 0.45f;
            ofDrawLine(prevX, prevY, px, py);
            prevX = px;
            prevY = py;
        }
    } else {
        ofSetColor(hudMutedColor(hudSkin_));
        drawBitmapStringScaled("Waiting for waveform", x + padding * 0.5f, centerY + lineH * 0.4f, scale);
    }

    const float meterWidth = (contentW - padding) * 0.5f;
    drawMeter("RMS", level_, x, meterTop, meterWidth, meterH, hudSkin_, withAlpha(hudAccentColor(hudSkin_), fresh ? 220 : 120));
    drawMeter("PEAK", peak_, x + meterWidth + padding, meterTop, meterWidth, meterH, hudSkin_, withAlpha(hudWarningColor(hudSkin_), fresh ? 220 : 120));

    const float bandTop = meterTop + lineH;
    const float bandWidth = (contentW - padding * 2.0f) / 3.0f;
    const std::array<float, 3> bands = { bass_, mids_, highs_ };
    const std::array<std::string, 3> labels = { "B", "M", "H" };
    for (std::size_t i = 0; i < bands.size(); ++i) {
        drawMeter(labels[i],
                  bands[i],
                  x + static_cast<float>(i) * (bandWidth + padding),
                  bandTop,
                  bandWidth,
                  meterH,
                  hudSkin_,
                  withAlpha(hudAccentColor(hudSkin_), fresh ? 180 : 90));
    }

    ofPopStyle();
}

float AudioMonitorWidget::preferredHeight(float width) const {
    (void)width;
    return metadata_.defaultHeight;
}
