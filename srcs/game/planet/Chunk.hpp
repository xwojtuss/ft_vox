#pragma once

#include <array>
#include <cstddef>

#include "game/block/Block.hpp"
#include "game/planet/ChunkSize.hpp"

namespace game::planet {
	class Chunk {
	private:
		std::array<Block, chunkVolume> m_blocks;

		[[nodiscard]] static std::size_t indexOf(int x, int y, int z);

	public:
		[[nodiscard]] static constexpr bool contains(const int x, const int y, const int z) {
			return x >= 0 && x < chunkXSize && y >= 0 && y < chunkYSize && z >= 0 && z < chunkZSize;
		}

		[[nodiscard]] Block&       getBlock(int x, int y, int z);
		[[nodiscard]] const Block& getBlock(int x, int y, int z) const;
		void                       setBlock(int x, int y, int z, const Block& block);
		void                       removeBlock(int x, int y, int z);
	};
}
