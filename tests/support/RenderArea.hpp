#pragma once

#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <unordered_set>
#include <vector>

#include "game/planet/RenderDistance.hpp"

namespace test {
	using ChunkSet = std::unordered_set<glm::ivec3>;

	[[nodiscard]] inline ChunkSet renderAreaAround(const glm::ivec3                   center,
												   const game::planet::RenderDistance distance) {
		ChunkSet area;
		if (distance.x == 0 || distance.y == 0 || distance.z == 0)
			return area;

		const glm::ivec3 reach = glm::ivec3(distance) / 2;
		const glm::dvec3 radii = glm::dvec3(reach) + 0.5;
		for (int x = -reach.x; x <= reach.x; ++x) {
			for (int y = -reach.y; y <= reach.y; ++y) {
				for (int z = -reach.z; z <= reach.z; ++z) {
					const double fraction = ((x * x) / (radii.x * radii.x)) + ((y * y) / (radii.y * radii.y)) +
											((z * z) / (radii.z * radii.z));
					if (fraction <= 1.0)
						area.insert(center + glm::ivec3(x, y, z));
				}
			}
		}
		return area;
	}

	[[nodiscard]] inline size_t renderAreaSize(const game::planet::RenderDistance distance) {
		return renderAreaAround({0, 0, 0}, distance).size();
	}

	[[nodiscard]] inline ChunkSet unionOf(ChunkSet first, const ChunkSet& second) {
		first.insert(second.begin(), second.end());
		return first;
	}

	[[nodiscard]] inline ChunkSet toSet(const std::vector<glm::ivec3>& chunks) {
		return {chunks.begin(), chunks.end()};
	}
}
