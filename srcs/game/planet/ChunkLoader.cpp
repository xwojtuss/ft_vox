#include "game/planet/ChunkLoader.hpp"

#include "profiling/Profiler.hpp"

using namespace game::planet;

ChunkLoader::ChunkLoader(const Seed seed) : m_earthGenerator(seed) {
}

std::unique_ptr<Chunk> ChunkLoader::loadChunk(const glm::ivec3 chunkPosition) const {
	FT_PROFILE_ZONE("Generate chunk");
	auto chunk = std::make_unique<Chunk>();

	m_earthGenerator.generateChunk(chunk.get(), chunkPosition);
	return chunk;
}
