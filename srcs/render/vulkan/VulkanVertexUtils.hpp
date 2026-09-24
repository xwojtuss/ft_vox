#pragma once

#include <array>
#include <vulkan/vulkan.h>

namespace render::vulkan {
	[[nodiscard]] VkVertexInputBindingDescription                  getBindingDescription();
	[[nodiscard]] VkVertexInputBindingDescription                  getInstanceBindingDescription();
	[[nodiscard]] std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
	[[nodiscard]] std::array<VkVertexInputAttributeDescription, 2> getInstanceAttributeDescriptions();
}
