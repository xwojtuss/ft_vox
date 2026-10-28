#include "ecs/system/types/ChunkStreamSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/ViewerLookup.hpp"

using namespace ecs;

ChunkStreamSystem::ChunkStreamSystem(Registry& registry, concurrency::IThreadPool& pool, profiling::ServerStats& stats,
									 const game::Seed seed, const game::planet::ChunkStreamSettings settings) :
	m_streamer(registry.getSystemManager().getDispatcher(), pool, stats, seed, settings) {
}

void ChunkStreamSystem::onSimulate([[maybe_unused]] const SimulateEvent& event) {
	m_streamer.setViewer(findViewer(*m_registry).value_or(game::planet::Viewer{}));
	m_streamer.update();
}

void ChunkStreamSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe(this, &ChunkStreamSystem::onSimulate);
}
