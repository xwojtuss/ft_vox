#pragma once

#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "concurrency/CancelToken.hpp"
#include "concurrency/IThreadPool.hpp"
#include "concurrency/ResultQueue.hpp"
#include "ecs/system/Dispatcher.hpp"
#include "game/Seed.hpp"
#include "game/planet/Chunk.hpp"
#include "game/planet/ChunkLoader.hpp"
#include "game/planet/ChunkOrder.hpp"
#include "game/planet/RenderDistance.hpp"
#include "profiling/ServerStats.hpp"
#include "scene/PlanetInfo.hpp"

namespace game::planet {
	struct ChunkStreamSettings {
		RenderDistance renderDistance = scene::planetinfo::renderDistance;

		std::size_t queuedJobsPerWorker = defaultQueuedJobsPerWorker;
	};

	class ChunkStreamer {
	private:
		struct GeneratedChunk {
			glm::ivec3               position;
			concurrency::CancelToken token;
			std::unique_ptr<Chunk>   chunk;
		};

		std::unordered_map<glm::ivec3, Chunk>                     m_chunks;
		std::unordered_set<glm::ivec3>                            m_waiting;
		std::unordered_map<glm::ivec3, concurrency::CancelToken>  m_inFlight;
		std::size_t                                               m_runningJobs = 0;
		std::shared_ptr<concurrency::ResultQueue<GeneratedChunk>> m_generated =
			std::make_shared<concurrency::ResultQueue<GeneratedChunk>>();
		ChunkLoader               m_chunkLoader;
		ecs::Dispatcher&          m_dispatcher;
		concurrency::IThreadPool& m_pool;
		profiling::ServerStats&   m_stats;
		ChunkStreamSettings       m_settings;
		Viewer                    m_viewer;
		std::optional<glm::ivec3> m_viewerChunk;

		void requestAroundViewer();
		void integrateGeneratedChunks();
		void startGenerationJobs();
		void refreshStats() const;

	public:
		ChunkStreamer(ecs::Dispatcher& dispatcher, concurrency::IThreadPool& pool, profiling::ServerStats& stats,
					  Seed seed, ChunkStreamSettings settings = {});
		ChunkStreamer(const ChunkStreamer&)            = delete;
		ChunkStreamer& operator=(const ChunkStreamer&) = delete;
		ChunkStreamer(ChunkStreamer&&)                 = delete;
		ChunkStreamer& operator=(ChunkStreamer&&)      = delete;
		~ChunkStreamer();

		void setViewer(const Viewer& viewer);

		void requestChunk(glm::ivec3 chunkPosition);
		void requestRange(glm::ivec3 start, glm::ivec3 end);
		void unloadChunk(glm::ivec3 chunkPosition);
		void unloadRange(glm::ivec3 start, glm::ivec3 end);
		void setBlock(glm::ivec3 chunkPosition, glm::ivec3 blockPosition, const Block& block);

		void update();

		[[nodiscard]] bool isLoaded(glm::ivec3 chunkPosition) const;
		[[nodiscard]] bool isIdle() const;
	};
}
