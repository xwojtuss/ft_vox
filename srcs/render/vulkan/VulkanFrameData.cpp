#include "render/vulkan/VulkanFrameData.hpp"
#include <chrono>
#include <algorithm>
#include <cstring>
#include "log/Log.hpp"
#include "render/GpuTypes.hpp"
#include "render/vulkan/VulkanError.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanResourceManager.hpp"

using namespace render::vulkan;

void VulkanFrameData::createFrameUBOs(VulkanContext& context) {
	m_frameUBOs.resize(maxFramesInFlight);
	m_frameUBOsAllocations.resize(maxFramesInFlight);
	m_frameUBOsMapped.resize(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; i++) {
		constexpr VkDeviceSize bufferSize = sizeof(FrameUBO);
		VmaAllocationInfo      allocationInfo{};
		VulkanResourceManager::createBuffer(context, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
											VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
												VMA_ALLOCATION_CREATE_MAPPED_BIT,
											m_frameUBOs[i], m_frameUBOsAllocations[i], &allocationInfo);
		m_frameUBOsMapped[i] = allocationInfo.pMappedData;
	}
}

void VulkanFrameData::createDrawBuffers(const VulkanContext& context, const size_t frame, const uint32_t capacity) {
	constexpr VmaAllocationCreateFlags flags =
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	DrawBuffers&      buffers = m_drawBuffers[frame];
	VmaAllocationInfo objectInfo{};
	VmaAllocationInfo indirectInfo{};
	VulkanResourceManager::createBuffer(context, sizeof(ObjectData) * capacity, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
										flags, buffers.objectBuffer, buffers.objectAllocation, &objectInfo);
	VulkanResourceManager::createBuffer(context, sizeof(VkDrawIndexedIndirectCommand) * capacity,
										VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, flags, buffers.indirectBuffer,
										buffers.indirectAllocation, &indirectInfo);
	buffers.objectMapped   = objectInfo.pMappedData;
	buffers.indirectMapped = indirectInfo.pMappedData;
	buffers.capacity       = capacity;

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffers.objectBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range  = sizeof(ObjectData) * capacity;

	VkWriteDescriptorSet write{};
	write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet          = m_frameDescriptorSets[frame];
	write.dstBinding      = 1;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo     = &bufferInfo;

	vkUpdateDescriptorSets(context.getLogicalDevice(), 1, &write, 0, nullptr);
}

void VulkanFrameData::destroyDrawBuffers(const VulkanContext& context, const size_t frame) {
	DrawBuffers& buffers = m_drawBuffers[frame];
	vmaDestroyBuffer(context.getAllocator(), buffers.objectBuffer, buffers.objectAllocation);
	vmaDestroyBuffer(context.getAllocator(), buffers.indirectBuffer, buffers.indirectAllocation);
	buffers = DrawBuffers{};
}

void VulkanFrameData::uploadDraws(const VulkanContext& context, const std::span<const ObjectData> objects,
								  const std::span<const VkDrawIndexedIndirectCommand> commands) {
	DrawBuffers& buffers = m_drawBuffers[m_currentFrame];
	if (objects.size() > buffers.capacity) {
		const auto capacity = static_cast<uint32_t>(std::max<size_t>(objects.size(), buffers.capacity * 2ULL));
		logging::get(error::Domain::Render)
			.info("Draw buffers of frame {} grow from {} to {} objects", m_currentFrame, buffers.capacity, capacity);
		destroyDrawBuffers(context, m_currentFrame);
		createDrawBuffers(context, m_currentFrame, capacity);
	}

	memcpy(buffers.objectMapped, objects.data(), objects.size_bytes());
	memcpy(buffers.indirectMapped, commands.data(), commands.size_bytes());

	const VkResult objectResult =
		vmaFlushAllocation(context.getAllocator(), buffers.objectAllocation, 0, objects.size_bytes());
	const VkResult commandResult =
		vmaFlushAllocation(context.getAllocator(), buffers.indirectAllocation, 0, commands.size_bytes());
	if (objectResult != VK_SUCCESS || commandResult != VK_SUCCESS)
		throw VulkanError("failed to upload the draw data", objectResult != VK_SUCCESS ? objectResult : commandResult);
}

VkBuffer VulkanFrameData::getCurrentIndirectBuffer() const {
	return m_drawBuffers[m_currentFrame].indirectBuffer;
}

void VulkanFrameData::createCommandBuffers(const VulkanContext& context, const VulkanResourceManager& resourceManager) {
	m_commandBuffers.resize(maxFramesInFlight);
	VkCommandBufferAllocateInfo allocInfo{};

	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = resourceManager.getCommandPool();
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (const VkResult result =
			vkAllocateCommandBuffers(context.getLogicalDevice(), &allocInfo, m_commandBuffers.data());
		result != VK_SUCCESS) {
		throw VulkanError("failed to allocate command buffers", result);
	}
}

void VulkanFrameData::createSyncObjects(const VulkanContext& context) {
	m_imageAvailableSemaphores.resize(maxFramesInFlight);
	m_inFlightFences.resize(maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (std::size_t i = 0; i < maxFramesInFlight; i++) {
		if (vkCreateSemaphore(context.getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
			vkCreateFence(context.getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
			throw VulkanError("failed to create synchronization objects for a frame");
		}
	}
}

void VulkanFrameData::createFrameDescriptorSets(const VulkanContext& context) {
	std::vector layouts(maxFramesInFlight, m_frameSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts        = layouts.data();

	m_frameDescriptorSets.resize(layouts.size());

	if (vkAllocateDescriptorSets(context.getLogicalDevice(), &allocInfo, m_frameDescriptorSets.data()) != VK_SUCCESS) {
		throw VulkanError("failed to allocate frame descriptor sets");
	}

	for (size_t i = 0; i < layouts.size(); ++i) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_frameUBOs[i];
		bufferInfo.offset = 0;
		bufferInfo.range  = sizeof(FrameUBO);

		VkWriteDescriptorSet write{};
		write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet          = m_frameDescriptorSets[i];
		write.dstBinding      = 0;
		write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo     = &bufferInfo;

		vkUpdateDescriptorSets(context.getLogicalDevice(), 1, &write, 0, nullptr);
	}
}

void VulkanFrameData::createFrameDescriptorSetLayout(const VulkanContext& context) {
	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding            = 0;
	uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount    = 1;
	uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding objectsBinding{};
	objectsBinding.binding         = 1;
	objectsBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	objectsBinding.descriptorCount = 1;
	objectsBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

	const std::array bindings = {uboBinding, objectsBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings    = bindings.data();

	if (const VkResult result =
			vkCreateDescriptorSetLayout(context.getLogicalDevice(), &layoutInfo, nullptr, &m_frameSetLayout);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create frame descriptor set layout", result);
	}
}

void VulkanFrameData::createTextureDescriptorSetLayout(const VulkanContext& context) {
	VkDescriptorSetLayoutBinding samplerBinding{};
	samplerBinding.binding            = 0;
	samplerBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.descriptorCount    = 1;
	samplerBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &samplerBinding;

	if (const VkResult result =
			vkCreateDescriptorSetLayout(context.getLogicalDevice(), &layoutInfo, nullptr, &m_textureSetLayout);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create texture descriptor set layout", result);
	}
}

void VulkanFrameData::createDescriptorPool(const VulkanContext& context) {
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(maxFramesInFlight);
	poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(maxFramesInFlight);
	poolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = textureLimit;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes    = poolSizes.data();
	poolInfo.maxSets       = static_cast<uint32_t>(maxFramesInFlight + textureLimit);

	if (const VkResult result =
			vkCreateDescriptorPool(context.getLogicalDevice(), &poolInfo, nullptr, &m_descriptorPool);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create descriptor pool", result);
	}
}

VulkanFrameData::VulkanFrameData(VulkanContext& context, const VulkanResourceManager& resourceManager) {
	createFrameDescriptorSetLayout(context);
	createTextureDescriptorSetLayout(context);
	createDescriptorPool(context);
	createFrameUBOs(context);
	createFrameDescriptorSets(context);
	m_drawBuffers.resize(maxFramesInFlight);
	for (size_t frame = 0; frame < maxFramesInFlight; ++frame)
		createDrawBuffers(context, frame, initialDrawCapacity);
	createCommandBuffers(context, resourceManager);
	createSyncObjects(context);
}

VkDescriptorSetLayout VulkanFrameData::getFrameDescriptorSetLayout() const {
	return m_frameSetLayout;
}

VkDescriptorSetLayout VulkanFrameData::getTextureDescriptorSetLayout() const {
	return m_textureSetLayout;
}

VkDescriptorSet VulkanFrameData::createTextureDescriptorSet(const VulkanContext& context) const {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &m_textureSetLayout;

	VkDescriptorSet descriptorSet = nullptr;

	if (const VkResult result = vkAllocateDescriptorSets(context.getLogicalDevice(), &allocInfo, &descriptorSet);
		result != VK_SUCCESS) {
		throw VulkanError("failed to allocate texture descriptor set", result);
	}
	return descriptorSet;
}

VkResult VulkanFrameData::waitForFences(const VulkanContext& context, const uint32_t currentFrame) const {
	return vkWaitForFences(context.getLogicalDevice(), 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
}

void VulkanFrameData::resetFences(const VulkanContext& context, const uint32_t currentFrame) const {
	vkResetFences(context.getLogicalDevice(), 1, &m_inFlightFences[currentFrame]);
}

VkCommandBuffer VulkanFrameData::getCommandBuffer(uint32_t index) const {
	return m_commandBuffers[index];
}

VkCommandBuffer VulkanFrameData::getCurrentCommandBuffer() const {
	return m_commandBuffers[m_currentFrame];
}

void VulkanFrameData::incrementCurrentFrame() {
	m_currentFrame = (m_currentFrame + 1) % maxFramesInFlight;
}

void VulkanFrameData::submitCommandBuffer(const VulkanContext& context, VkSemaphore renderFinishedSemaphore) const {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	auto* const                    waitSemaphore = m_imageAvailableSemaphores[m_currentFrame];
	constexpr VkPipelineStageFlags waitStage     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount                = 1;
	submitInfo.pWaitSemaphores                   = &waitSemaphore;
	submitInfo.pWaitDstStageMask                 = &waitStage;
	submitInfo.commandBufferCount                = 1;
	submitInfo.pCommandBuffers                   = &m_commandBuffers[m_currentFrame];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = &renderFinishedSemaphore;

	if (const VkResult result =
			vkQueueSubmit(context.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]);
		result != VK_SUCCESS) {
		throw VulkanError("failed to submit draw command buffer", result);
	}
}

uint32_t VulkanFrameData::getCurrentFrame() const {
	return m_currentFrame;
}

VkDescriptorSet* VulkanFrameData::getDescriptorSet(const uint32_t frameIndex) {
	return &m_frameDescriptorSets[frameIndex];
}

void VulkanFrameData::writeCurrentFrameUBO(const VulkanContext& context, const FrameUBO& frameUbo) const {
	memcpy(m_frameUBOsMapped[m_currentFrame], &frameUbo, sizeof(FrameUBO));
	if (const VkResult result =
			vmaFlushAllocation(context.getAllocator(), m_frameUBOsAllocations[m_currentFrame], 0, sizeof(FrameUBO));
		result != VK_SUCCESS) {
		throw VulkanError("failed to update the frame uniform buffer", result);
	}
}

VkSemaphore VulkanFrameData::getCurrentImageAvailableSemaphore() const {
	return m_imageAvailableSemaphores[m_currentFrame];
}

void VulkanFrameData::cleanup(const VulkanContext& context) const {
	for (size_t i = 0; i < maxFramesInFlight; i++) {
		vmaDestroyBuffer(context.getAllocator(), m_frameUBOs[i], m_frameUBOsAllocations[i]);
		vmaDestroyBuffer(context.getAllocator(), m_drawBuffers[i].objectBuffer, m_drawBuffers[i].objectAllocation);
		vmaDestroyBuffer(context.getAllocator(), m_drawBuffers[i].indirectBuffer, m_drawBuffers[i].indirectAllocation);
		vkDestroySemaphore(context.getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(context.getLogicalDevice(), m_inFlightFences[i], nullptr);
	}

	vkDestroyDescriptorPool(context.getLogicalDevice(), m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(context.getLogicalDevice(), m_frameSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.getLogicalDevice(), m_textureSetLayout, nullptr);
}

VkCommandBuffer VulkanFrameData::beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = nullptr;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanFrameData::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool,
											VkQueue graphicsQueue, VkDevice device) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
