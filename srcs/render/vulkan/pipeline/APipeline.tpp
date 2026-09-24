#pragma once
namespace render::vulkan {
	template<int N>
	void APipeline::destroyShaderStages(VkDevice device, std::array<VkPipelineShaderStageCreateInfo, N>& shaderStages) {
		for (int i = 0; i < N; i++) {
			vkDestroyShaderModule(device, shaderStages[i].module, nullptr);
		}
	}

	template<typename UBO>
	VkPushConstantRange APipeline::createPushConstantRange() {
		VkPushConstantRange pushConstantRange{};

		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(UBO);

		return pushConstantRange;
	}
}
