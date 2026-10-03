#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <glm/vec3.hpp>

#include "concurrency/IThreadPool.hpp"

namespace game::planet {
	enum class ChunkJob : std::uint8_t { Generate, Mesh };

	[[nodiscard]] inline concurrency::Priority chunkPriority(const ChunkJob job, const glm::ivec3 chunk,
															 const glm::ivec3 center) {
		constexpr unsigned int priorityLevelsPerJob = 128;
		constexpr int          farthestDistance     = priorityLevelsPerJob - 1;

		const glm::ivec3 offset = chunk - center;
		const int        distance =
			std::min(std::max({std::abs(offset.x), std::abs(offset.y), std::abs(offset.z)}), farthestDistance);
		const auto         nearness        = static_cast<unsigned int>(farthestDistance - distance);
		const unsigned int firstLevelOfJob = job == ChunkJob::Mesh ? priorityLevelsPerJob : 0U;

		return static_cast<concurrency::Priority>(firstLevelOfJob + nearness);
	}
}
