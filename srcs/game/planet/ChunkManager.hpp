#pragma once

#include <unordered_map>
#include <glm/vec3.hpp>
#include <memory>

#include "game/planet/Chunk.hpp"
#include "game/planet/ChunkMesher.hpp"
#include "game/planet/ChunkLoader.hpp"
#include "ecs/Registry.hpp"
#include "profiling/ServerStats.hpp"
#include "scene/PlanetInfo.hpp"

namespace game::planet {
	class ChunkManager {
	private:
		std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> m_chunks;
		std::unordered_map<glm::ivec3, ecs::Entity>            m_chunkEntities;
		ChunkMesher                                            m_chunkMesher;
		ChunkLoader                                            m_chunkLoader;
		block::BlockDatas&                                     m_blockDatas;
		ecs::Registry&                                         m_registry;
		profiling::ServerStats&                                m_stats;
		render::IRenderer&                                     m_renderer;
		assets::TextureHandle                                  m_chunkTexture;
		glm::vec<3, unsigned short>                            m_renderDistance;

		void removeChunkEntity(glm::ivec3 chunkPosition);
		void refreshStats() const;

	public:
		ChunkManager(block::BlockDatas& blockDatas, ecs::Registry& registry, render::IRenderer& renderer,
					 profiling::ServerStats&     stats,
					 glm::vec<3, unsigned short> renderDistance = scene::planetinfo::renderDistance);

		void makeChunkRenderable(ecs::Registry& registry, render::IRenderer& renderer, glm::ivec3 chunkPosition);
		void unloadChunk(glm::ivec3 chunkPosition);
		void unloadChunk(int x, int y, int z);
		void unloadRange(glm::ivec3 start, glm::ivec3 end);
		void loadChunk(glm::ivec3 chunkPosition);
		void loadChunk(int x, int y, int z);
		void loadRange(glm::ivec3 start, glm::ivec3 end);
	};
}
