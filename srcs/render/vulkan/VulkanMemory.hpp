#pragma once

#include <vk_mem_alloc.h>

namespace render::vulkan {
	[[nodiscard]] VmaAllocator createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
											   uint32_t apiVersion);
	void                       destroyAllocator(VmaAllocator allocator);
}
