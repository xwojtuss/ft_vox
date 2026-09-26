#include "Chunk.hpp"

using namespace game::world;

game::Block& Chunk::getBlock(const unsigned short x, const unsigned short y, const unsigned short z) {
	return m_blocks[x * chunkYSize * chunkZSize + y * chunkZSize + z];
}

const game::Block& Chunk::getBlock(const unsigned short x, const unsigned short y, const unsigned short z) const {
	return m_blocks[x * chunkYSize * chunkZSize + y * chunkZSize + z];
}

void Chunk::setBlock(const unsigned short x, const unsigned short y, const unsigned short z, const Block& block) {
	m_blocks[x * chunkYSize * chunkZSize + y * chunkZSize + z] = block;
}

void Chunk::removeBlock(const unsigned short x, const unsigned short y, const unsigned short z) {
	m_blocks[x * chunkYSize * chunkZSize + y * chunkZSize + z] = Block();
}
