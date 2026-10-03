#pragma once

#include <glm/vec3.hpp>

#include "ecs/system/DispatcherEvents.hpp"
#include "game/planet/Chunk.hpp"

namespace game::planet {
	struct ChunkRequestedEvent : public ecs::DispatchEvent {
		glm::ivec3 position;

		explicit ChunkRequestedEvent(const glm::ivec3 position) :
			DispatchEvent("ChunkRequestedEvent"), position(position) {
		}
	};

	struct ChunkLoadedEvent : public ecs::DispatchEvent {
		glm::ivec3 position;
		Chunk      chunk;

		ChunkLoadedEvent(const glm::ivec3 position, Chunk chunk) :
			DispatchEvent("ChunkLoadedEvent"), position(position), chunk(std::move(chunk)) {
		}
	};

	struct ChunkUnloadedEvent : public ecs::DispatchEvent {
		glm::ivec3 position;

		explicit ChunkUnloadedEvent(const glm::ivec3 position) :
			DispatchEvent("ChunkUnloadedEvent"), position(position) {
		}
	};

	struct ChunkChangedEvent : public ecs::DispatchEvent {
		glm::ivec3 position;
		glm::ivec3 blockPosition;
		Chunk      chunk;

		ChunkChangedEvent(const glm::ivec3 position, const glm::ivec3 blockPosition, Chunk chunk) :
			DispatchEvent("ChunkChangedEvent"), position(position), blockPosition(blockPosition),
			chunk(std::move(chunk)) {
		}
	};
}
