#include "ecs/system/types/ChunkSystem.hpp"
#include "ecs/World.hpp"

using namespace ecs;

ChunkSystem::ChunkSystem(World& world, render::IRenderer& renderer, const glm::vec<3, unsigned short> renderDistance) :
	m_chunkManager(world.getBlockDatas(), world, renderer, renderDistance) {
}

void ChunkSystem::bindEvents(Dispatcher& dispatcher) {
	(void)dispatcher;
}
