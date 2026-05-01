#pragma once

#include "../MenuSkin.h"

#include "ofGraphics.h"
#include "ofRectangle.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

inline float hudLineHeight(const HudSkin* hud) {
    return hud ? hud->lineHeight : 16.0f;
}

inline float hudTypographyScale(const HudSkin* hud) {
    return hud ? std::max(0.01f, hud->typographyScale) : 1.0f;
}

inline float hudBlockPadding(const HudSkin* hud) {
    return hud ? hud->blockPadding : 14.0f;
}

inline float hudBadgeRadius(const HudSkin* hud) {
    return hud ? hud->badgeCornerRadius : 4.0f;
}

inline float hudBadgePadding(const HudSkin* hud) {
    return hud ? hud->badgePadding : 6.0f;
}

inline float hudBadgeHeight(const HudSkin* hud) {
    return hud ? hud->badgeHeight : 20.0f;
}

inline ofColor hudPanelColor(const HudSkin* hud) {
    return hud ? hud->overlayBackground : ofColor(6, 6, 6, 220);
}

inline ofColor hudChromeColor(const HudSkin* hud) {
    return hud ? hud->overlayChrome : ofColor(0, 255, 140, 110);
}

inline ofColor hudTextColor(const HudSkin* hud) {
    return hud ? hud->textPrimary : ofColor(235);
}

inline ofColor hudMutedColor(const HudSkin* hud) {
    return hud ? hud->textMuted : ofColor(150);
}

inline ofColor hudAccentColor(const HudSkin* hud) {
    return hud ? hud->accent : ofColor(0, 255, 140);
}

inline ofColor hudWarningColor(const HudSkin* hud) {
    return hud ? hud->warning : ofColor(255, 120, 120);
}

inline ofColor hudBadgeFill(const HudSkin* hud) {
    return hud ? hud->badgeBackground : ofColor(18, 18, 18, 235);
}

inline ofColor hudBadgeStroke(const HudSkin* hud) {
    return hud ? hud->badgeStroke : hudChromeColor(hud);
}

inline ofColor hudBadgeText(const HudSkin* hud) {
    return hud ? hud->badgeText : hudTextColor(hud);
}

inline void drawHudPanelBackground(const ofRectangle& bounds, const HudSkin* hud) {
    ofFill();
    ofSetColor(hudPanelColor(hud));
    ofDrawRectRounded(bounds, hudBadgeRadius(hud));
    ofNoFill();
    ofSetColor(hudChromeColor(hud));
    ofSetLineWidth(1.5f);
    ofDrawRectRounded(bounds, hudBadgeRadius(hud));
    ofFill();
}

inline float computeHudTextHeight(const std::string& text, float minHeight, const HudSkin* hud) {
    if (text.empty()) {
        return minHeight;
    }
    int lines = 1;
    for (char c : text) {
        if (c == '\n') {
            ++lines;
        }
    }
    float height = static_cast<float>(lines) * hudLineHeight(hud) + hudBlockPadding(hud) * 2.0f;
    return std::max(minHeight, height);
}

inline int hudMaxVisibleLines(float boundsHeight, const HudSkin* hud) {
    float available = boundsHeight - hudBlockPadding(hud) * 2.0f;
    if (available <= 0.0f) {
        return 0;
    }
    float lineH = hudLineHeight(hud);
    if (lineH <= 0.0f) {
        return 0;
    }
    return static_cast<int>(std::floor(available / lineH));
}

inline std::string hudEllipsizeText(const std::string& text,
                                    float columnWidth,
                                    const HudSkin* hud,
                                    int maxLines = -1) {
    if (text.empty()) {
        return text;
    }
    if (maxLines == 0) {
        return std::string();
    }
    float contentWidth = columnWidth - hudBlockPadding(hud) * 2.0f;
    if (contentWidth <= 0.0f) {
        return std::string();
    }
    const float kCharWidth = 8.0f * hudTypographyScale(hud);
    int maxChars = static_cast<int>(std::floor(contentWidth / kCharWidth));
    if (maxChars <= 0) {
        return std::string();
    }
    auto truncateLine = [maxChars](const std::string& line) -> std::string {
        if (static_cast<int>(line.size()) <= maxChars) {
            return line;
        }
        if (maxChars <= 3) {
            return std::string(std::max(maxChars, 0), '.');
        }
        return line.substr(0, maxChars - 3) + "...";
    };
    std::vector<std::string> rawLines;
    std::string current;
    for (char c : text) {
        if (c == '\n') {
            rawLines.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current.push_back(c);
        }
    }
    rawLines.push_back(current);

    std::vector<std::string> resultLines;
    resultLines.reserve(rawLines.size());
    bool truncatedLines = false;
    for (std::size_t i = 0; i < rawLines.size(); ++i) {
        if (maxLines > 0 && static_cast<int>(resultLines.size()) >= maxLines) {
            truncatedLines = true;
            break;
        }
        resultLines.push_back(truncateLine(rawLines[i]));
    }
    if (maxLines > 0 && static_cast<int>(resultLines.size()) >= maxLines && rawLines.size() > static_cast<std::size_t>(maxLines)) {
        truncatedLines = true;
    }
    if (truncatedLines && !resultLines.empty()) {
        std::string& tail = resultLines.back();
        if (static_cast<int>(tail.size()) >= maxChars) {
            if (maxChars >= 3 && tail.size() >= 3) {
                tail = tail.substr(0, maxChars - 3) + "...";
            }
        } else {
            if (maxChars <= 3) {
                tail = std::string(std::max(maxChars, 0), '.');
            } else if (static_cast<int>(tail.size()) >= maxChars - 3) {
                tail = tail.substr(0, maxChars - 3) + "...";
            } else {
                tail += "...";
            }
        }
    }
    std::string result;
    for (std::size_t i = 0; i < resultLines.size(); ++i) {
        result += resultLines[i];
        if (i + 1 < resultLines.size()) {
            result.push_back('\n');
        }
    }
    return result;
}
