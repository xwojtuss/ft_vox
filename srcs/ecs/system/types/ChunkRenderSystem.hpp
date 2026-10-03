#pragma once

#include "ecs/system/ASystem.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "render/chunks/ChunkRenderer.hpp"

namespace ecs {
	class Registry;

	class ChunkRenderSystem : public ASystem {
	private:
		render::chunks::ChunkRenderer m_chunkRenderer;

	public:
		ChunkRenderSystem(Registry& registry, render::IRenderer& renderer, concurrency::IThreadPool& pool,
						  profiling::ClientStats& stats, render::chunks::ChunkRenderSettings settings = {});

		void onRender(const RenderEvent& event);
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
