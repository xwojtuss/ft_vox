#pragma once

#include <span>
#include <vector>
#include <vk_mem_alloc.h>

#include "assets/Resources.hpp"
#include "render/GpuTypes.hpp"

namespace render::vulkan {
	class VulkanContext;
	class VulkanResourceManager;

	constexpr unsigned int maxFramesInFlight   = 2;
	constexpr uint32_t     initialDrawCapacity = 4096;

	struct DrawBuffers {
		VkBuffer      objectBuffer{};
		VmaAllocation objectAllocation{};
		void*         objectMapped{};
		VkBuffer      indirectBuffer{};
		VmaAllocation indirectAllocation{};
		void*         indirectMapped{};
		uint32_t      capacity{};
	};

	class VulkanFrameData {
	private:
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkSemaphore>     m_imageAvailableSemaphores;
		std::vector<VkFence>         m_inFlightFences;
		std::vector<VkBuffer>        m_frameUBOs;
		std::vector<VmaAllocation>   m_frameUBOsAllocations;
		std::vector<void*>           m_frameUBOsMapped;
		VkDescriptorPool             m_descriptorPool{};
		std::vector<DrawBuffers>     m_drawBuffers;
		std::vector<VkDescriptorSet> m_frameDescriptorSets;
		VkDescriptorSetLayout        m_frameSetLayout{};
		VkDescriptorSetLayout        m_textureSetLayout{};
		uint32_t                     m_currentFrame{0};

		void createFrameUBOs(VulkanContext&);
		void createDrawBuffers(const VulkanContext&, size_t frame, uint32_t capacity);
		void destroyDrawBuffers(const VulkanContext&, size_t frame);
		void createCommandBuffers(const VulkanContext&, const VulkanResourceManager& resourceManager);
		void createSyncObjects(const VulkanContext&);
		void createFrameDescriptorSets(const VulkanContext&);
		void createFrameDescriptorSetLayout(const VulkanContext& context);
		void createTextureDescriptorSetLayout(const VulkanContext& context);
		void createDescriptorPool(const VulkanContext&);

	public:
		VulkanFrameData(VulkanContext&, const VulkanResourceManager&);

		[[nodiscard]] VkDescriptorSetLayout getFrameDescriptorSetLayout() const;
		[[nodiscard]] VkDescriptorSetLayout getTextureDescriptorSetLayout() const;
		[[nodiscard]] VkDescriptorSet       createTextureDescriptorSet(const VulkanContext& context) const;
		[[nodiscard]] VkResult              waitForFences(const VulkanContext& context, uint32_t currentFrame) const;
		void                                resetFences(const VulkanContext& context, uint32_t currentFrame) const;
		[[nodiscard]] VkCommandBuffer       getCommandBuffer(uint32_t index) const;
		[[nodiscard]] VkCommandBuffer       getCurrentCommandBuffer() const;
		void                                incrementCurrentFrame();
		void submitCommandBuffer(const VulkanContext& context, VkSemaphore renderFinishedSemaphore) const;
		[[nodiscard]] uint32_t    getCurrentFrame() const;
		VkDescriptorSet*          getDescriptorSet(uint32_t frameIndex);
		void                      writeCurrentFrameUBO(const VulkanContext& context, const FrameUBO& frameUbo) const;
		void                      uploadDraws(const VulkanContext& context, std::span<const ObjectData> objects,
											  std::span<const VkDrawIndexedIndirectCommand> commands);
		[[nodiscard]] VkBuffer    getCurrentIndirectBuffer() const;
		[[nodiscard]] VkSemaphore getCurrentImageAvailableSemaphore() const;
		void                      cleanup(const VulkanContext& context) const;

		static VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device);
		static void            endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool,
													 VkQueue graphicsQueue, VkDevice device);
	};
}
