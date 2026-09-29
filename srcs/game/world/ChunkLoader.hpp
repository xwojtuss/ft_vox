#pragma once

#include "game/world/generation/EarthGenerator.hpp"

#include <memory>

namespace game::world {
	class ChunkLoader {
	private:
		EarthGenerator m_earthGenerator;

	public:
		[[nodiscard]] std::unique_ptr<Chunk> loadChunk(glm::ivec3 chunkPosition) const;
	};
}
