#pragma once

#include <optional>
#include <vk_mem_alloc.h>

namespace render::vulkan {
	struct Range {
		VmaVirtualAllocation allocation{};
		VkDeviceSize         offset{};
	};

	struct RangeStatistics {
		VkDeviceSize allocatedBytes{};
		VkDeviceSize freeBytes{};
		VkDeviceSize largestFreeRange{};
		uint32_t     allocationCount{};
		uint32_t     freeRangeCount{};
	};

	class RangeAllocator {
	private:
		VmaVirtualBlock m_block{};
		VkDeviceSize    m_size{};

	public:
		explicit RangeAllocator(VkDeviceSize size);
		RangeAllocator(const RangeAllocator&)            = delete;
		RangeAllocator& operator=(const RangeAllocator&) = delete;
		RangeAllocator(RangeAllocator&& other) noexcept;
		RangeAllocator& operator=(RangeAllocator&& other) noexcept;
		~RangeAllocator();

		[[nodiscard]] std::optional<Range> allocate(VkDeviceSize size, VkDeviceSize alignment) const;
		void                               free(VmaVirtualAllocation allocation) const;
		[[nodiscard]] RangeStatistics      statistics() const;
		[[nodiscard]] VkDeviceSize         size() const;
	};
}
