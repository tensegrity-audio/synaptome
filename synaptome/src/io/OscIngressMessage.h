#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// Transport-neutral OSC observation envelope. Routing remains deliberately
// narrower than observation: every well-formed message can be inspected, while
// the existing parameter router consumes only a finite single numeric value.
enum class OscIngressAtomType {
    Int32,
    Int64,
    Float32,
    Float64,
    String,
    Symbol,
    Bool,
    Blob,
    Char,
    Midi,
    Timetag,
    Rgba,
    Nil,
    Impulse,
    Unsupported
};

struct OscIngressAtom {
    OscIngressAtomType type = OscIngressAtomType::Unsupported;
    double numericValue = 0.0;
    std::int64_t signedValue = 0;
    std::string textValue;
    std::uint64_t unsignedValue = 0;
    std::size_t byteCount = 0;

    static OscIngressAtom numeric(OscIngressAtomType type, double value) {
        OscIngressAtom atom;
        atom.type = type;
        atom.numericValue = value;
        return atom;
    }

    static OscIngressAtom integer(OscIngressAtomType type, std::int64_t value) {
        OscIngressAtom atom;
        atom.type = type;
        atom.signedValue = value;
        return atom;
    }

    static OscIngressAtom text(OscIngressAtomType type, std::string value) {
        OscIngressAtom atom;
        atom.type = type;
        atom.textValue = std::move(value);
        return atom;
    }
};

struct OscIngressMessage {
    std::string rawAddress;
    std::string canonicalAddress;
    std::string typeTags;
    std::vector<OscIngressAtom> arguments;
    std::string transport;
    std::string endpoint;
    std::uint64_t timestampMs = 0;
    std::size_t rawSize = 0;
    bool meshNamespaceAlias = false;
    bool meshRouteAliasApplied = false;
    bool duplicateSuppressed = false;

    bool finiteNumericScalar(float& value) const {
        if (arguments.size() != 1) {
            return false;
        }
        const auto type = arguments.front().type;
        if (type != OscIngressAtomType::Int32
            && type != OscIngressAtomType::Int64
            && type != OscIngressAtomType::Float32
            && type != OscIngressAtomType::Float64) {
            return false;
        }
        const double candidate =
            type == OscIngressAtomType::Int32 || type == OscIngressAtomType::Int64
            ? static_cast<double>(arguments.front().signedValue)
            : arguments.front().numericValue;
        if (!std::isfinite(candidate)) {
            return false;
        }
        value = static_cast<float>(candidate);
        return std::isfinite(value);
    }

    std::string payloadSummary(std::size_t maxAtoms = 4,
                               std::size_t maxTextLength = 80) const {
        if (arguments.empty()) {
            return "(no arguments)";
        }
        std::ostringstream out;
        const std::size_t count = arguments.size() < maxAtoms
            ? arguments.size()
            : maxAtoms;
        for (std::size_t i = 0; i < count; ++i) {
            if (i != 0) {
                out << ", ";
            }
            const auto& atom = arguments[i];
            switch (atom.type) {
                case OscIngressAtomType::Int32:
                case OscIngressAtomType::Int64:
                    out << atom.signedValue;
                    break;
                case OscIngressAtomType::Float32:
                case OscIngressAtomType::Float64:
                    out << std::setprecision(7) << atom.numericValue;
                    break;
                case OscIngressAtomType::String:
                case OscIngressAtomType::Symbol: {
                    std::string value = atom.textValue;
                    if (value.size() > maxTextLength) {
                        value.resize(maxTextLength);
                        value += "...";
                    }
                    out << '"' << value << '"';
                    break;
                }
                case OscIngressAtomType::Bool:
                    out << (atom.numericValue != 0.0 ? "true" : "false");
                    break;
                case OscIngressAtomType::Blob:
                    out << "<blob:" << atom.byteCount << " bytes>";
                    break;
                case OscIngressAtomType::Char:
                    out << "'" << atom.textValue << "'";
                    break;
                case OscIngressAtomType::Midi:
                    out << "<midi:0x" << std::hex << atom.unsignedValue << std::dec << ">";
                    break;
                case OscIngressAtomType::Timetag:
                    out << "<timetag:" << atom.unsignedValue << ">";
                    break;
                case OscIngressAtomType::Rgba:
                    out << "<rgba:0x" << std::hex << atom.unsignedValue << std::dec << ">";
                    break;
                case OscIngressAtomType::Nil:
                    out << "nil";
                    break;
                case OscIngressAtomType::Impulse:
                    out << "impulse";
                    break;
                case OscIngressAtomType::Unsupported:
                    out << "<unsupported>";
                    break;
            }
        }
        if (arguments.size() > count) {
            out << ", ... (" << arguments.size() << " args)";
        }
        return out.str();
    }
};

namespace OscIngressCompatibility {

inline bool startsWith(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size()
        && value.compare(0, prefix.size(), prefix) == 0;
}

inline bool endsWith(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size()
        && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Synaptome-owned compatibility for the fixture-backed
// synaptome-mesh-osc v0.1.0 producer contract.
inline void normalizeSynaptomeMeshV1(OscIngressMessage& message) {
    static const std::string meshPrefix = "/synaptome_mesh";
    message.canonicalAddress = message.rawAddress;
    if (startsWith(message.rawAddress, meshPrefix + "/")) {
        message.meshNamespaceAlias = true;
        message.canonicalAddress = message.rawAddress.substr(meshPrefix.size());
    }

    static const std::string hrPrefix = "/sensor/hr/";
    static const std::string meshHeartMetric = "/heart-bpm";
    if (startsWith(message.canonicalAddress, hrPrefix)
        && endsWith(message.canonicalAddress, meshHeartMetric)) {
        const std::size_t identityStart = hrPrefix.size();
        const std::size_t identityLength =
            message.canonicalAddress.size() - identityStart - meshHeartMetric.size();
        if (identityLength > 0
            && message.canonicalAddress.find('/', identityStart) ==
                message.canonicalAddress.size() - meshHeartMetric.size()) {
            message.canonicalAddress.replace(
                message.canonicalAddress.size() - meshHeartMetric.size(),
                meshHeartMetric.size(),
                "/bpm");
            message.meshRouteAliasApplied = true;
        }
    }
}

class MeshDualEmissionDeduper {
public:
    explicit MeshDualEmissionDeduper(std::uint64_t windowMs = 25)
        : windowMs_(windowMs) {}

    bool shouldSuppress(OscIngressMessage& message) {
        if (message.canonicalAddress.empty()) {
            return false;
        }
        const std::string key =
            message.transport + "\n"
            + message.endpoint + "\n"
            + message.canonicalAddress + "\n"
            + message.typeTags + "\n"
            + message.payloadSummary();
        auto found = pending_.find(key);
        if (found != pending_.end()) {
            const bool withinWindow =
                message.timestampMs >= found->second.timestampMs
                && message.timestampMs - found->second.timestampMs <= windowMs_;
            const bool oppositeNamespace =
                message.meshNamespaceAlias != found->second.meshNamespaceAlias;
            if (withinWindow && oppositeNamespace) {
                message.duplicateSuppressed = true;
                pending_.erase(found);
                return true;
            }
        }

        pending_[key] = Pending {
            message.timestampMs,
            message.meshNamespaceAlias
        };
        if (pending_.size() > 256) {
            prune(message.timestampMs);
        }
        return false;
    }

    void clear() {
        pending_.clear();
    }

private:
    struct Pending {
        std::uint64_t timestampMs = 0;
        bool meshNamespaceAlias = false;
    };

    std::uint64_t windowMs_ = 25;
    std::unordered_map<std::string, Pending> pending_;

    void prune(std::uint64_t nowMs) {
        for (auto it = pending_.begin(); it != pending_.end();) {
            if (nowMs >= it->second.timestampMs
                && nowMs - it->second.timestampMs > windowMs_) {
                it = pending_.erase(it);
            } else {
                ++it;
            }
        }
        if (pending_.size() > 256) {
            pending_.clear();
        }
    }
};

} // namespace OscIngressCompatibility
