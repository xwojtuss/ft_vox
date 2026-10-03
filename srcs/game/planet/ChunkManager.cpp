#include "game/planet/ChunkManager.hpp"

#include <array>
#include <ranges>

#include "error/Assert.hpp"
#include "render/IRenderer.hpp"
#include "ecs/component/Components.hpp"

using namespace game::planet;

ChunkManager::ChunkManager(block::BlockDatas& blockDatas, ecs::Registry& registry, render::IRenderer& renderer,
						   profiling::ServerStats& stats, const Seed seed,
						   const glm::vec<3, unsigned short> renderDistance) :
	m_chunkMesher(blockDatas), m_chunkLoader(seed), m_blockDatas(blockDatas), m_registry(registry), m_stats(stats),
	m_renderer(renderer), m_renderDistance(renderDistance) {
	m_chunkTexture = renderer.createTexture(m_blockDatas.getBlockData(1).textureData);

	const int halfWidth = m_renderDistance.x / 2;
	const int halfDepth = m_renderDistance.z / 2;
	const int height    = m_renderDistance.y;

	loadRange({-halfWidth, 0, -halfDepth}, {halfWidth, height - 1, halfDepth});
}

void ChunkManager::makeChunkRenderable(ecs::Registry& registry, render::IRenderer& renderer, glm::ivec3 chunkPosition) {
	const auto it = m_chunks.find(chunkPosition);
	if (it == m_chunks.end() || m_chunkEntities.contains(chunkPosition))
		return;
	if (it->second->isEmpty() || isBuried(chunkPosition))
		return;

	const assets::ChunkMeshData meshData = m_chunkMesher.toMeshData(*it->second, neighboursOf(chunkPosition));
	if (meshData.vertices.empty() || meshData.indices.empty())
		return;

	ecs::EntityHandle chunkEntity = registry.createEntity();

	chunkEntity.add(
		ecs::component::Mesh{.mesh = renderer.createMesh(meshData), .pipelineType = assets::PipelineType::Chunk});
	chunkEntity.add(ecs::component::Texture{.texture = m_chunkTexture});
	chunkEntity.add(ecs::component::Transform{
		.position = glm::vec3(chunkPosition * glm::ivec3(chunkXSize, chunkYSize, chunkZSize))});
	m_chunkEntities[chunkPosition] = chunkEntity.id();
}

void ChunkManager::removeChunkEntity(const glm::ivec3 chunkPosition) {
	const auto it = m_chunkEntities.find(chunkPosition);
	if (it == m_chunkEntities.end())
		return;

	ecs::EntityHandle chunkEntity = m_registry.getEntity(it->second);
	if (const auto* mesh = chunkEntity.tryGet<ecs::component::Mesh>(); mesh != nullptr)
		m_renderer.destroyMesh(mesh->mesh);
	m_registry.destroyEntity(it->second);
	m_chunkEntities.erase(it);
}

void ChunkManager::refreshStats() const {
	m_stats.loadedChunks   = m_chunks.size();
	m_stats.chunkDataBytes = 0;
	for (const auto& chunk: m_chunks | std::views::values)
		m_stats.chunkDataBytes += chunk->dataBytes();
}

bool ChunkManager::isBuried(const glm::ivec3 chunkPosition) const {
	if (!m_chunks.at(chunkPosition)->isFull())
		return false;

	constexpr std::array axisSizes = {chunkXSize, chunkYSize, chunkZSize};
	for (int axis = 0; axis < 3; ++axis) {
		for (const int direction: {-1, 1}) {
			glm::ivec3 neighbourPosition = chunkPosition;
			neighbourPosition[axis] += direction;

			const auto neighbour = m_chunks.find(neighbourPosition);
			if (neighbour == m_chunks.end())
				return false;

			if (const int touchingLayer = (direction > 0) ? 0 : axisSizes[static_cast<std::size_t>(axis)] - 1;
				!neighbour->second->isLayerSolid(axis, touchingLayer))
				return false;
		}
	}
	return true;
}

void ChunkManager::storeChunk(const glm::ivec3 chunkPosition) {
	removeChunkEntity(chunkPosition);
	m_chunks[chunkPosition] = m_chunkLoader.loadChunk(chunkPosition);
}

ChunkMesher::Neighbours ChunkManager::neighboursOf(const glm::ivec3 chunkPosition) const {
	ChunkMesher::Neighbours neighbours{};

	for (std::size_t face = 0; face < neighbours.size(); ++face) {
		if (const auto neighbour = m_chunks.find(chunkPosition + ChunkMesher::faceOffsets[face]);
			neighbour != m_chunks.end())
			neighbours[face] = neighbour->second.get();
	}
	return neighbours;
}

void ChunkManager::refreshMesh(const glm::ivec3 chunkPosition) {
	removeChunkEntity(chunkPosition);
	makeChunkRenderable(m_registry, m_renderer, chunkPosition);
}

void ChunkManager::refreshMeshesAround(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x - 1; x <= end.x + 1; ++x) {
		for (int y = start.y - 1; y <= end.y + 1; ++y) {
			for (int z = start.z - 1; z <= end.z + 1; ++z) {
				const int axesOutside = static_cast<int>(x < start.x || x > end.x) +
										static_cast<int>(y < start.y || y > end.y) +
										static_cast<int>(z < start.z || z > end.z);
				if (axesOutside <= 1 && m_chunks.contains({x, y, z}))
					refreshMesh({x, y, z});
			}
		}
	}
}

void ChunkManager::unloadChunk(glm::ivec3 chunkPosition) {
	removeChunkEntity(chunkPosition);
	m_chunks.erase(chunkPosition);
	refreshStats();
	refreshMeshesAround(chunkPosition, chunkPosition);
}

void ChunkManager::unloadChunk(int x, int y, int z) {
	unloadChunk({x, y, z});
}

void ChunkManager::unloadRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x) {
		for (int y = start.y; y <= end.y; ++y) {
			for (int z = start.z; z <= end.z; ++z) {
				removeChunkEntity({x, y, z});
				m_chunks.erase({x, y, z});
			}
		}
	}
	refreshStats();
	refreshMeshesAround(start, end);
}

void ChunkManager::loadRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x) {
		for (int y = start.y; y <= end.y; ++y) {
			for (int z = start.z; z <= end.z; ++z) {
				storeChunk({x, y, z});
			}
		}
	}
	refreshStats();
	refreshMeshesAround(start, end);
}

void ChunkManager::loadChunk(const glm::ivec3 chunkPosition) {
	storeChunk(chunkPosition);
	refreshStats();
	refreshMeshesAround(chunkPosition, chunkPosition);
}

void ChunkManager::setBlock(const glm::ivec3 chunkPosition, const glm::ivec3 blockPosition, const Block& block) {
	const auto chunk = m_chunks.find(chunkPosition);
	DEBUG_ASSERT(chunk != m_chunks.end(), "block set in a chunk that is not loaded", chunkPosition.x, chunkPosition.y,
				 chunkPosition.z);

	chunk->second->setBlock(blockPosition.x, blockPosition.y, blockPosition.z, block);
	refreshStats();

	refreshMesh(chunkPosition);
	constexpr std::array sizes = {chunkXSize, chunkYSize, chunkZSize};
	for (glm::length_t axis = 0; axis < 3; ++axis) {
		if (blockPosition[axis] == 0) {
			glm::ivec3 neighbour = chunkPosition;
			--neighbour[axis];
			if (m_chunks.contains(neighbour))
				refreshMesh(neighbour);
		}
		if (blockPosition[axis] == sizes[static_cast<std::size_t>(axis)] - 1) {
			glm::ivec3 neighbour = chunkPosition;
			++neighbour[axis];
			if (m_chunks.contains(neighbour))
				refreshMesh(neighbour);
		}
	}
}

void ChunkManager::loadChunk(int x, int y, int z) {
	loadChunk({x, y, z});
}
