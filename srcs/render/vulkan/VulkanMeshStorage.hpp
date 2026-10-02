#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>
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

	enum class VertexKind : uint8_t { Model, Chunk };

	struct MeshBytes {
		VertexKind                 kind;
		std::span<const std::byte> vertices;
		std::span<const uint32_t>  indices;
	};

	class VulkanMeshStorage {
	private:
		struct Arena {
			VertexKind     kind;
			VkBuffer       vertexBuffer;
			VmaAllocation  vertexMemory;
			VkBuffer       indexBuffer;
			VmaAllocation  indexMemory;
			RangeAllocator vertexRanges;
			RangeAllocator indexRanges;
		};

		std::vector<Arena> m_arenas;

		void addArena(const VulkanContext& context, VertexKind kind, VkDeviceSize vertexBytes, VkDeviceSize indexBytes);
		[[nodiscard]] std::optional<GpuMesh> allocate(VertexKind kind, VkDeviceSize vertexBytes,
													  VkDeviceSize indexBytes) const;

	public:
		[[nodiscard]] static VkDeviceSize vertexStride(VertexKind kind);
		[[nodiscard]] GpuMesh create(const VulkanContext& context, VkCommandPool commandPool, const MeshBytes& mesh);
		void                  destroy(const GpuMesh& mesh) const;
		void                  cleanup(const VulkanContext& context);
		void                  logStatistics() const;

		[[nodiscard]] VkBuffer vertexBuffer(uint32_t arena) const;
		[[nodiscard]] VkBuffer indexBuffer(uint32_t arena) const;
		[[nodiscard]] size_t   arenaCount() const;
	};
}
