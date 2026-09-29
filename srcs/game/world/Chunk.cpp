#include "game/world/Chunk.hpp"

#include "error/Assert.hpp"

using namespace game::world;

std::size_t Chunk::indexOf(const int x, const int y, const int z) {
	DEBUG_ASSERT(contains(x, y, z), "block outside the chunk", x, y, z);
	return (static_cast<std::size_t>(x) * chunkYSize * chunkZSize) + (static_cast<std::size_t>(y) * chunkZSize) +
		   static_cast<std::size_t>(z);
}

game::Block& Chunk::getBlock(const int x, const int y, const int z) {
	return m_blocks[indexOf(x, y, z)];
}

const game::Block& Chunk::getBlock(const int x, const int y, const int z) const {
	return m_blocks[indexOf(x, y, z)];
}

void Chunk::setBlock(const int x, const int y, const int z, const Block& block) {
	m_blocks[indexOf(x, y, z)] = block;
}

void Chunk::removeBlock(const int x, const int y, const int z) {
	m_blocks[indexOf(x, y, z)] = Block();
}
