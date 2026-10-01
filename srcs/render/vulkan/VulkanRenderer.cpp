#include "render/vulkan/VulkanRenderer.hpp"
#include <algorithm>
#include <cstring>
#include <functional>
#include "render/vulkan/VulkanValidationLayers.hpp"
#include "render/vulkan/VulkanVertexUtils.hpp"
#include "render/vulkan/pipeline/TexturePipeline.hpp"
#include "render/vulkan/pipeline/VertexColorPipeline.hpp"
#include "platform/filesystem/readFile.hpp"
#include "render/vulkan/VulkanError.hpp"
#include "ecs/component/Components.hpp"

#include <ranges>

using namespace render::vulkan;

VulkanRenderer::VulkanRenderer(platform::window::IWindow& window) {
	m_context         = std::make_unique<VulkanContext>(window);
	m_swapchain       = std::make_unique<VulkanSwapchain>(*m_context);
	m_resourceManager = std::make_unique<VulkanResourceManager>(*m_context);
	m_frameData       = std::make_unique<VulkanFrameData>(*m_context, *m_resourceManager);
	createPipelines();
	createRenderFinishedSemaphores();
}

assets::MeshHandle VulkanRenderer::createMesh(const assets::MeshData& meshData) {
	return m_resourceManager->createMesh(*m_context, meshData);
}

void VulkanRenderer::destroyMesh(const assets::MeshHandle handle) {
	m_resourceManager->destroyMesh(handle);
}

assets::TextureHandle VulkanRenderer::createTexture(const assets::TextureData& textureData) {
	return m_resourceManager->createTexture(textureData, *m_context, *m_frameData);
}

void VulkanRenderer::createPipelines() {
	const std::vector descriptorSetLayouts            = {m_frameData->getFrameDescriptorSetLayout(),
														 m_frameData->getTextureDescriptorSetLayout()};
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
		if (const VkResult result =
				vkCreateSemaphore(m_context->getLogicalDevice(), &semaphoreInfo, nullptr, &semaphore);
			result != VK_SUCCESS) {
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

void VulkanRenderer::drawMesh(const ecs::component::Mesh& mesh, const ecs::component::Texture* texture,
							  const ecs::component::Transform& transform) {
	const GpuMesh& gpuMesh = m_resourceManager->getMesh(mesh.mesh);

	m_drawItems.push_back(
		DrawItem{.pipelineType = mesh.pipelineType,
				 .texture      = texture != nullptr ? &m_resourceManager->getTexture(texture->texture) : nullptr,
				 .arena        = gpuMesh.arena,
				 .indexCount   = gpuMesh.indexCount,
				 .firstIndex   = gpuMesh.firstIndex,
				 .vertexOffset = gpuMesh.vertexOffset,
				 .model        = transform.toModelMatrix()});
}

namespace {
	bool batchesBefore(const DrawItem& a, const DrawItem& b) {
		if (a.pipelineType != b.pipelineType)
			return a.pipelineType < b.pipelineType;
		if (a.texture != b.texture)
			return std::less<const GpuTexture*>{}(a.texture, b.texture);
		return a.arena < b.arena;
	}

	bool sameBatch(const DrawItem& a, const DrawItem& b) {
		return !batchesBefore(a, b) && !batchesBefore(b, a);
	}
}

void VulkanRenderer::drawBatch(const APipeline* pipeline, const size_t first, const size_t count) const {
	VkCommandBuffer          commandBuffer = m_frameData->getCurrentCommandBuffer();
	const DrawItem&          item          = m_drawItems[first];
	const VulkanMeshStorage& storage       = m_resourceManager->getMeshStorage();

	if (item.texture != nullptr)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 1, 1,
								&item.texture->descriptorSet, 0, nullptr);

	VkBuffer               vertexBuffer = storage.vertexBuffer(item.arena);
	constexpr VkDeviceSize offset       = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, storage.indexBuffer(item.arena), 0, VK_INDEX_TYPE_UINT32);

	if (!m_context->isIndirectDrawEnabled()) {
		for (size_t i = first; i < first + count; ++i) {
			const VkDrawIndexedIndirectCommand& command = m_commands[i];
			vkCmdDrawIndexed(commandBuffer, command.indexCount, 1, command.firstIndex, command.vertexOffset,
							 command.firstInstance);
		}
		return;
	}

	const size_t maxCount = m_context->getMaxDrawIndirectCount();
	for (size_t drawn = 0; drawn < count; drawn += maxCount) {
		const auto batchCount = static_cast<uint32_t>(std::min(maxCount, count - drawn));
		vkCmdDrawIndexedIndirect(commandBuffer, m_frameData->getCurrentIndirectBuffer(),
								 (first + drawn) * sizeof(VkDrawIndexedIndirectCommand), batchCount,
								 sizeof(VkDrawIndexedIndirectCommand));
	}
}

void VulkanRenderer::flushDraws() {
	if (m_drawItems.empty())
		return;

	std::ranges::sort(m_drawItems, batchesBefore);

	m_objects.resize(m_drawItems.size());
	m_commands.resize(m_drawItems.size());
	for (size_t i = 0; i < m_drawItems.size(); ++i) {
		const DrawItem& item = m_drawItems[i];
		m_objects[i].model   = item.model;
		m_commands[i]        = VkDrawIndexedIndirectCommand{.indexCount    = item.indexCount,
															.instanceCount = 1,
															.firstIndex    = item.firstIndex,
															.vertexOffset  = item.vertexOffset,
															.firstInstance = static_cast<uint32_t>(i)};
	}
	m_frameData->uploadDraws(*m_context, m_objects, m_commands);

	VkCommandBuffer      commandBuffer = m_frameData->getCurrentCommandBuffer();
	const APipeline*     pipeline      = nullptr;
	assets::PipelineType boundType{};
	size_t               first = 0;
	while (first < m_drawItems.size()) {
		size_t end = first + 1;
		while (end < m_drawItems.size() && sameBatch(m_drawItems[first], m_drawItems[end]))
			++end;

		if (pipeline == nullptr || boundType != m_drawItems[first].pipelineType) {
			boundType = m_drawItems[first].pipelineType;
			pipeline  = m_pipelineHandles.at(boundType).get();
			vkCmdSetViewport(commandBuffer, 0, 1, &pipeline->getViewport());
			vkCmdSetScissor(commandBuffer, 0, 1, &pipeline->getScissor());
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 0, 1,
									m_frameData->getDescriptorSet(m_frameData->getCurrentFrame()), 0, nullptr);
		}

		drawBatch(pipeline, first, end - first);
		first = end;
	}
	m_drawItems.clear();
}

void VulkanRenderer::updateCamera(const ecs::component::Camera& camera) {
	FrameUBO frameUbo{};

	frameUbo.view = camera.view;
	frameUbo.proj = camera.projection;
	frameUbo.proj[1][1] *= -1;

	m_frameData->writeCurrentFrameUBO(*m_context, frameUbo);
}

void VulkanRenderer::recordCommandBuffer(ecs::SystemManager& systemManager, const uint32_t imageIndex) {
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
	renderPassInfo.framebuffer       = m_swapchain->getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = {.x = 0, .y = 0};
	renderPassInfo.renderArea.extent = m_swapchain->getExtent();

	renderPassInfo.clearValueCount = static_cast<uint32_t>(m_clearValues.size());
	renderPassInfo.pClearValues    = m_clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	systemManager.onRendererDraw(*this);
	flushDraws();
	systemManager.onRendererFrame(*this);

	vkCmdEndRenderPass(commandBuffer);
	if (const VkResult result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS) {
		throw VulkanError("failed to record command buffer", result);
	}
}

void VulkanRenderer::recreateSwapchain() {
	vkDeviceWaitIdle(m_context->getLogicalDevice());
	cleanupPipelines();

	m_swapchain->recreateSwapChain(*m_context);
	createPipelines();
	createRenderFinishedSemaphores();
}

std::optional<uint32_t> VulkanRenderer::acquireImage() {
	if (const VkResult result = m_frameData->waitForFences(*m_context, m_frameData->getCurrentFrame());
		result != VK_SUCCESS)
		throw VulkanError("failed to wait for the previous frame", result);

	uint32_t       imageIndex = 0;
	const VkResult result =
		vkAcquireNextImageKHR(m_context->getLogicalDevice(), m_swapchain->getSwapChain(), UINT64_MAX,
							  m_frameData->getCurrentImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return std::nullopt;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw VulkanError("failed to acquire swap chain image", result);
	}
	return imageIndex;
}

void VulkanRenderer::render(ecs::SystemManager& systemManager) {
	const std::optional<uint32_t> imageIndex = acquireImage();
	if (!imageIndex)
		return;

	m_resourceManager->releaseDestroyedMeshes();

	// Only reset the fence if we are submitting work
	m_frameData->resetFences(*m_context, m_frameData->getCurrentFrame());

	vkResetCommandBuffer(m_frameData->getCurrentCommandBuffer(), 0);
	recordCommandBuffer(systemManager, *imageIndex);
	m_frameData->submitCommandBuffer(*m_context, m_renderFinishedSemaphores[*imageIndex]);

	present(*imageIndex);
}

void VulkanRenderer::render(render::gui::IGui& gui) {
	gui.render(m_frameData->getCurrentCommandBuffer());
}

void VulkanRenderer::present(const uint32_t imageIndex) {
	VkSemaphore signalSemaphore = m_renderFinishedSemaphores[imageIndex];

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = &signalSemaphore;

	auto* const swapChain      = m_swapchain->getSwapChain();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains    = &swapChain;
	presentInfo.pImageIndices  = &imageIndex;
	presentInfo.pResults       = nullptr;

	if (const VkResult result = vkQueuePresentKHR(m_context->getPresentQueue(), &presentInfo);
		result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_context->getWindow().wasResized()) {
		recreateSwapchain();
	} else if (result != VK_SUCCESS) {
		throw VulkanError("failed to present swap chain image", result);
	}

	m_frameData->incrementCurrentFrame();
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a) {
	m_clearValues[0].color        = {{r, g, b, a}};
	m_clearValues[1].depthStencil = {.depth = 1.0f, .stencil = 0};
}

void VulkanRenderer::setClearColor(int hexColor) {
	const auto  rgb = static_cast<uint32_t>(hexColor);
	const float r   = static_cast<float>((rgb >> 16U) & 0xFFU) / 255.0f;
	const float g   = static_cast<float>((rgb >> 8U) & 0xFFU) / 255.0f;
	const float b   = static_cast<float>(rgb & 0xFFU) / 255.0f;
	setClearColor(r, g, b, 1.0f);
}

void VulkanRenderer::cleanupPipelines() {
	for (const auto& pipeline: m_pipelineHandles | std::views::values) {
		pipeline->cleanup(m_context->getLogicalDevice());
	}
	m_pipelineHandles.clear();
}

void VulkanRenderer::cleanup() {
	vkDeviceWaitIdle(m_context->getLogicalDevice());

	cleanupPipelines();
	cleanupRenderFinishedSemaphores();

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
