#pragma once

#include "Perlin2DMap.hpp"
#include "../Chunk.hpp"

namespace game::world {
	class EarthGenerator {
	private:
		Perlin2DMap m_heightMap;

	public:
		EarthGenerator();

		void generateChunk(Chunk* chunk, glm::ivec3 chunkPosition) const;
	};
}
