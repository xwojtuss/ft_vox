#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "assets/Resources.hpp"
#include "game/block/Block.hpp"

namespace game::block {
	struct BlockData {
		std::string      prettyName;
		assets::MeshData meshData;
		std::string      textureId;
		bool             isUnique = false;
		std::string      packName;
	};

	class BlockDatas {
	public:
		static constexpr BlockId airId = 0;

	private:
		std::vector<BlockData> m_blockDatas;

	public:
		BlockDatas();

		BlockId add(BlockData blockData);

		[[nodiscard]] BlockData&       getBlockData(BlockId id);
		[[nodiscard]] const BlockData& getBlockData(BlockId id) const;

		[[nodiscard]] std::size_t size() const noexcept;
	};
}
