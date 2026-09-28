#pragma once

#include <vector>

#include "render/vulkan/pipeline/APipeline.hpp"
#include "render/vulkan/VulkanContext.hpp"

namespace render::vulkan {
	class TexturePipeline : public APipeline {
	public:
		constexpr static const char* vertShaderPath = "shaders/shader.vert.spv";
		constexpr static const char* fragShaderPath = "shaders/shader.frag.spv";

		TexturePipeline(VulkanContext& context, const VkExtent2D& extent, VkRenderPass renderPass,
						const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = {});
		~TexturePipeline() override;
	};
}
