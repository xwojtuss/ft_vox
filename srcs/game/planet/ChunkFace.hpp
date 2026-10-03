#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/vec3.hpp>
#include <utility>

#include "game/planet/ChunkSize.hpp"
#include "scene/PlanetInfo.hpp"

namespace game::planet {
	enum class ChunkFace : std::uint8_t { PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ };

	constexpr std::size_t chunkFaceCount = 6;

	/** The step from a chunk to its neighbour on each face, indexed by ChunkFace. */
	constexpr std::array chunkFaceOffsets = {
		glm::ivec3(scene::planetinfo::right),    glm::ivec3(scene::planetinfo::left),
		glm::ivec3(scene::planetinfo::up),       glm::ivec3(scene::planetinfo::down),
		glm::ivec3(scene::planetinfo::backward), glm::ivec3(scene::planetinfo::forward),
	};

	[[nodiscard]] constexpr ChunkFace opposite(const ChunkFace face) {
		return static_cast<ChunkFace>(std::to_underlying(face) ^ 1U);
	}

	[[nodiscard]] inline bool isBlockOnChunkFace(const glm::ivec3 blockPosition, const ChunkFace face) {
		const auto index = static_cast<std::size_t>(std::to_underlying(face));
		const auto axis  = static_cast<glm::length_t>(index / 2);
		return blockPosition[axis] == ((index % 2 == 0) ? chunkSizes[index / 2] - 1 : 0);
	}
}
