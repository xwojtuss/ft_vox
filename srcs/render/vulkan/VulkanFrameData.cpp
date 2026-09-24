#include "VulkanFrameData.hpp"
#include <chrono>
#include "../../render/GpuTypes.hpp"
#include "VulkanError.hpp"
#include "VulkanContext.hpp"
#include "VulkanResourceManager.hpp"

using namespace render::vulkan;

void VulkanFrameData::createFrameUBOs(VulkanContext& context) {
	m_frameUBOs.resize(maxFramesInFlight);
	m_frameUBOsMemory.resize(maxFramesInFlight);
	m_frameUBOsMapped.resize(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; i++) {
		constexpr VkDeviceSize bufferSize = sizeof(FrameUBO);
		VulkanResourceManager::createBuffer(context, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
											VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
											m_frameUBOs[i], m_frameUBOsMemory[i]);

		vkMapMemory(context.getLogicalDevice(), m_frameUBOsMemory[i], 0, bufferSize, 0, &m_frameUBOsMapped[i]);
	}
}

void VulkanFrameData::createCommandBuffers(const VulkanContext& context, const VulkanResourceManager& resourceManager) {
	m_commandBuffers.resize(maxFramesInFlight);
	VkCommandBufferAllocateInfo allocInfo{};

	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = resourceManager.getCommandPool();
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (const VkResult result = vkAllocateCommandBuffers(context.getLogicalDevice(), &allocInfo,
														m_commandBuffers.data()); result != VK_SUCCESS) {
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
	std::vector layouts(
		maxFramesInFlight,
		m_frameSetLayout
	);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = m_descriptorPool;
	allocInfo.descriptorSetCount =
			static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	m_frameDescriptorSets.resize(layouts.size());

	if (vkAllocateDescriptorSets(
			context.getLogicalDevice(),
			&allocInfo,
			m_frameDescriptorSets.data()) != VK_SUCCESS) {
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

		vkUpdateDescriptorSets(
			context.getLogicalDevice(),
			1,
			&write,
			0,
			nullptr);
	}
}

void VulkanFrameData::createFrameDescriptorSetLayout(const VulkanContext& context) {
	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding            = 0;
	uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount    = 1;
	uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &uboBinding;

	if (const VkResult result = vkCreateDescriptorSetLayout(context.getLogicalDevice(), &layoutInfo, nullptr,
															&m_frameSetLayout); result != VK_SUCCESS) {
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

	if (const VkResult result = vkCreateDescriptorSetLayout(context.getLogicalDevice(), &layoutInfo, nullptr,
															&m_textureSetLayout); result != VK_SUCCESS) {
		throw VulkanError("failed to create texture descriptor set layout", result);
	}
}

void VulkanFrameData::createDescriptorPool(const VulkanContext& context) {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(maxFramesInFlight);
	poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = textureLimit;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes    = poolSizes.data();
	poolInfo.maxSets       = static_cast<uint32_t>(maxFramesInFlight + textureLimit);

	if (const VkResult result = vkCreateDescriptorPool(context.getLogicalDevice(), &poolInfo, nullptr,
														&m_descriptorPool); result != VK_SUCCESS) {
		throw VulkanError("failed to create descriptor pool", result);
	}
}

VulkanFrameData::VulkanFrameData(VulkanContext& context, const VulkanResourceManager& resourceManager) {
	createFrameDescriptorSetLayout(context);
	createTextureDescriptorSetLayout(context);
	createDescriptorPool(context);
	createFrameUBOs(context);
	createFrameDescriptorSets(context);
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

void VulkanFrameData::createVertexBuffer(VulkanContext&         context, VulkanResourceManager& resourceManager,
										const assets::MeshData& meshData, GpuMesh&              mesh) {
	const VkDeviceSize bufferSize = sizeof(meshData.vertices[0]) * meshData.vertices.size();

	VkBuffer       stagingBuffer       = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	VulkanResourceManager::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
										stagingBuffer, stagingBufferMemory);

	void* data = nullptr;
	vkMapMemory(context.getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, meshData.vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(context.getLogicalDevice(), stagingBufferMemory);

	VulkanResourceManager::createBuffer(context, bufferSize,
										VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
										VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.vertexBuffer, mesh.vertexMemory);

	resourceManager.copyBuffer(context, stagingBuffer, mesh.vertexBuffer, bufferSize);

	vkDestroyBuffer(context.getLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(context.getLogicalDevice(), stagingBufferMemory, nullptr);
}

void VulkanFrameData::createIndexBuffer(VulkanContext&          context, VulkanResourceManager& resourceManager,
										const assets::MeshData& meshData, GpuMesh&              mesh) {
	const VkDeviceSize bufferSize = sizeof(meshData.indices[0]) * meshData.indices.size();

	VkBuffer       stagingBuffer       = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	VulkanResourceManager::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
										stagingBuffer, stagingBufferMemory);

	void* data = nullptr;
	vkMapMemory(context.getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, meshData.indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(context.getLogicalDevice(), stagingBufferMemory);

	VulkanResourceManager::createBuffer(context, bufferSize,
										VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
										VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.indexBuffer, mesh.indexMemory);

	resourceManager.copyBuffer(context, stagingBuffer, mesh.indexBuffer, bufferSize);

	vkDestroyBuffer(context.getLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(context.getLogicalDevice(), stagingBufferMemory, nullptr);

	mesh.indexCount = static_cast<uint32_t>(meshData.indices.size());
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

	const VkSemaphore    waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
	VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount         = 1;
	submitInfo.pWaitSemaphores            = waitSemaphores;
	submitInfo.pWaitDstStageMask          = waitStages;
	submitInfo.commandBufferCount         = 1;
	submitInfo.pCommandBuffers            = &m_commandBuffers[m_currentFrame];

	const VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
	submitInfo.signalSemaphoreCount      = 1;
	submitInfo.pSignalSemaphores         = signalSemaphores;

	if (const VkResult result = vkQueueSubmit(context.getGraphicsQueue(), 1, &submitInfo,
											m_inFlightFences[m_currentFrame]); result != VK_SUCCESS) {
		throw VulkanError("failed to submit draw command buffer", result);
	}
}

uint32_t VulkanFrameData::getCurrentFrame() const {
	return m_currentFrame;
}

VkDescriptorSet* VulkanFrameData::getDescriptorSet(const uint32_t frameIndex) {
	return &m_frameDescriptorSets[frameIndex];
}

void* VulkanFrameData::getCurrentMappedFrameUBO() const {
	return m_frameUBOsMapped[m_currentFrame];
}

VkSemaphore VulkanFrameData::getCurrentImageAvailableSemaphore() const {
	return m_imageAvailableSemaphores[m_currentFrame];
}

void VulkanFrameData::cleanup(const VulkanContext& context) const {
	for (size_t i = 0; i < maxFramesInFlight; i++) {
		vkDestroyBuffer(context.getLogicalDevice(), m_frameUBOs[i], nullptr);
		vkFreeMemory(context.getLogicalDevice(), m_frameUBOsMemory[i], nullptr);
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
											VkQueue         graphicsQueue, VkDevice      device) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
