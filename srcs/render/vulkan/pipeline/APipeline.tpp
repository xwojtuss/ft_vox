#pragma once
namespace render::vulkan {
	template<int N>
	void APipeline::destroyShaderStages(VkDevice device, std::array<VkPipelineShaderStageCreateInfo, N>& shaderStages) {
		for (int i = 0; i < N; i++) {
			vkDestroyShaderModule(device, shaderStages[i].module, nullptr);
		}
	}
}
