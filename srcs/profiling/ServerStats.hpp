#pragma once

#include <cstddef>
#include <cstdint>

namespace profiling {
	struct ServerStats {
		size_t   loadedChunks{};
		size_t   pendingGeneration{};
		uint64_t chunkDataBytes{};
	};
}
