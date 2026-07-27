#include "BuiltinElementHostBindings.h"

#include "../core/ParameterRegistry.h"
#include "../visuals/TextLayerState.h"

#include <string>

namespace synaptome::runtime {
namespace {

ParameterRegistry::Range makeRange(float min, float max, float step) {
    ParameterRegistry::Range range;
    range.min = min;
    range.max = max;
    range.step = step;
    return range;
}

void addFloat(
    const std::string& id,
    ParameterRegistry& registry,
    float* value,
    float defaultValue,
    const std::string& label,
    const std::string& group,
    const ParameterRegistry::Range& range,
    bool quickAccess = false,
    int quickAccessOrder = 0,
    const std::string& units = std::string(),
    const std::string& description = std::string()) {
    ParameterRegistry::Descriptor meta;
    meta.label = label;
    meta.group = group;
    meta.units = units;
    meta.description = description;
    meta.range = range;
    meta.quickAccess = quickAccess;
    meta.quickAccessOrder = quickAccessOrder;
    registry.addFloat(id, value, defaultValue, meta);
}

void addString(
    const std::string& id,
    ParameterRegistry& registry,
    std::string* value,
    const std::string& defaultValue,
    const std::string& label,
    const std::string& group,
    const std::string& description = std::string()) {
    ParameterRegistry::Descriptor meta;
    meta.label = label;
    meta.group = group;
    meta.description = description;
    registry.addString(id, value, defaultValue, meta);
}

} // namespace

void registerBuiltinElementHostParameters(ParameterRegistry& registry) {
    auto& textState = TextLayerState::instance();
    textState.refreshAvailableFonts();
    const std::string overlayGroup = "Overlay";
    addString(
        "overlay.text.content",
        registry,
        &textState.content,
        textState.content,
        "Motion: Center Text",
        overlayGroup,
        "Text displayed in the center text slot");
    addString(
        "overlay.text.topLeft",
        registry,
        &textState.topLeft,
        textState.topLeft,
        "Motion: Top Left Text",
        overlayGroup,
        "Text displayed in the top-left text slot");
    addString(
        "overlay.text.topRight",
        registry,
        &textState.topRight,
        textState.topRight,
        "Motion: Top Right Text",
        overlayGroup,
        "Text displayed in the top-right text slot");
    addString(
        "overlay.text.bottomLeft",
        registry,
        &textState.bottomLeft,
        textState.bottomLeft,
        "Motion: Bottom Left Text",
        overlayGroup,
        "Text displayed in the bottom-left text slot");
    addString(
        "overlay.text.bottomRight",
        registry,
        &textState.bottomRight,
        textState.bottomRight,
        "Motion: Bottom Right Text",
        overlayGroup,
        "Text displayed in the bottom-right text slot");
    addString(
        "overlay.text.font",
        registry,
        &textState.font,
        textState.font,
        "Motion: Font File",
        overlayGroup,
        "TrueType font filename under data/fonts");
    const float fontIndexMax = textState.fontIndexMax();
    addFloat(
        "overlay.text.fontIndex",
        registry,
        &textState.fontIndex,
        textState.fontIndex,
        "Font Index",
        overlayGroup,
        makeRange(0.0f, fontIndexMax, 1.0f),
        false,
        0,
        std::string(),
        "Select discovered font by index");
    addFloat(
        "overlay.text.size",
        registry,
        &textState.fontSize,
        textState.fontSize,
        "Center Text Size",
        overlayGroup,
        makeRange(12.0f, 256.0f, 1.0f),
        false,
        0,
        "px",
        "Center text font size in pixels");
    addFloat(
        "overlay.text.corner.size",
        registry,
        &textState.cornerFontSize,
        textState.cornerFontSize,
        "Corner Text Size",
        overlayGroup,
        makeRange(8.0f, 256.0f, 1.0f),
        false,
        0,
        "px",
        "Corner text font size in pixels");
    addFloat(
        "overlay.text.color.r",
        registry,
        &textState.colorR,
        textState.colorR,
        "Text Color R",
        overlayGroup,
        makeRange(0.0f, 255.0f, 1.0f));
    addFloat(
        "overlay.text.color.g",
        registry,
        &textState.colorG,
        textState.colorG,
        "Text Color G",
        overlayGroup,
        makeRange(0.0f, 255.0f, 1.0f));
    addFloat(
        "overlay.text.color.b",
        registry,
        &textState.colorB,
        textState.colorB,
        "Text Color B",
        overlayGroup,
        makeRange(0.0f, 255.0f, 1.0f));
}

void updateBuiltinElementHostParameters() {
    TextLayerState::instance().syncFontSelection();
}

} // namespace synaptome::runtime
