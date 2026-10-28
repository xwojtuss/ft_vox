#pragma once

#include <glm/vec3.hpp>

namespace game::planet {
	using RenderDistance = glm::vec<3, unsigned short>;

	constexpr unsigned short defaultUnloadMargin = 2;

	[[nodiscard]] bool       isWithinRenderDistance(glm::ivec3 chunkOffset, const RenderDistance& distance);
	[[nodiscard]] glm::ivec3 renderDistanceReach(const RenderDistance& distance);
}
