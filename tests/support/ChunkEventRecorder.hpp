#pragma once

#include <glm/vec3.hpp>
#include <utility>
#include <vector>

#include "ecs/system/Dispatcher.hpp"
#include "game/planet/ChunkEvents.hpp"

namespace test {
	class ChunkEventRecorder {
	public:
		std::vector<glm::ivec3>                                 requested;
		std::vector<std::pair<glm::ivec3, game::planet::Chunk>> loaded;
		std::vector<glm::ivec3>                                 unloaded;
		std::vector<game::planet::ChunkChangedEvent>            changed;

		void listenTo(ecs::Dispatcher& dispatcher) {
			dispatcher.subscribe(this, &ChunkEventRecorder::onRequested);
			dispatcher.subscribe(this, &ChunkEventRecorder::onLoaded);
			dispatcher.subscribe(this, &ChunkEventRecorder::onUnloaded);
			dispatcher.subscribe(this, &ChunkEventRecorder::onChanged);
		}

		void onRequested(const game::planet::ChunkRequestedEvent& event) {
			requested.push_back(event.position);
		}

		void onLoaded(const game::planet::ChunkLoadedEvent& event) {
			loaded.emplace_back(event.position, event.chunk);
		}

		void onUnloaded(const game::planet::ChunkUnloadedEvent& event) {
			unloaded.push_back(event.position);
		}

		void onChanged(const game::planet::ChunkChangedEvent& event) {
			changed.push_back(event);
		}
	};
}
