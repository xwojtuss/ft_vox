#include "ChunkLoader.hpp"

using namespace game::world;

std::unique_ptr<Chunk>	ChunkLoader::loadChunk(glm::ivec3 chunkPosition) {
	auto chunk = std::make_unique<Chunk>();

	m_earthGenerator.generateChunk(chunk.get(), chunkPosition);
	if (!chunk)
		throw std::runtime_error("Failed to generate chunk");
	return chunk;
}

void	ChunkLoader::saveChunk(glm::ivec3 chunkPosition) {
	(void)chunkPosition;
}
