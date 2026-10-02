#pragma once

#ifdef FT_VOX_PROFILER

#include <volk.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

namespace profiling {
	class GpuProfiler {
	private:
		TracyVkCtx m_context = nullptr;

	public:
		GpuProfiler()                              = default;
		GpuProfiler(const GpuProfiler&)            = delete;
		GpuProfiler& operator=(const GpuProfiler&) = delete;
		GpuProfiler(GpuProfiler&&)                 = delete;
		GpuProfiler& operator=(GpuProfiler&&)      = delete;
		~GpuProfiler() {
			if (m_context != nullptr)
				TracyVkDestroy(m_context);
		}

		void init(const VkPhysicalDevice physicalDevice, const VkDevice device, const VkQueue queue,
				  const VkCommandBuffer commandBuffer) {
			m_context = TracyVkContext(physicalDevice, device, queue, commandBuffer);
		}

		void collect(const VkCommandBuffer commandBuffer) const {
			if (m_context != nullptr)
				TracyVkCollect(m_context, commandBuffer);
		}

		[[nodiscard]] TracyVkCtx context() const {
			return m_context;
		}
	};
}

#define FT_PROFILE_FRAME() FrameMark
#define FT_PROFILE_ZONE(name) ZoneScopedN(name)
#define FT_PROFILE_FUNCTION() ZoneScoped
#define FT_PROFILE_THREAD(name) tracy::SetThreadName(name)
#define FT_PROFILE_GPU_ZONE(profiler, commandBuffer, name) TracyVkZone((profiler).context(), commandBuffer, name)

#else

#include <volk.h>

namespace profiling {
	class GpuProfiler {
	public:
		GpuProfiler() = default;

		void init(VkPhysicalDevice, VkDevice, VkQueue, VkCommandBuffer) {
		}

		void collect(VkCommandBuffer) const {
		}
	};
}

#define FT_PROFILE_FRAME()
#define FT_PROFILE_ZONE(name)
#define FT_PROFILE_FUNCTION()
#define FT_PROFILE_THREAD(name)
#define FT_PROFILE_GPU_ZONE(profiler, commandBuffer, name)

#endif
