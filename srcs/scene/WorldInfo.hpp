#pragma once

#include <glm/vec3.hpp>

namespace scene::worldinfo {
	constexpr auto      right    = glm::vec3(1.0f, 0.0f, 0.0f);
	constexpr glm::vec3 left     = -right;
	constexpr auto      up       = glm::vec3(0.0f, 1.0f, 0.0f);
	constexpr glm::vec3 down     = -up;
	constexpr auto      forward  = glm::vec3(0.0f, 0.0f, -1.0f);
	constexpr glm::vec3 backward = -forward;

	constexpr float drag = 0.9999999f;

	constexpr unsigned short              maxHorizontalRenderDistance = 20;
	constexpr unsigned short              maxVerticalRenderDistance   = 4;
	constexpr glm::vec<3, unsigned short> renderDistance              = {
		maxHorizontalRenderDistance, maxVerticalRenderDistance, maxHorizontalRenderDistance
	};
	constexpr unsigned short terrainMaxHeightBlocks = 64;
}
