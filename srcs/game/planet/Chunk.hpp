#pragma once

#include <array>
#include <cstddef>
#include <glm/vec3.hpp>

#include "game/block/Block.hpp"

namespace game::planet {
	constexpr int    chunkXSize  = 16;
	constexpr int    chunkYSize  = 16;
	constexpr int    chunkZSize  = 16;
	constexpr size_t chunkVolume = static_cast<size_t>(chunkXSize) * chunkYSize * chunkZSize;

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
