#pragma once

#include "../ASystem.hpp"
#include "../DispatcherEvents.hpp"
#include "../../../game/world/ChunkManager.hpp"

namespace ecs {
	class World;

	class ChunkSystem : public ASystem {
	private:
		game::world::ChunkManager m_chunkManager;

	public:
		ChunkSystem(World&                      world, render::IRenderer& renderer,
					glm::vec<3, unsigned short> renderDistance = scene::worldinfo::renderDistance);

		void bindEvents(Dispatcher& dispatcher) override;
	};
}
