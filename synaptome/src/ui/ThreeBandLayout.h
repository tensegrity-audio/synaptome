#pragma once

#include "ofRectangle.h"
#include <algorithm>
#include <cmath>

struct ThreeBandLayout {
    struct Band {
        ofRectangle bounds;
    };

    Band hud;
    Band console;
    Band workbench;
};

class ThreeBandLayoutManager {
public:
    ThreeBandLayoutManager()
        : hudRatio_(1.0f / 3.0f)
        , consoleRatio_(1.0f / 3.0f)
        , workbenchRatio_(1.0f / 3.0f)
        , gutterHudConsole_(16.0f)
        , gutterConsoleWorkbench_(16.0f) {}

    void setBandRatios(float hud, float console, float workbench) {
        hud = std::max(0.0f, hud);
        console = std::max(0.0f, console);
        workbench = std::max(0.0f, workbench);
        float sum = hud + console + workbench;
        if (sum <= 0.0f) {
            hudRatio_ = consoleRatio_ = workbenchRatio_ = 1.0f / 3.0f;
            return;
        }
        hudRatio_ = hud / sum;
        consoleRatio_ = console / sum;
        workbenchRatio_ = workbench / sum;
    }

    void setGutter(float gutter) {
        gutterHudConsole_ = std::max(0.0f, gutter);
        gutterConsoleWorkbench_ = std::max(0.0f, gutter);
    }

    void setGutters(float hudToConsole, float consoleToWorkbench) {
        gutterHudConsole_ = std::max(0.0f, hudToConsole);
        gutterConsoleWorkbench_ = std::max(0.0f, consoleToWorkbench);
    }

    ThreeBandLayout layoutForSize(float width, float height) const {
        ThreeBandLayout layout;
        float safeWidth = std::max(0.0f, width);
        float safeHeight = std::max(0.0f, height);
        float hudConsole = std::min(gutterHudConsole_, safeHeight > 0.0f ? safeHeight * 0.25f : 0.0f);
        float consoleWorkbench = std::min(gutterConsoleWorkbench_, safeHeight > 0.0f ? safeHeight * 0.25f : 0.0f);
        float availableHeight = std::max(0.0f, safeHeight - hudConsole - consoleWorkbench);

        float hudHeight = availableHeight * hudRatio_;
        float consoleHeight = availableHeight * consoleRatio_;
        float workbenchHeight = availableHeight * workbenchRatio_;

        // Adjust for rounding so the bands always consume the remaining space.
        float usedHeight = hudHeight + consoleHeight + workbenchHeight;
        if (availableHeight > 0.0f) {
            float error = availableHeight - usedHeight;
            workbenchHeight = std::max(0.0f, workbenchHeight + error);
        }

        float y = 0.0f;
        layout.hud.bounds.set(0.0f, y, safeWidth, hudHeight);
        y += hudHeight + hudConsole;

        layout.console.bounds.set(0.0f, y, safeWidth, consoleHeight);
        y += consoleHeight + consoleWorkbench;

        layout.workbench.bounds.set(0.0f, y, safeWidth, workbenchHeight);
        return layout;
    }

private:
    float hudRatio_;
    float consoleRatio_;
    float workbenchRatio_;
    float gutterHudConsole_;
    float gutterConsoleWorkbench_;
};
