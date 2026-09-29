#include "ecs/system/types/ChunkSystem.hpp"
#include "ecs/Registry.hpp"

using namespace ecs;

ChunkSystem::ChunkSystem(Registry& registry, render::IRenderer& renderer,
						 const glm::vec<3, unsigned short> renderDistance) :
	m_chunkManager(registry.getBlockDatas(), registry, renderer, renderDistance) {
}

void ChunkSystem::bindEvents(Dispatcher& dispatcher) {
	(void)dispatcher;
}
