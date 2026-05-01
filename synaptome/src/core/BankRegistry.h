#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ofJson.h"

// Registry for MIDI/OSC bank definitions with inheritance support.
class BankRegistry {
public:
    enum class Scope {
        kGlobal,
        kScene,
        kLayer
    };

    struct Control {
        std::string id;            // Unique within a bank
        std::string label;         // UI label
        std::string targetId;      // Parameter or modifier target id
        std::string modifierId;    // Optional modifier id
        std::string description;   // Optional help text
        bool softTakeover = true;  // Whether takeover gating applies
    };

    struct Definition {
        std::string id;
        Scope scope = Scope::kScene;
        std::string label;
        std::string parentId;  // Optional inheritance parent
        std::vector<Control> controls;
    };

    using DefinitionList = std::vector<Definition>;
    using LayerDefinitionMap = std::unordered_map<std::string, DefinitionList>;

    void setGlobalBanks(DefinitionList defs) {
        normalizeDefs(defs, Scope::kGlobal);
        globalBanks_ = std::move(defs);
    }

    void setSceneBanks(DefinitionList defs) {
        normalizeDefs(defs, Scope::kScene);
        sceneBanks_ = std::move(defs);
    }

    void setLayerBanks(const std::string& layerId, DefinitionList defs) {
        if (layerId.empty()) {
            return;
        }
        normalizeDefs(defs, Scope::kLayer);
        if (defs.empty()) {
            layerBanks_.erase(layerId);
        } else {
            layerBanks_[layerId] = std::move(defs);
        }
    }

    void clearSceneBanks() { sceneBanks_.clear(); }
    void clearLayerBanks() { layerBanks_.clear(); }

    DefinitionList globalBanks() const { return globalBanks_; }
    DefinitionList sceneBanks() const { return sceneBanks_; }
    LayerDefinitionMap layerBanks() const { return layerBanks_; }

    bool hasBank(const std::string& bankId, const std::string& layerId = std::string()) const {
        return findBankInternal(bankId, layerId) != nullptr;
    }

    std::optional<Definition> getBank(const std::string& bankId, const std::string& layerId = std::string()) const {
        const Definition* def = findBankInternal(bankId, layerId);
        if (!def) {
            return std::nullopt;
        }
        return *def;
    }

    Definition resolveBank(const std::string& bankId, const std::string& layerId = std::string()) const {
        const Definition* base = findBankInternal(bankId, layerId);
        if (!base) {
            return Definition{};
        }
        Definition resolved = *base;
        std::vector<Control> mergedControls = resolved.controls;
        std::unordered_set<std::string> visited;
        visited.insert(resolved.id);
        std::string parentId = resolved.parentId;
        std::string lookupLayer = layerId;
        while (!parentId.empty()) {
            if (!visited.insert(parentId).second) {
                break;  // cycle detected
            }
            const Definition* parent = findBankInternal(parentId, lookupLayer);
            if (!parent && !lookupLayer.empty()) {
                parent = findBankInternal(parentId, std::string());
            }
            if (!parent) {
                break;
            }
            mergedControls = mergeControls(parent->controls, mergedControls);
            parentId = parent->parentId;
            if (parent->scope != Scope::kLayer) {
                lookupLayer.clear();  // climb scene/global once we leave layer scope
            }
        }
        resolved.controls = std::move(mergedControls);
        return resolved;
    }

    std::string firstBankId() const {
        if (!sceneBanks_.empty()) {
            return sceneBanks_.front().id;
        }
        if (!globalBanks_.empty()) {
            return globalBanks_.front().id;
        }
        for (const auto& kv : layerBanks_) {
            if (!kv.second.empty()) {
                return kv.second.front().id;
            }
        }
        return std::string();
    }

    static Definition definitionFromJson(const ofJson& node, Scope scope) {
        Definition def;
        def.scope = scope;
        if (node.contains("id") && node["id"].is_string()) {
            def.id = node["id"].get<std::string>();
        }
        if (node.contains("label") && node["label"].is_string()) {
            def.label = node["label"].get<std::string>();
        }
        if (node.contains("parent") && node["parent"].is_string()) {
            def.parentId = node["parent"].get<std::string>();
        }
        if (node.contains("controls") && node["controls"].is_array()) {
            for (const auto& ctrlNode : node["controls"]) {
                if (!ctrlNode.is_object()) {
                    continue;
                }
                Control ctrl;
                if (ctrlNode.contains("id") && ctrlNode["id"].is_string()) {
                    ctrl.id = ctrlNode["id"].get<std::string>();
                }
                if (ctrlNode.contains("label") && ctrlNode["label"].is_string()) {
                    ctrl.label = ctrlNode["label"].get<std::string>();
                }
                if (ctrlNode.contains("target") && ctrlNode["target"].is_string()) {
                    ctrl.targetId = ctrlNode["target"].get<std::string>();
                }
                if (ctrlNode.contains("modifier") && ctrlNode["modifier"].is_string()) {
                    ctrl.modifierId = ctrlNode["modifier"].get<std::string>();
                }
                if (ctrlNode.contains("description") && ctrlNode["description"].is_string()) {
                    ctrl.description = ctrlNode["description"].get<std::string>();
                }
                if (ctrlNode.contains("softTakeover") && ctrlNode["softTakeover"].is_boolean()) {
                    ctrl.softTakeover = ctrlNode["softTakeover"].get<bool>();
                }
                if (!ctrl.id.empty()) {
                    def.controls.push_back(std::move(ctrl));
                }
            }
        }
        return def;
    }

    static ofJson definitionToJson(const Definition& def) {
        ofJson node = ofJson::object();
        node["id"] = def.id;
        if (!def.label.empty()) node["label"] = def.label;
        if (!def.parentId.empty()) node["parent"] = def.parentId;
        if (!def.controls.empty()) {
            ofJson controls = ofJson::array();
            for (const auto& ctrl : def.controls) {
                ofJson ctrlNode = ofJson::object();
                ctrlNode["id"] = ctrl.id;
                if (!ctrl.label.empty()) ctrlNode["label"] = ctrl.label;
                if (!ctrl.targetId.empty()) ctrlNode["target"] = ctrl.targetId;
                if (!ctrl.modifierId.empty()) ctrlNode["modifier"] = ctrl.modifierId;
                if (!ctrl.description.empty()) ctrlNode["description"] = ctrl.description;
                if (!ctrl.softTakeover) ctrlNode["softTakeover"] = ctrl.softTakeover;
                controls.push_back(std::move(ctrlNode));
            }
            node["controls"] = std::move(controls);
        }
        return node;
    }

    static DefinitionList definitionsFromJson(const ofJson& arrayNode, Scope scope) {
        DefinitionList defs;
        if (!arrayNode.is_array()) {
            return defs;
        }
        for (const auto& item : arrayNode) {
            if (!item.is_object()) {
                continue;
            }
            Definition def = definitionFromJson(item, scope);
            if (!def.id.empty()) {
                defs.push_back(std::move(def));
            }
        }
        return defs;
    }

    static ofJson definitionsToJson(const DefinitionList& defs) {
        ofJson result = ofJson::array();
        for (const auto& def : defs) {
            result.push_back(definitionToJson(def));
        }
        return result;
    }

private:
    DefinitionList globalBanks_;
    DefinitionList sceneBanks_;
    LayerDefinitionMap layerBanks_;

    static void normalizeDefs(DefinitionList& defs, Scope scope) {
        for (auto& def : defs) {
            def.scope = scope;
        }
    }

    const Definition* findBankInternal(const std::string& bankId, const std::string& layerId) const {
        if (!layerId.empty()) {
            auto itLayer = layerBanks_.find(layerId);
            if (itLayer != layerBanks_.end()) {
                const Definition* def = findInList(itLayer->second, bankId);
                if (def) return def;
            }
        }
        const Definition* scene = findInList(sceneBanks_, bankId);
        if (scene) return scene;
        return findInList(globalBanks_, bankId);
    }

    static const Definition* findInList(const DefinitionList& defs, const std::string& id) {
        auto it = std::find_if(defs.begin(), defs.end(), [&](const Definition& def) {
            return def.id == id;
        });
        return it != defs.end() ? &(*it) : nullptr;
    }

    static std::vector<Control> mergeControls(const std::vector<Control>& parent,
                                              const std::vector<Control>& child) {
        std::vector<Control> merged = parent;
        for (const auto& ctrl : child) {
            auto it = std::find_if(merged.begin(), merged.end(), [&](const Control& existing) {
                return existing.id == ctrl.id;
            });
            if (it != merged.end()) {
                *it = ctrl;
            } else {
                merged.push_back(ctrl);
            }
        }
        return merged;
    }
};
