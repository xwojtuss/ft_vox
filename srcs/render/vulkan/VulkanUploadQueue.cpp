#include "render/vulkan/VulkanUploadQueue.hpp"

#include <algorithm>

#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanError.hpp"
#include "render/vulkan/VulkanResourceManager.hpp"

using namespace render::vulkan;

namespace {
	constexpr VkDeviceSize stagingAlignment = 16;
}

void VulkanUploadQueue::reclaimStagingMemory(const VulkanContext& context, const VulkanFrameData& frameData,
											 const uint32_t frame) {
	FrameUploads& uploads = m_frames[frame];
	if (!uploads.inFlight)
		return;

	if (const VkResult result = frameData.waitForFences(context, frame); result != VK_SUCCESS)
		throw VulkanError("failed to wait for the GPU to finish reading staged mesh data", result);

	for (const StagingBuffer& stagingBuffer: uploads.stagingBuffers)
		stagingBuffer.ranges.reset();
	uploads.inFlight = false;
}

VulkanUploadQueue::StagedData VulkanUploadQueue::reserveStagingMemory(const VulkanContext& context, FrameUploads& frame,
																	  const VkDeviceSize bytes) {
	for (StagingBuffer& stagingBuffer: frame.stagingBuffers) {
		if (const auto range = stagingBuffer.ranges.allocate(bytes, stagingAlignment))
			return {.stagingBuffer = &stagingBuffer, .offset = range->offset};
	}

	const VkDeviceSize capacity   = std::max(defaultStagingBufferBytes, bytes);
	VkBuffer           buffer     = nullptr;
	VmaAllocation      allocation = nullptr;
	VulkanResourceManager::createBuffer(context, capacity, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, buffer, allocation);

	StagingBuffer& created = frame.stagingBuffers.emplace_back(buffer, allocation, RangeAllocator(capacity));
	const auto     range   = created.ranges.allocate(bytes, stagingAlignment);
	if (!range)
		throw VulkanError("a new staging buffer is too small for the data it was made for");
	return {.stagingBuffer = &created, .offset = range->offset};
}

void VulkanUploadQueue::upload(const VulkanContext& context, const VulkanFrameData& frameData,
							   const std::span<const std::byte> data, VkBuffer destination,
							   const VkDeviceSize destinationOffset) {
	if (data.empty())
		return;

	const uint32_t frame = frameData.getCurrentFrame();
	reclaimStagingMemory(context, frameData, frame);

	FrameUploads&    uploads = m_frames[frame];
	const StagedData staged  = reserveStagingMemory(context, uploads, data.size_bytes());

	if (const VkResult result = vmaCopyMemoryToAllocation(
			context.getAllocator(), data.data(), staged.stagingBuffer->allocation, staged.offset, data.size_bytes());
		result != VK_SUCCESS)
		throw VulkanError("failed to fill staging memory", result);

	uploads.copies.push_back(
		{.source      = staged.stagingBuffer->buffer,
		 .destination = destination,
		 .region      = {.srcOffset = staged.offset, .dstOffset = destinationOffset, .size = data.size_bytes()}});
}

void VulkanUploadQueue::record(VkCommandBuffer commandBuffer, const uint32_t frame) {
	FrameUploads& uploads = m_frames[frame];
	if (uploads.copies.empty())
		return;

	for (const PendingCopy& copy: uploads.copies)
		vkCmdCopyBuffer(commandBuffer, copy.source, copy.destination, 1, &copy.region);

	constexpr VkMemoryBarrier barrier{.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
									  .pNext         = nullptr,
									  .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
									  .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT};
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1,
						 &barrier, 0, nullptr, 0, nullptr);

	uploads.copies.clear();
	uploads.inFlight = true;
}

void VulkanUploadQueue::cleanup(const VulkanContext& context) {
	for (FrameUploads& uploads: m_frames) {
		for (const StagingBuffer& stagingBuffer: uploads.stagingBuffers)
			vmaDestroyBuffer(context.getAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
		uploads.stagingBuffers.clear();
		uploads.copies.clear();
		uploads.inFlight = false;
	}
}
