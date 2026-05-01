#include "PostEffectChain.h"
#include "MotionExtractProcessor.h"
#include "ofGraphics.h"
#include "ofShader.h"
#include "ofTrueTypeFont.h"
#include "ofImage.h"
#include "ofUtils.h"
#include "ofMath.h"
#include "ofFileUtils.h"
#include "ofJson.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace {
    namespace fs = std::filesystem;
    constexpr const char* kRouteDescription = "0=Off 1=Console 2=Global";

    constexpr int kGlyphTableSize = 112;
    constexpr int kAsciiGlyphFirst = 32;
    constexpr int kAsciiGlyphCount = 95;
    constexpr int kSupersampleDescriptorTapCount = 6;
    constexpr const char* kAsciiSupersampleManifestPath = "fonts/ascii_supersample/manifest.json";

    constexpr int kGlyphLowercase[kGlyphTableSize] = {
        4, 0, 12, 4, 4, 4, 14,
        12, 4, 4, 4, 4, 4, 14,
        2, 0, 2, 2, 2, 18, 12,
        4, 4, 31, 4, 4, 5, 2,
        0, 0, 22, 25, 16, 16, 16,
        0, 0, 14, 17, 16, 17, 14,
        0, 0, 22, 25, 17, 17, 17,
        0, 0, 17, 17, 17, 19, 13,
        0, 0, 15, 16, 14, 1, 30,
        0, 0, 14, 17, 17, 17, 14,
        0, 0, 12, 2, 14, 18, 15,
        0, 0, 14, 17, 31, 16, 14,
        0, 0, 14, 19, 19, 13, 1,
        1, 1, 13, 19, 17, 19, 13,
        0, 0, 26, 21, 21, 21, 21,
        0, 0, 17, 17, 21, 21, 10
    };

    constexpr int kGlyphNumbers[kGlyphTableSize] = {
        4, 12, 4, 4, 4, 4, 14,
        31, 1, 1, 2, 4, 8, 16,
        2, 6, 10, 18, 31, 2, 2,
        14, 17, 1, 14, 16, 16, 31,
        31, 1, 2, 6, 1, 17, 14,
        31, 16, 30, 1, 1, 17, 14,
        7, 8, 16, 30, 17, 17, 14,
        14, 17, 17, 15, 1, 2, 28,
        14, 17, 19, 21, 25, 17, 14,
        14, 17, 17, 14, 17, 17, 14,
        14, 17, 17, 15, 1, 2, 28,
        14, 17, 19, 21, 25, 17, 14,
        7, 8, 16, 30, 17, 17, 14,
        14, 17, 17, 14, 17, 17, 14,
        14, 17, 17, 14, 17, 17, 14,
        14, 17, 17, 14, 17, 17, 14
    };

    constexpr int kGlyphBraille[kGlyphTableSize] = {
        0, 0, 0, 0, 0, 0, 0,
        0, 8, 8, 0, 0, 0, 0,
        0, 2, 2, 0, 0, 0, 0,
        0, 10, 10, 0, 0, 0, 0,
        0, 0, 0, 0, 8, 8, 0,
        0, 8, 8, 0, 8, 8, 0,
        0, 2, 2, 0, 8, 8, 0,
        0, 10, 10, 0, 8, 8, 0,
        0, 0, 0, 0, 2, 2, 0,
        0, 8, 8, 0, 2, 2, 0,
        0, 2, 2, 0, 2, 2, 0,
        0, 10, 10, 0, 2, 2, 0,
        0, 0, 0, 0, 10, 10, 0,
        0, 8, 8, 0, 10, 10, 0,
        0, 2, 2, 0, 10, 10, 0,
        0, 10, 10, 0, 10, 10, 0
    };

    const int* glyphTableForSet(int setIndex) {
        switch (setIndex) {
        case 1:
            return kGlyphNumbers;
        case 2:
            return kGlyphBraille;
        default:
            return kGlyphLowercase;
        }
    }

    std::vector<int> buildGlyphIndexList(const std::string& chars) {
        std::vector<int> indices;
        indices.reserve(chars.size());
        for (unsigned char c : chars) {
            if (c >= kAsciiGlyphFirst && c < kAsciiGlyphFirst + kAsciiGlyphCount) {
                indices.push_back(static_cast<int>(c) - kAsciiGlyphFirst);
            }
        }
        return indices;
    }

    std::vector<int> buildFullGlyphIndexList() {
        std::vector<int> indices;
        indices.reserve(kAsciiGlyphCount);
        for (int i = 0; i < kAsciiGlyphCount; ++i) {
            indices.push_back(i);
        }
        return indices;
    }

    struct TextureStats {
        bool readSucceeded = false;
        int minValue = 0;
        int maxValue = 0;
    };

    TextureStats computeTextureStats(const ofTexture& texture) {
        TextureStats stats;
        if (!texture.isAllocated()) {
            return stats;
        }
        ofPixels pixels;
        texture.readToPixels(pixels);
        if (pixels.size() == 0) {
            return stats;
        }
        int minValue = 255;
        int maxValue = 0;
        const unsigned char* data = pixels.getData();
        const size_t total = pixels.size();
        for (size_t i = 0; i < total; ++i) {
            const unsigned char value = data[i];
            if (value < minValue) {
                minValue = value;
            }
            if (value > maxValue) {
                maxValue = value;
            }
        }
        stats.readSucceeded = true;
        stats.minValue = minValue;
        stats.maxValue = maxValue;
        return stats;
    }

    void logGlyphSetSummary(int index, const std::vector<int>& set) {
        if (set.empty()) {
            ofLogNotice("AsciiSupersampleAtlas")
                << "glyphSet[" << index << "] empty; falling back to full ASCII table.";
            return;
        }
        std::string glyphChars;
        for (const int code : set) {
            char c = static_cast<char>(kAsciiGlyphFirst + code);
            glyphChars.push_back(c);
        }
        ofLogNotice("AsciiSupersampleAtlas")
            << "glyphSet[" << index << "] size=" << set.size()
            << " chars=\"" << glyphChars << "\"";
    }

    bool loadJsonRelativeToData(const std::string& relativePath, ofJson& out) {
        const std::string absolutePath = ofToDataPath(relativePath, true);
        if (!ofFile::doesFileExist(absolutePath)) {
            return false;
        }
        try {
            out = ofLoadJson(absolutePath);
        } catch (const std::exception& ex) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Failed to parse" << relativePath << "-" << ex.what();
            return false;
        }
        if (!out.is_object()) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "JSON file" << relativePath << "is not an object.";
            return false;
        }
        return true;
    }

    bool populateAtlasFromPrebakedLut(const ofJson& lutDoc,
                                      const std::string& atlasImagePath,
                                      AsciiSupersampleAtlas::FontAtlas& atlas) {
        if (!lutDoc.contains("rects") || !lutDoc["rects"].is_array()) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Prebaked LUT missing rects array.";
            return false;
        }
        const auto& rectsNode = lutDoc["rects"];
        if (rectsNode.size() != kAsciiGlyphCount) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Prebaked LUT rect count" << rectsNode.size()
                << "does not match expected" << kAsciiGlyphCount;
            return false;
        }
        for (int i = 0; i < kAsciiGlyphCount; ++i) {
            const auto& rectNode = rectsNode[i];
            if (!rectNode.is_array() || rectNode.size() < 4) {
                ofLogWarning("AsciiSupersampleAtlas")
                    << "Invalid rect data for glyph" << i;
                return false;
            }
            atlas.rects[i] = glm::vec4(
                rectNode[0].get<float>(),
                rectNode[1].get<float>(),
                rectNode[2].get<float>(),
                rectNode[3].get<float>());
        }

        if (!lutDoc.contains("descriptors") || !lutDoc["descriptors"].is_array()) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Prebaked LUT missing descriptors array.";
            return false;
        }
        const auto& descriptorsNode = lutDoc["descriptors"];
        if (descriptorsNode.size() != kAsciiGlyphCount) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Descriptor glyph count mismatch (" << descriptorsNode.size()
                << " != " << kAsciiGlyphCount << ")";
            return false;
        }
        float descriptorMin = std::numeric_limits<float>::max();
        float descriptorMax = std::numeric_limits<float>::lowest();
        for (int i = 0; i < kAsciiGlyphCount; ++i) {
            const auto& glyphDesc = descriptorsNode[i];
            if (!glyphDesc.is_array() ||
                glyphDesc.size() < kSupersampleDescriptorTapCount) {
                ofLogWarning("AsciiSupersampleAtlas")
                    << "Descriptor tap mismatch for glyph" << i;
                return false;
            }
            for (int tap = 0; tap < kSupersampleDescriptorTapCount; ++tap) {
                atlas.descriptors[i * kSupersampleDescriptorTapCount + tap] =
                    glyphDesc[tap].get<float>();
            }
            for (int tap = 0; tap < kSupersampleDescriptorTapCount; ++tap) {
                float value = atlas.descriptors[i * kSupersampleDescriptorTapCount + tap];
                descriptorMin = std::min(descriptorMin, value);
                descriptorMax = std::max(descriptorMax, value);
            }
        }

        if (lutDoc.contains("descriptor_means") &&
            lutDoc["descriptor_means"].is_array() &&
            lutDoc["descriptor_means"].size() == kAsciiGlyphCount) {
            const auto& meansNode = lutDoc["descriptor_means"];
            for (int i = 0; i < kAsciiGlyphCount; ++i) {
                atlas.descriptorMeans[i] = meansNode[i].get<float>();
            }
        } else {
            for (int i = 0; i < kAsciiGlyphCount; ++i) {
                float mean = 0.0f;
                for (int tap = 0; tap < kSupersampleDescriptorTapCount; ++tap) {
                    mean += atlas.descriptors[i * kSupersampleDescriptorTapCount + tap];
                }
                atlas.descriptorMeans[i] =
                    mean / static_cast<float>(kSupersampleDescriptorTapCount);
            }
        }

        ofPixels atlasPixels;
        const std::string imageAbsolutePath = ofToDataPath(atlasImagePath, true);
        if (!ofFile::doesFileExist(imageAbsolutePath)) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Prebaked atlas image missing at" << atlasImagePath;
            return false;
        }
        if (!ofLoadImage(atlasPixels, imageAbsolutePath)) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Failed to load prebaked atlas image" << atlasImagePath;
            return false;
        }
        if (atlasPixels.getNumChannels() >= 4) {
            unsigned char* raw = atlasPixels.getData();
            const int stride = atlasPixels.getNumChannels();
            const int totalPixels = atlasPixels.getWidth() * atlasPixels.getHeight();
            unsigned char minChannel = 255;
            unsigned char maxChannel = 0;
            for (int i = 0; i < totalPixels; ++i) {
                const int idx = i * stride;
                unsigned char alpha = raw[idx + 3];
                raw[idx] = alpha;
                raw[idx + 1] = alpha;
                raw[idx + 2] = alpha;
                minChannel = std::min(minChannel, alpha);
                maxChannel = std::max(maxChannel, alpha);
            }
            ofLogNotice("AsciiSupersampleAtlas")
                << "Normalized prebaked atlas RGB from alpha for" << atlasImagePath
                << " (min=" << static_cast<int>(minChannel)
                << ", max=" << static_cast<int>(maxChannel) << ")";
            auto sampleTexel = [&](int x, int y) {
                x = ofClamp(x, 0, atlasPixels.getWidth() - 1);
                y = ofClamp(y, 0, atlasPixels.getHeight() - 1);
                int idx = (y * atlasPixels.getWidth() + x) * stride;
                return static_cast<int>(raw[idx]);
            };
            ofLogNotice("AsciiSupersampleAtlas")
                << "Sample texels r(0,0)=" << sampleTexel(0, 0)
                << ", r(10,10)=" << sampleTexel(10, 10)
                << ", r(50,20)=" << sampleTexel(50, 20);
        }
        int glyphAIndex = std::max(0, std::min(97 - kAsciiGlyphFirst, kAsciiGlyphCount - 1));
        const glm::vec4& glyphRect = atlas.rects[glyphAIndex];
        float centerU = (glyphRect.x + glyphRect.z) * 0.5f;
        float centerV = (glyphRect.y + glyphRect.w) * 0.5f;
        int centerX = static_cast<int>(centerU * atlasPixels.getWidth());
        int centerY = static_cast<int>(centerV * atlasPixels.getHeight());
        centerX = ofClamp(centerX, 0, atlasPixels.getWidth() - 1);
        centerY = ofClamp(centerY, 0, atlasPixels.getHeight() - 1);
        int glyphIdx = (centerY * atlasPixels.getWidth() + centerX) * atlasPixels.getNumChannels();
        unsigned char glyphSample = atlasPixels.getData()[glyphIdx];
        ofLogNotice("AsciiSupersampleAtlas")
            << "Glyph 'a' sample texel value =" << static_cast<int>(glyphSample)
            << " at pixel (" << centerX << "," << centerY << ")";
        const bool wasArbTex = ofGetUsingArbTex();
        if (wasArbTex) {
            ofDisableArbTex();
        }
        atlas.texture.allocate(atlasPixels.getWidth(), atlasPixels.getHeight(), GL_RGBA);
        atlas.texture.loadData(atlasPixels);
        atlas.texture.setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
        atlas.texture.setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        if (wasArbTex) {
            ofEnableArbTex();
        }

        if (lutDoc.contains("font") && lutDoc["font"].is_object()) {
            const auto& fontNode = lutDoc["font"];
            if (fontNode.contains("label")) {
                atlas.label = fontNode["label"].get<std::string>();
            } else if (fontNode.contains("id")) {
                atlas.label = fontNode["id"].get<std::string>();
            }
        }
        ofLogNotice("AsciiSupersampleAtlas")
            << "Prebaked descriptors for" << atlas.label
            << " range [" << descriptorMin << ", " << descriptorMax << "]";
        atlas.ready = true;
        return true;
    }

    bool loadPrebakedAtlases(std::vector<std::shared_ptr<AsciiSupersampleAtlas::FontAtlas>>& dest) {
        ofJson manifest;
        if (!loadJsonRelativeToData(kAsciiSupersampleManifestPath, manifest)) {
            return false;
        }
        if (!manifest.contains("fonts") || !manifest["fonts"].is_array()) {
            ofLogWarning("AsciiSupersampleAtlas")
                << "Supersample manifest missing fonts array.";
            return false;
        }
        int loaded = 0;
        for (const auto& entry : manifest["fonts"]) {
            if (!entry.is_object()) {
                continue;
            }
            const std::string lutPath = entry.value("lut", "");
            const std::string atlasPath = entry.value("atlas", "");
            if (lutPath.empty() || atlasPath.empty()) {
                continue;
            }
            ofJson lutDoc;
            if (!loadJsonRelativeToData(lutPath, lutDoc)) {
                ofLogWarning("AsciiSupersampleAtlas")
                    << "Failed to load prebaked LUT" << lutPath;
                continue;
            }
            auto atlas = std::make_shared<AsciiSupersampleAtlas::FontAtlas>();
            if (!populateAtlasFromPrebakedLut(lutDoc, atlasPath, *atlas)) {
                continue;
            }
            const std::string label = entry.value("label", "");
            if (!label.empty()) {
                atlas->label = label;
            }
            dest.push_back(atlas);
            ++loaded;
            ofLogNotice("AsciiSupersampleAtlas")
                << "Loaded prebaked atlas for font" << atlas->label;
        }
        if (loaded > 0) {
            ofLogNotice("AsciiSupersampleAtlas")
                << "Loaded" << loaded << "prebaked supersample atlas(es).";
            return true;
        }
        return false;
    }

bool renderFontAtlas(const std::string& fontPath,
                         AsciiSupersampleAtlas::FontAtlas& atlas,
                         ofPixels& atlasPixels,
                         std::string& errorMessage) {
        ofTrueTypeFont font;
        ofTrueTypeFontSettings settings(fontPath, 48);
        settings.antialiased = true;
        settings.dpi = 96;
        if (!font.load(settings)) {
            errorMessage = "Unable to load font " + fontPath;
            return false;
        }

        const int cellWidth = 48;
        const int cellHeight = 64;
        const int columns = 16;
        const int rows = static_cast<int>(std::ceil(static_cast<float>(kAsciiGlyphCount) / columns));

        atlasPixels.allocate(columns * cellWidth, rows * cellHeight, OF_PIXELS_RGBA);
        atlasPixels.set(0);

        ofFbo::Settings fboSettings;
        fboSettings.width = cellWidth;
        fboSettings.height = cellHeight;
        fboSettings.internalformat = GL_RGBA;
        fboSettings.useDepth = false;
        fboSettings.useStencil = false;

        ofFbo glyphFbo;
        glyphFbo.allocate(fboSettings);

        for (int glyphIndex = 0; glyphIndex < kAsciiGlyphCount; ++glyphIndex) {
            char c = static_cast<char>(kAsciiGlyphFirst + glyphIndex);
            std::string glyphString(1, c);

            glyphFbo.begin();
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            ofSetColor(255);
            ofRectangle bbox = font.getStringBoundingBox(glyphString, 0.0f, 0.0f);
            if (bbox.width <= 0.0f) {
                bbox.width = cellWidth * 0.3f;
            }
            float drawX = (cellWidth - bbox.width) * 0.5f - bbox.x;
            float drawY = (cellHeight - bbox.height) * 0.5f - bbox.y;
            font.drawString(glyphString, drawX, drawY);
            ofPopStyle();
            glyphFbo.end();

            ofPixels glyphPixels;
            glyphFbo.readToPixels(glyphPixels);

            std::array<float, 6> samples{};
            int sampleIdx = 0;
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 2; ++col) {
                    int x0 = static_cast<int>(col * (cellWidth / 2.0f));
                    int y0 = static_cast<int>(row * (cellHeight / 3.0f));
                    int x1 = static_cast<int>((col + 1) * (cellWidth / 2.0f));
                    int y1 = static_cast<int>((row + 1) * (cellHeight / 3.0f));
                    x1 = std::min(x1, cellWidth);
                    y1 = std::min(y1, cellHeight);
                    double sum = 0.0;
                    int count = 0;
                    for (int y = y0; y < y1; ++y) {
                        for (int x = x0; x < x1; ++x) {
                            int index = (y * cellWidth + x) * 4;
                            unsigned char coverage = glyphPixels[index];
                            sum += coverage / 255.0;
                            ++count;
                        }
                    }
                    samples[sampleIdx++] = count > 0 ? static_cast<float>(sum / count) : 0.0f;
                }
            }
        float mean = 0.0f;
        for (int i = 0; i < 6; ++i) {
            atlas.descriptors[glyphIndex * 6 + i] = samples[i];
            mean += samples[i];
        }
        mean /= 6.0f;
        atlas.descriptorMeans[glyphIndex] = mean;

            int atlasCol = glyphIndex % columns;
            int atlasRow = glyphIndex / columns;
            float u0 = static_cast<float>(atlasCol * cellWidth) / atlasPixels.getWidth();
            float v0 = static_cast<float>(atlasRow * cellHeight) / atlasPixels.getHeight();
            float u1 = static_cast<float>((atlasCol + 1) * cellWidth) / atlasPixels.getWidth();
            float v1 = static_cast<float>((atlasRow + 1) * cellHeight) / atlasPixels.getHeight();
            atlas.rects[glyphIndex] = glm::vec4(u0, v0, u1, v1);

            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    int srcIndex = (y * cellWidth + x) * 4;
                    int dstX = atlasCol * cellWidth + x;
                    int dstY = atlasRow * cellHeight + y;
                    int dstIndex = (dstY * atlasPixels.getWidth() + dstX) * 4;
                    unsigned char coverage = glyphPixels[srcIndex + 0];
                    atlasPixels[dstIndex + 0] = coverage;
                    atlasPixels[dstIndex + 1] = coverage;
                    atlasPixels[dstIndex + 2] = coverage;
                    atlasPixels[dstIndex + 3] = coverage;
                }
            }
        }

        const bool wasArbTex = ofGetUsingArbTex();
        if (wasArbTex) {
            ofDisableArbTex();
        }
        atlas.texture.allocate(atlasPixels.getWidth(), atlasPixels.getHeight(), GL_RGBA);
        atlas.texture.loadData(atlasPixels);
        atlas.texture.setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
        atlas.texture.setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        if (wasArbTex) {
            ofEnableArbTex();
        }
        atlas.label = ofFilePath::getFileName(fontPath);
        atlas.ready = true;
        return true;
    }
}

bool AsciiSupersampleAtlas::build() {
    fonts_.clear();
    ready = false;

    if (loadPrebakedAtlases(fonts_) && !fonts_.empty()) {
        ready = true;
    } else {
        const std::string fontsDirPath = ofToDataPath("fonts", true);
        ofDirectory fontsDir(fontsDirPath);
        fontsDir.allowExt("ttf");
        fontsDir.allowExt("otf");
        fontsDir.listDir();

        if (fontsDir.size() == 0) {
            ofLogWarning("AsciiSupersampleAtlas") << "No fonts found in" << fontsDirPath << "for supersampling.";
        }

        for (const auto& file : fontsDir.getFiles()) {
            AsciiSupersampleAtlas::FontAtlas atlas;
            ofPixels atlasPixels;
            std::string errorMessage;
            if (renderFontAtlas(file.getAbsolutePath(), atlas, atlasPixels, errorMessage)) {
                fonts_.push_back(std::make_shared<FontAtlas>(atlas));
                ofLogNotice("AsciiSupersampleAtlas") << "Prepared supersample atlas for font" << file.getFileName();
                ready = true;
            } else {
                ofLogWarning("AsciiSupersampleAtlas") << "Skipping font" << file.getFileName() << "-" << errorMessage;
            }
        }
    }

    auto lowercaseSet = buildGlyphIndexList(" abcdefghijklmnopqrstuvwxyz");
    if (lowercaseSet.empty()) {
        lowercaseSet = buildFullGlyphIndexList();
    }
    for (auto& set : glyphSets_) {
        set = lowercaseSet;
    }
    for (size_t i = 0; i < glyphSets_.size(); ++i) {
        logGlyphSetSummary(static_cast<int>(i), glyphSets_[i]);
    }

    if (!ready) {
        ofLogWarning("AsciiSupersampleAtlas") << "Failed to build supersample atlas (no usable fonts).";
    } else {
        ofLogNotice("AsciiSupersampleAtlas") << "Prepared supersample atlases for" << fontCount() << "font(s).";
        for (size_t i = 0; i < fonts_.size(); ++i) {
            const auto& atlas = fonts_[i];
            if (!atlas) continue;
            const auto& texture = atlas->texture;
            const auto& texData = texture.getTextureData();
            const TextureStats stats = computeTextureStats(texture);
            ofLogNotice("AsciiSupersampleAtlas")
                << "atlas[" << i << "] label=\"" << atlas->label << "\""
                << " texId=" << texData.textureID
                << " target=0x" << std::hex << texData.textureTarget
                << " format=0x" << texData.glInternalFormat << std::dec
                << " size=" << texture.getWidth() << "x" << texture.getHeight()
                << " coverageMin=" << (stats.readSucceeded ? stats.minValue : -1)
                << " coverageMax=" << (stats.readSucceeded ? stats.maxValue : -1)
                << " readback=" << (stats.readSucceeded ? "ok" : "failed");
        }
    }
    return ready;
}


int AsciiSupersampleAtlas::fontCount() const {
    return static_cast<int>(fonts_.size());
}

int AsciiSupersampleAtlas::maxFontIndex() const {
    return fontCount() > 0 ? fontCount() - 1 : 0;
}

const AsciiSupersampleAtlas::FontAtlas* AsciiSupersampleAtlas::font(int index) const {
    if (fonts_.empty()) {
        return nullptr;
    }
    int clamped = std::max(0, std::min(index, static_cast<int>(fonts_.size()) - 1));
    return fonts_[clamped].get();
}

const std::vector<int>& AsciiSupersampleAtlas::glyphSet(int index) const {
    static const std::vector<int> fallback = buildFullGlyphIndexList();
    if (!ready) {
        return fallback;
    }
    int clamped = ofClamp(index, 0, static_cast<int>(glyphSets_.size()) - 1);
    const auto& set = glyphSets_[clamped];
    if (set.empty()) {
        return fallback;
    }
    return set;
}

    const char* kPassThroughVert = R"(#version 150
in vec4 position;
in vec2 texcoord;
uniform mat4 modelViewProjectionMatrix;
out vec2 vTexCoord;
void main() {
    vTexCoord = texcoord;
    gl_Position = modelViewProjectionMatrix * position;
}
)";

    const char* kDitherFrag = R"(#version 150
uniform sampler2D tex0;
uniform vec2 resolution;
uniform float cellSize;
uniform int bayerMode; // 0=2x2, 1=4x4, 2=8x8
in vec2 vTexCoord;
out vec4 fragColor;

const float bayer2[4] = float[4](
    0.0, 2.0,
    3.0, 1.0
);

const float bayer4[16] = float[16](
    0.0, 8.0, 2.0, 10.0,
    12.0, 4.0, 14.0, 6.0,
    3.0, 11.0, 1.0, 9.0,
    15.0, 7.0, 13.0, 5.0
);

const float bayer8[64] = float[64](
    0.0, 2.0, 32.0, 34.0, 8.0, 10.0, 40.0, 42.0,
    3.0, 1.0, 35.0, 33.0, 11.0, 9.0, 43.0, 41.0,
    48.0, 50.0, 16.0, 18.0, 56.0, 58.0, 24.0, 26.0,
    51.0, 49.0, 19.0, 17.0, 59.0, 57.0, 27.0, 25.0,
    12.0, 14.0, 44.0, 46.0, 4.0, 6.0, 36.0, 38.0,
    15.0, 13.0, 47.0, 45.0, 7.0, 5.0, 39.0, 37.0,
    60.0, 62.0, 28.0, 30.0, 52.0, 54.0, 20.0, 22.0,
    63.0, 61.0, 31.0, 29.0, 55.0, 53.0, 23.0, 21.0
);

float bayerThreshold(vec2 pixel) {
    if (bayerMode == 0) {
        int x = int(mod(floor(pixel.x), 2.0));
        int y = int(mod(floor(pixel.y), 2.0));
        int index = y * 2 + x;
        return (bayer2[index] + 0.5) / 4.0;
    } else if (bayerMode == 1) {
        int x = int(mod(floor(pixel.x), 4.0));
        int y = int(mod(floor(pixel.y), 4.0));
        int index = y * 4 + x;
        return (bayer4[index] + 0.5) / 16.0;
    } else {
        int x = int(mod(floor(pixel.x), 8.0));
        int y = int(mod(floor(pixel.y), 8.0));
        int index = y * 8 + x;
        return (bayer8[index] + 0.5) / 64.0;
    }
}

void main() {
    float stepSize = max(cellSize, 1.0);
    vec2 pixel = vTexCoord * resolution;
    vec2 block = floor(pixel / stepSize);
    vec2 sampleCoord = (block + vec2(0.5)) * stepSize / resolution;
    vec3 color = texture(tex0, sampleCoord).rgb;
    vec3 levels = vec3(16.0);
    float t = bayerThreshold(pixel);
    vec3 outColor = floor(color * levels + t) / levels;
    fragColor = vec4(outColor, 1.0);
}
 )";

    const char* kAsciiFrag = R"(#version 150
uniform sampler2D tex0;
uniform vec2 resolution;
uniform float blockSize;
uniform int colorMode;
uniform vec3 greenTint;
uniform float gamma;
uniform float jitterAmount;
uniform float time;
uniform int glyphTable[112];
uniform float aspectMode;
uniform float glyphPadding;
in vec2 vTexCoord;
out vec4 fragColor;

int glyphRowBits(int glyphIndex, int row) {
    glyphIndex = clamp(glyphIndex, 0, 15);
    row = clamp(row, 0, 6);
    int base = glyphIndex * 7 + row;
    return glyphTable[base];
}

float glyphSample(int glyphIndex, vec2 cell) {
    ivec2 grid = ivec2(floor(cell * vec2(5.0, 7.0)));
    grid.x = clamp(grid.x, 0, 4);
    grid.y = clamp(grid.y, 0, 6);
    int mask = 1 << (4 - grid.x);
    int rowBits = glyphRowBits(glyphIndex, grid.y);
    return (rowBits & mask) != 0 ? 1.0 : 0.0;
}

void main() {
    float bSize = max(3.0, blockSize);
    float aspect = clamp(aspectMode, 0.0, 1.0);
    vec2 cellSize = vec2(bSize, bSize * mix(7.0 / 5.0, 1.0, aspect));
    vec2 pixel = vTexCoord * resolution;
    vec2 block = floor(pixel / cellSize);
    vec2 blockCenter = (block + vec2(0.5)) * cellSize;
    vec2 sampleCoord = blockCenter / resolution;
    float jitterScale = jitterAmount * blockSize * 0.35;
    float hashBase = dot(block, vec2(12.9898, 78.233)) + time * 0.25;
    float randA = fract(sin(hashBase) * 43758.5453);
    float randB = fract(sin(hashBase + 3.14159) * 24682.3154);
    vec2 jitterOffset = jitterScale * vec2(randA - 0.5, randB - 0.5);
    vec3 baseColor = texture(tex0, (blockCenter + jitterOffset) / resolution).rgb;
    float lum = clamp(dot(baseColor, vec3(0.299, 0.587, 0.114)), 0.0, 1.0);
    float gammaSafe = max(gamma, 0.001);
    lum = pow(lum, gammaSafe);

    int glyphIndex = int(clamp(floor(lum * 15.0 + 0.5), 0.0, 15.0));
    vec2 cell = fract(pixel / cellSize);
    float pad = clamp(glyphPadding, 0.0, 0.45);
    vec2 innerCell = (cell - vec2(pad)) / max(1.0 - 2.0 * pad, 0.01);
    innerCell = clamp(innerCell, 0.0, 0.999);
    float mask = step(pad, cell.x) * step(cell.x, 1.0 - pad) * step(pad, cell.y) * step(cell.y, 1.0 - pad);
    float coverage = mask * glyphSample(glyphIndex, innerCell);

    vec3 displayColor;
    if (colorMode == 1) {
        displayColor = vec3(lum);
    } else if (colorMode == 2) {
        displayColor = greenTint * lum;
    } else {
        displayColor = baseColor;
    }

    vec3 outColor = displayColor * coverage;
    fragColor = vec4(outColor, 1.0);
}
)";

    const char* kAsciiSupersampleFrag = R"(#version 150
uniform sampler2D tex0;
uniform sampler2D fontAtlas;
uniform vec2 resolution;
uniform float blockSize;
uniform int colorMode;
uniform vec3 greenTint;
uniform float gamma;
uniform float jitterAmount;
uniform float time;
uniform int glyphCount;
uniform vec4 glyphRects[95];
uniform float glyphDescriptors[95 * 6];
uniform float glyphDescriptorMeans[95];
uniform int glyphIndexLUT[95];
uniform int debugMode;
in vec2 vTexCoord;
out vec4 fragColor;

const vec3 LUMA = vec3(0.299, 0.587, 0.114);

float sampleLum(vec2 coord) {
    vec3 c = texture(tex0, coord).rgb;
    float lum = clamp(dot(c, LUMA), 0.0, 1.0);
    return pow(lum, gamma);
}

void computeSamples(vec2 blockOrigin, vec2 cellSize, vec2 jitter, out float values[6]) {
    int idx = 0;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 2; ++col) {
            vec2 offset = vec2((float(col) + 0.5) / 2.0, (float(row) + 0.5) / 3.0);
            vec2 samplePos = blockOrigin + offset * cellSize + jitter;
            samplePos /= resolution;
            float lum = sampleLum(samplePos);
            values[idx++] = lum;
        }
    }
}

int chooseGlyph(float samples[6]) {
    float sampleMean = 0.0;
    for (int i = 0; i < 6; ++i) {
        sampleMean += samples[i];
    }
    sampleMean /= 6.0;

    float bestScore = 1e9;
    int bestIndex = 0;
    for (int i = 0; i < glyphCount; ++i) {
        int glyph = clamp(glyphIndexLUT[i], 0, 94);
        float targetMean = glyphDescriptorMeans[glyph];
        float score = 0.0;
        for (int d = 0; d < 6; ++d) {
            float target = glyphDescriptors[glyph * 6 + d];
            float diff = (samples[d] - sampleMean) - (target - targetMean);
            score += diff * diff;
        }
        if (score < bestScore) {
            bestScore = score;
            bestIndex = glyph;
        }
    }
    return bestIndex;
}

void main() {
    float bSize = max(3.0, blockSize);
    const float atlasAspect = 64.0 / 48.0;
    vec2 cellSize = vec2(bSize, bSize * atlasAspect);
    vec2 pixel = vTexCoord * resolution;
    vec2 block = floor(pixel / cellSize);
    vec2 blockCenter = (block + vec2(0.5)) * cellSize;
    float jitterScale = jitterAmount * blockSize * 0.35;
    float hashBase = dot(block, vec2(12.9898, 78.233)) + time * 0.25;
    float randA = fract(sin(hashBase) * 43758.5453);
    float randB = fract(sin(hashBase + 3.14159) * 24682.3154);
    vec2 jitterOffset = vec2(randA - 0.5, randB - 0.5) * jitterScale;

    float samples[6];
    computeSamples(block * cellSize, cellSize, jitterOffset, samples);
    int glyphIndex = chooseGlyph(samples);

    vec2 cell = fract(pixel / cellSize);
    float pad = 0.1;
    float padMask = step(pad, cell.x) * step(cell.x, 1.0 - pad) * step(pad, cell.y) * step(cell.y, 1.0 - pad);
    vec2 uvLocal = (cell - vec2(pad)) / max(1.0 - 2.0 * pad, 0.01);
    uvLocal = clamp(uvLocal, 0.0, 0.999);
    vec4 rect = glyphRects[glyphIndex];
    vec2 uv = vec2(mix(rect.x, rect.z, uvLocal.x), mix(rect.y, rect.w, uvLocal.y));
    vec4 glyphSample = texture(fontAtlas, uv);
    float glyphAlpha = glyphSample.a;
    float coverage = glyphAlpha * padMask;

    if (debugMode == 1) {
        fragColor = vec4(vec3(glyphAlpha), 1.0);
        return;
    } else if (debugMode == 2) {
        float norm = glyphCount > 1 ? float(glyphIndex) / float(glyphCount - 1) : 0.0;
        fragColor = vec4(vec3(norm), 1.0);
        return;
    } else if (debugMode == 3) {
        float atlasSample = texture(fontAtlas, uv).r;
        fragColor = vec4(vec3(atlasSample), 1.0);
        return;
    } else if (debugMode == 4) {
        vec4 rectDbg = glyphRects[glyphIndex];
        vec2 uvDbg = vec2(mix(rectDbg.x, rectDbg.z, uvLocal.x),
                          mix(rectDbg.y, rectDbg.w, uvLocal.y));
        fragColor = vec4(uvDbg.x, uvDbg.y, padMask, 1.0);
        return;
    } else if (debugMode == 5) {
        fragColor = vec4(vec3(coverage), 1.0);
        return;
    }

    vec2 blockSampleCoord = (blockCenter + jitterOffset) / resolution;
    vec3 blockColor = texture(tex0, blockSampleCoord).rgb;
    float blockLum = clamp(dot(blockColor, LUMA), 0.0, 1.0);
    float gammaSafe = max(gamma, 0.001);
    blockLum = pow(blockLum, gammaSafe);

    vec3 glyphColor;
    if (colorMode == 1) {
        glyphColor = vec3(blockLum);
    } else if (colorMode == 2) {
        glyphColor = greenTint * blockLum;
    } else {
        glyphColor = blockColor;
    }

    vec3 outColor = glyphColor * coverage;
    fragColor = vec4(outColor, 1.0);
}
)";

    const char* kCrtFrag = R"(#version 150
uniform sampler2D tex0;
uniform vec2 resolution;
uniform float scanlineIntensity;
uniform float vignetteIntensity;
uniform float bleed;
uniform float softness;
uniform float glow;
uniform float perChannelOffset;
uniform float scanlineJitter;
uniform float subpixelDensity;
uniform float subpixelAspect;
uniform float subpixelPadding;
uniform float rgbMisalignment;
uniform float syncTear;
uniform float trackingWobble;
uniform float lumaNoise;
uniform float headSwitchFlicker;
uniform float time;
in vec2 vTexCoord;
out vec4 fragColor;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 23.45);
    return fract(p.x * p.y);
}

float noise1(float x) {
    return fract(sin(x * 123.45 + 45.67) * 43758.5453);
}

float smoothNoise(float x) {
    float i = floor(x);
    float f = fract(x);
    float a = noise1(i);
    float b = noise1(i + 1.0);
    float fade = f * f * (3.0 - 2.0 * f);
    return mix(a, b, fade);
}

float vignetteMask(vec2 uv) {
    vec2 centered = uv * 2.0 - 1.0;
    float dist = dot(centered, centered);
    return 1.0 - clamp(dist, 0.0, 1.0);
}

vec2 applyTracking(vec2 uv) {
    if (trackingWobble <= 0.001) return uv;
    float line = floor(uv.y * resolution.y);
    float slow = smoothNoise(line * 0.08 + time * 0.2);
    float fast = smoothNoise(line * 0.5 + time * 1.3);
    float wobble = (slow * 0.7 + fast * 0.3) - 0.5;
    uv.y += wobble * trackingWobble * 0.03;
    return clamp(uv, 0.0, 1.0);
}

vec2 applySyncTear(vec2 uv) {
    if (syncTear <= 0.001) return uv;
    float basePos = smoothNoise(time * 0.12) * 0.8 + 0.1;
    float flicker = smoothNoise(time * 1.7) * 0.3;
    float tearLine = fract(basePos + flicker);
    float width = mix(0.02, 0.08, smoothNoise(time * 0.9));
    float dist = abs(uv.y - tearLine);
    float mask = smoothstep(width, 0.0, dist);
    float amplitude = syncTear * (0.5 + 0.5 * smoothNoise(time * 1.3 + uv.y * 5.0));
    uv.x += mask * amplitude * 0.12;
    return clamp(uv, 0.0, 1.0);
}

vec2 applyScanlineJitter(vec2 uv) {
    if (scanlineJitter <= 0.001) return uv;
    float line = floor(uv.y * resolution.y);
    float slow = smoothNoise(line * 0.12 + time * 0.4);
    float fast = smoothNoise(line * 0.5 + time * 2.8);
    float jitter = (slow * 0.6 + fast * 0.4) - 0.5;
    uv.x += jitter * scanlineJitter * 0.03;
    return clamp(uv, 0.0, 1.0);
}

vec3 softenSample(vec2 uv) {
    vec2 texel = 1.0 / resolution;
    vec2 clamped = clamp(uv, 0.0, 1.0);
    vec3 center = texture(tex0, clamped).rgb;
    if (softness <= 0.001) {
        return center;
    }
    float amt = clamp(softness, 0.0, 1.0);
    vec3 blur = center * 0.4;
    blur += texture(tex0, clamp(clamped + texel * vec2(1.5, 0.0), 0.0, 1.0)).rgb * 0.15;
    blur += texture(tex0, clamp(clamped - texel * vec2(1.5, 0.0), 0.0, 1.0)).rgb * 0.15;
    blur += texture(tex0, clamp(clamped + texel * vec2(0.0, 1.5), 0.0, 1.0)).rgb * 0.15;
    blur += texture(tex0, clamp(clamped - texel * vec2(0.0, 1.5), 0.0, 1.0)).rgb * 0.15;
    blur += texture(tex0, clamp(clamped + texel * vec2(1.0, 1.0), 0.0, 1.0)).rgb * 0.05;
    blur += texture(tex0, clamp(clamped - texel * vec2(1.0, 1.0), 0.0, 1.0)).rgb * 0.05;
    return mix(center, blur, amt);
}

vec3 glowSample(vec2 uv) {
    vec2 texel = 1.0 / resolution;
    vec2 clamped = clamp(uv, 0.0, 1.0);
    vec3 sum = texture(tex0, clamped).rgb * 0.2;
    sum += texture(tex0, clamp(clamped + texel * vec2(2.0, 0.0), 0.0, 1.0)).rgb * 0.15;
    sum += texture(tex0, clamp(clamped - texel * vec2(2.0, 0.0), 0.0, 1.0)).rgb * 0.15;
    sum += texture(tex0, clamp(clamped + texel * vec2(0.0, 2.0), 0.0, 1.0)).rgb * 0.15;
    sum += texture(tex0, clamp(clamped - texel * vec2(0.0, 2.0), 0.0, 1.0)).rgb * 0.15;
    sum += texture(tex0, clamp(clamped + texel * vec2(1.5, 1.5), 0.0, 1.0)).rgb * 0.1;
    sum += texture(tex0, clamp(clamped - texel * vec2(1.5, 1.5), 0.0, 1.0)).rgb * 0.1;
    return sum;
}

vec3 chromaBleed(vec2 uv, vec2 res, float amount) {
    float bleedAmt = clamp(amount, 0.0, 0.4);
    vec2 center = vec2(0.5);
    vec2 offset = (uv - center) * bleedAmt;
    float r = texture(tex0, clamp(uv + vec2(offset.x, 0.0), 0.0, 1.0)).r;
    float g = texture(tex0, uv).g;
    float b = texture(tex0, clamp(uv - vec2(offset.x, 0.0), 0.0, 1.0)).b;
    return vec3(r, g, b);
}

vec3 sampleChannels(vec2 uv) {
    vec2 texel = 1.0 / resolution;
    float offsetBase = perChannelOffset * 6.0;
    float misalign = rgbMisalignment * 6.0;
    vec3 rc = softenSample(uv + vec2((offsetBase + misalign) * texel.x, 0.0));
    vec3 gc = softenSample(uv - vec2(offsetBase * 0.5 * texel.x, 0.0));
    vec3 bc = softenSample(uv - vec2((offsetBase + misalign) * texel.x, 0.0));
    return vec3(rc.r, gc.g, bc.b);
}

vec3 applyScanlines(vec3 color, float yPixel) {
    float scan = 0.5 + 0.5 * sin(yPixel * 3.14159);
    float factor = mix(1.0, scan, clamp(scanlineIntensity, 0.0, 1.0));
    return color * factor;
}

vec3 applySubpixels(vec3 color, vec2 pixel, vec2 spacingPx, float aspect) {
    float spacingX = spacingPx.x;
    float spacingY = spacingPx.y;
    vec2 grid = vec2(pixel.x / spacingX, pixel.y / spacingY);
    vec2 local = fract(grid);
    float colPhase = mod(floor(grid.x), 3.0);
    float rowPhase = mod(floor(grid.y), 2.0);
    float adjustedPhase = colPhase;
    if (rowPhase >= 0.5) {
        adjustedPhase = mod(colPhase + 1.0, 3.0); // stagger rows
    }
    vec3 mask;
    if (adjustedPhase < 0.5) {
        mask = vec3(1.0, 0.1, 0.1);
    } else if (adjustedPhase < 1.5) {
        mask = vec3(0.1, 1.0, 0.1);
    } else {
        mask = vec3(0.1, 0.1, 1.0);
    }
    float rowMask = rowPhase < 0.5 ? 0.95 : 0.85;
    float pad = clamp(subpixelPadding, 0.0, 0.5);
    vec2 coord = abs(local - 0.5);
    coord.y *= aspect;
    float padX = pad;
    float padY = pad * aspect;
    vec2 halfSize = vec2(max(0.0, 0.5 - padX), max(0.0, 0.5 * aspect - padY));
    vec2 d = coord - halfSize;
    vec2 q = max(d, 0.0);
    float dist = length(q) + min(max(d.x, d.y), 0.0);
    float padMask = 1.0 - smoothstep(0.0, pad * 1.2 + 0.0001, dist);
    return color * mask * rowMask * padMask * 3.0;
}

vec3 applyHeadSwitch(vec3 color, vec2 uv) {
    if (headSwitchFlicker <= 0.001) return color;
    float band = smoothstep(0.85, 1.0, uv.y);
    float flicker = 0.6 + 0.4 * sin(time * 25.0);
    float dim = mix(1.0, flicker, headSwitchFlicker * band);
    return color * dim;
}

vec3 addLumaNoise(vec3 color, vec2 uv) {
    if (lumaNoise <= 0.001) return color;
    float grain = hash21(uv * resolution + vec2(time)) - 0.5;
    return clamp(color + grain * lumaNoise, 0.0, 1.0);
}

vec2 distortUv(vec2 uv) {
    uv = applyTracking(uv);
    uv = applySyncTear(uv);
    uv = applyScanlineJitter(uv);
    return clamp(uv, 0.0, 1.0);
}

void main() {
    float density = clamp(subpixelDensity, 0.0, 8.0);
    float normalized = clamp(density / 8.0, 0.0, 1.0);
    float spacingX = mix(5.0, 1.0, normalized);
    float aspect = max(subpixelAspect, 0.1);
    float spacingY = spacingX * aspect;
    vec2 spacingPx = vec2(spacingX, spacingY);

    vec2 maskPixel = vTexCoord * resolution;
    float colIndex = floor(maskPixel.x / spacingX);
    float columnOffset = (mod(colIndex, 2.0) >= 0.5) ? 0.5 : 0.0;
    vec2 adjustedPixel = vec2(maskPixel.x, maskPixel.y + columnOffset * spacingY);

    vec2 cellIndex = floor(adjustedPixel / spacingPx);
    vec2 cellCenterPx = (cellIndex + 0.5) * spacingPx;
    vec2 cellCenterUv = cellCenterPx / resolution;

    vec2 sampleUv = distortUv(cellCenterUv);

    vec3 channelColor = sampleChannels(sampleUv);
    vec3 bleedColor = chromaBleed(sampleUv, resolution, bleed);
    vec3 combined = mix(channelColor, bleedColor, clamp(bleed, 0.0, 1.0));
    combined = addLumaNoise(combined, sampleUv);
    vec3 glowColor = glowSample(sampleUv);
    combined = mix(combined, glowColor, clamp(glow, 0.0, 1.0));
    combined = applyHeadSwitch(combined, sampleUv);

    vec3 subpixel = applySubpixels(combined, adjustedPixel, spacingPx, aspect);
    vec3 scanColor = applyScanlines(subpixel, maskPixel.y);
    float vign = mix(1.0, vignetteMask(vTexCoord), clamp(vignetteIntensity, 0.0, 1.0));
    fragColor = vec4(scanColor * vign, 1.0);
}
)";

    const char* kMotionExtractFrag = R"(#version 150
uniform sampler2D tex0;
uniform sampler2D historyTex;
uniform vec2 resolution;
uniform float threshold;
uniform float boost;
uniform float mixAmount;
uniform float softness;
in vec2 vTexCoord;
out vec4 fragColor;

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void main() {
    vec3 current = texture(tex0, vTexCoord).rgb;
    vec3 previous = texture(historyTex, vTexCoord).rgb;
    vec3 diff = abs(current - previous);
    float energy = luminance(diff) * boost;
    float edgeWidth = max(softness, 0.001);
    float edge0 = clamp(threshold - edgeWidth * 0.5, 0.0, 1.0);
    float edge1 = clamp(threshold + edgeWidth * 0.5, 0.0, 1.0);
    if (edge1 <= edge0) {
        edge1 = edge0 + 0.0001;
    }
    float mask = smoothstep(edge0, edge1, energy);
    vec3 boosted = clamp(diff * boost, 0.0, 1.0);
    vec3 tint = vec3(0.0, 0.95, 0.8);
    vec3 highlight = mix(vec3(mask), boosted, 0.5) * tint;
    highlight = clamp(highlight, 0.0, 1.0);
    vec3 result = mix(current, highlight, mixAmount);
    fragColor = vec4(result, 1.0);
}
)";

    class DitherEffect : public PostEffectChain::Effect {
    public:
        explicit DitherEffect(float* cellSizeParam, float* modeParam)
            : cellSizeParam_(cellSizeParam), modeParam_(modeParam) {
            shader_.setupShaderFromSource(GL_VERTEX_SHADER, kPassThroughVert);
            shader_.setupShaderFromSource(GL_FRAGMENT_SHADER, kDitherFrag);
            shader_.bindDefaults();
            shader_.linkProgram();
        }

        void apply(const ofFbo& src, ofFbo& dst) override {
            dst.begin();
            bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
            bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
            if (scissorWasEnabled) {
                glDisable(GL_SCISSOR_TEST);
            }
            ofPushView();
            ofViewport(0, 0, dst.getWidth(), dst.getHeight());
            ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            if (depthWasEnabled) {
                ofDisableDepthTest();
            }
            ofSetColor(255);
            shader_.begin();
            shader_.setUniformTexture("tex0", src.getTexture(), 0);
            shader_.setUniform2f("resolution", src.getWidth(), src.getHeight());
            float cell = cellSizeParam_ ? std::max(1.0f, *cellSizeParam_) : 1.0f;
            int mode = modeParam_ ? static_cast<int>(std::round(ofClamp(*modeParam_, 0.0f, 2.0f))) : 1;
            shader_.setUniform1i("bayerMode", mode);
            shader_.setUniform1f("cellSize", cell);
            src.draw(0, 0, dst.getWidth(), dst.getHeight());
            shader_.end();
            ofPopStyle();
            ofPopView();
            if (depthWasEnabled) {
                ofEnableDepthTest();
            }
            if (scissorWasEnabled) {
                glEnable(GL_SCISSOR_TEST);
            }
            dst.end();
        }

    private:
        ofShader shader_;
        float* cellSizeParam_ = nullptr;
        float* modeParam_ = nullptr;
    };

    class AsciiEffect : public PostEffectChain::Effect {
    public:
        AsciiEffect(float* blockSizeParam,
                    float* colorModeParam,
                    float* characterSetParam,
                    float* aspectParam,
                    float* paddingParam,
                    float* gammaParam,
                    float* jitterParam,
                    glm::vec3* tintParam)
            : blockSizeParam_(blockSizeParam)
            , colorModeParam_(colorModeParam)
            , characterSetParam_(characterSetParam)
            , aspectParam_(aspectParam)
            , paddingParam_(paddingParam)
            , gammaParam_(gammaParam)
            , jitterParam_(jitterParam)
            , tintParam_(tintParam) {
            shader_.setupShaderFromSource(GL_VERTEX_SHADER, kPassThroughVert);
            shader_.setupShaderFromSource(GL_FRAGMENT_SHADER, kAsciiFrag);
            shader_.bindDefaults();
            shader_.linkProgram();
        }

        void apply(const ofFbo& src, ofFbo& dst) override {
            dst.begin();
            bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
            bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
            if (scissorWasEnabled) {
                glDisable(GL_SCISSOR_TEST);
            }
            ofPushView();
            ofViewport(0, 0, dst.getWidth(), dst.getHeight());
            ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            if (depthWasEnabled) {
                ofDisableDepthTest();
            }
            ofSetColor(255);
            shader_.begin();
            shader_.setUniformTexture("tex0", src.getTexture(), 0);
            shader_.setUniform2f("resolution", src.getWidth(), src.getHeight());
            float block = blockSizeParam_ ? std::max(3.0f, *blockSizeParam_) : 8.0f;
            shader_.setUniform1f("blockSize", block);
            float mode = colorModeParam_ ? ofClamp(*colorModeParam_, 0.0f, 2.0f) : 0.0f;
            shader_.setUniform1i("colorMode", static_cast<int>(std::round(mode)));
            int debugMode = characterSetParam_
                ? static_cast<int>(std::round(ofClamp(*characterSetParam_, 0.0f, 2.0f)))
                : 0;
            shader_.setUniform1i("debugMode", debugMode);
            int setIndex = 0;
            const int* table = glyphTableForSet(setIndex);
            shader_.setUniform1iv("glyphTable", table, kGlyphTableSize);
            glm::vec3 tint = tintParam_ ? *tintParam_ : glm::vec3(0.3f, 1.0f, 0.3f);
            shader_.setUniform3f("greenTint", tint.x, tint.y, tint.z);
            float gamma = gammaParam_ ? ofClamp(*gammaParam_, 0.2f, 3.0f) : 1.0f;
            shader_.setUniform1f("gamma", gamma);
            float jitter = jitterParam_ ? ofClamp(*jitterParam_, 0.0f, 1.0f) : 0.0f;
            shader_.setUniform1f("jitterAmount", jitter);
            shader_.setUniform1f("time", ofGetElapsedTimef());
            src.draw(0, 0, dst.getWidth(), dst.getHeight());
            shader_.end();
            ofPopStyle();
            ofPopView();
            if (depthWasEnabled) {
                ofEnableDepthTest();
            }
            if (scissorWasEnabled) {
                glEnable(GL_SCISSOR_TEST);
            }
            dst.end();
        }

    private:
        ofShader shader_;
        float* blockSizeParam_ = nullptr;
        float* colorModeParam_ = nullptr;
        float* characterSetParam_ = nullptr;
        float* aspectParam_ = nullptr;
        float* paddingParam_ = nullptr;
        float* gammaParam_ = nullptr;
        float* jitterParam_ = nullptr;
        glm::vec3* tintParam_ = nullptr;
    };

    class AsciiSupersampleEffect : public PostEffectChain::Effect {
    public:
        AsciiSupersampleEffect(float* blockSizeParam,
                               float* colorModeParam,
                               float* characterSetParam,
                               float* fontIndexParam,
                               float* gammaParam,
                               float* jitterParam,
                               float* debugParam,
                               glm::vec3* tintParam,
                               AsciiSupersampleAtlas* atlas)
            : blockSizeParam_(blockSizeParam)
            , colorModeParam_(colorModeParam)
            , characterSetParam_(characterSetParam)
            , fontIndexParam_(fontIndexParam)
            , gammaParam_(gammaParam)
            , jitterParam_(jitterParam)
            , debugParam_(debugParam)
            , tintParam_(tintParam)
            , atlas_(atlas) {
            shader_.setupShaderFromSource(GL_VERTEX_SHADER, kPassThroughVert);
            shader_.setupShaderFromSource(GL_FRAGMENT_SHADER, kAsciiSupersampleFrag);
            shader_.bindDefaults();
            shader_.linkProgram();
        }

        void apply(const ofFbo& src, ofFbo& dst) override {
            if (!atlas_ || !atlas_->ready) {
                dst.begin();
                ofClear(0, 0, 0, 0);
                src.draw(0, 0, dst.getWidth(), dst.getHeight());
                dst.end();
                return;
            }
            dst.begin();
            bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
            bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
            if (scissorWasEnabled) {
                glDisable(GL_SCISSOR_TEST);
            }
            ofPushView();
            ofViewport(0, 0, dst.getWidth(), dst.getHeight());
            ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            if (depthWasEnabled) {
                ofDisableDepthTest();
            }
            ofSetColor(255);
            shader_.begin();
            shader_.setUniformTexture("tex0", src.getTexture(), 0);
            shader_.setUniform2f("resolution", src.getWidth(), src.getHeight());
            float block = blockSizeParam_ ? std::max(3.0f, *blockSizeParam_) : 8.0f;
            shader_.setUniform1f("blockSize", block);
            float mode = colorModeParam_ ? ofClamp(*colorModeParam_, 0.0f, 2.0f) : 0.0f;
            shader_.setUniform1i("colorMode", static_cast<int>(std::round(mode)));
            int setIndex = 0;
            int fontIndex = 0;
            if (fontIndexParam_) {
                float maxFont = static_cast<float>(atlas_->maxFontIndex());
                fontIndex = static_cast<int>(std::round(ofClamp(*fontIndexParam_, 0.0f, maxFont)));
            }
            const auto* fontAtlas = atlas_->font(fontIndex);
            if (!fontAtlas || !fontAtlas->ready) {
                shader_.end();
                ofPopStyle();
                ofPopView();
                if (depthWasEnabled) {
                    ofEnableDepthTest();
                }
                if (scissorWasEnabled) {
                    glEnable(GL_SCISSOR_TEST);
                }
                dst.end();
                src.draw(0, 0, dst.getWidth(), dst.getHeight());
                return;
            }
            shader_.setUniform1f("aspectMode", 0.0f);
            shader_.setUniform1f("glyphPadding", 0.1f);
            glm::vec3 tint = tintParam_ ? *tintParam_ : glm::vec3(0.3f, 1.0f, 0.3f);
            shader_.setUniform3f("greenTint", tint.x, tint.y, tint.z);
            float gamma = gammaParam_ ? ofClamp(*gammaParam_, 0.2f, 3.0f) : 1.0f;
            shader_.setUniform1f("gamma", gamma);
            float jitter = jitterParam_ ? ofClamp(*jitterParam_, 0.0f, 1.0f) : 0.0f;
            shader_.setUniform1f("jitterAmount", jitter);
            shader_.setUniform1f("time", ofGetElapsedTimef());
            int debugMode = 0;
            if (debugParam_) {
                debugMode = static_cast<int>(std::round(ofClamp(*debugParam_, 0.0f, 5.0f)));
            }
            shader_.setUniform1i("debugMode", debugMode);
            const auto& glyphIndices = atlas_->glyphSet(setIndex);
            const auto& fallbackIndices = atlas_->glyphSet(2);
            const auto& activeIndices = glyphIndices.empty() ? fallbackIndices : glyphIndices;
            int glyphCount = static_cast<int>(std::min<size_t>(activeIndices.size(), kAsciiGlyphCount));
            if (glyphCount <= 0) {
                glyphCount = kAsciiGlyphCount;
            }
            if (!loggedState_) {
                loggedState_ = true;
                const auto& texData = fontAtlas->texture.getTextureData();
                std::ostringstream lutPreview;
                const int previewCount = std::min(glyphCount, 8);
                for (int i = 0; i < previewCount; ++i) {
                    if (i > 0) lutPreview << ",";
                    lutPreview << activeIndices[i];
                }
                ofLogNotice("AsciiSupersampleEffect")
                    << "bind fontIndex=" << fontIndex
                    << " texId=" << texData.textureID
                    << " target=0x" << std::hex << texData.textureTarget
                    << " format=0x" << texData.glInternalFormat << std::dec
                    << " glyphCount=" << glyphCount
                    << " LUTsize=" << activeIndices.size()
                    << " setIndex=" << setIndex
                    << " debugMode=" << debugMode
                    << " LUTpreview=[" << lutPreview.str() << "]";
            }
            shader_.setUniform1i("glyphCount", glyphCount);
            shader_.setUniform1iv("glyphIndexLUT", activeIndices.data(), glyphCount);
            shader_.setUniform4fv("glyphRects", reinterpret_cast<const float*>(fontAtlas->rects.data()), kAsciiGlyphCount);
            shader_.setUniform1fv("glyphDescriptors", fontAtlas->descriptors.data(), static_cast<int>(fontAtlas->descriptors.size()));
            shader_.setUniform1fv("glyphDescriptorMeans", fontAtlas->descriptorMeans.data(), kAsciiGlyphCount);
            shader_.setUniformTexture("fontAtlas", fontAtlas->texture, 1);
            src.draw(0, 0, dst.getWidth(), dst.getHeight());
            shader_.end();
            ofPopStyle();
            ofPopView();
            if (depthWasEnabled) {
                ofEnableDepthTest();
            }
            if (scissorWasEnabled) {
                glEnable(GL_SCISSOR_TEST);
            }
            dst.end();
        }

    private:
        ofShader shader_;
        float* blockSizeParam_ = nullptr;
        float* colorModeParam_ = nullptr;
        float* characterSetParam_ = nullptr;
        float* fontIndexParam_ = nullptr;
        float* gammaParam_ = nullptr;
        float* jitterParam_ = nullptr;
        float* debugParam_ = nullptr;
        glm::vec3* tintParam_ = nullptr;
        AsciiSupersampleAtlas* atlas_ = nullptr;
        bool loggedState_ = false;
    };

    class CrtEffect : public PostEffectChain::Effect {
    public:
        CrtEffect(float* scanlineParam,
                  float* vignetteParam,
                  float* bleedParam,
                  float* softnessParam,
                  float* glowParam,
                  float* perChannelOffsetParam,
                  float* scanlineJitterParam,
                  float* subpixelDensityParam,
                  float* subpixelAspectParam,
                  float* subpixelPaddingParam,
                  float* rgbMisalignmentParam,
                  float* syncTearParam,
                  float* trackingWobbleParam,
                  float* lumaNoiseParam,
                  float* headSwitchFlickerParam)
            : scanlineParam_(scanlineParam)
            , vignetteParam_(vignetteParam)
            , bleedParam_(bleedParam)
            , softnessParam_(softnessParam)
            , glowParam_(glowParam)
            , perChannelOffsetParam_(perChannelOffsetParam)
            , scanlineJitterParam_(scanlineJitterParam)
            , subpixelDensityParam_(subpixelDensityParam)
            , subpixelAspectParam_(subpixelAspectParam)
            , subpixelPaddingParam_(subpixelPaddingParam)
            , rgbMisalignmentParam_(rgbMisalignmentParam)
            , syncTearParam_(syncTearParam)
            , trackingWobbleParam_(trackingWobbleParam)
            , lumaNoiseParam_(lumaNoiseParam)
            , headSwitchFlickerParam_(headSwitchFlickerParam) {
            shader_.setupShaderFromSource(GL_VERTEX_SHADER, kPassThroughVert);
            shader_.setupShaderFromSource(GL_FRAGMENT_SHADER, kCrtFrag);
            shader_.bindDefaults();
            shader_.linkProgram();
        }

        void apply(const ofFbo& src, ofFbo& dst) override {
            dst.begin();
            bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
            bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
            if (scissorWasEnabled) {
                glDisable(GL_SCISSOR_TEST);
            }
            ofPushView();
            ofViewport(0, 0, dst.getWidth(), dst.getHeight());
            ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            if (depthWasEnabled) {
                ofDisableDepthTest();
            }
            ofSetColor(255);
            shader_.begin();
            shader_.setUniformTexture("tex0", src.getTexture(), 0);
            shader_.setUniform2f("resolution", src.getWidth(), src.getHeight());
            const float scan = scanlineParam_ ? ofClamp(*scanlineParam_, 0.0f, 1.0f) : 0.4f;
            const float vign = vignetteParam_ ? ofClamp(*vignetteParam_, 0.0f, 1.0f) : 0.25f;
            const float bleed = bleedParam_ ? ofClamp(*bleedParam_, 0.0f, 0.4f) : 0.15f;
            const float softness = softnessParam_ ? ofClamp(*softnessParam_, 0.0f, 1.0f) : 0.0f;
            const float glowAmt = glowParam_ ? ofClamp(*glowParam_, 0.0f, 1.0f) : 0.0f;
            const float perChannel = perChannelOffsetParam_ ? ofClamp(*perChannelOffsetParam_, 0.0f, 1.0f) : 0.0f;
            const float jitter = scanlineJitterParam_ ? ofClamp(*scanlineJitterParam_, 0.0f, 1.0f) : 0.0f;
            const float density = subpixelDensityParam_ ? ofClamp(*subpixelDensityParam_, 0.0f, 8.0f) : 0.0f;
            const float aspect = subpixelAspectParam_ ? ofClamp(*subpixelAspectParam_, 0.1f, 5.0f) : 2.0f;
            const float padding = subpixelPaddingParam_ ? ofClamp(*subpixelPaddingParam_, 0.0f, 0.5f) : 0.0f;
            const float misalignment = rgbMisalignmentParam_ ? ofClamp(*rgbMisalignmentParam_, 0.0f, 1.0f) : 0.0f;
            const float tear = syncTearParam_ ? ofClamp(*syncTearParam_, 0.0f, 1.0f) : 0.0f;
            const float wobble = trackingWobbleParam_ ? ofClamp(*trackingWobbleParam_, 0.0f, 1.0f) : 0.0f;
            const float noise = lumaNoiseParam_ ? ofClamp(*lumaNoiseParam_, 0.0f, 1.0f) : 0.0f;
            const float headSwitch = headSwitchFlickerParam_ ? ofClamp(*headSwitchFlickerParam_, 0.0f, 1.0f) : 0.0f;

            shader_.setUniform1f("scanlineIntensity", scan);
            shader_.setUniform1f("vignetteIntensity", vign);
            shader_.setUniform1f("bleed", bleed);
            shader_.setUniform1f("softness", softness);
            shader_.setUniform1f("glow", glowAmt);
            shader_.setUniform1f("perChannelOffset", perChannel);
            shader_.setUniform1f("scanlineJitter", jitter);
            shader_.setUniform1f("subpixelDensity", density);
            shader_.setUniform1f("subpixelAspect", aspect);
            shader_.setUniform1f("subpixelPadding", padding);
            shader_.setUniform1f("rgbMisalignment", misalignment);
            shader_.setUniform1f("syncTear", tear);
            shader_.setUniform1f("trackingWobble", wobble);
            shader_.setUniform1f("lumaNoise", noise);
            shader_.setUniform1f("headSwitchFlicker", headSwitch);
            shader_.setUniform1f("time", ofGetElapsedTimef());
            src.draw(0, 0, dst.getWidth(), dst.getHeight());
            shader_.end();
            ofPopStyle();
            ofPopView();
            if (depthWasEnabled) {
                ofEnableDepthTest();
            }
            if (scissorWasEnabled) {
                glEnable(GL_SCISSOR_TEST);
            }
            dst.end();
        }

    private:
        ofShader shader_;
        float* scanlineParam_ = nullptr;
        float* vignetteParam_ = nullptr;
        float* bleedParam_ = nullptr;
        float* softnessParam_ = nullptr;
        float* glowParam_ = nullptr;
        float* perChannelOffsetParam_ = nullptr;
        float* scanlineJitterParam_ = nullptr;
        float* subpixelDensityParam_ = nullptr;
        float* subpixelAspectParam_ = nullptr;
        float* subpixelPaddingParam_ = nullptr;
        float* rgbMisalignmentParam_ = nullptr;
        float* syncTearParam_ = nullptr;
        float* trackingWobbleParam_ = nullptr;
        float* lumaNoiseParam_ = nullptr;
        float* headSwitchFlickerParam_ = nullptr;
    };

    class MotionExtractEffect : public PostEffectChain::Effect {
    public:
        MotionExtractEffect(float* thresholdParam,
                            float* boostParam,
                            float* mixParam,
                            float* softnessParam)
            : thresholdParam_(thresholdParam)
            , boostParam_(boostParam)
            , mixParam_(mixParam)
            , softnessParam_(softnessParam) {
            shader_.setupShaderFromSource(GL_VERTEX_SHADER, kPassThroughVert);
            shader_.setupShaderFromSource(GL_FRAGMENT_SHADER, kMotionExtractFrag);
            shader_.bindDefaults();
            shader_.linkProgram();
        }

        void resize(int width, int height) override {
            ensureHistory(width, height);
        }

        void apply(const ofFbo& src, ofFbo& dst) override {
            ensureHistory(src.getWidth(), src.getHeight());
            if (!historyReady_) {
                blit(src, dst);
                copyToHistory(src);
                return;
            }

            dst.begin();
            bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
            bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
            if (scissorWasEnabled) {
                glDisable(GL_SCISSOR_TEST);
            }
            ofPushView();
            ofViewport(0, 0, dst.getWidth(), dst.getHeight());
            ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            if (depthWasEnabled) {
                ofDisableDepthTest();
            }
            ofSetColor(255);
            shader_.begin();
            shader_.setUniformTexture("tex0", src.getTexture(), 0);
            shader_.setUniformTexture("historyTex", historyFbo_.getTexture(), 1);
            shader_.setUniform2f("resolution", src.getWidth(), src.getHeight());
            float threshold = thresholdParam_ ? ofClamp(*thresholdParam_, 0.0f, 1.0f) : 0.15f;
            float boost = boostParam_ ? ofClamp(*boostParam_, 0.0f, 8.0f) : 2.5f;
            float mix = mixParam_ ? ofClamp(*mixParam_, 0.0f, 1.0f) : 0.85f;
            float softness = softnessParam_ ? ofClamp(*softnessParam_, 0.01f, 0.5f) : 0.2f;
            shader_.setUniform1f("threshold", threshold);
            shader_.setUniform1f("boost", boost);
            shader_.setUniform1f("mixAmount", mix);
            shader_.setUniform1f("softness", softness);
            src.draw(0, 0, dst.getWidth(), dst.getHeight());
            shader_.end();
            ofPopStyle();
            ofPopView();
            if (depthWasEnabled) {
                ofEnableDepthTest();
            }
            if (scissorWasEnabled) {
                glEnable(GL_SCISSOR_TEST);
            }
            dst.end();

            copyToHistory(src);
        }

    private:
        void ensureHistory(int width, int height) {
            int w = std::max(1, width);
            int h = std::max(1, height);
            if (historyFbo_.isAllocated() && historyFbo_.getWidth() == w && historyFbo_.getHeight() == h) {
                return;
            }
            ofFbo::Settings settings;
            settings.width = w;
            settings.height = h;
            settings.useDepth = false;
            settings.useStencil = false;
            settings.internalformat = GL_RGBA;
            settings.textureTarget = GL_TEXTURE_2D;
            settings.minFilter = GL_LINEAR;
            settings.maxFilter = GL_LINEAR;
            settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
            settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
            historyFbo_.allocate(settings);
            historyFbo_.begin();
            ofClear(0, 0, 0, 255);
            historyFbo_.end();
            historyReady_ = false;
        }

        void blit(const ofFbo& src, ofFbo& dst) {
            dst.begin();
            bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
            bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
            if (scissorWasEnabled) {
                glDisable(GL_SCISSOR_TEST);
            }
            ofPushView();
            ofViewport(0, 0, dst.getWidth(), dst.getHeight());
            ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            if (depthWasEnabled) {
                ofDisableDepthTest();
            }
            ofSetColor(255);
            src.draw(0, 0, dst.getWidth(), dst.getHeight());
            ofPopStyle();
            ofPopView();
            if (depthWasEnabled) {
                ofEnableDepthTest();
            }
            if (scissorWasEnabled) {
                glEnable(GL_SCISSOR_TEST);
            }
            dst.end();
        }

        void copyToHistory(const ofFbo& src) {
            blit(src, historyFbo_);
            historyReady_ = true;
        }

        ofShader shader_;
        ofFbo historyFbo_;
        bool historyReady_ = false;
        float* thresholdParam_ = nullptr;
        float* boostParam_ = nullptr;
        float* mixParam_ = nullptr;
        float* softnessParam_ = nullptr;
    };

    class MotionEffectWrapper : public PostEffectChain::Effect {
    public:
        explicit MotionEffectWrapper(MotionExtractProcessor* processor)
            : processor_(processor) {}

        void resize(int width, int height) override {
            if (processor_) {
                processor_->resize(width, height);
            }
        }

        void apply(const ofFbo& src, ofFbo& dst) override {
            if (processor_) {
                processor_->apply(src, dst);
                return;
            }
            dst.begin();
            bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
            bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
            if (scissorWasEnabled) {
                glDisable(GL_SCISSOR_TEST);
            }
            ofPushView();
            ofViewport(0, 0, dst.getWidth(), dst.getHeight());
            ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
            ofPushStyle();
            ofClear(0, 0, 0, 0);
            if (depthWasEnabled) {
                ofDisableDepthTest();
            }
            ofSetColor(255);
            src.draw(0, 0, dst.getWidth(), dst.getHeight());
            ofPopStyle();
            ofPopView();
            if (depthWasEnabled) {
                ofEnableDepthTest();
            }
            if (scissorWasEnabled) {
                glEnable(GL_SCISSOR_TEST);
            }
            dst.end();
        }

    private:
        MotionExtractProcessor* processor_ = nullptr;
    };
void PostEffectChain::setup(ParameterRegistry& registry) {
    ditherCellSize_ = std::max(1.0f, ditherCellSize_);
    asciiBlockSize_ = std::max(3.0f, asciiBlockSize_);
    asciiColorMode_ = ofClamp(asciiColorMode_, 0.0f, 2.0f);
    asciiCharacterSet_ = ofClamp(asciiCharacterSet_, 0.0f, 2.0f);
    asciiAspectMode_ = ofClamp(asciiAspectMode_, 0.0f, 1.0f);
    asciiPadding_ = ofClamp(asciiPadding_, 0.0f, 0.45f);
    asciiGamma_ = ofClamp(asciiGamma_, 0.2f, 3.0f);
    asciiJitter_ = ofClamp(asciiJitter_, 0.0f, 1.0f);
    asciiSupersampleBlockSize_ = std::max(3.0f, asciiSupersampleBlockSize_);
    asciiSupersampleColorMode_ = ofClamp(asciiSupersampleColorMode_, 0.0f, 2.0f);
    asciiSupersampleCharacterSet_ = ofClamp(asciiSupersampleCharacterSet_, 0.0f, 2.0f);
    asciiSupersampleGamma_ = ofClamp(asciiSupersampleGamma_, 0.2f, 3.0f);
    asciiSupersampleJitter_ = ofClamp(asciiSupersampleJitter_, 0.0f, 1.0f);
    asciiSupersampleDebugMode_ = ofClamp(asciiSupersampleDebugMode_, 0.0f, 4.0f);
    if (!asciiSupersampleAtlas_) {
        asciiSupersampleAtlas_ = std::make_unique<AsciiSupersampleAtlas>();
    }
    asciiSupersampleAtlas_->build();
    const int supersampleFontCount = asciiSupersampleAtlas_->fontCount();
    const float asciiSsFontMax = supersampleFontCount > 0 ? static_cast<float>(supersampleFontCount - 1) : 0.0f;
    asciiSupersampleFontIndex_ = ofClamp(asciiSupersampleFontIndex_, 0.0f, asciiSsFontMax);
    registry_ = &registry;

    ParameterRegistry::Descriptor routeMeta;
    routeMeta.group = "Effects";
    routeMeta.range.min = 0.0f;
    routeMeta.range.max = 2.0f;
    routeMeta.range.step = 1.0f;
    routeMeta.description = kRouteDescription;
    routeMeta.quickAccess = true;

    ParameterRegistry::Descriptor coverageMeta;
    coverageMeta.group = "Effects";
    coverageMeta.range.min = 0.0f;
    coverageMeta.range.max = 8.0f;
    coverageMeta.range.step = 1.0f;
    coverageMeta.description = "Columns upstream processed by this effect (0 = All)";
    coverageMeta.quickAccess = true;

    ParameterRegistry::Descriptor maskMeta;
    maskMeta.group = "Effects";
    maskMeta.description = "Enable coverage mask (limit effect rendering to upstream window)";
    maskMeta.quickAccess = true;


    routeMeta.label = "Dither Route";
    routeMeta.quickAccessOrder = 60;
    registry.addFloat("effects.dither.route", &ditherRoute_, ditherRoute_, routeMeta);
    coverageMeta.label = "Dither Coverage";
    coverageMeta.quickAccessOrder = 60.5f;
    registry.addFloat("effects.dither.coverage", &ditherCoverage_, ditherCoverage_, coverageMeta);
    maskMeta.label = "Dither Coverage Mask";
    maskMeta.quickAccessOrder = 60.6f;
    registry.addBool("effects.dither.coverageMask", &ditherCoverageMask_, ditherCoverageMask_, maskMeta);

    ParameterRegistry::Descriptor ditherCellMeta;
    ditherCellMeta.label = "Dither Cell Size";
    ditherCellMeta.group = "Effects";
    ditherCellMeta.range.min = 1.0f;
    ditherCellMeta.range.max = 12.0f;
    ditherCellMeta.range.step = 1.0f;
    ditherCellMeta.quickAccess = true;
    ditherCellMeta.quickAccessOrder = 61;
    registry.addFloat("effects.dither.cellSize", &ditherCellSize_, ditherCellSize_, ditherCellMeta);

    ParameterRegistry::Descriptor ditherModeMeta;
    ditherModeMeta.label = "Dither Mode";
    ditherModeMeta.group = "Effects";
    ditherModeMeta.range.min = 0.0f;
    ditherModeMeta.range.max = 2.0f;
    ditherModeMeta.range.step = 1.0f;
    ditherModeMeta.description = "0=2x2 1=4x4 2=8x8";
    ditherModeMeta.quickAccess = true;
    ditherModeMeta.quickAccessOrder = 61.5f;
    registry.addFloat("effects.dither.mode", &ditherMode_, ditherMode_, ditherModeMeta);

    routeMeta.label = "ASCII Route";
    routeMeta.quickAccessOrder = 62;
    registry.addFloat("effects.ascii.route", &asciiRoute_, asciiRoute_, routeMeta);
    coverageMeta.label = "ASCII Coverage";
    coverageMeta.quickAccessOrder = 62.5f;
    registry.addFloat("effects.ascii.coverage", &asciiCoverage_, asciiCoverage_, coverageMeta);
    maskMeta.label = "ASCII Coverage Mask";
    maskMeta.quickAccessOrder = 62.6f;
    registry.addBool("effects.ascii.coverageMask", &asciiCoverageMask_, asciiCoverageMask_, maskMeta);

    ParameterRegistry::Descriptor asciiSetMeta;
    asciiSetMeta.label = "ASCII Character Set";
    asciiSetMeta.group = "Effects";
    asciiSetMeta.range.min = 0.0f;
    asciiSetMeta.range.max = 2.0f;
    asciiSetMeta.range.step = 1.0f;
    asciiSetMeta.description = "0=Lowercase 1=Numbers 2=Braille";
    asciiSetMeta.quickAccess = true;
    asciiSetMeta.quickAccessOrder = 63;
    registry.addFloat("effects.ascii.characterSet", &asciiCharacterSet_, asciiCharacterSet_, asciiSetMeta);

    ParameterRegistry::Descriptor asciiAspectMeta;
    asciiAspectMeta.label = "ASCII Aspect";
    asciiAspectMeta.group = "Effects";
    asciiAspectMeta.range.min = 0.0f;
    asciiAspectMeta.range.max = 1.0f;
    asciiAspectMeta.range.step = 1.0f;
    asciiAspectMeta.description = "0=5x7 1=Square";
    asciiAspectMeta.quickAccess = true;
    asciiAspectMeta.quickAccessOrder = 64;
    registry.addFloat("effects.ascii.aspect", &asciiAspectMode_, asciiAspectMode_, asciiAspectMeta);

    ParameterRegistry::Descriptor asciiPaddingMeta;
    asciiPaddingMeta.label = "ASCII Padding";
    asciiPaddingMeta.group = "Effects";
    asciiPaddingMeta.range.min = 0.0f;
    asciiPaddingMeta.range.max = 0.45f;
    asciiPaddingMeta.range.step = 0.01f;
    asciiPaddingMeta.description = "0=No gap 1=Max gap";
    asciiPaddingMeta.quickAccess = true;
    asciiPaddingMeta.quickAccessOrder = 64.5f;
    registry.addFloat("effects.ascii.padding", &asciiPadding_, asciiPadding_, asciiPaddingMeta);

    ParameterRegistry::Descriptor asciiColorMeta;
    asciiColorMeta.label = "ASCII Color Mode";
    asciiColorMeta.group = "Effects";
    asciiColorMeta.range.min = 0.0f;
    asciiColorMeta.range.max = 2.0f;
    asciiColorMeta.range.step = 1.0f;
    asciiColorMeta.description = "0=Source 1=Greyscale 2=Green Mono";
    asciiColorMeta.quickAccess = true;
    asciiColorMeta.quickAccessOrder = 65;
    registry.addFloat("effects.ascii.colorMode", &asciiColorMode_, asciiColorMode_, asciiColorMeta);

    ParameterRegistry::Descriptor asciiBlockMeta;
    asciiBlockMeta.label = "ASCII Block Size";
    asciiBlockMeta.group = "Effects";
    asciiBlockMeta.range.min = 3.0f;
    asciiBlockMeta.range.max = 24.0f;
    asciiBlockMeta.range.step = 1.0f;
    asciiBlockMeta.quickAccess = true;
    asciiBlockMeta.quickAccessOrder = 66;
    registry.addFloat("effects.ascii.block", &asciiBlockSize_, asciiBlockSize_, asciiBlockMeta);

    ParameterRegistry::Descriptor asciiGammaMeta;
    asciiGammaMeta.label = "ASCII Gamma";
    asciiGammaMeta.group = "Effects";
    asciiGammaMeta.range.min = 0.2f;
    asciiGammaMeta.range.max = 3.0f;
    asciiGammaMeta.range.step = 0.01f;
    asciiGammaMeta.description = "Remap luminance before glyph selection";
    asciiGammaMeta.quickAccess = true;
    asciiGammaMeta.quickAccessOrder = 66.5f;
    registry.addFloat("effects.ascii.gamma", &asciiGamma_, asciiGamma_, asciiGammaMeta);

    ParameterRegistry::Descriptor asciiJitterMeta;
    asciiJitterMeta.label = "ASCII Jitter";
    asciiJitterMeta.group = "Effects";
    asciiJitterMeta.range.min = 0.0f;
    asciiJitterMeta.range.max = 1.0f;
    asciiJitterMeta.range.step = 0.01f;
    asciiJitterMeta.description = "Jitter sample position (0=stable,1=max)";
    asciiJitterMeta.quickAccess = true;
    asciiJitterMeta.quickAccessOrder = 67;
    registry.addFloat("effects.ascii.jitter", &asciiJitter_, asciiJitter_, asciiJitterMeta);

    routeMeta.label = "ASCII Supersample Route";
    routeMeta.quickAccessOrder = 67.1f;
    registry.addFloat("effects.asciiSupersample.route", &asciiSupersampleRoute_, asciiSupersampleRoute_, routeMeta);
    coverageMeta.label = "ASCII Supersample Coverage";
    coverageMeta.quickAccessOrder = 67.2f;
    registry.addFloat("effects.asciiSupersample.coverage", &asciiSupersampleCoverage_, asciiSupersampleCoverage_, coverageMeta);
    maskMeta.label = "ASCII Supersample Coverage Mask";
    maskMeta.quickAccessOrder = 67.25f;
    registry.addBool("effects.asciiSupersample.coverageMask", &asciiSupersampleCoverageMask_, asciiSupersampleCoverageMask_, maskMeta);

    ParameterRegistry::Descriptor asciiSsSetMeta = asciiSetMeta;
    asciiSsSetMeta.label = "ASCII Supersample Character Set";
    asciiSsSetMeta.range.max = 2.0f;
    asciiSsSetMeta.quickAccessOrder = 67.3f;
    asciiSsSetMeta.description = "Locked to lowercase characters";
    registry.addFloat("effects.asciiSupersample.characterSet", &asciiSupersampleCharacterSet_, asciiSupersampleCharacterSet_, asciiSsSetMeta);

    ParameterRegistry::Descriptor asciiSsFontMeta;
    asciiSsFontMeta.label = "ASCII Supersample Font";
    asciiSsFontMeta.group = "Effects";
    asciiSsFontMeta.range.min = 0.0f;
    asciiSsFontMeta.range.max = asciiSsFontMax;
    asciiSsFontMeta.range.step = 1.0f;
    asciiSsFontMeta.description = "Index into fonts/ascii_supersample (drop curated mono fonts there)";
    asciiSsFontMeta.quickAccess = true;
    asciiSsFontMeta.quickAccessOrder = 67.32f;
    registry.addFloat("effects.asciiSupersample.font", &asciiSupersampleFontIndex_, asciiSupersampleFontIndex_, asciiSsFontMeta);

    ParameterRegistry::Descriptor asciiSsColorMeta = asciiColorMeta;
    asciiSsColorMeta.label = "ASCII Supersample Color Mode";
    asciiSsColorMeta.quickAccessOrder = 67.45f;
    registry.addFloat("effects.asciiSupersample.colorMode", &asciiSupersampleColorMode_, asciiSupersampleColorMode_, asciiSsColorMeta);

    ParameterRegistry::Descriptor asciiSsBlockMeta = asciiBlockMeta;
    asciiSsBlockMeta.label = "ASCII Supersample Block Size";
    asciiSsBlockMeta.quickAccessOrder = 67.5f;
    registry.addFloat("effects.asciiSupersample.block", &asciiSupersampleBlockSize_, asciiSupersampleBlockSize_, asciiSsBlockMeta);

    ParameterRegistry::Descriptor asciiSsGammaMeta = asciiGammaMeta;
    asciiSsGammaMeta.label = "ASCII Supersample Gamma";
    asciiSsGammaMeta.quickAccessOrder = 67.55f;
    registry.addFloat("effects.asciiSupersample.gamma", &asciiSupersampleGamma_, asciiSupersampleGamma_, asciiSsGammaMeta);

    ParameterRegistry::Descriptor asciiSsJitterMeta = asciiJitterMeta;
    asciiSsJitterMeta.label = "ASCII Supersample Jitter";
    asciiSsJitterMeta.quickAccessOrder = 67.6f;
    registry.addFloat("effects.asciiSupersample.jitter", &asciiSupersampleJitter_, asciiSupersampleJitter_, asciiSsJitterMeta);
    ParameterRegistry::Descriptor asciiSsDebugMeta;
    asciiSsDebugMeta.group = "Effects";
    asciiSsDebugMeta.label = "ASCII Supersample Debug Mode";
    asciiSsDebugMeta.description = "0=normal, 1=alpha preview, 2=index, 3=atlas sample, 4=UV/pad mask, 5=coverage";
    asciiSsDebugMeta.range.min = 0.0f;
    asciiSsDebugMeta.range.max = 5.0f;
    asciiSsDebugMeta.range.step = 1.0f;
    asciiSsDebugMeta.quickAccessOrder = 67.65f;
    registry.addFloat("effects.asciiSupersample.debugMode", &asciiSupersampleDebugMode_, asciiSupersampleDebugMode_, asciiSsDebugMeta);

    routeMeta.label = "CRT Route";
    routeMeta.quickAccessOrder = 65;
    registry.addFloat("effects.crt.route", &crtRoute_, crtRoute_, routeMeta);
    coverageMeta.label = "CRT Coverage";
    coverageMeta.quickAccessOrder = 65.5f;
    registry.addFloat("effects.crt.coverage", &crtCoverage_, crtCoverage_, coverageMeta);
    maskMeta.label = "CRT Coverage Mask";
    maskMeta.quickAccessOrder = 65.6f;
    registry.addBool("effects.crt.coverageMask", &crtCoverageMask_, crtCoverageMask_, maskMeta);

    ParameterRegistry::Descriptor crtMeta;
    crtMeta.group = "Effects";
    crtMeta.range.min = 0.0f;
    crtMeta.range.max = 1.0f;
    crtMeta.range.step = 0.01f;
    crtMeta.quickAccess = true;

    crtMeta.label = "CRT Scanline";
    crtMeta.quickAccessOrder = 66;
    registry.addFloat("effects.crt.scanline", &crtScanlineIntensity_, crtScanlineIntensity_, crtMeta);

    crtMeta.label = "CRT Vignette";
    crtMeta.quickAccessOrder = 66.25f;
    registry.addFloat("effects.crt.vignette", &crtVignetteIntensity_, crtVignetteIntensity_, crtMeta);

    crtMeta.label = "CRT Bleed";
    crtMeta.quickAccessOrder = 66.5f;
    crtMeta.range.max = 0.4f;
    registry.addFloat("effects.crt.bleed", &crtBleed_, crtBleed_, crtMeta);
    crtMeta.range.max = 1.0f;

    ParameterRegistry::Descriptor crtSoftMeta = crtMeta;
    crtSoftMeta.label = "CRT Softness";
    crtSoftMeta.quickAccessOrder = 66.75f;
    registry.addFloat("effects.crt.softness", &crtSoftness_, crtSoftness_, crtSoftMeta);

    ParameterRegistry::Descriptor crtGlowMeta = crtMeta;
    crtGlowMeta.label = "CRT Glow";
    crtGlowMeta.quickAccessOrder = 66.78f;
    registry.addFloat("effects.crt.glow", &crtGlow_, crtGlow_, crtGlowMeta);

    ParameterRegistry::Descriptor crtOffsetMeta = crtMeta;
    crtOffsetMeta.label = "CRT Per-Channel Offset";
    crtOffsetMeta.quickAccessOrder = 66.8f;
    registry.addFloat("effects.crt.perChannelOffset", &crtPerChannelOffset_, crtPerChannelOffset_, crtOffsetMeta);

    ParameterRegistry::Descriptor crtJitterMeta = crtMeta;
    crtJitterMeta.label = "CRT Scanline Jitter";
    crtJitterMeta.quickAccessOrder = 66.85f;
    registry.addFloat("effects.crt.scanlineJitter", &crtScanlineJitter_, crtScanlineJitter_, crtJitterMeta);

    ParameterRegistry::Descriptor crtSubpixelMeta = crtMeta;
    crtSubpixelMeta.label = "CRT Subpixel Density";
    crtSubpixelMeta.range.min = 0.0f;
    crtSubpixelMeta.range.max = 8.0f;
    crtSubpixelMeta.quickAccessOrder = 66.9f;
    registry.addFloat("effects.crt.subpixelDensity", &crtSubpixelDensity_, crtSubpixelDensity_, crtSubpixelMeta);

    ParameterRegistry::Descriptor crtAspectMeta = crtMeta;
    crtAspectMeta.label = "CRT Subpixel Aspect";
    crtAspectMeta.range.min = 0.5f;
    crtAspectMeta.range.max = 4.0f;
    crtAspectMeta.quickAccessOrder = 66.91f;
    registry.addFloat("effects.crt.subpixelAspect", &crtSubpixelAspect_, crtSubpixelAspect_, crtAspectMeta);

    ParameterRegistry::Descriptor crtPadMeta = crtMeta;
    crtPadMeta.label = "CRT Subpixel Padding";
    crtPadMeta.range.min = 0.0f;
    crtPadMeta.range.max = 0.5f;
    crtPadMeta.quickAccessOrder = 66.92f;
    registry.addFloat("effects.crt.subpixelPadding", &crtSubpixelPadding_, crtSubpixelPadding_, crtPadMeta);

    ParameterRegistry::Descriptor crtMisalignMeta = crtMeta;
    crtMisalignMeta.label = "CRT RGB Misalignment";
    crtMisalignMeta.quickAccessOrder = 66.95f;
    registry.addFloat("effects.crt.rgbMisalignment", &crtRgbMisalignment_, crtRgbMisalignment_, crtMisalignMeta);

    ParameterRegistry::Descriptor crtSyncMeta = crtMeta;
    crtSyncMeta.label = "CRT Sync Tear";
    crtSyncMeta.quickAccessOrder = 67.0f;
    registry.addFloat("effects.crt.syncTear", &crtSyncTear_, crtSyncTear_, crtSyncMeta);

    ParameterRegistry::Descriptor crtTrackMeta = crtMeta;
    crtTrackMeta.label = "CRT Tracking Wobble";
    crtTrackMeta.quickAccessOrder = 67.05f;
    registry.addFloat("effects.crt.trackingWobble", &crtTrackingWobble_, crtTrackingWobble_, crtTrackMeta);

    ParameterRegistry::Descriptor crtNoiseMeta = crtMeta;
    crtNoiseMeta.label = "CRT Luma Noise";
    crtNoiseMeta.quickAccessOrder = 67.1f;
    registry.addFloat("effects.crt.lumaNoise", &crtLumaNoise_, crtLumaNoise_, crtNoiseMeta);

    ParameterRegistry::Descriptor crtHeadMeta = crtMeta;
    crtHeadMeta.label = "CRT Head-Switch Flicker";
    crtHeadMeta.quickAccessOrder = 67.15f;
    registry.addFloat("effects.crt.headSwitch", &crtHeadSwitchFlicker_, crtHeadSwitchFlicker_, crtHeadMeta);

    routeMeta.label = "Motion Route";
    routeMeta.quickAccessOrder = 69;
    registry.addFloat("effects.motion.route", &motionRoute_, motionRoute_, routeMeta);
    coverageMeta.label = "Motion Coverage";
    coverageMeta.quickAccessOrder = 69.5f;
    registry.addFloat("effects.motion.coverage", &motionCoverage_, motionCoverage_, coverageMeta);
    maskMeta.label = "Motion Coverage Mask";
    maskMeta.quickAccessOrder = 69.6f;
    registry.addBool("effects.motion.coverageMask", &motionCoverageMask_, motionCoverageMask_, maskMeta);

    ParameterRegistry::Descriptor motionThresholdMeta;
    motionThresholdMeta.group = "Effects";
    motionThresholdMeta.label = "Motion Threshold";
    motionThresholdMeta.range.min = 0.0f;
    motionThresholdMeta.range.max = 1.0f;
    motionThresholdMeta.range.step = 0.01f;
    motionThresholdMeta.quickAccess = true;
    motionThresholdMeta.quickAccessOrder = 70;
    registry.addFloat("effects.motion.threshold", &motionThreshold_, motionThreshold_, motionThresholdMeta);

    ParameterRegistry::Descriptor motionBoostMeta = motionThresholdMeta;
    motionBoostMeta.label = "Motion Boost";
    motionBoostMeta.range.max = 8.0f;
    motionBoostMeta.quickAccessOrder = 71;
    registry.addFloat("effects.motion.boost", &motionBoost_, motionBoost_, motionBoostMeta);

    ParameterRegistry::Descriptor motionMixMeta = motionThresholdMeta;
    motionMixMeta.label = "Motion Mix";
    motionMixMeta.quickAccessOrder = 72;
    registry.addFloat("effects.motion.mix", &motionMix_, motionMix_, motionMixMeta);

    ParameterRegistry::Descriptor motionSoftMeta = motionThresholdMeta;
    motionSoftMeta.label = "Motion Softness";
    motionSoftMeta.range.min = 0.01f;
    motionSoftMeta.range.max = 0.5f;
    motionSoftMeta.quickAccessOrder = 73;
    registry.addFloat("effects.motion.softness", &motionSoftness_, motionSoftness_, motionSoftMeta);

    ParameterRegistry::Descriptor motionFadeMeta = motionThresholdMeta;
    motionFadeMeta.label = "Motion Fade (frames)";
    motionFadeMeta.range.min = 0.0f;
    motionFadeMeta.range.max = 240.0f;
    motionFadeMeta.range.step = 1.0f;
    motionFadeMeta.units = "frames";
    motionFadeMeta.description = "Number of frames motion trails linger (tempo-sync placeholder)";
    motionFadeMeta.quickAccessOrder = 73.5f;
    registry.addFloat("effects.motion.fadeBeats", &motionFadeBeats_, motionFadeBeats_, motionFadeMeta);

    ParameterRegistry::Descriptor motionBlurMeta = motionThresholdMeta;
    motionBlurMeta.label = "Motion Blur Strength";
    motionBlurMeta.range.min = 0.0f;
    motionBlurMeta.range.max = 1.0f;
    motionBlurMeta.quickAccessOrder = 74;
    registry.addFloat("effects.motion.blur", &motionBlurStrength_, motionBlurStrength_, motionBlurMeta);

    ParameterRegistry::Descriptor motionAlphaMeta = motionThresholdMeta;
    motionAlphaMeta.label = "Motion Alpha Mix";
    motionAlphaMeta.range.min = 0.0f;
    motionAlphaMeta.range.max = 1.0f;
    motionAlphaMeta.description = "Blend between opaque output (0) and motion-derived alpha (1)";
    motionAlphaMeta.quickAccessOrder = 75;
    registry.addFloat("effects.motion.alphaMix", &motionAlphaWeight_, motionAlphaWeight_, motionAlphaMeta);

    ParameterRegistry::Descriptor motionHeadMeta = motionThresholdMeta;
    motionHeadMeta.range.min = 0.0f;
    motionHeadMeta.range.max = 255.0f;
    motionHeadMeta.range.step = 1.0f;
    motionHeadMeta.description = "Newest motion trail color";
    motionHeadMeta.quickAccessOrder = 75.1f;
    motionHeadMeta.label = "Motion Head Color R";
    registry.addFloat("effects.motion.headColorR", &motionHeadColorR_, motionHeadColorR_, motionHeadMeta);
    motionHeadMeta.quickAccessOrder = 75.2f;
    motionHeadMeta.label = "Motion Head Color G";
    registry.addFloat("effects.motion.headColorG", &motionHeadColorG_, motionHeadColorG_, motionHeadMeta);
    motionHeadMeta.quickAccessOrder = 75.3f;
    motionHeadMeta.label = "Motion Head Color B";
    registry.addFloat("effects.motion.headColorB", &motionHeadColorB_, motionHeadColorB_, motionHeadMeta);

    ParameterRegistry::Descriptor motionTailMeta = motionHeadMeta;
    motionTailMeta.description = "Oldest visible motion trail color";
    motionTailMeta.quickAccessOrder = 75.4f;
    motionTailMeta.label = "Motion Tail Color R";
    registry.addFloat("effects.motion.tailColorR", &motionTailColorR_, motionTailColorR_, motionTailMeta);
    motionTailMeta.quickAccessOrder = 75.5f;
    motionTailMeta.label = "Motion Tail Color G";
    registry.addFloat("effects.motion.tailColorG", &motionTailColorG_, motionTailColorG_, motionTailMeta);
    motionTailMeta.quickAccessOrder = 75.6f;
    motionTailMeta.label = "Motion Tail Color B";
    registry.addFloat("effects.motion.tailColorB", &motionTailColorB_, motionTailColorB_, motionTailMeta);

    ParameterRegistry::Descriptor motionTailOpacityMeta = motionThresholdMeta;
    motionTailOpacityMeta.label = "Motion Tail Opacity %";
    motionTailOpacityMeta.range.min = 0.0f;
    motionTailOpacityMeta.range.max = 100.0f;
    motionTailOpacityMeta.range.step = 1.0f;
    motionTailOpacityMeta.units = "%";
    motionTailOpacityMeta.description = "Opacity of the oldest visible motion trail";
    motionTailOpacityMeta.quickAccessOrder = 75.7f;
    registry.addFloat("effects.motion.tailOpacity", &motionTailOpacity_, motionTailOpacity_, motionTailOpacityMeta);

    const float* transportBpmPtr = nullptr;
    if (auto* bpmParam = registry.findFloat("transport.bpm")) {
        transportBpmPtr = bpmParam->value;
    }

    ditherEffect_ = std::make_unique<DitherEffect>(&ditherCellSize_, &ditherMode_);
    asciiEffect_ = std::make_unique<AsciiEffect>(&asciiBlockSize_,
                                                 &asciiColorMode_,
                                                 &asciiCharacterSet_,
                                                 &asciiAspectMode_,
                                                 &asciiPadding_,
                                                 &asciiGamma_,
                                                 &asciiJitter_,
                                                 &asciiGreenTint_);
    asciiSupersampleEffect_ = std::make_unique<AsciiSupersampleEffect>(&asciiSupersampleBlockSize_,
                                                                       &asciiSupersampleColorMode_,
                                                                       &asciiSupersampleCharacterSet_,
                                                                       &asciiSupersampleFontIndex_,
                                                                       &asciiSupersampleGamma_,
                                                                       &asciiSupersampleJitter_,
                                                                       &asciiSupersampleDebugMode_,
                                                                       &asciiSupersampleGreenTint_,
                                                                       asciiSupersampleAtlas_.get());
    crtEffect_ = std::make_unique<CrtEffect>(&crtScanlineIntensity_,
                                             &crtVignetteIntensity_,
                                             &crtBleed_,
                                             &crtSoftness_,
                                             &crtGlow_,
                                             &crtPerChannelOffset_,
                                             &crtScanlineJitter_,
                                             &crtSubpixelDensity_,
                                             &crtSubpixelAspect_,
                                             &crtSubpixelPadding_,
                                             &crtRgbMisalignment_,
                                             &crtSyncTear_,
                                             &crtTrackingWobble_,
                                             &crtLumaNoise_,
                                             &crtHeadSwitchFlicker_);
    motionProcessor_ = std::make_unique<MotionExtractProcessor>(&motionThreshold_,
                                                                &motionBoost_,
                                                                &motionMix_,
                                                                &motionSoftness_,
                                                                &motionFadeBeats_,
                                                                &motionBlurStrength_,
                                                                &motionAlphaWeight_,
                                                                &motionHeadColorR_,
                                                                &motionHeadColorG_,
                                                                &motionHeadColorB_,
                                                                &motionTailColorR_,
                                                                &motionTailColorG_,
                                                                &motionTailColorB_,
                                                                &motionTailOpacity_,
                                                                transportBpmPtr);
    motionEffect_ = std::make_unique<MotionEffectWrapper>(motionProcessor_.get());
}

void PostEffectChain::resize(int width, int height) {
    ensureBuffers(width, height);
    if (ditherEffect_) ditherEffect_->resize(width, height);
    if (asciiEffect_) asciiEffect_->resize(width, height);
    if (asciiSupersampleEffect_) asciiSupersampleEffect_->resize(width, height);
    if (crtEffect_) crtEffect_->resize(width, height);
    if (motionEffect_) motionEffect_->resize(width, height);
}

void PostEffectChain::applyConsole(ofFbo& fbo) {
    std::vector<Effect*> active;
    if (routeFromValue(ditherRoute_) == Route::Console && ditherEffect_) active.push_back(ditherEffect_.get());
    if (routeFromValue(asciiRoute_) == Route::Console && asciiEffect_) active.push_back(asciiEffect_.get());
    if (routeFromValue(asciiSupersampleRoute_) == Route::Console && asciiSupersampleEffect_)
        active.push_back(asciiSupersampleEffect_.get());
    if (routeFromValue(crtRoute_) == Route::Console && crtEffect_) active.push_back(crtEffect_.get());
    if (routeFromValue(motionRoute_) == Route::Console && motionEffect_) active.push_back(motionEffect_.get());
    process(fbo, active);
}

void PostEffectChain::applyGlobal(ofFbo& fbo) {
    std::vector<Effect*> active;
    if (routeFromValue(ditherRoute_) == Route::Global && ditherEffect_) active.push_back(ditherEffect_.get());
    if (routeFromValue(asciiRoute_) == Route::Global && asciiEffect_) active.push_back(asciiEffect_.get());
    if (routeFromValue(asciiSupersampleRoute_) == Route::Global && asciiSupersampleEffect_)
        active.push_back(asciiSupersampleEffect_.get());
    if (routeFromValue(crtRoute_) == Route::Global && crtEffect_) active.push_back(crtEffect_.get());
    if (routeFromValue(motionRoute_) == Route::Global && motionEffect_) active.push_back(motionEffect_.get());
    process(fbo, active);
}

void PostEffectChain::applyDither(const ofFbo& src, ofFbo& dst) {
    if (ditherEffect_) {
        ditherEffect_->apply(src, dst);
    }
}

void PostEffectChain::applyAscii(const ofFbo& src, ofFbo& dst) {
    if (asciiEffect_) {
        asciiEffect_->apply(src, dst);
    }
}

void PostEffectChain::applyAsciiSupersample(const ofFbo& src, ofFbo& dst) {
    if (asciiSupersampleEffect_) {
        asciiSupersampleEffect_->apply(src, dst);
    }
}

void PostEffectChain::applyCrt(const ofFbo& src, ofFbo& dst) {
    if (crtEffect_) {
        crtEffect_->apply(src, dst);
    }
}

void PostEffectChain::applyMotionExtract(const ofFbo& src, ofFbo& dst) {
    if (motionEffect_) {
        motionEffect_->apply(src, dst);
    }
}

float PostEffectChain::defaultCoverageForType(const std::string& effectType) const {
    if (effectType == "fx.dither") return ditherCoverage_;
    if (effectType == "fx.ascii") return asciiCoverage_;
    if (effectType == "fx.ascii_supersample") return asciiSupersampleCoverage_;
    if (effectType == "fx.crt") return crtCoverage_;
    if (effectType == "fx.motion_extract") return motionCoverage_;
    return 0.0f;
}

bool PostEffectChain::coverageMaskEnabled(const std::string& effectType) const {
    if (effectType == "fx.dither") return ditherCoverageMask_;
    if (effectType == "fx.ascii") return asciiCoverageMask_;
    if (effectType == "fx.ascii_supersample") return asciiSupersampleCoverageMask_;
    if (effectType == "fx.crt") return crtCoverageMask_;
    if (effectType == "fx.motion_extract") return motionCoverageMask_;
    return false;
}

void PostEffectChain::ensureBuffers(int width, int height) {
    if (width == bufferWidth_ && height == bufferHeight_ && pingFbo_.isAllocated() && pongFbo_.isAllocated()) {
        return;
    }
    bufferWidth_ = width;
    bufferHeight_ = height;

    ofFbo::Settings settings;
    settings.width = std::max(1, width);
    settings.height = std::max(1, height);
    settings.useDepth = false;
    settings.useStencil = false;
    settings.internalformat = GL_RGBA;
    settings.textureTarget = GL_TEXTURE_2D;
    settings.minFilter = GL_LINEAR;
    settings.maxFilter = GL_LINEAR;
    settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    settings.wrapModeVertical = GL_CLAMP_TO_EDGE;

    pingFbo_.allocate(settings);
    pongFbo_.allocate(settings);
}

void PostEffectChain::prepareLayerBuffers(int layerCount, int width, int height) {
    if (layerCount <= 0 || width <= 0 || height <= 0) {
        layerBuffers_.clear();
        layerBufferWidth_ = 0;
        layerBufferHeight_ = 0;
        return;
    }

    const int targetWidth = std::max(1, width);
    const int targetHeight = std::max(1, height);
    const bool dimensionsChanged = layerBufferWidth_ != targetWidth || layerBufferHeight_ != targetHeight;

    layerBufferWidth_ = targetWidth;
    layerBufferHeight_ = targetHeight;

    if (static_cast<int>(layerBuffers_.size()) != layerCount) {
        layerBuffers_.resize(layerCount);
    }

    ofFbo::Settings settings;
    settings.width = targetWidth;
    settings.height = targetHeight;
    settings.useDepth = false;
    settings.useStencil = false;
    settings.internalformat = GL_RGBA;
    settings.textureTarget = GL_TEXTURE_2D;
    settings.minFilter = GL_LINEAR;
    settings.maxFilter = GL_LINEAR;
    settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    settings.wrapModeVertical = GL_CLAMP_TO_EDGE;

    for (auto& buffer : layerBuffers_) {
        if (!buffer.isAllocated() || dimensionsChanged || buffer.getWidth() != targetWidth ||
            buffer.getHeight() != targetHeight) {
            buffer.allocate(settings);
        }
    }
}

ofFbo* PostEffectChain::layerBufferPtr(int index) {
    if (index < 0 || index >= static_cast<int>(layerBuffers_.size())) {
        return nullptr;
    }
    return &layerBuffers_[index];
}

PostEffectChain::CoverageWindow PostEffectChain::resolveCoverageWindow(int effectColumnIndex,
                                                                       float coverageParamValue) const {
    CoverageWindow window;
    window.effectColumn = std::max(1, effectColumnIndex);
    window.lastColumn = std::max(0, window.effectColumn - 1);
    window.requestedColumns = coverageParamValue <= 0.0f
                                  ? 0
                                  : static_cast<int>(std::floor(coverageParamValue + 0.0001f));

    if (window.lastColumn == 0) {
        window.firstColumn = 1;
        window.includesAll = true;
        return window;
    }

    if (window.requestedColumns <= 0 || window.requestedColumns >= window.lastColumn) {
        window.firstColumn = 1;
        window.includesAll = true;
    } else {
        window.includesAll = false;
        window.firstColumn = std::max(1, window.effectColumn - window.requestedColumns);
    }
    return window;
}

void PostEffectChain::process(ofFbo& fbo, const std::vector<Effect*>& effects) {
    if (effects.empty()) return;
    ensureBuffers(fbo.getWidth(), fbo.getHeight());

    ofFbo* src = &fbo;
    ofFbo* dst = &pingFbo_;
    bool used = false;

    for (auto* effect : effects) {
        if (!effect) continue;
        effect->apply(*src, *dst);
        src = dst;
        dst = (dst == &pingFbo_) ? &pongFbo_ : &pingFbo_;
        used = true;
    }

    if (used && src != &fbo) {
        fbo.begin();
        bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
        if (scissorWasEnabled) {
            glDisable(GL_SCISSOR_TEST);
        }
        ofPushView();
        ofViewport(0, 0, fbo.getWidth(), fbo.getHeight());
        ofSetupScreenOrtho(fbo.getWidth(), fbo.getHeight(), -1, 1);
        ofPushStyle();
        ofClear(0, 0, 0, 0);
        if (depthWasEnabled) {
            ofDisableDepthTest();
        }
        ofSetColor(255);
        src->draw(0, 0, fbo.getWidth(), fbo.getHeight());
        ofPopStyle();
        ofPopView();
        if (depthWasEnabled) {
            ofEnableDepthTest();
        }
        if (scissorWasEnabled) {
            glEnable(GL_SCISSOR_TEST);
        }
        fbo.end();
    }
}

PostEffectChain::Route PostEffectChain::routeFromValue(float value) const {
    int intValue = static_cast<int>(std::round(ofClamp(value, 0.0f, 2.0f)));
    switch (intValue) {
    case 1: return Route::Console;
    case 2: return Route::Global;
    default: return Route::Off;
    }
}
