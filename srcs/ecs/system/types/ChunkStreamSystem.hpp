#pragma once

#include "ecs/system/ASystem.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "game/planet/ChunkStreamer.hpp"

namespace ecs {
	class Registry;

	class ChunkStreamSystem : public ASystem {
	private:
		game::planet::ChunkStreamer m_streamer;

	public:
		ChunkStreamSystem(Registry& registry, concurrency::IThreadPool& pool, profiling::ServerStats& stats,
						  game::Seed seed, game::planet::ChunkStreamSettings settings = {});

		void onSimulate(const SimulateEvent& event);
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
