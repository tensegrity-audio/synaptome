#include "ColumnControls.h"

#include "ofMain.h"
#include "ofLog.h"
#include "ofUtils.h"
#include "ofMath.h"
#include <limits>
#include <cmath>

namespace {
std::string formatNumeric(float value, const std::string& units) {
    std::string text = ofToString(value, 3);
    if (!units.empty()) {
        text += " " + units;
    }
    return text;
}
}

// ColumnRowDropdown ---------------------------------------------------------

ColumnRowDropdown::ColumnRowDropdown(std::string id, std::string label)
    : id_(std::move(id))
    , label_(std::move(label)) {}

void ColumnRowDropdown::setOptions(std::vector<Option> options) {
    options_ = std::move(options);
    highlightedIndex_ = selectedIndex();
}

void ColumnRowDropdown::setSelectedValue(const std::string& value) {
    selectedValue_ = value;
    highlightedIndex_ = selectedIndex();
}

int ColumnRowDropdown::selectedIndex() const {
    for (std::size_t i = 0; i < options_.size(); ++i) {
        if (options_[i].value == selectedValue_) {
            return static_cast<int>(i);
        }
    }
    return options_.empty() ? -1 : 0;
}

void ColumnRowDropdown::openDropdown() {
    if (options_.empty()) {
        return;
    }
    open_ = true;
    highlightedIndex_ = selectedIndex();
    if (highlightedIndex_ < 0) {
        highlightedIndex_ = 0;
    }
    typeBuffer_.clear();
    ofLogNotice("DevicesPanel") << "Dropdown opened: " << id_ << " (options=" << options_.size() << ") selected=" << selectedValue_;
}

void ColumnRowDropdown::closeDropdown() {
    open_ = false;
    typeBuffer_.clear();
}

void ColumnRowDropdown::applyHighlight(int index) {
    if (options_.empty()) {
        highlightedIndex_ = -1;
        return;
    }
    int count = static_cast<int>(options_.size());
    if (index < 0) {
        index = count - 1;
    } else if (index >= count) {
        index = 0;
    }
    highlightedIndex_ = index;
}

void ColumnRowDropdown::acceptHighlight() {
    if (!open_) {
        return;
    }
    if (highlightedIndex_ < 0 || highlightedIndex_ >= static_cast<int>(options_.size())) {
        return;
    }
    const auto& option = options_[highlightedIndex_];
    if (selectedValue_ != option.value) {
        selectedValue_ = option.value;
        pendingChanges_ = false;
        if (onValueChanged_) {
            onValueChanged_(selectedValue_);
        }
    }
    closeDropdown();
}

void ColumnRowDropdown::advanceHighlight(int delta) {
    if (!open_) {
        openDropdown();
    }
    applyHighlight(highlightedIndex_ + delta);
}

void ColumnRowDropdown::handleTypeAhead(int baseKey) {
    uint64_t now = ofGetElapsedTimeMillis();
    if (now - lastTypeTimeMs_ > 600) {
        typeBuffer_.clear();
    }
    lastTypeTimeMs_ = now;
    if (baseKey >= 'a' && baseKey <= 'z') {
        typeBuffer_ += static_cast<char>(baseKey);
    } else if (baseKey >= 'A' && baseKey <= 'Z') {
        typeBuffer_ += static_cast<char>(baseKey - 'A' + 'a');
    } else {
        return;
    }
    if (typeBuffer_.empty()) {
        return;
    }
    std::string needle = typeBuffer_;
    for (std::size_t i = 0; i < options_.size(); ++i) {
        std::string optionLabel = ofToLower(options_[i].label);
        if (optionLabel.rfind(needle, 0) == 0) {
            highlightedIndex_ = static_cast<int>(i);
            break;
        }
    }
}

MenuController::EntryView ColumnRowDropdown::toEntryView(bool selected) const {
    MenuController::EntryView entry;
    entry.id = id_;
    entry.label = label_;
    std::string valueLabel;
    int idx = selectedIndex();
    if (idx >= 0 && idx < static_cast<int>(options_.size())) {
        valueLabel = options_[idx].label;
    } else {
        valueLabel = "Select";
    }
    entry.description = valueLabel;
    if (!descriptionSuffix_.empty()) {
        entry.description += "  " + descriptionSuffix_;
    }
    entry.selectable = true;
    entry.selected = selected;
    entry.modifierCount = modifierCount_;
    entry.pendingChanges = pendingChanges_;
    return entry;
}

bool ColumnRowDropdown::handleKey(int key) {
    int baseKey = key & 0xFFFF;
    if (!open_) {
        if (baseKey == OF_KEY_RETURN || baseKey == ' ' || baseKey == OF_KEY_RIGHT) {
            openDropdown();
            return true;
        }
        if (baseKey == 'p' || baseKey == 'P') {
            if (onPageNudge_) onPageNudge_();
            return true;
        }
        if (baseKey == 'm' || baseKey == 'M') {
            if (onModifierToggle_) onModifierToggle_();
            return true;
        }
        return false;
    }

    switch (baseKey) {
    case OF_KEY_RETURN:
        acceptHighlight();
        return true;
    case OF_KEY_ESC:
        closeDropdown();
        return true;
    case OF_KEY_UP:
        advanceHighlight(-1);
        return true;
    case OF_KEY_DOWN:
        advanceHighlight(1);
        return true;
    case OF_KEY_PAGE_UP:
        advanceHighlight(-5);
        return true;
    case OF_KEY_PAGE_DOWN:
        advanceHighlight(5);
        return true;
    default:
        handleTypeAhead(baseKey);
        return true;
    }
}

// ColumnNumericCell ---------------------------------------------------------

ColumnNumericCell::ColumnNumericCell(std::string id,
                                     std::string label,
                                     float value,
                                     float minimum,
                                     float maximum,
                                     float fineStep,
                                     float coarseStep)
    : id_(std::move(id))
    , label_(std::move(label))
    , value_(value)
    , min_(minimum)
    , max_(maximum)
    , fineStep_(fineStep)
    , coarseStep_(coarseStep) {}

void ColumnNumericCell::setValue(float value) {
    value_ = ofClamp(value, min_, max_);
}

void ColumnNumericCell::applyDelta(float delta) {
    float next = ofClamp(value_ + delta, min_, max_);
    if (std::abs(next - value_) > std::numeric_limits<float>::epsilon()) {
        value_ = next;
        pendingChanges_ = true;
        if (onCommit_) onCommit_(value_);
    }
}

void ColumnNumericCell::beginEdit() {
    editing_ = true;
    buffer_ = ofToString(value_, 3);
    pendingError_ = false;
    errorMessage_.clear();
}

void ColumnNumericCell::cancelEdit() {
    editing_ = false;
    buffer_.clear();
    pendingError_ = false;
    errorMessage_.clear();
}

void ColumnNumericCell::commitEdit() {
    std::string trimmed = ofTrim(buffer_);
    if (trimmed.empty()) {
        pendingError_ = true;
        errorMessage_ = "Value required";
        return;
    }
    double parsed = 0.0;
    try {
        parsed = std::stod(trimmed);
    } catch (...) {
        pendingError_ = true;
        errorMessage_ = "Invalid number";
        return;
    }
    value_ = ofClamp(static_cast<float>(parsed), min_, max_);
    editing_ = false;
    buffer_.clear();
    pendingError_ = false;
    errorMessage_.clear();
    pendingChanges_ = true;
    if (onCommit_) onCommit_(value_);
}

MenuController::EntryView ColumnNumericCell::toEntryView(bool selected) const {
    MenuController::EntryView entry;
    entry.id = id_;
    entry.label = label_;
    entry.description = editing_ ? buffer_ : formatNumeric(value_, units_);
    if (pendingError_) {
        entry.description += "  (" + errorMessage_ + ")";
    }
    entry.selectable = true;
    entry.selected = selected;
    entry.modifierCount = modifierCount_;
    entry.pendingChanges = pendingChanges_;
    return entry;
}

bool ColumnNumericCell::handleKey(int key) {
    int baseKey = key & 0xFFFF;
    bool shift = (key & MenuController::HOTKEY_MOD_SHIFT) != 0;
    if (editing_) {
        if (baseKey == OF_KEY_ESC) {
            cancelEdit();
            return true;
        }
        if (baseKey == OF_KEY_RETURN) {
            commitEdit();
            return true;
        }
        if (baseKey == OF_KEY_BACKSPACE) {
            if (!buffer_.empty()) buffer_.pop_back();
            return true;
        }
        if (baseKey >= 32 && baseKey <= 126) {
            buffer_.push_back(static_cast<char>(baseKey));
            return true;
        }
        return false;
    }

    switch (baseKey) {
    case OF_KEY_RETURN:
        beginEdit();
        return true;
    case '+':
    case '=':
        applyDelta(fineStep_);
        return true;
    case '-':
    case '_':
        applyDelta(-fineStep_);
        return true;
    case OF_KEY_UP:
        applyDelta(shift ? coarseStep_ : fineStep_);
        return true;
    case OF_KEY_DOWN:
        applyDelta(shift ? -coarseStep_ : -fineStep_);
        return true;
    default:
        return false;
    }
}

// ColumnTextCell ------------------------------------------------------------

ColumnTextCell::ColumnTextCell(std::string id, std::string label, std::string value)
    : id_(std::move(id))
    , label_(std::move(label))
    , value_(std::move(value)) {}

void ColumnTextCell::setValue(const std::string& value) {
    value_ = value;
}

void ColumnTextCell::beginEdit() {
    editing_ = true;
    buffer_ = value_;
}

void ColumnTextCell::cancelEdit() {
    editing_ = false;
    buffer_.clear();
}

void ColumnTextCell::commitEdit() {
    value_ = buffer_;
    editing_ = false;
    buffer_.clear();
    pendingChanges_ = true;
    if (onCommit_) onCommit_(value_);
}

MenuController::EntryView ColumnTextCell::toEntryView(bool selected) const {
    MenuController::EntryView entry;
    entry.id = id_;
    entry.label = label_;
    entry.description = editing_ ? buffer_ : value_;
    entry.selectable = true;
    entry.selected = selected;
    entry.pendingChanges = pendingChanges_;
    entry.modifierCount = modifierCount_;
    return entry;
}

bool ColumnTextCell::handleKey(int key) {
    int baseKey = key & 0xFFFF;
    if (editing_) {
        if (baseKey == OF_KEY_ESC) {
            cancelEdit();
            return true;
        }
        if (baseKey == OF_KEY_RETURN) {
            commitEdit();
            return true;
        }
        if (baseKey == OF_KEY_BACKSPACE) {
            if (!buffer_.empty()) buffer_.pop_back();
            return true;
        }
        if (baseKey >= 32 && baseKey <= 126) {
            if (buffer_.size() < 256) {
                buffer_.push_back(static_cast<char>(baseKey));
            }
            return true;
        }
        return false;
    }

    if (baseKey == OF_KEY_RETURN) {
        beginEdit();
        return true;
    }
    return false;
}
