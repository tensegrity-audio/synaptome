#pragma once
#include "ofMain.h"

#include "MenuController.h"

#include "ofConstants.h"
#include "ofUtils.h"

#include <functional>
#include <string>
#include <vector>

// Shared UI controls for Control Hub-style columns.

class ColumnRowDropdown {
public:
    struct Option {
        std::string value;
        std::string label;
    };

    using ChangeCallback = std::function<void(const std::string&)>;
    using SimpleCallback = std::function<void()>;

    ColumnRowDropdown(std::string id, std::string label);

    void setOptions(std::vector<Option> options);
    void setSelectedValue(const std::string& value);
    const std::string& selectedValue() const { return selectedValue_; }
    const std::vector<Option>& options() const { return options_; }
    int highlightedIndex() const { return highlightedIndex_; }

    void setModifierCount(int count) { modifierCount_ = count; }
    void setPendingChanges(bool pending) { pendingChanges_ = pending; }
    void setDescriptionSuffix(const std::string& suffix) { descriptionSuffix_ = suffix; }

    void onValueChanged(ChangeCallback cb) { onValueChanged_ = std::move(cb); }
    void onPageNudge(SimpleCallback cb) { onPageNudge_ = std::move(cb); }
    void onModifierToggle(SimpleCallback cb) { onModifierToggle_ = std::move(cb); }

    MenuController::EntryView toEntryView(bool selected) const;

    bool handleKey(int key);
    bool isOpen() const { return open_; }

private:
    std::string id_;
    std::string label_;
    std::vector<Option> options_;
    std::string selectedValue_;
    std::string descriptionSuffix_;
    int highlightedIndex_ = -1;
    bool open_ = false;
    int modifierCount_ = 0;
    bool pendingChanges_ = false;
    ChangeCallback onValueChanged_;
    SimpleCallback onPageNudge_;
    SimpleCallback onModifierToggle_;
    std::string typeBuffer_;
    uint64_t lastTypeTimeMs_ = 0;

    int selectedIndex() const;
    void openDropdown();
    void closeDropdown();
    void applyHighlight(int index);
    void acceptHighlight();
    void advanceHighlight(int delta);
    void handleTypeAhead(int baseKey);
};

class ColumnNumericCell {
public:
    using CommitCallback = std::function<void(float)>;

    ColumnNumericCell(std::string id,
                      std::string label,
                      float value,
                      float minimum,
                      float maximum,
                      float fineStep,
                      float coarseStep);

    void setValue(float value);
    float value() const { return value_; }

    void onCommit(CommitCallback cb) { onCommit_ = std::move(cb); }
    void setUnitLabel(const std::string& unit) { units_ = unit; }
    void setModifierCount(int count) { modifierCount_ = count; }
    void setPendingChanges(bool pending) { pendingChanges_ = pending; }

    MenuController::EntryView toEntryView(bool selected) const;
    bool handleKey(int key);

private:
    std::string id_;
    std::string label_;
    float value_ = 0.0f;
    float min_ = 0.0f;
    float max_ = 1.0f;
    float fineStep_ = 0.01f;
    float coarseStep_ = 0.1f;
    std::string units_;
    bool editing_ = false;
    std::string buffer_;
    bool pendingError_ = false;
    std::string errorMessage_;
    int modifierCount_ = 0;
    bool pendingChanges_ = false;
    CommitCallback onCommit_;

    void applyDelta(float delta);
    void beginEdit();
    void cancelEdit();
    void commitEdit();
};

class ColumnTextCell {
public:
    using CommitCallback = std::function<void(const std::string&)>;

    ColumnTextCell(std::string id, std::string label, std::string value);

    void setValue(const std::string& value);
    const std::string& value() const { return value_; }

    void onCommit(CommitCallback cb) { onCommit_ = std::move(cb); }
    void setModifierCount(int count) { modifierCount_ = count; }
    void setPendingChanges(bool pending) { pendingChanges_ = pending; }

    MenuController::EntryView toEntryView(bool selected) const;
    bool handleKey(int key);

private:
    std::string id_;
    std::string label_;
    std::string value_;
    bool editing_ = false;
    std::string buffer_;
    bool pendingChanges_ = false;
    int modifierCount_ = 0;
    CommitCallback onCommit_;

    void beginEdit();
    void cancelEdit();
    void commitEdit();
};
