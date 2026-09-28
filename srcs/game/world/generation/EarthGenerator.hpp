#pragma once

#include "game/world/generation/Perlin2DMap.hpp"
#include "game/world/Chunk.hpp"

namespace game::world {
	class EarthGenerator {
	private:
		Perlin2DMap m_heightMap;

	public:
		EarthGenerator();

		void generateChunk(Chunk* chunk, glm::ivec3 chunkPosition) const;
	};
}
