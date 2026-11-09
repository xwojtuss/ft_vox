#include "game/block/BlockData.hpp"

#include <format>
#include <utility>

#include "error/Exception.hpp"

using namespace game::block;
using game::BlockId;

BlockDatas::BlockDatas() {
	BlockData air;
	air.prettyName = "Air";

	m_blockDatas.push_back(std::move(air));
}

BlockId BlockDatas::add(BlockData blockData) {
	const auto id = static_cast<BlockId>(m_blockDatas.size());
	m_blockDatas.push_back(std::move(blockData));
	return id;
}

BlockData& BlockDatas::getBlockData(const BlockId id) {
	if (id >= m_blockDatas.size())
		throw error::EcsError(std::format("block id={} does not exist", id));
	return m_blockDatas[id];
}

const BlockData& BlockDatas::getBlockData(const BlockId id) const {
	if (id >= m_blockDatas.size())
		throw error::EcsError(std::format("block id={} does not exist", id));
	return m_blockDatas[id];
}

std::size_t BlockDatas::size() const noexcept {
	return m_blockDatas.size();
}
