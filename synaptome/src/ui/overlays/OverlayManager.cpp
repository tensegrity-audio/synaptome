#include "OverlayManager.h"

#include <algorithm>
#include <iterator>

#include "ofMath.h"
#include "ofLog.h"

OverlayManager::OverlayManager() {
    ensureDefaultColumns();
}

void OverlayManager::setLayoutDirtyCallback(std::function<void()> callback) {
    layoutDirtyCallback_ = std::move(callback);
}

void OverlayManager::setHost(ofApp* app) {
    hostApp_ = app;
    for (auto& entry : entries_) {
        if (!entry.setupComplete && entry.widget) {
            OverlayWidget::SetupParams setup;
            setup.app = hostApp_;
            setup.hudSkin = &hudSkin_;
            entry.widget->setup(setup);
            entry.setupComplete = true;
        }
    }
}

void OverlayManager::setColumnSpecs(std::vector<ColumnSpec> specs) {
    bool changed = false;
    if (columnSpecs_.size() != specs.size()) {
        changed = true;
    } else {
        for (std::size_t i = 0; i < columnSpecs_.size(); ++i) {
            if (columnSpecs_[i].id != specs[i].id || columnSpecs_[i].width != specs[i].width) {
                changed = true;
                break;
            }
        }
    }
    columnSpecs_ = std::move(specs);
    ensureDefaultColumns();
    if (changed) {
        notifyLayoutChanged();
    }
}

void OverlayManager::setHudSkin(const HudSkin& hudSkin) {
    hudSkin_ = hudSkin;
}

void OverlayManager::setHudVisible(bool visible) {
    if (hudVisible_ == visible) {
        return;
    }
    hudVisible_ = visible;
}

bool OverlayManager::registerWidget(std::unique_ptr<OverlayWidget> widget) {
    if (!widget) {
        return false;
    }
    const auto& meta = widget->metadata();
    if (meta.id.empty()) {
        return false;
    }
    if (hasWidget(meta.id)) {
        return false;
    }

    ensureDefaultColumns();
    WidgetEntry entry;
    entry.metadata = meta;
    int maxIndex = static_cast<int>(columnSpecs_.size());
    if (maxIndex <= 0) {
        entry.columnIndex = 0;
    } else {
        int lastIndex = maxIndex - 1;
        int desired = meta.defaultColumn;
        if (desired < 0) desired = 0;
        if (desired > lastIndex) desired = lastIndex;
        entry.columnIndex = desired;
    }
    entry.widget = std::move(widget);
    entry.visible = true;
    entry.collapsed = false;
    entry.setupComplete = false;

    if (hostApp_) {
        OverlayWidget::SetupParams setup;
        setup.app = hostApp_;
        setup.hudSkin = &hudSkin_;
        entry.widget->setup(setup);
        entry.setupComplete = true;
    }

    entries_.push_back(std::move(entry));
    return true;
}

bool OverlayManager::hasWidget(const std::string& id) const {
    return findEntry(id) != nullptr;
}

void OverlayManager::setWidgetVisible(const std::string& id, bool visible) {
    if (auto* entry = findEntry(id)) {
        if (entry->visible != visible) {
            entry->visible = visible;
            notifyLayoutChanged();
        }
    }
}

void OverlayManager::setWidgetCollapsed(const std::string& id, bool collapsed) {
    if (auto* entry = findEntry(id)) {
        if (entry->collapsed != collapsed) {
            entry->collapsed = collapsed;
            notifyLayoutChanged();
        }
    }
}

void OverlayManager::setWidgetColumn(const std::string& id, int columnIndex) {
    if (auto* entry = findEntry(id)) {
        int clamped = columnIndex;
        if (clamped < 0) {
            clamped = 0;
        }
        int lastColumn = static_cast<int>(columnSpecs_.size());
        if (lastColumn > 0) {
            --lastColumn;
            if (clamped > lastColumn) {
                clamped = lastColumn;
            }
        }
        if (entry->columnIndex != clamped) {
            entry->columnIndex = clamped;
            notifyLayoutChanged();
        }
    }
}

void OverlayManager::setWidgetBand(const std::string& id, OverlayWidget::Band band) {
    if (auto* entry = findEntry(id)) {
        if (entry->metadata.band != band) {
            entry->metadata.band = band;
            notifyLayoutChanged();
        }
    }
}

OverlayWidget::Band OverlayManager::widgetBand(const std::string& id) const {
    if (const auto* entry = findEntry(id)) {
        return entry->metadata.band;
    }
    return OverlayWidget::Band::Hud;
}

int OverlayManager::widgetColumn(const std::string& id) const {
    if (const auto* entry = findEntry(id)) {
        return entry->columnIndex;
    }
    return 0;
}

bool OverlayManager::widgetCollapsed(const std::string& id) const {
    if (const auto* entry = findEntry(id)) {
        return entry->collapsed;
    }
    return false;
}

OverlayManager::LayoutState OverlayManager::captureState() const {
    LayoutState state;
    state.columns = columnSpecs_;
    for (const auto& entry : entries_) {
        LayoutState::WidgetState widget;
        widget.id = entry.metadata.id;
        widget.columnIndex = entry.columnIndex;
        widget.visible = entry.visible;
        widget.collapsed = entry.collapsed;
        widget.bandId = overlayBandToString(entry.metadata.band);
        state.widgets.push_back(widget);
    }
    return state;
}

bool OverlayManager::applyState(const LayoutState& state, bool notify) {
    bool previousSuppress = suppressLayoutCallbacks_;
    suppressLayoutCallbacks_ = true;
    bool changed = false;

    if (!state.columns.empty()) {
        if (columnSpecs_.size() != state.columns.size()) {
            changed = true;
        } else {
            for (std::size_t i = 0; i < columnSpecs_.size(); ++i) {
                if (columnSpecs_[i].id != state.columns[i].id || columnSpecs_[i].width != state.columns[i].width) {
                    changed = true;
                    break;
                }
            }
        }
        columnSpecs_ = state.columns;
        ensureDefaultColumns();
    }

    const int columnCount = static_cast<int>(columnSpecs_.size());

    for (const auto& widgetState : state.widgets) {
        if (auto* entry = findEntry(widgetState.id)) {
            int targetColumn = widgetState.columnIndex;
            if (targetColumn < 0) {
                targetColumn = 0;
            }
            if (columnCount > 0) {
                int lastColumn = columnCount - 1;
                if (targetColumn > lastColumn) {
                    targetColumn = lastColumn;
                }
            }
            if (entry->columnIndex != targetColumn) {
                entry->columnIndex = targetColumn;
                changed = true;
            }
            if (entry->visible != widgetState.visible) {
                entry->visible = widgetState.visible;
                changed = true;
            }
            if (entry->collapsed != widgetState.collapsed) {
                entry->collapsed = widgetState.collapsed;
                changed = true;
            }
            if (!widgetState.bandId.empty()) {
                OverlayWidget::Band desiredBand = overlayBandFromString(widgetState.bandId, entry->metadata.band);
                if (entry->metadata.band != desiredBand) {
                    entry->metadata.band = desiredBand;
                    changed = true;
                }
            }
        }
    }

    suppressLayoutCallbacks_ = previousSuppress;
    if (notify && changed) {
        notifyLayoutChanged();
    }
    return changed;
}

void OverlayManager::update(const UpdateParams& params) {
    for (auto& entry : entries_) {
        if (!entry.widget) {
            continue;
        }
        if (!entry.setupComplete && (params.app || hostApp_)) {
            OverlayWidget::SetupParams setup;
            setup.app = params.app ? params.app : hostApp_;
            setup.hudSkin = &hudSkin_;
            entry.widget->setup(setup);
            entry.setupComplete = true;
        }
        OverlayWidget::UpdateParams updateParams;
        updateParams.deltaTime = params.deltaTime;
        updateParams.app = params.app ? params.app : hostApp_;
        updateParams.hudSkin = &hudSkin_;
        entry.widget->update(updateParams);
    }
}

void OverlayManager::draw(const DrawParams& params) {
    if (!hudVisible_) {
        return;
    }
    if (entries_.empty()) {
        return;
    }

    ensureDefaultColumns();

    const auto columnCount = columnSpecs_.size();
    if (columnCount == 0) {
        return;
    }

    const float marginX = 12.0f;
    const float marginY = 12.0f;

    struct BandLayoutCache {
        bool initialized = false;
        ofRectangle bounds;
        float columnWidth = 0.0f;
        std::vector<float> columnX;
        std::vector<float> columnY;
    };

    std::array<BandLayoutCache, 3> bandLayouts;

    auto boundsForBand = [&](OverlayWidget::Band band) -> ofRectangle {
        if (!params.useThreeBandLayout) {
            return params.bounds;
        }
        switch (band) {
        case OverlayWidget::Band::Hud: return params.layout.hud.bounds;
        case OverlayWidget::Band::Console: return params.layout.console.bounds;
        case OverlayWidget::Band::Workbench: return params.layout.workbench.bounds;
        }
        return params.bounds;
    };

    auto ensureBandLayout = [&](int bandIndex, const ofRectangle& bandBounds) -> BandLayoutCache& {
        BandLayoutCache& cache = bandLayouts[bandIndex];
        bool boundsChanged = !cache.initialized ||
                             cache.bounds.x != bandBounds.x ||
                             cache.bounds.y != bandBounds.y ||
                             cache.bounds.width != bandBounds.width ||
                             cache.bounds.height != bandBounds.height;
        if (!cache.initialized || boundsChanged || static_cast<int>(cache.columnX.size()) != columnCount) {
            cache.bounds = bandBounds;
            cache.initialized = true;
            cache.columnX.assign(columnCount, 0.0f);
            cache.columnY.assign(columnCount, bandBounds.y + marginY);
            float usableWidth = std::max(0.0f, bandBounds.width - marginX * (static_cast<float>(columnCount) + 1.0f));
            cache.columnWidth = columnCount > 0 ? usableWidth / static_cast<float>(columnCount) : bandBounds.width;
            for (std::size_t i = 0; i < columnCount; ++i) {
                cache.columnX[i] = bandBounds.x + marginX + static_cast<float>(i) * (cache.columnWidth + marginX);
            }
        }
        return cache;
    };

    for (auto& entry : entries_) {
        if (!entry.widget || !entry.visible || entry.collapsed) {
            continue;
        }
        int columnIndex = entry.columnIndex;
        if (columnIndex < 0) {
            columnIndex = 0;
        }
        int lastColumn = static_cast<int>(columnCount) - 1;
        if (columnIndex > lastColumn) {
            columnIndex = lastColumn;
        }

        const auto bandIndex = static_cast<int>(entry.metadata.band);
        ofRectangle bandBounds = boundsForBand(entry.metadata.band);
        if (bandBounds.width <= 0.0f || bandBounds.height <= 0.0f) {
            continue;
        }

        BandLayoutCache& cache = ensureBandLayout(bandIndex, bandBounds);
        if (cache.columnX.empty() || columnIndex >= static_cast<int>(cache.columnX.size())) {
            continue;
        }

        float columnWidth = cache.columnWidth;
        float height = entry.widget->preferredHeight(columnWidth);
        height = std::max(entry.metadata.minHeight, height);
        float yCursor = cache.columnY[columnIndex];
        float maxHeight = (bandBounds.y + bandBounds.height) - marginY - yCursor;
        if (maxHeight <= 0.0f) {
            continue;
        }
        height = std::min(height, maxHeight);
        if (height <= 0.0f) {
            continue;
        }

        OverlayWidget::DrawParams drawParams;
        drawParams.app = params.app ? params.app : hostApp_;
        drawParams.hudSkin = &hudSkin_;
        float x = cache.columnX[columnIndex];
        float y = yCursor;
        drawParams.bounds = ofRectangle(x, y, columnWidth, height);

#ifndef NDEBUG
        if (!bandBounds.inside(drawParams.bounds.getTopLeft()) ||
            !bandBounds.inside(drawParams.bounds.getBottomRight())) {
            ofLogWarning("OverlayManager") << "Widget '" << entry.metadata.id
                                           << "' attempted to draw outside band bounds";
        }
#endif

        entry.widget->draw(drawParams);
        cache.columnY[columnIndex] = drawParams.bounds.getBottom() + marginY;
    }
}

OverlayManager::WidgetEntry* OverlayManager::findEntry(const std::string& id) {
    auto it = std::find_if(entries_.begin(), entries_.end(), [&](const WidgetEntry& entry) {
        return entry.metadata.id == id;
    });
    return (it != entries_.end()) ? &(*it) : nullptr;
}

const OverlayManager::WidgetEntry* OverlayManager::findEntry(const std::string& id) const {
    auto it = std::find_if(entries_.begin(), entries_.end(), [&](const WidgetEntry& entry) {
        return entry.metadata.id == id;
    });
    return (it != entries_.end()) ? &(*it) : nullptr;
}

void OverlayManager::ensureDefaultColumns() {
    static const ColumnSpec kDefaultColumns[] = {
        {"primary", 420.0f},
        {"secondary", 320.0f},
        {"tertiary", 280.0f},
        {"quaternary", 260.0f}
    };
    if (columnSpecs_.empty()) {
        columnSpecs_.assign(std::begin(kDefaultColumns), std::end(kDefaultColumns));
        return;
    }
    const std::size_t desiredCount = std::size(kDefaultColumns);
    if (columnSpecs_.size() >= desiredCount) {
        return;
    }
    for (std::size_t i = columnSpecs_.size(); i < desiredCount; ++i) {
        columnSpecs_.push_back(kDefaultColumns[i]);
    }
}

void OverlayManager::notifyLayoutChanged() {
    if (suppressLayoutCallbacks_) {
        return;
    }
    if (layoutDirtyCallback_) {
        layoutDirtyCallback_();
    }
}
