#include "game/planet/ChunkOrder.hpp"

#include <algorithm>
#include <glm/geometric.hpp>
#include <ranges>
#include <tuple>

#include "game/planet/ChunkSize.hpp"

using namespace game::planet;

namespace {
	constexpr float behindPenaltyInChunks       = 4.0f;
	constexpr float fullPenaltyDistanceInChunks = 2.0f;
	constexpr float tinyLength                  = 1e-4f;

	constexpr glm::vec3 chunkSize = glm::vec3(chunkXSize, chunkYSize, chunkZSize);
}

glm::ivec3 game::planet::chunkContaining(const glm::vec3& position) {
	return {glm::floor(position / chunkSize)};
}

float game::planet::chunkOrderCost(const glm::ivec3 chunk, const Viewer& viewer) {
	const glm::vec3 center         = (glm::vec3(chunk) + 0.5f) * chunkSize;
	const glm::vec3 offsetInChunks = (center - viewer.position) / chunkSize;
	const float     distance       = glm::length(offsetInChunks);

	if (distance < tinyLength || glm::length(viewer.forward) < tinyLength)
		return distance;

	const float cosine = glm::dot(glm::normalize(viewer.forward), offsetInChunks / distance);
	const float behind = (1.0f - cosine) * 0.5f;
	const float ramp   = std::clamp(distance / fullPenaltyDistanceInChunks, 0.0f, 1.0f);

	return distance + (behindPenaltyInChunks * behind * ramp);
}

// TODO: it is a possible bottleneck
std::vector<glm::ivec3> game::planet::mostUrgentChunks(const std::vector<glm::ivec3>& chunks, const std::size_t count,
													   const Viewer& viewer) {
	struct Candidate {
		float      cost;
		glm::ivec3 chunk;
	};

	std::vector<Candidate> candidates;
	candidates.reserve(chunks.size());
	for (const glm::ivec3& chunk: chunks)
		candidates.push_back({.cost = chunkOrderCost(chunk, viewer), .chunk = chunk});

	const std::size_t kept = std::min(count, candidates.size());
	std::ranges::partial_sort(candidates, candidates.begin() + static_cast<std::ptrdiff_t>(kept),
							  [](const Candidate& a, const Candidate& b) {
								  return std::tie(a.cost, a.chunk.x, a.chunk.y, a.chunk.z) <
										 std::tie(b.cost, b.chunk.x, b.chunk.y, b.chunk.z);
							  });

	std::vector<glm::ivec3> result;
	result.reserve(kept);
	for (const Candidate& candidate: candidates | std::views::take(kept))
		result.push_back(candidate.chunk);
	return result;
}

std::size_t game::planet::jobQueueLimit(const concurrency::IThreadPool& pool, const std::size_t jobsPerWorker) {
	return std::max<std::size_t>(1, pool.workerCount()) * jobsPerWorker;
}
