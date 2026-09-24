#include "VulkanRenderer.hpp"
#include "VulkanValidationLayers.hpp"
#include "VulkanVertexUtils.hpp"
#include "pipeline/TexturePipeline.hpp"
#include "pipeline/VertexColorPipeline.hpp"
#include "../../platform/filesystem/readFile.hpp"
#include "VulkanError.hpp"

using namespace render::vulkan;

VulkanRenderer::VulkanRenderer(platform::window::IWindow& window) {
	m_context         = std::make_unique<VulkanContext>(window);
	m_swapchain       = std::make_unique<VulkanSwapchain>(*m_context);
	m_resourceManager = std::make_unique<VulkanResourceManager>(*m_context);
	m_frameData       = std::make_unique<VulkanFrameData>(*m_context, *m_resourceManager);
	createPipelines();
	createRenderFinishedSemaphores();

	createTextMesh();
	VulkanResourceManager::createBuffer(
		*m_context,
		maxTextChars * sizeof(render::InstanceData),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_instanceBuffer,
		m_instanceBufferMemory
	);
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code, VkDevice device) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType            = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize         = code.size();
	createInfo.pCode            = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule = nullptr;
	if (const VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create shader module", result);
	}
	return shaderModule;
}

assets::MeshHandle VulkanRenderer::createMesh(const assets::MeshData& meshData) {
	GpuMesh mesh;

	VulkanFrameData::createVertexBuffer(*m_context, *m_resourceManager, meshData, mesh);
	VulkanFrameData::createIndexBuffer(*m_context, *m_resourceManager, meshData, mesh);

	const assets::MeshHandle handle = m_resourceManager->addMesh(mesh);

	return handle;
}

void VulkanRenderer::createTextMesh() {
	assets::MeshData textMeshData{};

	const std::vector<render::Vertex> quadVertices = {
		{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f / fontBitMapWidth, 0.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f / fontBitMapWidth, 1.0f / fontBitMapHeight}},
		{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f / fontBitMapHeight}},
	};

	const std::vector<uint32_t> quadIndices = {
		0, 1, 2,
		2, 3, 0
	};

	textMeshData.vertices = quadVertices;
	textMeshData.indices  = quadIndices;

	m_textMeshHandle = createMesh(textMeshData);
}

assets::TextureHandle VulkanRenderer::createTexture(const assets::TextureData& textureData) {
	return m_resourceManager->createTexture(textureData, *m_context, *m_frameData);
}

void VulkanRenderer::copyTextToInstanceBuffer(const std::string& text, size_t offset) const {
	void* mappedData = nullptr;

	const size_t bufferOffset = offset * sizeof(render::InstanceData);
	const size_t bufferSize   = text.length() * sizeof(render::InstanceData);

	vkMapMemory(m_context->getLogicalDevice(), m_instanceBufferMemory, bufferOffset, bufferSize, 0, &mappedData);

	for (size_t i = 0; i < text.length(); ++i) {
		const int index = text[i] - ' ';

		render::InstanceData instanceData{};
		instanceData.texCoord = {
			static_cast<float>(index % fontBitMapWidth) / static_cast<float>(fontBitMapWidth),
			static_cast<float>(index / fontBitMapWidth) / static_cast<float>(fontBitMapHeight),
		};
		instanceData.charIndex = static_cast<int>(i);

		memcpy(static_cast<char*>(mappedData) + i * sizeof(render::InstanceData), &instanceData,
				sizeof(render::InstanceData));
	}

	vkUnmapMemory(m_context->getLogicalDevice(), m_instanceBufferMemory);
}

void VulkanRenderer::createPipelines() {
	const std::vector descriptorSetLayouts = {
		m_frameData->getFrameDescriptorSetLayout(),
		m_frameData->getTextureDescriptorSetLayout()
	};
	m_pipelineHandles[assets::PipelineType::Textured] = std::make_unique<TexturePipeline>(
		*m_context, m_swapchain->getExtent(), m_swapchain->getRenderPass(), descriptorSetLayouts);
	m_pipelineHandles[assets::PipelineType::VertexColor] = std::make_unique<VertexColorPipeline>(
		*m_context, m_swapchain->getExtent(), m_swapchain->getRenderPass(), descriptorSetLayouts);
}

void VulkanRenderer::createRenderFinishedSemaphores() {
	cleanupRenderFinishedSemaphores();
	m_renderFinishedSemaphores.resize(m_swapchain->getImageCount());

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (VkSemaphore& semaphore: m_renderFinishedSemaphores) {
		if (const VkResult result = vkCreateSemaphore(m_context->getLogicalDevice(), &semaphoreInfo, nullptr,
													&semaphore); result != VK_SUCCESS) {
			throw VulkanError("failed to create render finished semaphore", result);
		}
	}
}

void VulkanRenderer::cleanupRenderFinishedSemaphores() {
	for (VkSemaphore semaphore: m_renderFinishedSemaphores) {
		vkDestroySemaphore(m_context->getLogicalDevice(), semaphore, nullptr);
	}
	m_renderFinishedSemaphores.clear();
}

void VulkanRenderer::drawMesh(const ecs::component::Mesh&    mesh, const ecs::component::Texture* texture,
							const ecs::component::Transform& transform) {
	const APipeline& pipeline = *m_pipelineHandles[mesh.pipelineType];
	const GpuMesh&   gpuMesh  = m_resourceManager->getMesh(mesh.mesh);
	vkCmdSetViewport(m_frameData->getCurrentCommandBuffer(), 0, 1, &pipeline.getViewport());
	vkCmdSetScissor(m_frameData->getCurrentCommandBuffer(), 0, 1, &pipeline.getScissor());
	vkCmdBindPipeline(m_frameData->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());
	vkCmdBindDescriptorSets(m_frameData->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipeline.getPipelineLayout(), 0, 1,
							m_frameData->getDescriptorSet(m_frameData->getCurrentFrame()), 0, nullptr);

	VkBuffer               vertexBuffer = gpuMesh.vertexBuffer;
	constexpr VkDeviceSize offset       = 0;
	vkCmdBindVertexBuffers(m_frameData->getCurrentCommandBuffer(), 0, 1, &vertexBuffer, &offset);

	vkCmdBindIndexBuffer(m_frameData->getCurrentCommandBuffer(), gpuMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	if (texture) {
		const GpuTexture& gpuTexture = m_resourceManager->getTexture(texture->texture);
		vkCmdBindDescriptorSets(m_frameData->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
								pipeline.getPipelineLayout(), 1, 1, &gpuTexture.descriptorSet, 0, nullptr);
	}

	ObjectUBO objectUbo{};
	objectUbo.model = transform.toModelMatrix();
	vkCmdPushConstants(m_frameData->getCurrentCommandBuffer(), pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT,
						0, sizeof(ObjectUBO), &objectUbo);

	vkCmdDrawIndexed(m_frameData->getCurrentCommandBuffer(), gpuMesh.indexCount, 1, 0, 0, 0);
}

void VulkanRenderer::updateCamera(const ecs::component::Camera& camera) {
	FrameUBO frameUbo{};

	frameUbo.view       = camera.view;
	frameUbo.proj       = camera.projection;
	frameUbo.proj[1][1] *= -1;

	memcpy(m_frameData->getCurrentMappedFrameUBO(), &frameUbo, sizeof(frameUbo));
}

void VulkanRenderer::recordCurrentCommandBuffer(ecs::SystemManager& systemManager) {
	if (!m_frameIndex.has_value())
		return;

	VkCommandBuffer commandBuffer = m_frameData->getCurrentCommandBuffer();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags            = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (const VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo); result != VK_SUCCESS) {
		throw VulkanError("failed to begin recording command buffer", result);
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = m_swapchain->getRenderPass();
	renderPassInfo.framebuffer       = m_swapchain->getFramebuffer(m_frameIndex.value());
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_swapchain->getExtent();

	renderPassInfo.clearValueCount = static_cast<uint32_t>(m_clearValues.size());
	renderPassInfo.pClearValues    = m_clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	systemManager.onRendererDraw(*this);
	systemManager.onTextDraw(*this);
	systemManager.onRendererFrame(*this);

	vkCmdEndRenderPass(commandBuffer);
	if (const VkResult result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS) {
		throw VulkanError("failed to record command buffer", result);
	}
}

void VulkanRenderer::beginFrame() {
	m_frameIndex = 0;

	(void)m_frameData->waitForFences(*m_context, m_frameData->getCurrentFrame());

	const VkResult result = vkAcquireNextImageKHR(m_context->getLogicalDevice(), m_swapchain->getSwapChain(),
												UINT64_MAX, m_frameData->getCurrentImageAvailableSemaphore(),
												VK_NULL_HANDLE, &m_frameIndex.value());

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		vkDeviceWaitIdle(m_context->getLogicalDevice());
		cleanupPipelines();

		m_swapchain->recreateSwapChain(*m_context);
		createPipelines();
		createRenderFinishedSemaphores();

		m_frameIndex.reset();
		return;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw VulkanError("failed to acquire swap chain image", result);
	}
}

void VulkanRenderer::render(ecs::SystemManager& systemManager) {
	beginFrame();
	if (!m_frameIndex.has_value()) {
		endFrame();
		return;
	}

	// Only reset the fence if we are submitting work
	m_frameData->resetFences(*m_context, m_frameData->getCurrentFrame());

	vkResetCommandBuffer(m_frameData->getCurrentCommandBuffer(), 0);
	recordCurrentCommandBuffer(systemManager);
	m_frameData->submitCommandBuffer(*m_context, m_renderFinishedSemaphores[m_frameIndex.value()]);

	endFrame();
}

void VulkanRenderer::render(render::gui::IGui& gui) {
	gui.render(m_frameData->getCurrentCommandBuffer());
}

void VulkanRenderer::endFrame() {
	if (!m_frameIndex.has_value())
		return;

	VkSemaphore signalSemaphore = m_renderFinishedSemaphores[m_frameIndex.value()];

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = &signalSemaphore;

	const VkSwapchainKHR swapChains[] = {m_swapchain->getSwapChain()};
	presentInfo.swapchainCount        = 1;
	presentInfo.pSwapchains           = swapChains;
	presentInfo.pImageIndices         = &m_frameIndex.value();
	presentInfo.pResults              = nullptr;

	if (const VkResult result = vkQueuePresentKHR(m_context->getPresentQueue(), &presentInfo);
		result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_context->getWindow().wasResized()) {
		vkDeviceWaitIdle(m_context->getLogicalDevice());
		cleanupPipelines();

		m_swapchain->recreateSwapChain(*m_context);
		createPipelines();
		createRenderFinishedSemaphores();
	} else if (result != VK_SUCCESS) {
		throw VulkanError("failed to present swap chain image", result);
	}

	m_frameData->incrementCurrentFrame();
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a) {
	m_clearValues[0].color        = {{r, g, b, a}};
	m_clearValues[1].depthStencil = {1.0f, 0};
}

void VulkanRenderer::setClearColor(int hexColor) {
	const auto  rgb = static_cast<uint32_t>(hexColor);
	const float r   = static_cast<float>((rgb >> 16U) & 0xFFU) / 255.0f;
	const float g   = static_cast<float>((rgb >> 8U) & 0xFFU) / 255.0f;
	const float b   = static_cast<float>(rgb & 0xFFU) / 255.0f;
	setClearColor(r, g, b, 1.0f);
}

const assets::MeshHandle& VulkanRenderer::getTextMeshHandle() const {
	return m_textMeshHandle;
}

void VulkanRenderer::cleanupPipelines() {
	for (auto const& [_, pipeline]: m_pipelineHandles) {
		pipeline->cleanup(m_context->getLogicalDevice());
	}
	m_pipelineHandles.clear();
}

void VulkanRenderer::cleanup() {
	vkDeviceWaitIdle(m_context->getLogicalDevice());

	cleanupPipelines();
	cleanupRenderFinishedSemaphores();

	vkDestroyBuffer(m_context->getLogicalDevice(), m_instanceBuffer, nullptr);
	vkFreeMemory(m_context->getLogicalDevice(), m_instanceBufferMemory, nullptr);

	m_resourceManager->cleanup(*m_context);
	m_frameData->cleanup(*m_context);
	m_swapchain->cleanup(*m_context);
}

VulkanRenderer::~VulkanRenderer() {
	VulkanRenderer::cleanup();
}

VulkanContext& VulkanRenderer::getContext() const {
	return *m_context;
}

VulkanSwapchain& VulkanRenderer::getSwapchain() const {
	return *m_swapchain;
}

platform::window::IWindow& VulkanRenderer::getWindow() const {
	return m_context->getWindow();
}
