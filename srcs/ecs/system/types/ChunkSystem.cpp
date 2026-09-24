#include "ChunkSystem.hpp"
#include "../../World.hpp"

using namespace ecs;

ChunkSystem::ChunkSystem(World& world, render::IRenderer& renderer) : ASystem(Dependencies()),
																	m_chunkManager(
																		world.getBlockDatas(), world, renderer) {
}

void ChunkSystem::bindEvents(Dispatcher& dispatcher) {
	(void)dispatcher;
}
