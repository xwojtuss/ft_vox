#include "VulkanVertexUtils.hpp"
#include "../GpuTypes.hpp"

namespace render::vulkan {
	VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding   = 0;
		bindingDescription.stride    = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	VkVertexInputBindingDescription getInstanceBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding   = 1;
		bindingDescription.stride    = sizeof(InstanceData);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset   = offsetof(render::Vertex, pos);

		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset   = offsetof(render::Vertex, color);

		attributeDescriptions[2].binding  = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset   = offsetof(render::Vertex, texCoord);

		return attributeDescriptions;
	}

	std::array<VkVertexInputAttributeDescription, 2> getInstanceAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding  = 1;
		attributeDescriptions[0].location = 3;
		attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset   = offsetof(render::InstanceData, texCoord);

		attributeDescriptions[1].binding  = 1;
		attributeDescriptions[1].location = 4;
		attributeDescriptions[1].format   = VK_FORMAT_R32_SINT;
		attributeDescriptions[1].offset   = offsetof(render::InstanceData, charIndex);

		return attributeDescriptions;
	}
}
