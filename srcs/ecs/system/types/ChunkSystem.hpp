#pragma once

#include "ecs/system/ASystem.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "game/planet/ChunkManager.hpp"

namespace ecs {
	class Registry;

	class ChunkSystem : public ASystem {
	private:
		game::planet::ChunkManager m_chunkManager;

	public:
		ChunkSystem(Registry& registry, render::IRenderer& renderer,
					glm::vec<3, unsigned short> renderDistance = scene::planetinfo::renderDistance);

		void bindEvents(Dispatcher& dispatcher) override;
	};
}
