#pragma once

#include <array>
#include <cstddef>

namespace game::planet {
	constexpr int                chunkXSize  = 16;
	constexpr int                chunkYSize  = 16;
	constexpr int                chunkZSize  = 16;
	constexpr std::array<int, 3> chunkSizes  = {chunkXSize, chunkYSize, chunkZSize};
	constexpr std::size_t        chunkVolume = static_cast<std::size_t>(chunkXSize) * chunkYSize * chunkZSize;
}
