#include "render/vulkan/pipeline/ChunkPipeline.hpp"
#include "render/vulkan/VulkanVertexUtils.hpp"
#include "render/vulkan/VulkanError.hpp"

using namespace render::vulkan;

ChunkPipeline::ChunkPipeline(VulkanContext& context, const VkExtent2D& extent, VkRenderPass renderPass,
							 const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
	constexpr std::size_t                                         shaderStageCount = 2;
	std::array<VkPipelineShaderStageCreateInfo, shaderStageCount> shaderStages{};
	VkPipelineViewportStateCreateInfo                             viewportState;
	VkPipelineDynamicStateCreateInfo                              dynamicState;

	const ChunkVertexSpecialization specialization;

	createShaderStages(context.getLogicalDevice(), vertShaderPath, fragShaderPath, shaderStages[0], shaderStages[1]);
	shaderStages[0].pSpecializationInfo = specialization.info();
	createViewport(extent);
	createScissor(extent);
	const std::vector dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	createViewportState(viewportState, dynamicState, dynamicStates);

	auto pipelineLayoutInfo = createPipelineLayoutInfo(descriptorSetLayouts);

	if (const VkResult result =
			vkCreatePipelineLayout(context.getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create pipeline layout", result);
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType                                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount                                   = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages                                      = shaderStages.data();
	VkPipelineVertexInputStateCreateInfo vertexInputState     = {};
	const auto                           bindingDescription   = getChunkBindingDescription();
	const auto                           attributeDescription = getChunkAttributeDescription();
	vertexInputState.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.vertexBindingDescriptionCount   = 1;
	vertexInputState.vertexAttributeDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions      = &bindingDescription;
	vertexInputState.pVertexAttributeDescriptions    = &attributeDescription;
	pipelineInfo.pVertexInputState                   = &vertexInputState;
	auto inputAssemblyState                          = createInputAssemblyState();
	pipelineInfo.pInputAssemblyState                 = &inputAssemblyState;
	pipelineInfo.pViewportState                      = &viewportState;
	auto rasterizationState                          = createRasterizationState();
	pipelineInfo.pRasterizationState                 = &rasterizationState;
	auto multisampleState                            = createMultisampleState(context.getMsaaSamples());
	pipelineInfo.pMultisampleState                   = &multisampleState;
	pipelineInfo.pDepthStencilState                  = nullptr;
	auto colorBlendAttachmentState                   = createColorBlendAttachmentState();
	auto colorBlendState                             = createColorBlendState(colorBlendAttachmentState);
	pipelineInfo.pColorBlendState                    = &colorBlendState;
	pipelineInfo.pDynamicState                       = &dynamicState;
	pipelineInfo.layout                              = m_pipelineLayout;
	pipelineInfo.renderPass                          = renderPass;
	pipelineInfo.subpass                             = 0;
	pipelineInfo.basePipelineHandle                  = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex                   = -1;
	auto depthStencil                                = createDepthStencilState();
	pipelineInfo.pDepthStencilState                  = &depthStencil;

	if (const VkResult result = vkCreateGraphicsPipelines(context.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
														  nullptr, &m_pipeline);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create graphics pipeline", result);
	}

	destroyShaderStages<shaderStageCount>(context.getLogicalDevice(), shaderStages);
}

ChunkPipeline::~ChunkPipeline() = default;
