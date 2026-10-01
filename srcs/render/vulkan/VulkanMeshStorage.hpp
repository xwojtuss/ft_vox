#pragma once

#include <optional>
#include <vector>
#include <volk.h>
#include <vk_mem_alloc.h>

#include "assets/Resources.hpp"
#include "render/vulkan/RangeAllocator.hpp"

namespace render::vulkan {
	class VulkanContext;

	struct GpuMesh {
		uint32_t             arena;
		int32_t              vertexOffset;
		uint32_t             firstIndex;
		uint32_t             indexCount;
		VmaVirtualAllocation vertexAllocation;
		VmaVirtualAllocation indexAllocation;
	};

	constexpr VkDeviceSize defaultVertexArenaBytes = 64ULL * 1024 * 1024;
	constexpr VkDeviceSize defaultIndexArenaBytes  = 16ULL * 1024 * 1024;

	class VulkanMeshStorage {
	private:
		struct Arena {
			VkBuffer       vertexBuffer;
			VmaAllocation  vertexMemory;
			VkBuffer       indexBuffer;
			VmaAllocation  indexMemory;
			RangeAllocator vertexRanges;
			RangeAllocator indexRanges;
		};

		std::vector<Arena> m_arenas;

		void addArena(const VulkanContext& context, VkDeviceSize vertexBytes, VkDeviceSize indexBytes);
		[[nodiscard]] std::optional<GpuMesh> allocate(VkDeviceSize vertexBytes, VkDeviceSize indexBytes) const;

	public:
		[[nodiscard]] GpuMesh create(const VulkanContext& context, VkCommandPool commandPool,
									 const assets::MeshData& meshData);
		void                  destroy(const GpuMesh& mesh) const;
		void                  cleanup(const VulkanContext& context);
		void                  logStatistics() const;

		[[nodiscard]] VkBuffer vertexBuffer(uint32_t arena) const;
		[[nodiscard]] VkBuffer indexBuffer(uint32_t arena) const;
		[[nodiscard]] size_t   arenaCount() const;
	};
}
