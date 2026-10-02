#include "render/vulkan/VulkanMeshStorage.hpp"
#include <algorithm>
#include "log/Log.hpp"
#include "render/ChunkVertex.hpp"
#include "render/GpuTypes.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanError.hpp"
#include "render/vulkan/VulkanFrameData.hpp"
#include "render/vulkan/VulkanResourceManager.hpp"

using namespace render::vulkan;

namespace {
	constexpr VkDeviceSize indexStride = sizeof(uint32_t);
	constexpr VkDeviceSize mebibyte    = 1024ULL * 1024;

	constexpr VmaAllocationCreateFlags arenaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	void upload(const VulkanContext& context, VkCommandPool commandPool, const MeshBytes& mesh, VkBuffer vertexBuffer,
				const VkDeviceSize vertexOffset, VkBuffer indexBuffer, const VkDeviceSize indexOffset) {
		const VkDeviceSize vertexBytes = mesh.vertices.size_bytes();
		const VkDeviceSize indexBytes  = mesh.indices.size_bytes();

		VkBuffer      stagingBuffer     = nullptr;
		VmaAllocation stagingAllocation = nullptr;
		VulkanResourceManager::createBuffer(context, vertexBytes + indexBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer,
											stagingAllocation);

		VmaAllocator allocator = context.getAllocator();
		VkResult result = vmaCopyMemoryToAllocation(allocator, mesh.vertices.data(), stagingAllocation, 0, vertexBytes);
		if (result == VK_SUCCESS)
			result =
				vmaCopyMemoryToAllocation(allocator, mesh.indices.data(), stagingAllocation, vertexBytes, indexBytes);
		if (result != VK_SUCCESS) {
			vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
			throw VulkanError("failed to fill staging buffer", result);
		}

		VkCommandBuffer commandBuffer =
			VulkanFrameData::beginSingleTimeCommands(commandPool, context.getLogicalDevice());

		const VkBufferCopy vertexCopy{.srcOffset = 0, .dstOffset = vertexOffset, .size = vertexBytes};
		vkCmdCopyBuffer(commandBuffer, stagingBuffer, vertexBuffer, 1, &vertexCopy);
		const VkBufferCopy indexCopy{.srcOffset = vertexBytes, .dstOffset = indexOffset, .size = indexBytes};
		vkCmdCopyBuffer(commandBuffer, stagingBuffer, indexBuffer, 1, &indexCopy);

		VulkanFrameData::endSingleTimeCommands(commandBuffer, commandPool, context.getGraphicsQueue(),
											   context.getLogicalDevice());
		vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	}
}

VkDeviceSize VulkanMeshStorage::vertexStride(const VertexKind kind) {
	return (kind == VertexKind::Chunk) ? sizeof(render::ChunkVertex) : sizeof(render::Vertex);
}

void VulkanMeshStorage::addArena(const VulkanContext& context, const VertexKind kind, const VkDeviceSize vertexBytes,
								 const VkDeviceSize indexBytes) {
	const VkDeviceSize arenaVertexBytes = std::max(defaultVertexArenaBytes, vertexBytes);
	const VkDeviceSize arenaIndexBytes  = std::max(defaultIndexArenaBytes, indexBytes);

	VkBuffer      vertexBuffer = nullptr;
	VmaAllocation vertexMemory = nullptr;
	VulkanResourceManager::createBuffer(context, arenaVertexBytes,
										VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
										arenaFlags, vertexBuffer, vertexMemory);

	VkBuffer      indexBuffer = nullptr;
	VmaAllocation indexMemory = nullptr;
	try {
		VulkanResourceManager::createBuffer(context, arenaIndexBytes,
											VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
											arenaFlags, indexBuffer, indexMemory);
	} catch (...) {
		vmaDestroyBuffer(context.getAllocator(), vertexBuffer, vertexMemory);
		throw;
	}

	m_arenas.push_back(Arena{.kind         = kind,
							 .vertexBuffer = vertexBuffer,
							 .vertexMemory = vertexMemory,
							 .indexBuffer  = indexBuffer,
							 .indexMemory  = indexMemory,
							 .vertexRanges = RangeAllocator(arenaVertexBytes),
							 .indexRanges  = RangeAllocator(arenaIndexBytes)});

	VkDeviceSize reservedBytes = 0;
	for (const Arena& arena: m_arenas)
		reservedBytes += arena.vertexRanges.size() + arena.indexRanges.size();

	logging::get(error::Domain::Render)
		.info("Mesh storage arena {} added ({} MiB of vertices, {} MiB of indices), {} MiB reserved in {} arenas",
			  m_arenas.size() - 1, arenaVertexBytes / mebibyte, arenaIndexBytes / mebibyte, reservedBytes / mebibyte,
			  m_arenas.size());
}

std::optional<GpuMesh> VulkanMeshStorage::allocate(const VertexKind kind, const VkDeviceSize vertexBytes,
												   const VkDeviceSize indexBytes) const {
	const VkDeviceSize stride = vertexStride(kind);
	for (uint32_t i = 0; i < m_arenas.size(); ++i) {
		if (m_arenas[i].kind != kind)
			continue;
		const auto vertex = m_arenas[i].vertexRanges.allocate(vertexBytes, stride);
		if (!vertex)
			continue;
		const auto index = m_arenas[i].indexRanges.allocate(indexBytes, indexStride);
		if (!index) {
			m_arenas[i].vertexRanges.free(vertex->allocation);
			continue;
		}
		return GpuMesh{.arena            = i,
					   .vertexOffset     = static_cast<int32_t>(vertex->offset / stride),
					   .firstIndex       = static_cast<uint32_t>(index->offset / indexStride),
					   .indexCount       = static_cast<uint32_t>(indexBytes / indexStride),
					   .vertexAllocation = vertex->allocation,
					   .indexAllocation  = index->allocation};
	}
	return std::nullopt;
}

GpuMesh VulkanMeshStorage::create(const VulkanContext& context, VkCommandPool commandPool, const MeshBytes& meshBytes) {
	const VkDeviceSize vertexBytes = meshBytes.vertices.size_bytes();
	const VkDeviceSize indexBytes  = meshBytes.indices.size_bytes();
	const VkDeviceSize stride      = vertexStride(meshBytes.kind);

	std::optional<GpuMesh> mesh = allocate(meshBytes.kind, vertexBytes, indexBytes);
	if (!mesh) {
		addArena(context, meshBytes.kind, vertexBytes, indexBytes);
		mesh = allocate(meshBytes.kind, vertexBytes, indexBytes);
	}
	if (!mesh)
		throw VulkanError("a mesh does not fit into a new mesh storage arena");

	const Arena& arena = m_arenas[mesh->arena];
	try {
		upload(context, commandPool, meshBytes, arena.vertexBuffer,
			   static_cast<VkDeviceSize>(mesh->vertexOffset) * stride, arena.indexBuffer,
			   static_cast<VkDeviceSize>(mesh->firstIndex) * indexStride);
	} catch (...) {
		destroy(*mesh);
		throw;
	}
	return *mesh;
}

void VulkanMeshStorage::destroy(const GpuMesh& mesh) const {
	m_arenas[mesh.arena].vertexRanges.free(mesh.vertexAllocation);
	m_arenas[mesh.arena].indexRanges.free(mesh.indexAllocation);
}

void VulkanMeshStorage::logStatistics() const {
	for (size_t i = 0; i < m_arenas.size(); ++i) {
		const RangeStatistics vertices = m_arenas[i].vertexRanges.statistics();
		const RangeStatistics indices  = m_arenas[i].indexRanges.statistics();
		logging::get(error::Domain::Render)
			.info("Mesh storage arena {}: {} meshes, vertices {} of {} KiB used ({} free ranges, largest {} KiB), "
				  "indices {} of {} KiB used ({} free ranges, largest {} KiB)",
				  i, vertices.allocationCount, vertices.allocatedBytes / 1024, m_arenas[i].vertexRanges.size() / 1024,
				  vertices.freeRangeCount, vertices.largestFreeRange / 1024, indices.allocatedBytes / 1024,
				  m_arenas[i].indexRanges.size() / 1024, indices.freeRangeCount, indices.largestFreeRange / 1024);
	}
}

void VulkanMeshStorage::cleanup(const VulkanContext& context) {
	logStatistics();
	for (const Arena& arena: m_arenas) {
		vmaDestroyBuffer(context.getAllocator(), arena.vertexBuffer, arena.vertexMemory);
		vmaDestroyBuffer(context.getAllocator(), arena.indexBuffer, arena.indexMemory);
	}
	m_arenas.clear();
}

VkBuffer VulkanMeshStorage::vertexBuffer(const uint32_t arena) const {
	return m_arenas[arena].vertexBuffer;
}

VkBuffer VulkanMeshStorage::indexBuffer(const uint32_t arena) const {
	return m_arenas[arena].indexBuffer;
}

size_t VulkanMeshStorage::arenaCount() const {
	return m_arenas.size();
}
