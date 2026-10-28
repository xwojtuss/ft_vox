#include "game/planet/RenderDistance.hpp"

#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>

using namespace game::planet;

glm::ivec3 game::planet::renderDistanceReach(const RenderDistance& distance) {
	return glm::ivec3(distance) / 2;
}

bool game::planet::isWithinRenderDistance(const glm::ivec3 chunkOffset, const RenderDistance& distance) {
	if (glm::any(glm::equal(distance, RenderDistance(0))))
		return false;

	const glm::vec3 radii  = glm::vec3(renderDistanceReach(distance)) + 0.5f;
	const glm::vec3 scaled = glm::vec3(chunkOffset) / radii;
	return glm::dot(scaled, scaled) <= 1.0f;
}
