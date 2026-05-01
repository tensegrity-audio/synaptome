#include "GameOfLifeLayer.h"
#include "ofGraphics.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace {
    constexpr int kPresetCount = 4;
    const char* kPresetLabels[kPresetCount] = {
        "Random",
        "Gliders",
        "Waves",
        "Crossfire"
    };

    std::string presetDescriptions() {
        std::string desc;
        for (int i = 0; i < kPresetCount; ++i) {
            if (!desc.empty()) desc += "  ";
            desc += ofToString(i) + "=" + kPresetLabels[i];
        }
        return desc;
    }
}

void GameOfLifeLayer::configure(const ofJson& config) {
    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramWrap_ = def.value("wrap", paramWrap_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramAliveAlpha_ = def.value("aliveAlpha", paramAliveAlpha_);
        paramDeadAlpha_ = def.value("deadAlpha", paramDeadAlpha_);
        if (def.contains("aliveColor") && def["aliveColor"].is_array() && def["aliveColor"].size() >= 3) {
            paramAliveR_ = def["aliveColor"][0].get<float>();
            paramAliveG_ = def["aliveColor"][1].get<float>();
            paramAliveB_ = def["aliveColor"][2].get<float>();
        }
        if (def.contains("deadColor") && def["deadColor"].is_array() && def["deadColor"].size() >= 3) {
            paramDeadR_ = def["deadColor"][0].get<float>();
            paramDeadG_ = def["deadColor"][1].get<float>();
            paramDeadB_ = def["deadColor"][2].get<float>();
        }
        paramPaused_ = def.value("paused", paramPaused_);
        paramSeedDensity_ = def.value("seedDensity", paramSeedDensity_);
        paramPresetIndex_ = def.value("preset", paramPresetIndex_);
        paramFadeFrames_ = def.value("fadeFrames", paramFadeFrames_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
        paramReseedQuantizeBeats_ = def.value("reseedQuantizeBeats", paramReseedQuantizeBeats_);
        paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
    }

    if (config.contains("gridSize") && config["gridSize"].is_array() && config["gridSize"].size() >= 2) {
        gridW_ = std::max(16, config["gridSize"][0].get<int>());
        gridH_ = std::max(16, config["gridSize"][1].get<int>());
    }
}

void GameOfLifeLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.gameOfLife" : registryPrefix();

    ParameterRegistry::Descriptor visMeta;
    visMeta.label = "Game of Life Visible";
    visMeta.group = "Generative";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, visMeta);

    ParameterRegistry::Descriptor speedMeta;
    speedMeta.label = "Life Speed";
    speedMeta.group = "Generative";
    speedMeta.range.min = 0.0f;
    speedMeta.range.max = 20.0f;
    speedMeta.range.step = 0.1f;
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_, speedMeta);

    ParameterRegistry::Descriptor bpmSyncMeta;
    bpmSyncMeta.label = "Life BPM Sync";
    bpmSyncMeta.group = "Generative";
    bpmSyncMeta.description = "Use transport BPM instead of free-running speed";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, bpmSyncMeta);

    ParameterRegistry::Descriptor bpmMultMeta;
    bpmMultMeta.label = "Life BPM Mult";
    bpmMultMeta.group = "Generative";
    bpmMultMeta.range.min = 0.25f;
    bpmMultMeta.range.max = 8.0f;
    bpmMultMeta.range.step = 0.25f;
    bpmMultMeta.description = "Generation rate multiplier when BPM sync is enabled";
    registry.addFloat(prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, bpmMultMeta);

    ParameterRegistry::Descriptor pauseMeta;
    pauseMeta.label = "Life Paused";
    pauseMeta.group = "Generative";
    registry.addBool(prefix + ".paused", &paramPaused_, paramPaused_, pauseMeta);

    ParameterRegistry::Descriptor reseedMeta;
    reseedMeta.label = "Life Reseed";
    reseedMeta.group = "Generative";
    reseedMeta.description = "Trigger a fresh random board using current density";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, reseedMeta);

    ParameterRegistry::Descriptor densityMeta;
    densityMeta.label = "Life Seed Density";
    densityMeta.group = "Generative";
    densityMeta.range.min = 0.0f;
    densityMeta.range.max = 1.0f;
    densityMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".density", &paramSeedDensity_, paramSeedDensity_, densityMeta);

    ParameterRegistry::Descriptor fadeMeta;
    fadeMeta.label = "Life Fade Frames";
    fadeMeta.group = "Generative";
    fadeMeta.range.min = 1.0f;
    fadeMeta.range.max = 32.0f;
    fadeMeta.range.step = 1.0f;
    fadeMeta.description = "Quantized decay trail length in simulation frames";
    registry.addFloat(prefix + ".fadeFrames", &paramFadeFrames_, paramFadeFrames_, fadeMeta);

    ParameterRegistry::Descriptor wrapMeta;
    wrapMeta.label = "Life Wrap";
    wrapMeta.group = "Generative";
    registry.addBool(prefix + ".wrap", &paramWrap_, paramWrap_, wrapMeta);

    ParameterRegistry::Descriptor presetMeta;
    presetMeta.label = "Life Preset";
    presetMeta.group = "Generative";
    presetMeta.range.min = 0.0f;
    presetMeta.range.max = static_cast<float>(std::max(0, kPresetCount - 1));
    presetMeta.range.step = 1.0f;
    presetMeta.description = presetDescriptions();
    registry.addFloat(prefix + ".preset", &paramPresetIndex_, paramPresetIndex_, presetMeta);

    ParameterRegistry::Descriptor colorMeta;
    colorMeta.group = "Generative";
    colorMeta.range.min = 0.0f;
    colorMeta.range.max = 1.0f;
    colorMeta.range.step = 0.01f;

    colorMeta.label = "Life Alive R";
    registry.addFloat(prefix + ".aliveR", &paramAliveR_, paramAliveR_, colorMeta);
    colorMeta.label = "Life Alive G";
    registry.addFloat(prefix + ".aliveG", &paramAliveG_, paramAliveG_, colorMeta);
    colorMeta.label = "Life Alive B";
    registry.addFloat(prefix + ".aliveB", &paramAliveB_, paramAliveB_, colorMeta);
    colorMeta.label = "Life Dead R";
    registry.addFloat(prefix + ".deadR", &paramDeadR_, paramDeadR_, colorMeta);
    colorMeta.label = "Life Dead G";
    registry.addFloat(prefix + ".deadG", &paramDeadG_, paramDeadG_, colorMeta);
    colorMeta.label = "Life Dead B";
    registry.addFloat(prefix + ".deadB", &paramDeadB_, paramDeadB_, colorMeta);

    ParameterRegistry::Descriptor alphaMeta;
    alphaMeta.label = "Life Alpha";
    alphaMeta.group = "Generative";
    alphaMeta.range.min = 0.0f;
    alphaMeta.range.max = 1.0f;
    alphaMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_, alphaMeta);

    ParameterRegistry::Descriptor aliveAlphaMeta = alphaMeta;
    aliveAlphaMeta.label = "Life Alive Alpha";
    registry.addFloat(prefix + ".aliveAlpha", &paramAliveAlpha_, paramAliveAlpha_, aliveAlphaMeta);

    ParameterRegistry::Descriptor deadAlphaMeta = alphaMeta;
    deadAlphaMeta.label = "Life Dead Alpha";
    registry.addFloat(prefix + ".deadAlpha", &paramDeadAlpha_, paramDeadAlpha_, deadAlphaMeta);

    ParameterRegistry::Descriptor quantizeMeta;
    quantizeMeta.label = "Life Reseed Quantize";
    quantizeMeta.group = "Generative";
    quantizeMeta.range.min = 0.0f;
    quantizeMeta.range.max = 16.0f;
    quantizeMeta.range.step = 1.0f;
    quantizeMeta.description = "0=instant, otherwise queue reseeds to the next beat multiple";
    registry.addFloat(prefix + ".reseedQuantizeBeats", &paramReseedQuantizeBeats_, paramReseedQuantizeBeats_, quantizeMeta);

    ParameterRegistry::Descriptor autoReseedMeta;
    autoReseedMeta.label = "Life Auto Reseed";
    autoReseedMeta.group = "Generative";
    autoReseedMeta.description = "Automatically reseed on a transport-quantized cadence";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, autoReseedMeta);

    ParameterRegistry::Descriptor autoReseedEveryMeta;
    autoReseedEveryMeta.label = "Life Auto Reseed Beats";
    autoReseedEveryMeta.group = "Generative";
    autoReseedEveryMeta.range.min = 1.0f;
    autoReseedEveryMeta.range.max = 64.0f;
    autoReseedEveryMeta.range.step = 1.0f;
    autoReseedEveryMeta.description = "Beat interval used by automatic reseeding";
    registry.addFloat(prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, autoReseedEveryMeta);

    cells_.assign(static_cast<size_t>(gridW_ * gridH_), 0);
    next_ = cells_;
    fadeFramesRemaining_.assign(cells_.size(), 0);

    pixels_.allocate(gridW_, gridH_, 4);
    texture_.allocate(gridW_, gridH_, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

    paramSeedDensity_ = ofClamp(paramSeedDensity_, 0.0f, 1.0f);
    paramPresetIndex_ = ofClamp(paramPresetIndex_, 0.0f, static_cast<float>(std::max(0, kPresetCount - 1)));
    paramFadeFrames_ = std::round(ofClamp(paramFadeFrames_, 1.0f, 32.0f));
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 8.0f);
    paramReseedQuantizeBeats_ = std::round(ofClamp(paramReseedQuantizeBeats_, 0.0f, 16.0f));
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 64.0f));
    activePreset_ = ofClamp(static_cast<int>(std::round(paramPresetIndex_)), 0, std::max(0, kPresetCount - 1));
    applyPresetInternal(activePreset_);
    syncTexture();
}

void GameOfLifeLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    paramFadeFrames_ = std::round(ofClamp(paramFadeFrames_, 1.0f, 32.0f));
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 8.0f);
    paramReseedQuantizeBeats_ = std::round(ofClamp(paramReseedQuantizeBeats_, 0.0f, 16.0f));
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 64.0f));

    const float beatPosition = currentBeatPosition(params.time, params.bpm);

    if (paramReseedRequested_) {
        if (paramReseedQuantizeBeats_ > 0.0f && params.bpm > 0.0f) {
            schedulePendingReseed(beatPosition);
        } else {
            triggerReseed();
        }
        paramReseedRequested_ = false;
    }

    if (reseedPending_ && pendingReseedBeat_ >= 0.0f && beatPosition >= pendingReseedBeat_) {
        triggerReseed();
    }

    if (paramAutoReseed_ && params.bpm > 0.0f) {
        if (nextAutoReseedBeat_ < 0.0f) {
            float interval = std::max(1.0f, paramAutoReseedEveryBeats_);
            nextAutoReseedBeat_ = std::floor(beatPosition / interval) * interval + interval;
        }
        while (nextAutoReseedBeat_ >= 0.0f && beatPosition >= nextAutoReseedBeat_) {
            triggerReseed();
            nextAutoReseedBeat_ += std::max(1.0f, paramAutoReseedEveryBeats_);
        }
    } else {
        nextAutoReseedBeat_ = -1.0f;
    }

    int desiredPreset = ofClamp(static_cast<int>(std::round(paramPresetIndex_)), 0, std::max(0, kPresetCount - 1));
    if (desiredPreset != activePreset_) {
        applyPresetInternal(desiredPreset);
        activePreset_ = desiredPreset;
    }

    float stepRate = stepRateFor(params);
    bool paused = paramPaused_ || stepRate <= 0.0f;

    if (paused) {
        if (dirty_) {
            syncTexture();
            dirty_ = false;
        }
        return;
    }

    stepInterval_ = stepRate > 0.0f ? 1.0f / stepRate : std::numeric_limits<float>::infinity();

    accumulator_ += params.dt;
    bool stepped = false;
    while (accumulator_ >= stepInterval_) {
        stepGeneration();
        accumulator_ -= stepInterval_;
        stepped = true;
    }

    if (stepped || dirty_) {
        syncTexture();
        dirty_ = false;
    }
}

void GameOfLifeLayer::draw(const LayerDrawParams& params) {
    if (!enabled_) return;
    if (params.slotOpacity <= 0.0f) return;
    if (!texture_.isAllocated()) return;

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    ofSetColor(255, 255, 255, static_cast<int>(ofClamp(params.slotOpacity, 0.0f, 1.0f) * 255.0f));
    texture_.draw(0, 0, params.viewport.x, params.viewport.y);

    ofPopView();
    ofPopStyle();
}

void GameOfLifeLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void GameOfLifeLayer::randomize(float density) {
    if (density < 0.0f) density = paramSeedDensity_;
    density = ofClamp(density, 0.0f, 1.0f);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (auto& cell : cells_) {
        cell = dist(rng) < density ? 1 : 0;
    }
    std::fill(fadeFramesRemaining_.begin(), fadeFramesRemaining_.end(), 0);
    dirty_ = true;
    activePreset_ = 0;
    paramPresetIndex_ = 0.0f;
    paramReseedRequested_ = false;
    reseedPending_ = false;
    pendingReseedBeat_ = -1.0f;
}

void GameOfLifeLayer::setPaused(bool paused) {
    paramPaused_ = paused;
}

void GameOfLifeLayer::applyPreset(int presetIndex) {
    presetIndex = ofClamp(presetIndex, 0, std::max(0, kPresetCount - 1));
    paramPresetIndex_ = static_cast<float>(presetIndex);
    applyPresetInternal(presetIndex);
    activePreset_ = presetIndex;
}

int GameOfLifeLayer::presetCount() const {
    return kPresetCount;
}

void GameOfLifeLayer::stepGeneration() {
    const uint16_t fadeFrames = static_cast<uint16_t>(std::max(1.0f, std::round(paramFadeFrames_)));
    auto aliveNeighbours = [&](int x, int y) {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx;
                int ny = y + dy;
                if (paramWrap_) {
                    nx = (nx + gridW_) % gridW_;
                    ny = (ny + gridH_) % gridH_;
                }
                if (nx < 0 || nx >= gridW_ || ny < 0 || ny >= gridH_) continue;
                count += cells_[index(nx, ny)];
            }
        }
        return count;
    };

    for (int y = 0; y < gridH_; ++y) {
        for (int x = 0; x < gridW_; ++x) {
            int neighbours = aliveNeighbours(x, y);
            uint8_t current = cells_[index(x, y)];
            uint8_t nextState = current;
            if (current) {
                nextState = (neighbours == 2 || neighbours == 3) ? 1 : 0;
            } else {
                nextState = (neighbours == 3) ? 1 : 0;
            }
            const int idx = index(x, y);
            next_[idx] = nextState;
            if (nextState) {
                fadeFramesRemaining_[idx] = fadeFrames;
            } else if (current) {
                fadeFramesRemaining_[idx] = fadeFrames;
            } else if (fadeFramesRemaining_[idx] > 0) {
                fadeFramesRemaining_[idx] -= 1;
            }
        }
    }

    cells_.swap(next_);
    dirty_ = true;
}

int GameOfLifeLayer::index(int x, int y) const {
    return y * gridW_ + x;
}

void GameOfLifeLayer::syncTexture() {
    if (!pixels_.isAllocated()) return;

    const float masterAlpha = ofClamp(paramAlpha_, 0.0f, 1.0f);
    const float aliveAlpha = ofClamp(paramAliveAlpha_, 0.0f, 1.0f) * masterAlpha;
    const float deadAlpha = ofClamp(paramDeadAlpha_, 0.0f, 1.0f) * masterAlpha;
    const float fadeFrames = std::max(1.0f, std::round(paramFadeFrames_));

    for (int y = 0; y < gridH_; ++y) {
        for (int x = 0; x < gridW_; ++x) {
            uint8_t alive = cells_[index(x, y)];
            ofFloatColor c;
            const std::size_t idx = static_cast<std::size_t>(index(x, y));
            if (alive) {
                c.r = ofClamp(paramAliveR_, 0.0f, 1.0f);
                c.g = ofClamp(paramAliveG_, 0.0f, 1.0f);
                c.b = ofClamp(paramAliveB_, 0.0f, 1.0f);
                c.a = aliveAlpha;
            } else {
                const float fadeMix = ofClamp(static_cast<float>(fadeFramesRemaining_[idx]) / fadeFrames, 0.0f, 1.0f);
                c.r = ofLerp(ofClamp(paramDeadR_, 0.0f, 1.0f), ofClamp(paramAliveR_, 0.0f, 1.0f), fadeMix);
                c.g = ofLerp(ofClamp(paramDeadG_, 0.0f, 1.0f), ofClamp(paramAliveG_, 0.0f, 1.0f), fadeMix);
                c.b = ofLerp(ofClamp(paramDeadB_, 0.0f, 1.0f), ofClamp(paramAliveB_, 0.0f, 1.0f), fadeMix);
                c.a = ofLerp(deadAlpha, aliveAlpha, fadeMix);
            }
            pixels_.setColor(x, y, c);
        }
    }

    texture_.loadData(pixels_);
}

void GameOfLifeLayer::clearCells() {
    std::fill(cells_.begin(), cells_.end(), 0);
    std::fill(fadeFramesRemaining_.begin(), fadeFramesRemaining_.end(), 0);
    dirty_ = true;
}

void GameOfLifeLayer::seedGlider(int x, int y) {
    auto set = [&](int px, int py) {
        if (px < 0 || px >= gridW_ || py < 0 || py >= gridH_) return;
        cells_[index(px, py)] = 1;
    };

    set(x + 1, y);
    set(x + 2, y + 1);
    set(x, y + 2);
    set(x + 1, y + 2);
    set(x + 2, y + 2);
}

void GameOfLifeLayer::seedBlinker(int x, int y) {
    for (int dx = -1; dx <= 1; ++dx) {
        int px = x + dx;
        if (px < 0 || px >= gridW_ || y < 0 || y >= gridH_) continue;
        cells_[index(px, y)] = 1;
    }
}

void GameOfLifeLayer::seedCross(int x, int y, int radius) {
    for (int dx = -radius; dx <= radius; ++dx) {
        int px = x + dx;
        if (px < 0 || px >= gridW_ || y < 0 || y >= gridH_) continue;
        cells_[index(px, y)] = 1;
    }
    for (int dy = -radius; dy <= radius; ++dy) {
        int py = y + dy;
        if (py < 0 || py >= gridH_ || x < 0 || x >= gridW_) continue;
        cells_[index(x, py)] = 1;
    }
}

void GameOfLifeLayer::applyPresetInternal(int presetIndex) {
    presetIndex = ofClamp(presetIndex, 0, std::max(0, kPresetCount - 1));
    if (presetIndex == 0) {
        randomize(paramSeedDensity_);
        return;
    }

    clearCells();

    switch (presetIndex) {
    case 1: { // Gliders grid
        const int spacing = 16;
        for (int y = 0; y < gridH_; y += spacing) {
            for (int x = 0; x < gridW_; x += spacing) {
                seedGlider(x, y);
            }
        }
        break;
    }
    case 2: { // Waves (diagonal stripes)
        for (int y = 0; y < gridH_; ++y) {
            for (int x = 0; x < gridW_; ++x) {
                if (((x + y) % 8) == 0) {
                    cells_[index(x, y)] = 1;
                }
            }
        }
        break;
    }
    case 3: { // Crossfire pattern
        for (int y = 8; y < gridH_; y += 24) {
            for (int x = 8; x < gridW_; x += 24) {
                seedCross(x, y, 5);
                seedBlinker(x + 8, y);
                seedBlinker(x, y + 8);
            }
        }
        break;
    }
    default:
        randomize(paramSeedDensity_);
        return;
    }

    dirty_ = true;
}

float GameOfLifeLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) return 0.0f;
    return std::max(0.0f, timeSeconds) * (bpm / 60.0f);
}

float GameOfLifeLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        if (params.bpm <= 0.0f) return 0.0f;
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

void GameOfLifeLayer::schedulePendingReseed(float beatPosition) {
    const float quantizeBeats = std::round(std::max(0.0f, paramReseedQuantizeBeats_));
    if (quantizeBeats <= 0.0f) {
        triggerReseed();
        return;
    }

    const float currentMultiple = std::floor(beatPosition / quantizeBeats);
    pendingReseedBeat_ = (currentMultiple + 1.0f) * quantizeBeats;
    reseedPending_ = true;
}

void GameOfLifeLayer::triggerReseed(float density) {
    randomize(density);
    reseedPending_ = false;
    pendingReseedBeat_ = -1.0f;
}
