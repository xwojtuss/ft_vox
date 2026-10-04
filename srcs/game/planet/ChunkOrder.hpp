#pragma once

#include <cstddef>
#include <glm/vec3.hpp>
#include <vector>

#include "concurrency/IThreadPool.hpp"

namespace game::planet {
	struct Viewer {
		glm::vec3 position{0.0f};
		glm::vec3 forward{0.0f, 0.0f, -1.0f};
	};

	constexpr std::size_t defaultQueuedJobsPerWorker = 128;

	constexpr concurrency::Priority generationJobPriority = 64;
	constexpr concurrency::Priority meshJobPriority       = 192;

	[[nodiscard]] glm::ivec3 chunkContaining(const glm::vec3& position);

	[[nodiscard]] float chunkOrderCost(glm::ivec3 chunk, const Viewer& viewer);

	[[nodiscard]] std::vector<glm::ivec3> mostUrgentChunks(const std::vector<glm::ivec3>& chunks, std::size_t count,
														   const Viewer& viewer);
	[[nodiscard]] std::size_t jobQueueLimit(const concurrency::IThreadPool& pool, std::size_t jobsPerWorker);
}
