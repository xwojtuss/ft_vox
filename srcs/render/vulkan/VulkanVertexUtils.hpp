#pragma once

#include <array>
#include <volk.h>

namespace render::vulkan {
	[[nodiscard]] VkVertexInputBindingDescription                  getBindingDescription();
	[[nodiscard]] std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
}
