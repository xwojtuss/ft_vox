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
		ChunkSystem(World& world, render::IRenderer& renderer);

		void bindEvents(Dispatcher& dispatcher) override;
	};
}
