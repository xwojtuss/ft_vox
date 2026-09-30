#define VMA_IMPLEMENTATION
#include "render/vulkan/VulkanMemory.hpp"
#include <string>
#include "log/Log.hpp"
#include "render/vulkan/VulkanError.hpp"

namespace render::vulkan {
	VmaAllocator createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
								 const uint32_t apiVersion) {
		VmaVulkanFunctions functions{};
		functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		functions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo createInfo{};
		createInfo.instance         = instance;
		createInfo.physicalDevice   = physicalDevice;
		createInfo.device           = device;
		createInfo.vulkanApiVersion = apiVersion;
		createInfo.pVulkanFunctions = &functions;

		VmaAllocator allocator = nullptr;
		if (const VkResult result = vmaCreateAllocator(&createInfo, &allocator); result != VK_SUCCESS)
			throw VulkanError("failed to create the memory allocator", result);
		return allocator;
	}

	void destroyAllocator(VmaAllocator allocator) {
		if (allocator == nullptr)
			return;

		VmaTotalStatistics statistics{};
		vmaCalculateStatistics(allocator, &statistics);
		if (const VmaStatistics& total = statistics.total.statistics; total.allocationCount > 0) {
			char* report = nullptr;
			vmaBuildStatsString(allocator, &report, VK_TRUE);
			logging::get(error::Domain::Render)
				.error("{} GPU allocations ({} bytes) were leaked, allocator report: {}", total.allocationCount,
					   total.allocationBytes, std::string(report));
			vmaFreeStatsString(allocator, report);
		}
		vmaDestroyAllocator(allocator);
	}
}
