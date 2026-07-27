#include "BuiltinElements.h"
#include "SignalBloomRegistration.h"

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

namespace synaptome::runtime {

void registerBuiltinElements(LayerFactory& elementTypes) {
    using synaptome::element::ElementDescriptor;
    using synaptome::element::ElementKind;

    elementTypes.registerType(
        ElementDescriptor{"grid", ElementKind::Visual, {}},
        []() { return std::make_unique<GridLayer>(); });
    elementTypes.registerType(
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
    elementTypes.registerType(
        ElementDescriptor{"audioWaveform", ElementKind::Visual, {}},
        []() { return std::make_unique<AudioWaveformLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"oscilloscope", ElementKind::Visual, {}},
        []() { return std::make_unique<OscilloscopeLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"perlin", ElementKind::Visual, {}},
        []() { return std::make_unique<PerlinNoiseLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"stlModel", ElementKind::Visual, {}},
        []() { return std::make_unique<StlModelLayer>(); });
    elementTypes.registerType(
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
    elementTypes.registerType(
        ElementDescriptor{"reactionDiffusion", ElementKind::Visual, {}},
        []() { return std::make_unique<ReactionDiffusionLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"lenia", ElementKind::Visual, {}},
        []() { return std::make_unique<LeniaLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"excitableMedia", ElementKind::Visual, {}},
        []() { return std::make_unique<ExcitableMediaLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"agentField", ElementKind::Visual, {}},
        []() { return std::make_unique<AgentFieldLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"circuitTrace", ElementKind::Visual, {}},
        []() { return std::make_unique<CircuitTraceLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"flocking", ElementKind::Visual, {}},
        []() { return std::make_unique<FlockingLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"flowField", ElementKind::Visual, {}},
        []() { return std::make_unique<FlowFieldLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"riverFormation", ElementKind::Visual, {}},
        []() { return std::make_unique<RiverFormationLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"arcticAuroraScene", ElementKind::Visual, {}},
        []() { return std::make_unique<ArcticAuroraSceneLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"mountainIsland", ElementKind::Visual, {}},
        []() { return std::make_unique<MountainIslandLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"solarSystem", ElementKind::Visual, {}},
        []() { return std::make_unique<SolarSystemLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"cosmosFormation", ElementKind::Visual, {}},
        []() { return std::make_unique<CosmosFormationLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"media.webcam", ElementKind::Visual, {}},
        []() { return std::make_unique<VideoGrabberLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"media.clip", ElementKind::Visual, {}},
        []() { return std::make_unique<VideoClipLayer>(); });
    elementTypes.registerType(
        ElementDescriptor{"text", ElementKind::Visual, {}},
        []() { return std::make_unique<TextLayer>(); });
    registerSignalBloomElement(elementTypes);
}

} // namespace synaptome::runtime
