#include "APipeline.hpp"
#include <stdexcept>
#include "../VulkanVertexUtils.hpp"
#include "../../../platform/filesystem/readFile.hpp"
#include "../VulkanError.hpp"

using namespace render::vulkan;

APipeline::APipeline() = default;

VkShaderModule APipeline::createShaderModule(const std::vector<char>& code, VkDevice device) {
	VkShaderModuleCreateInfo createInfo{};
	VkShaderModule           shaderModule = nullptr;

	createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode    = std::__bit_cast<const uint32_t*>(code.data());

	if (const VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create shader module", result);
	}

	return shaderModule;
}

void APipeline::createShaderStages(VkDevice                          device, const char* vertPath, const char* fragPath,
									VkPipelineShaderStageCreateInfo& vertShaderStageInfo,
									VkPipelineShaderStageCreateInfo& fragShaderStageInfo) {
	vertShaderStageInfo        = {};
	vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = createShaderModule(readFile(vertPath), device);
	vertShaderStageInfo.pName  = "main";

	fragShaderStageInfo        = {};
	fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = createShaderModule(readFile(fragPath), device);
	fragShaderStageInfo.pName  = "main";
}

void APipeline::createScissor(const VkExtent2D& extent) {
	m_scissor = {};

	m_scissor.offset = {0, 0};
	m_scissor.extent = extent;
}

void APipeline::createViewport(const VkExtent2D& extent) {
	m_viewport = {};

	m_viewport.x        = 0.0f;
	m_viewport.y        = 0.0f;
	m_viewport.width    = static_cast<float>(extent.width);
	m_viewport.height   = static_cast<float>(extent.height);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
}

void APipeline::createViewportState(VkPipelineViewportStateCreateInfo& viewportState,
									VkPipelineDynamicStateCreateInfo&  dynamicState,
									const std::vector<VkDynamicState>& dynamicStates) const {
	dynamicState                   = {};
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates    = dynamicStates.data();

	viewportState               = {};
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports    = &m_viewport;
	viewportState.scissorCount  = 1;
	viewportState.pScissors     = &m_scissor;
	viewportState.pScissors     = nullptr;
}

VkPipelineInputAssemblyStateCreateInfo APipeline::createInputAssemblyState() {
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

	inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	return inputAssembly;
}

VkPipelineRasterizationStateCreateInfo APipeline::createRasterizationState() {
	VkPipelineRasterizationStateCreateInfo rasterizer{};

	rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth               = 1.0f;
	rasterizer.depthBiasEnable         = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp          = 0.0f;
	rasterizer.depthBiasSlopeFactor    = 0.0f;
	rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	return rasterizer;
}

VkPipelineMultisampleStateCreateInfo APipeline::createMultisampleState(VkSampleCountFlagBits msaaSamples) {
	VkPipelineMultisampleStateCreateInfo multisampling{};

	multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = msaaSamples;
	multisampling.minSampleShading      = 1.0f;
	multisampling.pSampleMask           = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable      = VK_FALSE;

	return multisampling;
}

VkPipelineColorBlendAttachmentState APipeline::createColorBlendAttachmentState() {
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};

	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
										| VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable         = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	return colorBlendAttachment;
}

VkPipelineColorBlendStateCreateInfo APipeline::createColorBlendState(
	const VkPipelineColorBlendAttachmentState& colorBlendAttachment) {
	VkPipelineColorBlendStateCreateInfo colorBlending{};

	colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable     = VK_FALSE;
	colorBlending.logicOp           = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount   = 1;
	colorBlending.pAttachments      = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	return colorBlending;
}

VkPipelineDepthStencilStateCreateInfo APipeline::createDepthStencilState() {
	VkPipelineDepthStencilStateCreateInfo depthStencil{};

	depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable       = VK_TRUE;
	depthStencil.depthWriteEnable      = VK_TRUE;
	depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds        = 0.0f;
	depthStencil.maxDepthBounds        = 1.0f;
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};
	depthStencil.back                  = {};

	return depthStencil;
}

VkPipelineLayoutCreateInfo APipeline::createPipelineLayoutInfo(
	const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const VkPushConstantRange& pushConstantRange) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = static_cast<unsigned int>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts            = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

	return pipelineLayoutInfo;
}

const VkPipelineLayout& APipeline::getPipelineLayout() const {
	return m_pipelineLayout;
}

const VkPipeline& APipeline::getPipeline() const {
	return m_pipeline;
}

const VkViewport& APipeline::getViewport() const {
	return m_viewport;
}

const VkRect2D& APipeline::getScissor() const {
	return m_scissor;
}

void APipeline::cleanup(VkDevice device) {
	if (m_pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, m_pipeline, nullptr);
		m_pipeline = VK_NULL_HANDLE;
	}
	if (m_pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
	}
}

APipeline::~APipeline() = default;
