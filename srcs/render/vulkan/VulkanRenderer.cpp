#include "render/vulkan/VulkanRenderer.hpp"
#include <algorithm>
#include "render/vulkan/VulkanValidationLayers.hpp"
#include "render/vulkan/pipeline/TexturePipeline.hpp"
#include "render/vulkan/pipeline/VertexColorPipeline.hpp"
#include "platform/filesystem/readFile.hpp"
#include "render/vulkan/VulkanError.hpp"
#include "ecs/component/Components.hpp"
#include "profiling/Profiler.hpp"

#include <ranges>

using namespace render::vulkan;

VulkanRenderer::VulkanRenderer(platform::window::IWindow& window) {
	m_context         = std::make_unique<VulkanContext>(window);
	m_swapchain       = std::make_unique<VulkanSwapchain>(*m_context);
	m_resourceManager = std::make_unique<VulkanResourceManager>(*m_context);
	m_frameData       = std::make_unique<VulkanFrameData>(*m_context, *m_resourceManager);
	m_gpuProfiler.init(m_context->getPhysicalDevice(), m_context->getLogicalDevice(), m_context->getGraphicsQueue(),
					   m_frameData->getCommandBuffer(0));
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

DrawBatch& VulkanRenderer::batchFor(const DrawBatchKey& key) {
	if (m_currentBatch < m_batches.size() && m_batches[m_currentBatch].key == key)
		return m_batches[m_currentBatch];

	const auto existing = std::ranges::find(m_batches, key, &DrawBatch::key);
	if (existing != m_batches.end()) {
		m_currentBatch = static_cast<size_t>(existing - m_batches.begin());
		return *existing;
	}

	m_currentBatch = m_batches.size();
	return m_batches.emplace_back(DrawBatch{.key = key, .items = {}, .firstCommand = 0});
}

void VulkanRenderer::drawMesh(const ecs::component::Mesh& mesh, const ecs::component::Texture* texture,
							  const ecs::component::Transform& transform) {
	const GpuMesh& gpuMesh = m_resourceManager->getMesh(mesh.mesh);

	batchFor(DrawBatchKey{.pipelineType = mesh.pipelineType,
						  .texture = texture != nullptr ? &m_resourceManager->getTexture(texture->texture) : nullptr,
						  .arena   = gpuMesh.arena})
		.items.push_back(DrawItem{.indexCount   = gpuMesh.indexCount,
								  .firstIndex   = gpuMesh.firstIndex,
								  .vertexOffset = gpuMesh.vertexOffset,
								  .model        = transform.toModelMatrix()});
}

void VulkanRenderer::drawBatch(const APipeline* pipeline, const DrawBatch& batch) const {
	VkCommandBuffer          commandBuffer = m_frameData->getCurrentCommandBuffer();
	const VulkanMeshStorage& storage       = m_resourceManager->getMeshStorage();
	const size_t             first         = batch.firstCommand;
	const size_t             count         = batch.items.size();

	if (batch.key.texture != nullptr)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 1, 1,
								&batch.key.texture->descriptorSet, 0, nullptr);

	VkBuffer               vertexBuffer = storage.vertexBuffer(batch.key.arena);
	constexpr VkDeviceSize offset       = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, storage.indexBuffer(batch.key.arena), 0, VK_INDEX_TYPE_UINT32);

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
	FT_PROFILE_FUNCTION();
	size_t total = 0;
	for (DrawBatch& batch: m_batches) {
		batch.firstCommand = total;
		total += batch.items.size();
	}
	if (total == 0)
		return;

	m_objects.resize(total);
	m_commands.resize(total);
	for (const DrawBatch& batch: m_batches) {
		for (size_t i = 0; i < batch.items.size(); ++i) {
			const DrawItem& item   = batch.items[i];
			const size_t    index  = batch.firstCommand + i;
			m_objects[index].model = item.model;
			m_commands[index]      = VkDrawIndexedIndirectCommand{.indexCount    = item.indexCount,
																  .instanceCount = 1,
																  .firstIndex    = item.firstIndex,
																  .vertexOffset  = item.vertexOffset,
																  .firstInstance = static_cast<uint32_t>(index)};
		}
	}
	m_frameData->uploadDraws(*m_context, m_objects, m_commands);

	VkCommandBuffer commandBuffer = m_frameData->getCurrentCommandBuffer();
	FT_PROFILE_GPU_ZONE(m_gpuProfiler, commandBuffer, "Draw chunks");
	const APipeline*     pipeline = nullptr;
	assets::PipelineType boundType{};
	for (DrawBatch& batch: m_batches) {
		if (batch.items.empty())
			continue;

		if (pipeline == nullptr || boundType != batch.key.pipelineType) {
			boundType = batch.key.pipelineType;
			pipeline  = m_pipelineHandles.at(boundType).get();
			vkCmdSetViewport(commandBuffer, 0, 1, &pipeline->getViewport());
			vkCmdSetScissor(commandBuffer, 0, 1, &pipeline->getScissor());
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 0, 1,
									m_frameData->getDescriptorSet(m_frameData->getCurrentFrame()), 0, nullptr);
		}

		drawBatch(pipeline, batch);
		batch.items.clear();
	}
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
	m_gpuProfiler.collect(commandBuffer);
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
	FT_PROFILE_FUNCTION();
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
	FT_PROFILE_FUNCTION();
	const std::optional<uint32_t> imageIndex = acquireImage();
	if (!imageIndex)
		return;

	m_resourceManager->releaseDestroyedMeshes();

	// Only reset the fence if we are submitting work
	m_frameData->resetFences(*m_context, m_frameData->getCurrentFrame());

	vkResetCommandBuffer(m_frameData->getCurrentCommandBuffer(), 0);
	recordCommandBuffer(systemManager, *imageIndex);
	{
		FT_PROFILE_ZONE("Submit");
		m_frameData->submitCommandBuffer(*m_context, m_renderFinishedSemaphores[*imageIndex]);
	}

	present(*imageIndex);
}

void VulkanRenderer::render(render::gui::IGui& gui) {
	gui.render(m_frameData->getCurrentCommandBuffer());
}

void VulkanRenderer::present(const uint32_t imageIndex) {
	FT_PROFILE_FUNCTION();
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
