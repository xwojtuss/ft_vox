#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "concurrency/CancelToken.hpp"
#include "concurrency/IThreadPool.hpp"
#include "concurrency/ResultQueue.hpp"
#include "ecs/Registry.hpp"
#include "game/planet/Chunk.hpp"
#include "game/planet/ChunkEvents.hpp"
#include "game/planet/ChunkMesher.hpp"
#include "profiling/ClientStats.hpp"

namespace render::chunks {
	constexpr std::size_t defaultMeshUploadsPerFrame = 64;

	struct ChunkRenderSettings {
		std::size_t meshUploadsPerFrame = defaultMeshUploadsPerFrame;
	};

	class ChunkRenderer {
	private:
		struct MeshedChunk {
			glm::ivec3               position;
			concurrency::CancelToken token;
			assets::ChunkMeshData    mesh;
		};

		std::unordered_map<glm::ivec3, game::planet::Chunk>      m_chunks;
		std::unordered_set<glm::ivec3>                           m_expected;
		std::unordered_map<glm::ivec3, ecs::Entity>              m_chunkEntities;
		std::unordered_set<glm::ivec3>                           m_dirty;
		std::unordered_map<glm::ivec3, concurrency::CancelToken> m_meshJobs;
		std::deque<MeshedChunk>                                  m_readyMeshes;
		std::shared_ptr<concurrency::ResultQueue<MeshedChunk>>   m_meshed =
			std::make_shared<concurrency::ResultQueue<MeshedChunk>>();
		game::planet::ChunkMesher m_chunkMesher;
		ecs::Registry&            m_registry;
		IRenderer&                m_renderer;
		concurrency::IThreadPool& m_pool;
		profiling::ClientStats&   m_stats;
		assets::TextureHandle     m_chunkTexture;
		ChunkRenderSettings       m_settings;
		glm::ivec3                m_priorityCenter{0};

		using NeighbourSnapshots = std::array<std::optional<game::planet::Chunk>, game::planet::chunkFaceCount>;

		[[nodiscard]] bool               isBuried(glm::ivec3 chunkPosition) const;
		[[nodiscard]] bool               isReadyToMesh(glm::ivec3 chunkPosition) const;
		[[nodiscard]] NeighbourSnapshots snapshotNeighbours(glm::ivec3 chunkPosition) const;
		[[nodiscard]] concurrency::Task  makeMeshJob(glm::ivec3                      chunkPosition,
													 const concurrency::CancelToken& token) const;

		void removeChunkEntity(glm::ivec3 chunkPosition);
		void markDirty(glm::ivec3 chunkPosition);
		void markDirtyWithNeighbours(glm::ivec3 chunkPosition);
		void startReadyMeshJobs();
		void startMeshing(glm::ivec3 chunkPosition);
		void submitMeshJob(glm::ivec3 chunkPosition);
		void integrateMeshes();
		void applyMesh(const MeshedChunk& meshed);

	public:
		ChunkRenderer(game::block::BlockDatas& blockDatas, ecs::Registry& registry, IRenderer& renderer,
					  concurrency::IThreadPool& pool, profiling::ClientStats& stats, ChunkRenderSettings settings = {});
		ChunkRenderer(const ChunkRenderer&)            = delete;
		ChunkRenderer& operator=(const ChunkRenderer&) = delete;
		ChunkRenderer(ChunkRenderer&&)                 = delete;
		ChunkRenderer& operator=(ChunkRenderer&&)      = delete;
		~ChunkRenderer();

		void onChunkRequested(const game::planet::ChunkRequestedEvent& event);
		void onChunkLoaded(const game::planet::ChunkLoadedEvent& event);
		void onChunkUnloaded(const game::planet::ChunkUnloadedEvent& event);
		void onChunkChanged(const game::planet::ChunkChangedEvent& event);

		/** Starts meshing what is ready and puts finished meshes into the world. Call once per frame. */
		void update();

		[[nodiscard]] bool isIdle() const;
	};
}
