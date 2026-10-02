#include "game/planet/Chunk.hpp"

#include <array>

#include "error/Assert.hpp"

using namespace game::planet;

namespace {
	const game::Block airBlock;

	constexpr std::array axisSizes = {chunkXSize, chunkYSize, chunkZSize};
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
		if ((*m_blocks)[index].id != 0)
			--m_nonAirCount;
		(*m_blocks)[index] = Block();
		if (m_nonAirCount == 0)
			m_blocks.reset();
		return;
	}

	if (!m_blocks)
		m_blocks = std::make_unique<Blocks>();
	if ((*m_blocks)[index].id == 0)
		++m_nonAirCount;
	(*m_blocks)[index] = block;
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

bool Chunk::isLayerSolid(const int axis, const int index) const {
	DEBUG_ASSERT(axis >= 0 && axis < 3 && index >= 0 && index < axisSizes[static_cast<std::size_t>(axis)],
				 "layer outside the chunk", axis, index);
	if (isFull())
		return true;
	if (isEmpty())
		return false;

	const int first  = axis == 0 ? 1 : 0;
	const int second = axis == 2 ? 1 : 2;
	for (int a = 0; a < axisSizes[static_cast<std::size_t>(first)]; ++a) {
		for (int b = 0; b < axisSizes[static_cast<std::size_t>(second)]; ++b) {
			std::array<int, 3> position{};
			position[static_cast<std::size_t>(axis)]   = index;
			position[static_cast<std::size_t>(first)]  = a;
			position[static_cast<std::size_t>(second)] = b;
			if (getBlock(position[0], position[1], position[2]).id == 0)
				return false;
		}
	}
	return true;
}
