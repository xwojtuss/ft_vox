#include "VulkanResourceManager.hpp"
#include "../../render/GpuTypes.hpp"
#include "VulkanError.hpp"
#include "../../log/Log.hpp"
#include "VulkanContext.hpp"
#include "VulkanFrameData.hpp"

using namespace render::vulkan;

namespace {
	uint32_t supportedMipLevels(const VulkanContext& context, const uint32_t requestedMipLevels) {
		constexpr VkFormatFeatureFlags mipmapFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
														VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(context.getPhysicalDevice(), VK_FORMAT_R8G8B8A8_SRGB, &formatProperties);

		if (requestedMipLevels > 1 && (formatProperties.optimalTilingFeatures & mipmapFeatures) != mipmapFeatures) {
			logging::get(error::Domain::Render).warn("The GPU cannot generate mipmaps, distant textures may flicker");
			return 1;
		}
		return requestedMipLevels;
	}
}

VulkanResourceManager::VulkanResourceManager(const VulkanContext& context) {
	createCommandPool(context);
}

VulkanResourceManager::~VulkanResourceManager() = default;

VkCommandPool VulkanResourceManager::getCommandPool() const {
	return m_commandPool;
}

void VulkanResourceManager::createCommandPool(const VulkanContext& context) {
	QueueFamilyIndices queueFamilyIndices = context.getQueueFamilyIndices();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (const VkResult result = vkCreateCommandPool(context.getLogicalDevice(), &poolInfo, nullptr, &m_commandPool);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create command pool", result);
	}
}

void VulkanResourceManager::createBuffer(const VulkanContext& context, VkDeviceSize size, VkBufferUsageFlags usage,
										VkMemoryPropertyFlags properties, VkBuffer& buffer,
										VkDeviceMemory&       bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size        = size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (const VkResult result = vkCreateBuffer(context.getLogicalDevice(), &bufferInfo, nullptr, &buffer);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create buffer", result);
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(context.getLogicalDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = context.findMemoryType(memRequirements.memoryTypeBits, properties);

	if (const VkResult result = vkAllocateMemory(context.getLogicalDevice(), &allocInfo, nullptr, &bufferMemory);
		result != VK_SUCCESS) {
		throw VulkanError("failed to allocate buffer memory", result);
	}

	vkBindBufferMemory(context.getLogicalDevice(), buffer, bufferMemory, 0);
}

void VulkanResourceManager::transitionImageLayout(const VulkanContext&    context, VkImage      image,
												[[maybe_unused]] VkFormat format, VkImageLayout oldLayout,
												VkImageLayout             newLayout, uint32_t   mipLevels) const {
	VkCommandBuffer commandBuffer = VulkanFrameData::beginSingleTimeCommands(m_commandPool, context.getLogicalDevice());

	VkImageMemoryBarrier barrier{};
	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                       = oldLayout;
	barrier.newLayout                       = newLayout;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                           = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;

	VkPipelineStageFlags sourceStage      = 0;
	VkPipelineStageFlags destinationStage = 0;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout ==
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		throw VulkanError("unsupported layout transition");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	VulkanFrameData::endSingleTimeCommands(commandBuffer, m_commandPool, context.getGraphicsQueue(),
											context.getLogicalDevice());
}

void VulkanResourceManager::copyBufferToImage(const VulkanContext& context, VkBuffer buffer, VkImage image,
											uint32_t               width,
											uint32_t               height) const {
	VkCommandBuffer commandBuffer = VulkanFrameData::beginSingleTimeCommands(m_commandPool, context.getLogicalDevice());

	VkBufferImageCopy region{};
	region.bufferOffset      = 0;
	region.bufferRowLength   = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {
		width,
		height,
		1
	};
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	VulkanFrameData::endSingleTimeCommands(commandBuffer, m_commandPool, context.getGraphicsQueue(),
											context.getLogicalDevice());
}

void VulkanResourceManager::generateMipmaps(const VulkanContext& context, VkImage image,
											[[maybe_unused]] VkFormat imageFormat,
											int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const {
	VkCommandBuffer commandBuffer = VulkanFrameData::beginSingleTimeCommands(m_commandPool, context.getLogicalDevice());

	VkImageMemoryBarrier barrier{};
	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image                           = image;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	barrier.subresourceRange.levelCount     = 1;

	int32_t mipWidth  = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							0, nullptr,
							0, nullptr,
							1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0]                 = {0, 0, 0};
		blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
		blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel       = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount     = 1;
		blit.dstOffsets[0]                 = {0, 0, 0};
		blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
		blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel       = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount     = 1;

		vkCmdBlitImage(commandBuffer,
						image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, &blit,
						VK_FILTER_LINEAR);

		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							0, nullptr,
							0, nullptr,
							1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						0, nullptr,
						0, nullptr,
						1, &barrier);

	VulkanFrameData::endSingleTimeCommands(commandBuffer, m_commandPool, context.getGraphicsQueue(),
											context.getLogicalDevice());
}

SwapChainImage
VulkanResourceManager::createTextureImage(const assets::TextureData& textureData, const VulkanContext& context) const {
	const VkDeviceSize imageSize = static_cast<VkDeviceSize>(textureData.width) * textureData.height * 4;

	VkBuffer       stagingBuffer       = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
				stagingBufferMemory);

	void* data = nullptr;
	vkMapMemory(context.getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, textureData.pixels, imageSize);
	vkUnmapMemory(context.getLogicalDevice(), stagingBufferMemory);

	SwapChainImage swapChainImage{};
	VulkanSwapchain::createImage(
		context, VkExtent2D{.width = textureData.width, .height = textureData.height},
		textureData.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, swapChainImage);

	if (textureData.freePixels) {
		textureData.freePixels(textureData.pixels);
	}

	transitionImageLayout(context, swapChainImage.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureData.mipLevels);
	copyBufferToImage(context, stagingBuffer, swapChainImage.image, textureData.width,
					textureData.height);

	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

	vkDestroyBuffer(context.getLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(context.getLogicalDevice(), stagingBufferMemory, nullptr);

	generateMipmaps(context, swapChainImage.image, VK_FORMAT_R8G8B8A8_SRGB, static_cast<int32_t>(textureData.width),
					static_cast<int32_t>(textureData.height), textureData.mipLevels);
	return swapChainImage;
}

VkImageView VulkanResourceManager::createTextureImageView(const assets::TextureData& textureData,
														const VulkanContext&         context, VkImage textureImage) {
	return VulkanSwapchain::createImageView(context.getLogicalDevice(), textureImage, VK_FORMAT_R8G8B8A8_SRGB,
											VK_IMAGE_ASPECT_COLOR_BIT, textureData.mipLevels);
}

VkSampler VulkanResourceManager::createTextureSampler(const VulkanContext& context) {
	VkSampler textureSampler = VK_NULL_HANDLE;

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter        = VK_FILTER_LINEAR;
	samplerInfo.minFilter        = VK_FILTER_LINEAR;
	samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = context.isSamplerAnisotropyEnabled() ? VK_TRUE : VK_FALSE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context.getPhysicalDevice(), &properties);
	samplerInfo.maxAnisotropy = context.isSamplerAnisotropyEnabled() ? properties.limits.maxSamplerAnisotropy : 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	if (const VkResult result = vkCreateSampler(context.getLogicalDevice(), &samplerInfo, nullptr, &textureSampler);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create texture sampler", result);
	}
	return textureSampler;
}

VkSampler VulkanResourceManager::createPixelPerfectTextureSampler(const VulkanContext& context) {
	VkSampler textureSampler = VK_NULL_HANDLE;

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter        = VK_FILTER_NEAREST;
	samplerInfo.minFilter        = VK_FILTER_NEAREST;
	samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context.getPhysicalDevice(), &properties);
	samplerInfo.maxAnisotropy           = 1.0f;
	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable           = VK_FALSE;
	samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias              = 0.0f;
	samplerInfo.minLod                  = 0.0f;
	samplerInfo.maxLod                  = VK_LOD_CLAMP_NONE;

	if (const VkResult result = vkCreateSampler(context.getLogicalDevice(), &samplerInfo, nullptr, &textureSampler);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create texture sampler", result);
	}
	return textureSampler;
}

assets::TextureHandle VulkanResourceManager::createTexture(const assets::TextureData& textureData,
															VulkanContext& context, const VulkanFrameData& frameData) {
	assets::TextureData supportedTexture = textureData;
	supportedTexture.mipLevels           = supportedMipLevels(context, textureData.mipLevels);

	const SwapChainImage textureImage     = createTextureImage(supportedTexture, context);
	VkImageView          textureImageView = createTextureImageView(supportedTexture, context, textureImage.image);
	VkSampler            textureSampler   = textureData.pixelPerfect
									? createPixelPerfectTextureSampler(context)
									: createTextureSampler(context);

	VkDescriptorSet descriptorSet = frameData.createTextureDescriptorSet(context);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView   = textureImageView;
	imageInfo.sampler     = textureSampler;

	VkWriteDescriptorSet write{};
	write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet          = descriptorSet;
	write.dstBinding      = 0;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	write.pImageInfo      = &imageInfo;

	vkUpdateDescriptorSets(context.getLogicalDevice(), 1, &write, 0, nullptr);

	assets::TextureHandle handle;

	m_textures[handle.id] = GpuTexture{
		.image = SwapChainImage{
			.image = textureImage.image, .imageView = textureImageView, .imageMemory = textureImage.imageMemory
		},
		.sampler       = textureSampler,
		.descriptorSet = descriptorSet,
		.mipLevels     = textureData.mipLevels
	};

	return handle;
}

void VulkanResourceManager::copyBuffer(const VulkanContext& context, VkBuffer srcBuffer, VkBuffer dstBuffer,
										VkDeviceSize        size) const {
	VkCommandBuffer commandBuffer = VulkanFrameData::beginSingleTimeCommands(m_commandPool, context.getLogicalDevice());

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	VulkanFrameData::endSingleTimeCommands(commandBuffer, m_commandPool, context.getGraphicsQueue(),
											context.getLogicalDevice());
}

assets::MeshHandle VulkanResourceManager::addMesh(const GpuMesh& meshData) {
	assets::MeshHandle handle;
	m_meshes[handle.id] = meshData;
	return handle;
}

const GpuMesh& VulkanResourceManager::getMesh(assets::MeshHandle handle) const {
	return m_meshes.at(handle.id);
}

const GpuTexture& VulkanResourceManager::getTexture(assets::TextureHandle handle) const {
	return m_textures.at(handle.id);
}

size_t VulkanResourceManager::getTextureCount() const {
	return m_textures.size();
}

void VulkanResourceManager::cleanup(const VulkanContext& context) {
	for (const auto& [id, mesh]: m_meshes) {
		vkDestroyBuffer(context.getLogicalDevice(), mesh.vertexBuffer, nullptr);
		vkFreeMemory(context.getLogicalDevice(), mesh.vertexMemory, nullptr);
		vkDestroyBuffer(context.getLogicalDevice(), mesh.indexBuffer, nullptr);
		vkFreeMemory(context.getLogicalDevice(), mesh.indexMemory, nullptr);
	}

	for (const auto& [id, texture]: m_textures) {
		vkDestroySampler(context.getLogicalDevice(), texture.sampler, nullptr);
		vkDestroyImageView(context.getLogicalDevice(), texture.image.imageView, nullptr);
		vkDestroyImage(context.getLogicalDevice(), texture.image.image, nullptr);
		vkFreeMemory(context.getLogicalDevice(), texture.image.imageMemory, nullptr);
	}

	vkDestroyCommandPool(context.getLogicalDevice(), m_commandPool, nullptr);
}
