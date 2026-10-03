#include "game/planet/Chunk.hpp"

#include <utility>

#include "error/Assert.hpp"

using namespace game::planet;

namespace {
	const game::Block airBlock;
}

std::size_t Chunk::indexOf(const int x, const int y, const int z) {
	DEBUG_ASSERT(contains(x, y, z), "block outside the chunk", x, y, z);
	return (static_cast<std::size_t>(x) * chunkYSize * chunkZSize) + (static_cast<std::size_t>(y) * chunkZSize) +
		   static_cast<std::size_t>(z);
}

const game::Block& Chunk::getBlock(const int x, const int y, const int z) const {
	const std::size_t index = indexOf(x, y, z);
	return m_blocks ? (*m_blocks)[index] : airBlock;
}

void Chunk::setBlock(const int x, const int y, const int z, const Block& block) {
	const std::size_t index = indexOf(x, y, z);

	if (block.id == 0) {
		if (!m_blocks)
			return;
		makeBlocksUnique();
		if ((*m_blocks)[index].id != 0)
			--m_nonAirCount;
		(*m_blocks)[index] = Block();
		if (m_nonAirCount == 0)
			m_blocks.reset();
		return;
	}

	if (!m_blocks)
		m_blocks = std::make_shared<Blocks>();
	makeBlocksUnique();
	if ((*m_blocks)[index].id == 0)
		++m_nonAirCount;
	(*m_blocks)[index] = block;
}

void Chunk::makeBlocksUnique() {
	if (m_blocks.use_count() > 1)
		m_blocks = std::make_shared<Blocks>(*m_blocks);
}

void Chunk::removeBlock(const int x, const int y, const int z) {
	setBlock(x, y, z, Block());
}

bool Chunk::isEmpty() const {
	return m_nonAirCount == 0;
}

bool Chunk::isFull() const {
	return m_nonAirCount == chunkVolume;
}

std::size_t Chunk::dataBytes() const {
	return m_blocks ? sizeof(Blocks) : 0;
}

bool Chunk::isFaceSolid(const ChunkFace face) const {
	if (isFull())
		return true;
	if (isEmpty())
		return false;

	const auto index  = static_cast<std::size_t>(std::to_underlying(face));
	const auto axis   = index / 2;
	const int  layer  = (index % 2 == 0) ? chunkSizes[axis] - 1 : 0;
	const auto first  = (axis == 0) ? 1uz : 0uz;
	const auto second = (axis == 2) ? 1uz : 2uz;

	const auto blockAt = [this, axis, layer](const int a, const int b) -> const Block& {
		if (axis == 0)
			return getBlock(layer, a, b);
		if (axis == 1)
			return getBlock(a, layer, b);
		return getBlock(a, b, layer);
	};

	for (int a = 0; a < chunkSizes[first]; ++a)
		for (int b = 0; b < chunkSizes[second]; ++b)
			if (blockAt(a, b).id == 0)
				return false;
	return true;
}
