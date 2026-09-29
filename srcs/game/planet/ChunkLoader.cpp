#include "game/planet/ChunkLoader.hpp"

using namespace game::planet;

std::unique_ptr<Chunk> ChunkLoader::loadChunk(const glm::ivec3 chunkPosition) const {
	auto chunk = std::make_unique<Chunk>();

	m_earthGenerator.generateChunk(chunk.get(), chunkPosition);
	return chunk;
}
