#include "ecs/system/types/ChunkSystem.hpp"
#include "ecs/Registry.hpp"

using namespace ecs;

ChunkSystem::ChunkSystem(Registry& registry, render::IRenderer& renderer, profiling::ServerStats& stats,
						 const game::Seed seed, const glm::vec<3, unsigned short> renderDistance) :
	m_chunkManager(registry.getBlockDatas(), registry, renderer, stats, seed, renderDistance) {
}

void ChunkSystem::bindEvents(Dispatcher& dispatcher) {
	(void)dispatcher;
}
