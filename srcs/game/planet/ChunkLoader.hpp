#pragma once

#include "game/planet/generation/EarthGenerator.hpp"

#include <memory>

namespace game::planet {
	class ChunkLoader {
	private:
		EarthGenerator m_earthGenerator;

	public:
		explicit ChunkLoader(Seed seed);

		[[nodiscard]] std::unique_ptr<Chunk> loadChunk(glm::ivec3 chunkPosition) const;
	};
}
