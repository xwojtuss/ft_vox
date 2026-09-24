#include "ChunkManager.hpp"
#include "../../scene/WorldInfo.hpp"
#include "../../ecs/component/Components.hpp"
#include "../../ecs/system/types/RenderSystem.hpp"

#include "entity/EntityHandle.hpp"

using namespace game::world;

ChunkManager::ChunkManager(block::BlockDatas&  blockDatas, ecs::World& world,
							render::IRenderer& renderer) : m_chunkMesher(blockDatas), m_blockDatas(blockDatas),
															m_world(world), m_renderer(renderer) {
	m_chunkTexture = renderer.createTexture(m_blockDatas.getBlockData(1).textureData);

	for (short x = -scene::worldinfo::maxHorizontalRenderDistance / 2;
		x <= scene::worldinfo::maxHorizontalRenderDistance / 2; ++x) {
		for (short z = -scene::worldinfo::maxHorizontalRenderDistance / 2;
			z <= scene::worldinfo::maxHorizontalRenderDistance / 2; ++z) {
			for (short y = 0; y < scene::worldinfo::maxVerticalRenderDistance; ++y) {
				loadChunk({x, y, z});
			}
		}
	}
}

void ChunkManager::makeAllChunksRenderable(const ecs::World& world, const render::IRenderer& renderer) const {
	// TODO: implement
	(void)world;
	(void)renderer;
}

void ChunkManager::makeChunkRenderable(ecs::World& world, render::IRenderer& renderer, glm::ivec3 chunkPosition) {
	auto it = m_chunks.find(chunkPosition);
	if (it == m_chunks.end())
		return;

	const assets::MeshData meshData = m_chunkMesher.toMeshData(*it->second);
	if (meshData.vertices.empty() || meshData.indices.empty())
		return;

	ecs::EntityHandle         chunkEntity = world.createEntity();
	ecs::component::Mesh      meshComponent;
	ecs::component::Texture   textureComponent;
	ecs::component::Transform positionComponent;

	meshComponent.mesh         = renderer.createMesh(meshData);
	meshComponent.pipelineType = assets::PipelineType::Textured;
	textureComponent.texture   = m_chunkTexture;
	positionComponent.position = glm::vec3(chunkPosition * glm::ivec3(chunkXSize, chunkYSize, chunkZSize));

	chunkEntity.addComponent(meshComponent);
	chunkEntity.addComponent(textureComponent);
	chunkEntity.addComponent(positionComponent);

	chunkEntity.registerToSystem<ecs::RenderSystem>();
}

void ChunkManager::unloadChunk(glm::ivec3 chunkPosition) {
	auto it = m_chunks.find(chunkPosition);
	if (it != m_chunks.end()) {
		m_chunks.erase(it);
	}
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
	m_chunks[chunkPosition] = m_chunkLoader.loadChunk(chunkPosition);

	makeChunkRenderable(m_world, m_renderer, chunkPosition);
}

void ChunkManager::loadChunk(int x, int y, int z) {
	loadChunk({x, y, z});
}
