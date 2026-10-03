#include "render/vulkan/RangeAllocator.hpp"
#include <utility>
#include "render/vulkan/VulkanError.hpp"

using namespace render::vulkan;

RangeAllocator::RangeAllocator(const VkDeviceSize size) : m_size(size) {
	VmaVirtualBlockCreateInfo createInfo{};
	createInfo.size = size;
	if (const VkResult result = vmaCreateVirtualBlock(&createInfo, &m_block); result != VK_SUCCESS)
		throw VulkanError("failed to create the range allocator", result);
}

RangeAllocator::RangeAllocator(RangeAllocator&& other) noexcept :
	m_block(std::exchange(other.m_block, nullptr)), m_size(other.m_size) {
}

RangeAllocator& RangeAllocator::operator=(RangeAllocator&& other) noexcept {
	if (this != &other) {
		if (m_block != nullptr) {
			vmaClearVirtualBlock(m_block);
			vmaDestroyVirtualBlock(m_block);
		}
		m_block = std::exchange(other.m_block, nullptr);
		m_size  = other.m_size;
	}
	return *this;
}

RangeAllocator::~RangeAllocator() {
	if (m_block != nullptr) {
		vmaClearVirtualBlock(m_block);
		vmaDestroyVirtualBlock(m_block);
	}
}

std::optional<Range> RangeAllocator::allocate(const VkDeviceSize size, const VkDeviceSize alignment) const {
	VmaVirtualAllocationCreateInfo createInfo{};
	createInfo.size      = size;
	createInfo.alignment = alignment;

	Range range{};
	if (vmaVirtualAllocate(m_block, &createInfo, &range.allocation, &range.offset) != VK_SUCCESS)
		return std::nullopt;
	return range;
}

void RangeAllocator::free(VmaVirtualAllocation allocation) const {
	vmaVirtualFree(m_block, allocation);
}

void RangeAllocator::reset() const {
	vmaClearVirtualBlock(m_block);
}

RangeStatistics RangeAllocator::statistics() const {
	VmaStatistics statistics{};
	vmaGetVirtualBlockStatistics(m_block, &statistics);

	VmaDetailedStatistics detailed{};
	vmaCalculateVirtualBlockStatistics(m_block, &detailed);

	return RangeStatistics{.allocatedBytes   = statistics.allocationBytes,
						   .freeBytes        = m_size - statistics.allocationBytes,
						   .largestFreeRange = detailed.unusedRangeSizeMax,
						   .allocationCount  = statistics.allocationCount,
						   .freeRangeCount   = detailed.unusedRangeCount};
}

VkDeviceSize RangeAllocator::size() const {
	return m_size;
}
