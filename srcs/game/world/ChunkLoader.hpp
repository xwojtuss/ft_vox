#pragma once

#include "generation/EarthGenerator.hpp"

#include <memory>

namespace game::world {
	class ChunkLoader {
	private:
		EarthGenerator m_earthGenerator;

	public:
		[[nodiscard]] std::unique_ptr<Chunk> loadChunk(glm::ivec3 chunkPosition) const;
		void                                 saveChunk(glm::ivec3 chunkPosition) const;
	};
}
