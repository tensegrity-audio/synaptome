#pragma once

#include <string>
#include <algorithm>
#include <cctype>

#include "ofRectangle.h"
#include "../MenuSkin.h"

class OverlayWidget {
public:
    enum class Band : int {
        Hud = 0,
        Console,
        Workbench
    };

    enum class Target : int {
        Projector = 0,
        Controller,
        Both
    };

    struct Metadata {
        std::string id;
        std::string label;
        std::string category;
        std::string description;
        int defaultColumn = 0;
        float defaultHeight = 220.0f;
        float minHeight = 120.0f;
        bool allowsDetach = false;
        Band band = Band::Hud;
        Target target = Target::Projector;
    };

    struct SetupParams {
        class ofApp* app = nullptr;
        const HudSkin* hudSkin = nullptr;
    };

    struct UpdateParams {
        float deltaTime = 0.0f;
        class ofApp* app = nullptr;
        const HudSkin* hudSkin = nullptr;
    };

    struct DrawParams {
        ofRectangle bounds;
        class ofApp* app = nullptr;
        const HudSkin* hudSkin = nullptr;
    };

    virtual ~OverlayWidget() = default;

    virtual const Metadata& metadata() const = 0;
    virtual void setup(const SetupParams& params) { (void)params; }
    virtual void update(const UpdateParams& params) { (void)params; }
    virtual void draw(const DrawParams& params) = 0;
    virtual float preferredHeight(float width) const { (void)width; return metadata().defaultHeight; }
    virtual bool isVisible() const { return true; }
};

inline std::string overlayBandToString(OverlayWidget::Band band) {
    switch (band) {
    case OverlayWidget::Band::Hud: return "hud";
    case OverlayWidget::Band::Console: return "console";
    case OverlayWidget::Band::Workbench: return "workbench";
    }
    return "hud";
}

inline OverlayWidget::Band overlayBandFromString(const std::string& value,
                                                 OverlayWidget::Band fallback = OverlayWidget::Band::Hud) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lowered == "hud") return OverlayWidget::Band::Hud;
    if (lowered == "console") return OverlayWidget::Band::Console;
    if (lowered == "workbench") return OverlayWidget::Band::Workbench;
    return fallback;
}

inline std::string overlayTargetToString(OverlayWidget::Target target) {
    switch (target) {
    case OverlayWidget::Target::Controller: return "controller";
    case OverlayWidget::Target::Both: return "both";
    case OverlayWidget::Target::Projector:
    default:
        return "projector";
    }
}

inline OverlayWidget::Target overlayTargetFromString(const std::string& value,
                                                     OverlayWidget::Target fallback = OverlayWidget::Target::Projector) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lowered == "controller") return OverlayWidget::Target::Controller;
    if (lowered == "both") return OverlayWidget::Target::Both;
    if (lowered == "projector") return OverlayWidget::Target::Projector;
    return fallback;
}
