#pragma once

#include <unordered_map>
#include <glm/vec3.hpp>
#include <memory>

#include "Chunk.hpp"
#include "ChunkMesher.hpp"
#include "ChunkLoader.hpp"
#include "../../ecs/World.hpp"
#include "../../scene/WorldInfo.hpp"

namespace game::world {
	class ChunkManager {
	private:
		std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> m_chunks;
		ChunkMesher                                            m_chunkMesher;
		ChunkLoader                                            m_chunkLoader;
		block::BlockDatas&                                     m_blockDatas;
		ecs::World&                                            m_world;
		render::IRenderer&                                     m_renderer;
		assets::TextureHandle                                  m_chunkTexture;
		glm::vec<3, unsigned short>                            m_renderDistance;

	public:
		ChunkManager(block::BlockDatas&         blockDatas, ecs::World& world, render::IRenderer& renderer,
					glm::vec<3, unsigned short> renderDistance = scene::worldinfo::renderDistance);

		void makeChunkRenderable(ecs::World& world, render::IRenderer& renderer, glm::ivec3 chunkPosition);
		void unloadChunk(glm::ivec3 chunkPosition);
		void unloadChunk(int x, int y, int z);
		void unloadRange(glm::ivec3 start, glm::ivec3 end);
		void loadChunk(glm::ivec3 chunkPosition);
		void loadChunk(int x, int y, int z);
		void loadRange(glm::ivec3 start, glm::ivec3 end);
	};
}
