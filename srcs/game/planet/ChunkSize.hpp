#pragma once

namespace game::planet {
	constexpr int    chunkXSize  = 16;
	constexpr int    chunkYSize  = 16;
	constexpr int    chunkZSize  = 16;
	constexpr size_t chunkVolume = static_cast<size_t>(chunkXSize) * chunkYSize * chunkZSize;
}
