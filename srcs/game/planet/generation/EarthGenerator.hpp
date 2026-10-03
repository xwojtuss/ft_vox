#pragma once

#include <glm/vec3.hpp>
#include <memory>

#include "game/Seed.hpp"
#include "game/planet/Chunk.hpp"
#include "noise/INoise2D.hpp"

namespace game::planet {
	class EarthGenerator {
	private:
		std::unique_ptr<const noise::INoise2D> m_heightNoise;

		[[nodiscard]] float normalizedHeightAt(int planetX, int planetZ) const;

	public:
		explicit EarthGenerator(Seed seed);

		void generateChunk(Chunk* chunk, glm::ivec3 chunkPosition) const;
	};
}
