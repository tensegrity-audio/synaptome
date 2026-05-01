#pragma once

#include <memory>
#include <string>
#include <algorithm>
#include <cctype>
#include "ofGraphics.h"
#include "ui/MenuSkin.h"

class OverlayWidget {
public:
    enum class Band : int {
        Hud = 0,
        Console,
        Workbench
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
    };

    struct SetupParams { void* app = nullptr; const HudSkin* hudSkin = nullptr; };
    struct UpdateParams { float deltaTime = 0.0f; void* app = nullptr; const HudSkin* hudSkin = nullptr; };
    struct DrawParams { ofRectangle bounds; void* app = nullptr; const HudSkin* hudSkin = nullptr; };

    virtual ~OverlayWidget() = default;
    virtual const Metadata& metadata() const = 0;
    virtual void setup(const SetupParams&) {}
    virtual void update(const UpdateParams&) {}
    virtual void draw(const DrawParams&) {}
    virtual float preferredHeight(float) const { return metadata().defaultHeight; }
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

// Simple concrete widget for tests
class SimpleOverlayWidget : public OverlayWidget {
public:
    explicit SimpleOverlayWidget(const Metadata& m) : meta_(m) {}
    const Metadata& metadata() const override { return meta_; }
private:
    Metadata meta_;
};
