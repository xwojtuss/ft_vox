#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>
#include <volk.h>
#include <vk_mem_alloc.h>

#include "render/vulkan/RangeAllocator.hpp"
#include "render/vulkan/VulkanFrameData.hpp"

namespace render::vulkan {
	class VulkanContext;

	constexpr VkDeviceSize defaultStagingBufferBytes = 16ULL * 1024 * 1024;

	class VulkanUploadQueue {
	private:
		struct StagingBuffer {
			VkBuffer       buffer{};
			VmaAllocation  allocation{};
			RangeAllocator ranges;
		};

		struct StagedData {
			StagingBuffer* stagingBuffer{};
			VkDeviceSize   offset{};
		};

		struct PendingCopy {
			VkBuffer     source{};
			VkBuffer     destination{};
			VkBufferCopy region{};
		};

		struct FrameUploads {
			std::vector<StagingBuffer> stagingBuffers;
			std::vector<PendingCopy>   copies;
			bool                       inFlight = false;
		};

		std::array<FrameUploads, maxFramesInFlight> m_frames;

		void reclaimStagingMemory(const VulkanContext& context, const VulkanFrameData& frameData, uint32_t frame);
		[[nodiscard]] static StagedData reserveStagingMemory(const VulkanContext& context, FrameUploads& frame,
															 VkDeviceSize bytes);

	public:
		void upload(const VulkanContext& context, const VulkanFrameData& frameData, std::span<const std::byte> data,
					VkBuffer destination, VkDeviceSize destinationOffset);
		void record(VkCommandBuffer commandBuffer, uint32_t frame);
		void cleanup(const VulkanContext& context);
	};
}
