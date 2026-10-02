#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include "game/block/Block.hpp"
#include "game/planet/ChunkSize.hpp"

namespace game::planet {
	class Chunk {
	private:
		using Blocks = std::array<Block, chunkVolume>;

		std::unique_ptr<Blocks> m_blocks;
		std::size_t             m_nonAirCount = 0;

		[[nodiscard]] static std::size_t indexOf(int x, int y, int z);

	public:
		[[nodiscard]] static constexpr bool contains(const int x, const int y, const int z) {
			return x >= 0 && x < chunkXSize && y >= 0 && y < chunkYSize && z >= 0 && z < chunkZSize;
		}

		[[nodiscard]] const Block& getBlock(int x, int y, int z) const;
		void                       setBlock(int x, int y, int z, const Block& block);
		void                       removeBlock(int x, int y, int z);

		[[nodiscard]] bool        isEmpty() const;
		[[nodiscard]] bool        isFull() const;
		[[nodiscard]] std::size_t dataBytes() const;

		/** Whether every block of the one-block-thick layer at `index` on `axis` (0 = x, 1 = y, 2 = z) is not air. */
		[[nodiscard]] bool isLayerSolid(int axis, int index) const;
	};
}
