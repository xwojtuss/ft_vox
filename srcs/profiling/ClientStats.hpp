#pragma once

#include <cstddef>
#include <cstdint>

#include "profiling/FrameTimes.hpp"

namespace profiling {
	struct ClientStats {
		FrameTimes frameTimes;
		size_t     drawnMeshes{};
		size_t     drawCalls{};
		uint64_t   triangles{};
		uint64_t   gpuMemoryUsedBytes{};
		uint64_t   gpuMemoryReservedBytes{};
		uint64_t   cpuMemoryBytes{};
		size_t     pendingMeshing{};
	};
}
