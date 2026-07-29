#include "BuiltinElements.h"
#include "BuiltinElementParameterContracts.h"
#include "GeneratedElementPackageRegistrations.h"

#include "../visuals/AgentFieldLayer.h"
#include "../visuals/ArcticAuroraSceneLayer.h"
#include "../visuals/AudioWaveformLayer.h"
#include "../visuals/CircuitTraceLayer.h"
#include "../visuals/CosmosFormationLayer.h"
#include "../visuals/ExcitableMediaLayer.h"
#include "../visuals/FlockingLayer.h"
#include "../visuals/FlowFieldLayer.h"
#include "../visuals/GameOfLifeLayer.h"
#include "../visuals/GeodesicLayer.h"
#include "../visuals/GridLayer.h"
#include "../visuals/LayerFactory.h"
#include "../visuals/LeniaLayer.h"
#include "../visuals/MountainIslandLayer.h"
#include "../visuals/OscilloscopeLayer.h"
#include "../visuals/PerlinNoiseLayer.h"
#include "../visuals/ReactionDiffusionLayer.h"
#include "../visuals/RiverFormationLayer.h"
#include "../visuals/SolarSystemLayer.h"
#include "../visuals/StlModelLayer.h"
#include "../visuals/TextLayer.h"
#include "../visuals/VideoClipLayer.h"
#include "../visuals/VideoGrabberLayer.h"

#include <synaptome/element/ElementDescriptor.h>

#include <memory>
#include <string>
#include <utility>

namespace synaptome::runtime {

void registerBuiltinElements(LayerFactory& elementTypes) {
    using synaptome::element::ElementDescriptor;
    using synaptome::element::ElementKind;
    using synaptome::element::ElementTypeContract;

    const auto registerBuiltin =
        [&](ElementDescriptor descriptor,
            LayerFactory::Creator creator) {
        const std::string typeId = descriptor.typeId;
        elementTypes.registerType(
            ElementTypeContract{
                std::move(descriptor),
                builtinElementParameterDeclarations(typeId),
            },
            std::move(creator),
            LayerFactory::ParameterBindingMode::
                LegacySetupAdapter);
    };

    registerBuiltin(
        ElementDescriptor{"grid", ElementKind::Visual, {}},
        []() { return std::make_unique<GridLayer>(); });
    registerBuiltin(
        ElementDescriptor{
            "geodesic",
            ElementKind::Visual,
            {
                {
                    "subdivision.increment",
                    "Increase Subdivision",
                    "geometry",
                    "Increase geodesic subdivision by one, up to the current maximum.",
                },
                {
                    "subdivision.decrement",
                    "Decrease Subdivision",
                    "geometry",
                    "Decrease geodesic subdivision by one, down to the current minimum.",
                },
            },
        },
        []() { return std::make_unique<GeodesicLayer>(); });
    registerBuiltin(
        ElementDescriptor{"audioWaveform", ElementKind::Visual, {}},
        []() { return std::make_unique<AudioWaveformLayer>(); });
    registerBuiltin(
        ElementDescriptor{"oscilloscope", ElementKind::Visual, {}},
        []() { return std::make_unique<OscilloscopeLayer>(); });
    registerBuiltin(
        ElementDescriptor{"perlin", ElementKind::Visual, {}},
        []() { return std::make_unique<PerlinNoiseLayer>(); });
    registerBuiltin(
        ElementDescriptor{"stlModel", ElementKind::Visual, {}},
        []() { return std::make_unique<StlModelLayer>(); });
    registerBuiltin(
        ElementDescriptor{
            "gameOfLife",
            ElementKind::Visual,
            {
                {
                    "simulation.randomize",
                    "Randomize Simulation",
                    "simulation",
                    "Immediately randomize the board using the current density.",
                },
            },
        },
        []() { return std::make_unique<GameOfLifeLayer>(); });
    registerBuiltin(
        ElementDescriptor{"reactionDiffusion", ElementKind::Visual, {}},
        []() { return std::make_unique<ReactionDiffusionLayer>(); });
    registerBuiltin(
        ElementDescriptor{"lenia", ElementKind::Visual, {}},
        []() { return std::make_unique<LeniaLayer>(); });
    registerBuiltin(
        ElementDescriptor{"excitableMedia", ElementKind::Visual, {}},
        []() { return std::make_unique<ExcitableMediaLayer>(); });
    registerBuiltin(
        ElementDescriptor{"agentField", ElementKind::Visual, {}},
        []() { return std::make_unique<AgentFieldLayer>(); });
    registerBuiltin(
        ElementDescriptor{"circuitTrace", ElementKind::Visual, {}},
        []() { return std::make_unique<CircuitTraceLayer>(); });
    registerBuiltin(
        ElementDescriptor{"flocking", ElementKind::Visual, {}},
        []() { return std::make_unique<FlockingLayer>(); });
    registerBuiltin(
        ElementDescriptor{"flowField", ElementKind::Visual, {}},
        []() { return std::make_unique<FlowFieldLayer>(); });
    registerBuiltin(
        ElementDescriptor{"riverFormation", ElementKind::Visual, {}},
        []() { return std::make_unique<RiverFormationLayer>(); });
    registerBuiltin(
        ElementDescriptor{"arcticAuroraScene", ElementKind::Visual, {}},
        []() { return std::make_unique<ArcticAuroraSceneLayer>(); });
    registerBuiltin(
        ElementDescriptor{"mountainIsland", ElementKind::Visual, {}},
        []() { return std::make_unique<MountainIslandLayer>(); });
    registerBuiltin(
        ElementDescriptor{"solarSystem", ElementKind::Visual, {}},
        []() { return std::make_unique<SolarSystemLayer>(); });
    registerBuiltin(
        ElementDescriptor{"cosmosFormation", ElementKind::Visual, {}},
        []() { return std::make_unique<CosmosFormationLayer>(); });
    registerBuiltin(
        ElementDescriptor{"media.webcam", ElementKind::Visual, {}},
        []() { return std::make_unique<VideoGrabberLayer>(); });
    registerBuiltin(
        ElementDescriptor{"media.clip", ElementKind::Visual, {}},
        []() { return std::make_unique<VideoClipLayer>(); });
    registerBuiltin(
        ElementDescriptor{"text", ElementKind::Visual, {}},
        []() { return std::make_unique<TextLayer>(); });
    registerGeneratedElementPackages(elementTypes);
}

} // namespace synaptome::runtime
