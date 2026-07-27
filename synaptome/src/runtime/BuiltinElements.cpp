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

#include <memory>

namespace synaptome::runtime {

void registerBuiltinElements(LayerFactory& elementTypes) {
    elementTypes.registerType("grid", []() { return std::make_unique<GridLayer>(); });
    elementTypes.registerType("geodesic", []() { return std::make_unique<GeodesicLayer>(); });
    elementTypes.registerType("audioWaveform", []() { return std::make_unique<AudioWaveformLayer>(); });
    elementTypes.registerType("oscilloscope", []() { return std::make_unique<OscilloscopeLayer>(); });
    elementTypes.registerType("perlin", []() { return std::make_unique<PerlinNoiseLayer>(); });
    elementTypes.registerType("stlModel", []() { return std::make_unique<StlModelLayer>(); });
    elementTypes.registerType("gameOfLife", []() { return std::make_unique<GameOfLifeLayer>(); });
    elementTypes.registerType("reactionDiffusion", []() { return std::make_unique<ReactionDiffusionLayer>(); });
    elementTypes.registerType("lenia", []() { return std::make_unique<LeniaLayer>(); });
    elementTypes.registerType("excitableMedia", []() { return std::make_unique<ExcitableMediaLayer>(); });
    elementTypes.registerType("agentField", []() { return std::make_unique<AgentFieldLayer>(); });
    elementTypes.registerType("circuitTrace", []() { return std::make_unique<CircuitTraceLayer>(); });
    elementTypes.registerType("flocking", []() { return std::make_unique<FlockingLayer>(); });
    elementTypes.registerType("flowField", []() { return std::make_unique<FlowFieldLayer>(); });
    elementTypes.registerType("riverFormation", []() { return std::make_unique<RiverFormationLayer>(); });
    elementTypes.registerType("arcticAuroraScene", []() { return std::make_unique<ArcticAuroraSceneLayer>(); });
    elementTypes.registerType("mountainIsland", []() { return std::make_unique<MountainIslandLayer>(); });
    elementTypes.registerType("solarSystem", []() { return std::make_unique<SolarSystemLayer>(); });
    elementTypes.registerType("cosmosFormation", []() { return std::make_unique<CosmosFormationLayer>(); });
    elementTypes.registerType("media.webcam", []() { return std::make_unique<VideoGrabberLayer>(); });
    elementTypes.registerType("media.clip", []() { return std::make_unique<VideoClipLayer>(); });
    elementTypes.registerType("text", []() { return std::make_unique<TextLayer>(); });
    registerSignalBloomElement(elementTypes);
}

} // namespace synaptome::runtime
