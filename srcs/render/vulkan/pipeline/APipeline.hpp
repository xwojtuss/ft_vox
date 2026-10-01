#pragma once

#include <volk.h>
#include <vector>

namespace render::vulkan {
	class APipeline {
	protected:
		VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
		VkPipeline       m_pipeline{VK_NULL_HANDLE};
		VkViewport       m_viewport{};
		VkRect2D         m_scissor{};

		APipeline();

		static VkShaderModule createShaderModule(const std::vector<uint32_t>& code, VkDevice device);
		static void           createShaderStages(VkDevice device, const char* vertPath, const char* fragPath,
												 VkPipelineShaderStageCreateInfo& vertShaderStageInfo,
												 VkPipelineShaderStageCreateInfo& fragShaderStageInfo);
		void                  createScissor(const VkExtent2D& extent);
		void                  createViewport(const VkExtent2D& extent);
		void                  createViewportState(VkPipelineViewportStateCreateInfo& viewportState,
												  VkPipelineDynamicStateCreateInfo&  dynamicState,
												  const std::vector<VkDynamicState>& dynamicStates) const;
		static VkPipelineInputAssemblyStateCreateInfo createInputAssemblyState();
		static VkPipelineRasterizationStateCreateInfo createRasterizationState();
		static VkPipelineMultisampleStateCreateInfo   createMultisampleState(VkSampleCountFlagBits msaaSamples);
		static VkPipelineColorBlendAttachmentState    createColorBlendAttachmentState();
		static VkPipelineColorBlendStateCreateInfo    createColorBlendState(
			   const VkPipelineColorBlendAttachmentState& colorBlendAttachment);
		static VkPipelineDepthStencilStateCreateInfo createDepthStencilState();
		static VkPipelineLayoutCreateInfo            createPipelineLayoutInfo(
					   const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
		template<int N>
		static void destroyShaderStages(VkDevice device, std::array<VkPipelineShaderStageCreateInfo, N>& shaderStages);

	public:
		virtual ~APipeline();

		[[nodiscard]] const VkPipelineLayout& getPipelineLayout() const;
		[[nodiscard]] const VkPipeline&       getPipeline() const;
		[[nodiscard]] const VkViewport&       getViewport() const;
		[[nodiscard]] const VkRect2D&         getScissor() const;
		void                                  cleanup(VkDevice device);
	};
}

#include "render/vulkan/pipeline/APipeline.tpp"
