#include "ecs/system/types/ChunkRenderSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/ViewerLookup.hpp"

using namespace ecs;

ChunkRenderSystem::ChunkRenderSystem(Registry& registry, render::IRenderer& renderer, concurrency::IThreadPool& pool,
									 profiling::ClientStats&                   stats,
									 const render::chunks::ChunkRenderSettings settings) :
	m_chunkRenderer(registry.getBlockDatas(), registry, renderer, pool, stats, settings) {
}

void ChunkRenderSystem::onRender([[maybe_unused]] const RenderEvent& event) {
	m_chunkRenderer.setViewer(findViewer(*m_registry).value_or(game::planet::Viewer{}));
	m_chunkRenderer.update();
}

void ChunkRenderSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe(&m_chunkRenderer, &render::chunks::ChunkRenderer::onChunkRequested);
	dispatcher.subscribe(&m_chunkRenderer, &render::chunks::ChunkRenderer::onChunkLoaded);
	dispatcher.subscribe(&m_chunkRenderer, &render::chunks::ChunkRenderer::onChunkUnloaded);
	dispatcher.subscribe(&m_chunkRenderer, &render::chunks::ChunkRenderer::onChunkChanged);
	dispatcher.subscribe(this, &ChunkRenderSystem::onRender);
}
