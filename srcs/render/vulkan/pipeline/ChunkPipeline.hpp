#pragma once

#include <vector>

#include "render/vulkan/pipeline/APipeline.hpp"
#include "render/vulkan/VulkanContext.hpp"

namespace render::vulkan {
	class ChunkPipeline : public APipeline {
	public:
		constexpr static const char* vertShaderPath = "shaders/chunk.vert.spv";
		constexpr static const char* fragShaderPath = "shaders/chunk.frag.spv";

		ChunkPipeline(VulkanContext& context, const VkExtent2D& extent, VkRenderPass renderPass,
					  const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = {});
		~ChunkPipeline() override;
	};
}
