#pragma once

#include <glm/vec3.hpp>

#include "game/planet/generation/Perlin2DMap.hpp"
#include "game/planet/Chunk.hpp"

namespace game::planet {
	class EarthGenerator {
	private:
		Perlin2DMap m_heightMap;

	public:
		EarthGenerator();

		void generateChunk(Chunk* chunk, glm::ivec3 chunkPosition) const;
	};
}
