#pragma once

#include <vector>

#include "APipeline.hpp"
#include "../VulkanContext.hpp"

namespace render::vulkan {
	class VertexColorPipeline : public APipeline {
	public:
		constexpr static const char* vertShaderPath = "shaders/vertexColorShader.vert.spv";
		constexpr static const char* fragShaderPath = "shaders/vertexColorShader.frag.spv";

		VertexColorPipeline(VulkanContext& context, const VkExtent2D& extent, VkRenderPass renderPass,
							const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = {});
		~VertexColorPipeline() override;
	};
}
