#pragma once

#include <unordered_map>
#include <vk_mem_alloc.h>

#include "render/vulkan/VulkanMeshStorage.hpp"
#include "render/vulkan/VulkanSwapchain.hpp"
#include "assets/Resources.hpp"

namespace render::vulkan {
	class VulkanContext;
	class VulkanFrameData;

	struct GpuTexture {
		SwapChainImage  image;
		VkSampler       sampler;
		VkDescriptorSet descriptorSet;
		uint32_t        mipLevels;
	};

	constexpr uint32_t textureLimit = 10;

	struct DestroyedMesh {
		GpuMesh  mesh;
		unsigned framesLeft;
	};

	class VulkanResourceManager {
	public:
		explicit VulkanResourceManager(const VulkanContext&);
		~VulkanResourceManager();

		[[nodiscard]] VkCommandPool getCommandPool() const;
		assets::MeshHandle          createMesh(const VulkanContext& context, const assets::MeshData& meshData);
		void                        destroyMesh(assets::MeshHandle handle);
		void                        releaseDestroyedMeshes();
		[[nodiscard]] const VulkanMeshStorage& getMeshStorage() const;
		assets::TextureHandle        createTexture(const assets::TextureData&, VulkanContext&, const VulkanFrameData&);
		[[nodiscard]] const GpuMesh& getMesh(assets::MeshHandle handle) const;
		[[nodiscard]] const GpuTexture& getTexture(assets::TextureHandle handle) const;
		[[nodiscard]] size_t            getTextureCount() const;
		void                            cleanup(const VulkanContext& context);

		static void createBuffer(const VulkanContext& context, VkDeviceSize size, VkBufferUsageFlags usage,
								 VmaAllocationCreateFlags allocationFlags, VkBuffer& buffer, VmaAllocation& allocation,
								 VmaAllocationInfo* allocationInfo = nullptr);
		static void createStagingBuffer(const VulkanContext& context, const void* data, VkDeviceSize size,
										VkBuffer& buffer, VmaAllocation& allocation);

	private:
		VkCommandPool                            m_commandPool{};
		VulkanMeshStorage                        m_meshStorage;
		std::unordered_map<uint64_t, GpuMesh>    m_meshes;
		std::vector<DestroyedMesh>               m_destroyedMeshes;
		std::unordered_map<uint64_t, GpuTexture> m_textures;

		void createCommandPool(const VulkanContext&);
		void copyBufferToImage(const VulkanContext& context, VkBuffer buffer, VkImage image, uint32_t width,
							   uint32_t height) const;
		void generateMipmaps(const VulkanContext& context, VkImage image, VkFormat imageFormat, int32_t texWidth,
							 int32_t texHeight, uint32_t mipLevels) const;
		void transitionImageLayout(const VulkanContext& context, VkImage image, VkFormat format,
								   VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const;
		[[nodiscard]] SwapChainImage createTextureImage(const assets::TextureData& textureData,
														const VulkanContext&       context) const;
		static VkImageView createTextureImageView(const assets::TextureData& textureData, const VulkanContext& context,
												  VkImage textureImage);
		static VkSampler   createTextureSampler(const VulkanContext& context);
		static VkSampler   createPixelPerfectTextureSampler(const VulkanContext& context);
	};
}
