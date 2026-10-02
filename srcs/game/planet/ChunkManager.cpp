#include "game/planet/ChunkManager.hpp"

#include "render/IRenderer.hpp"
#include "ecs/component/Components.hpp"

using namespace game::planet;

ChunkManager::ChunkManager(block::BlockDatas& blockDatas, ecs::Registry& registry, render::IRenderer& renderer,
						   profiling::ServerStats& stats, const glm::vec<3, unsigned short> renderDistance) :
	m_chunkMesher(blockDatas), m_blockDatas(blockDatas), m_registry(registry), m_stats(stats), m_renderer(renderer),
	m_renderDistance(renderDistance) {
	m_chunkTexture = renderer.createTexture(m_blockDatas.getBlockData(1).textureData);

	const int halfWidth = m_renderDistance.x / 2;
	const int halfDepth = m_renderDistance.z / 2;
	const int height    = m_renderDistance.y;

	for (int x = -halfWidth; x <= halfWidth; ++x) {
		for (int z = -halfDepth; z <= halfDepth; ++z) {
			for (int y = 0; y < height; ++y) {
				loadChunk({x, y, z});
			}
		}
	}
}

void ChunkManager::makeChunkRenderable(ecs::Registry& registry, render::IRenderer& renderer, glm::ivec3 chunkPosition) {
	const auto it = m_chunks.find(chunkPosition);
	if (it == m_chunks.end() || m_chunkEntities.contains(chunkPosition))
		return;

	const assets::ChunkMeshData meshData = m_chunkMesher.toMeshData(*it->second);
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
	m_stats.chunkDataBytes = m_chunks.size() * sizeof(Chunk);
}

void ChunkManager::unloadChunk(glm::ivec3 chunkPosition) {
	removeChunkEntity(chunkPosition);
	m_chunks.erase(chunkPosition);
	refreshStats();
}

void ChunkManager::unloadChunk(int x, int y, int z) {
	unloadChunk({x, y, z});
}

void ChunkManager::unloadRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x) {
		for (int y = start.y; y <= end.y; ++y) {
			for (int z = start.z; z <= end.z; ++z) {
				unloadChunk({x, y, z});
			}
		}
	}
}

void ChunkManager::loadRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x) {
		for (int y = start.y; y <= end.y; ++y) {
			for (int z = start.z; z <= end.z; ++z) {
				loadChunk({x, y, z});
			}
		}
	}
}

void ChunkManager::loadChunk(const glm::ivec3 chunkPosition) {
	removeChunkEntity(chunkPosition);
	m_chunks[chunkPosition] = m_chunkLoader.loadChunk(chunkPosition);
	refreshStats();

	makeChunkRenderable(m_registry, m_renderer, chunkPosition);
}

void ChunkManager::loadChunk(int x, int y, int z) {
	loadChunk({x, y, z});
}
