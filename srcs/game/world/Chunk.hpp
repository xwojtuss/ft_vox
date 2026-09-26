#pragma once

#include <array>
#include <glm/vec3.hpp>

#include "../block/Block.hpp"

namespace game::world {
	constexpr unsigned short chunkXSize  = 16;
	constexpr unsigned short chunkYSize  = 16;
	constexpr unsigned short chunkZSize  = 16;
	constexpr size_t         chunkVolume = static_cast<size_t>(chunkXSize) * chunkYSize * chunkZSize;

	class Chunk {
	private:
		std::array<Block, chunkVolume> m_blocks;

	public:
		[[nodiscard]] Block&       getBlock(unsigned short x, unsigned short y, unsigned short z);
		[[nodiscard]] const Block& getBlock(unsigned short x, unsigned short y, unsigned short z) const;
		void                       setBlock(unsigned short x, unsigned short y, unsigned short z, const Block& block);
		void                       removeBlock(unsigned short x, unsigned short y, unsigned short z);
	};
}
