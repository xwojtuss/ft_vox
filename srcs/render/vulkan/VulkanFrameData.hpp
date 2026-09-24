#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "../../assets/Resources.hpp"

namespace render::vulkan {
	class VulkanContext;
	class VulkanResourceManager;
	struct GpuMesh;

	constexpr unsigned int maxFramesInFlight = 2;

	class VulkanFrameData {
	private:
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkSemaphore>     m_imageAvailableSemaphores;
		std::vector<VkFence>         m_inFlightFences;
		std::vector<VkBuffer>        m_frameUBOs;
		std::vector<VkDeviceMemory>  m_frameUBOsMemory;
		std::vector<void*>           m_frameUBOsMapped;
		VkDescriptorPool             m_descriptorPool{};
		std::vector<VkDescriptorSet> m_frameDescriptorSets;
		VkDescriptorSetLayout        m_frameSetLayout{};
		VkDescriptorSetLayout        m_textureSetLayout{};
		uint32_t                     m_currentFrame{0};

		void createFrameUBOs(VulkanContext&);
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
		VkDescriptorSet createTextureDescriptorSet(const VulkanContext& context) const;
		static void createVertexBuffer(VulkanContext&, VulkanResourceManager& resourceManager,
										const assets::MeshData& meshData, GpuMesh& mesh);
		static void createIndexBuffer(VulkanContext&, VulkanResourceManager& resourceManager,
									const assets::MeshData& meshData, GpuMesh& mesh);
		[[nodiscard]] VkResult waitForFences(const VulkanContext& context, uint32_t currentFrame) const;
		void resetFences(const VulkanContext& context, uint32_t currentFrame) const;
		[[nodiscard]] VkCommandBuffer getCommandBuffer(uint32_t index) const;
		[[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const;
		void incrementCurrentFrame();
		void submitCommandBuffer(const VulkanContext& context, VkSemaphore renderFinishedSemaphore) const;
		[[nodiscard]] uint32_t getCurrentFrame() const;
		VkDescriptorSet* getDescriptorSet(uint32_t frameIndex);
		[[nodiscard]] void* getCurrentMappedFrameUBO() const;
		[[nodiscard]] VkSemaphore getCurrentImageAvailableSemaphore() const;
		void cleanup(const VulkanContext& context) const;

		static VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device);
		static void            endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool,
										VkQueue                      graphicsQueue, VkDevice      device);
	};
}
