#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include "game/block/Block.hpp"
#include "game/planet/ChunkSize.hpp"
#include "game/planet/ChunkFace.hpp"

namespace game::planet {
	class Chunk {
	private:
		using Blocks = std::array<Block, chunkVolume>;

		std::shared_ptr<Blocks> m_blocks;
		std::size_t             m_nonAirCount = 0;

		[[nodiscard]] static std::size_t indexOf(int x, int y, int z);
		void                             makeBlocksUnique();

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

		/** Whether every block of the one-block-thick layer on that side of the chunk is not air. */
		[[nodiscard]] bool isFaceSolid(ChunkFace face) const;
	};
}
