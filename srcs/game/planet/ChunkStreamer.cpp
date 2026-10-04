#include "game/planet/ChunkStreamer.hpp"

#include <glm/vector_relational.hpp>
#include <ranges>
#include <utility>
#include <vector>

#include "error/Assert.hpp"
#include "game/planet/ChunkEvents.hpp"

using namespace game::planet;

ChunkStreamer::ChunkStreamer(ecs::Dispatcher& dispatcher, concurrency::IThreadPool& pool, profiling::ServerStats& stats,
							 const Seed seed, const ChunkStreamSettings settings) :
	m_chunkLoader(seed), m_dispatcher(dispatcher), m_pool(pool), m_stats(stats), m_settings(settings) {
}

ChunkStreamer::~ChunkStreamer() {
	for (const auto& token: m_inFlight | std::views::values)
		token.cancel();
	m_pool.waitUntilIdle();
}

void ChunkStreamer::setViewer(const Viewer& viewer) {
	m_viewer = viewer;
}

void ChunkStreamer::requestChunk(const glm::ivec3 chunkPosition) {
	concurrency::cancelAndErase(m_inFlight, chunkPosition);
	m_waiting.insert(chunkPosition);
	m_dispatcher.emit(ChunkRequestedEvent(chunkPosition));
}

void ChunkStreamer::requestRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x)
		for (int y = start.y; y <= end.y; ++y)
			for (int z = start.z; z <= end.z; ++z)
				requestChunk({x, y, z});
}

void ChunkStreamer::unloadChunk(const glm::ivec3 chunkPosition) {
	removeChunk(chunkPosition);
	refreshStats();
}

void ChunkStreamer::unloadRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x)
		for (int y = start.y; y <= end.y; ++y)
			for (int z = start.z; z <= end.z; ++z)
				removeChunk({x, y, z});
	refreshStats();
}

bool ChunkStreamer::eraseLoadedChunk(const glm::ivec3 chunkPosition) {
	const auto chunk = m_chunks.find(chunkPosition);
	if (chunk == m_chunks.end())
		return false;

	m_chunkDataBytes -= chunk->second.dataBytes();
	m_chunks.erase(chunk);
	return true;
}

void ChunkStreamer::removeChunk(const glm::ivec3 chunkPosition) {
	const bool wasWaiting  = m_waiting.erase(chunkPosition) > 0;
	const bool wasInFlight = concurrency::cancelAndErase(m_inFlight, chunkPosition);
	const bool wasLoaded   = eraseLoadedChunk(chunkPosition);

	if (wasWaiting || wasInFlight || wasLoaded)
		m_dispatcher.emit(ChunkUnloadedEvent(chunkPosition));
}

void ChunkStreamer::setBlock(const glm::ivec3 chunkPosition, const glm::ivec3 blockPosition, const Block& block) {
	const auto chunk = m_chunks.find(chunkPosition);
	DEBUG_ASSERT(chunk != m_chunks.end(), "block set in a chunk that is not loaded", chunkPosition.x, chunkPosition.y,
				 chunkPosition.z);

	m_chunkDataBytes -= chunk->second.dataBytes();
	chunk->second.setBlock(blockPosition.x, blockPosition.y, blockPosition.z, block);
	m_chunkDataBytes += chunk->second.dataBytes();
	refreshStats();
	m_dispatcher.emit(ChunkChangedEvent(chunkPosition, blockPosition, chunk->second));
}

void ChunkStreamer::update() {
	integrateGeneratedChunks();
	followViewer();
	startGenerationJobs();
	refreshStats();
}

void ChunkStreamer::followViewer() {
	const glm::ivec3 viewerChunk = chunkContaining(m_viewer.position);
	if (m_viewerChunk == viewerChunk)
		return;
	m_viewerChunk = viewerChunk;

	if (glm::any(glm::equal(m_settings.renderDistance, RenderDistance(0))))
		return;

	unloadFarChunks(viewerChunk);
	requestNearChunks(viewerChunk);
}

void ChunkStreamer::unloadFarChunks(const glm::ivec3 viewerChunk) {
	const RenderDistance keptDistance = m_settings.renderDistance + m_settings.unloadMargin;
	const auto           isFar        = [&](const glm::ivec3& position) {
        return !isWithinRenderDistance(position - viewerChunk, keptDistance);
	};

	std::vector<glm::ivec3> farChunks;
	const auto              collectFar = [&](const auto& positions) {
        for (const glm::ivec3& position: positions) {
            if (isFar(position))
                farChunks.push_back(position);
        }
	};
	collectFar(m_chunks | std::views::keys);
	collectFar(m_waiting);
	collectFar(m_inFlight | std::views::keys);

	for (const glm::ivec3& position: farChunks)
		removeChunk(position);
	refreshStats();
}

void ChunkStreamer::requestNearChunks(const glm::ivec3 viewerChunk) {
	const glm::ivec3 reach = renderDistanceReach(m_settings.renderDistance);

	for (int x = -reach.x; x <= reach.x; ++x) {
		for (int y = -reach.y; y <= reach.y; ++y) {
			for (int z = -reach.z; z <= reach.z; ++z) {
				if (!isWithinRenderDistance({x, y, z}, m_settings.renderDistance))
					continue;

				const glm::ivec3 position = viewerChunk + glm::ivec3(x, y, z);
				if (!m_chunks.contains(position) && !m_waiting.contains(position) && !m_inFlight.contains(position))
					requestChunk(position);
			}
		}
	}
}

void ChunkStreamer::integrateGeneratedChunks() {
	for (GeneratedChunk& result: m_generated->drain()) {
		--m_runningJobs;
		if (result.token.isCancelled() || !result.chunk)
			continue;

		m_inFlight.erase(result.position);
		eraseLoadedChunk(result.position);
		const Chunk& stored = m_chunks.emplace(result.position, std::move(*result.chunk)).first->second;
		m_chunkDataBytes += stored.dataBytes();
		m_dispatcher.emit(ChunkLoadedEvent(result.position, stored));
	}
}

void ChunkStreamer::startGenerationJobs() {
	const std::size_t queueLimit = jobQueueLimit(m_pool, m_settings.queuedJobsPerWorker);
	if (m_waiting.empty() || m_runningJobs >= queueLimit)
		return;

	const std::vector waiting(m_waiting.begin(), m_waiting.end());
	for (const glm::ivec3& position: mostUrgentChunks(waiting, queueLimit - m_runningJobs, m_viewer)) {
		m_waiting.erase(position);

		const concurrency::CancelToken token;
		m_inFlight[position] = token;
		++m_runningJobs;

		m_pool.submit(generationJobPriority, [loader = &m_chunkLoader, position, token, results = m_generated] {
			if (token.isCancelled()) {
				results->push({.position = position, .token = token, .chunk = nullptr});
				return;
			}
			results->push({.position = position, .token = token, .chunk = loader->loadChunk(position)});
		});
	}
}

bool ChunkStreamer::isLoaded(const glm::ivec3 chunkPosition) const {
	return m_chunks.contains(chunkPosition);
}

bool ChunkStreamer::isIdle() const {
	return m_waiting.empty() && m_inFlight.empty();
}

void ChunkStreamer::refreshStats() const {
	m_stats.loadedChunks      = m_chunks.size();
	m_stats.pendingGeneration = m_waiting.size() + m_inFlight.size();
	m_stats.chunkDataBytes    = m_chunkDataBytes;
}
