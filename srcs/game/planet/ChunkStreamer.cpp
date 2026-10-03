#include "game/planet/ChunkStreamer.hpp"

#include <ranges>
#include <utility>
#include <vector>

#include "error/Assert.hpp"
#include "game/planet/ChunkEvents.hpp"
#include "game/planet/ChunkPriority.hpp"

using namespace game::planet;

ChunkStreamer::ChunkStreamer(ecs::Dispatcher& dispatcher, concurrency::IThreadPool& pool, profiling::ServerStats& stats,
							 const Seed seed, const ChunkStreamSettings settings) :
	m_chunkLoader(seed), m_dispatcher(dispatcher), m_pool(pool), m_stats(stats), m_settings(settings) {
}

ChunkStreamer::~ChunkStreamer() {
	for (const auto& token: m_pendingGeneration | std::views::values)
		token.cancel();
	m_pool.waitUntilIdle();
}

void ChunkStreamer::requestSpawnArea() {
	const int halfWidth = m_settings.renderDistance.x / 2;
	const int halfDepth = m_settings.renderDistance.z / 2;
	const int height    = m_settings.renderDistance.y;

	requestRange({-halfWidth, 0, -halfDepth}, {halfWidth, height - 1, halfDepth});
}

void ChunkStreamer::requestChunk(const glm::ivec3 chunkPosition) {
	concurrency::cancelAndErase(m_pendingGeneration, chunkPosition);

	const concurrency::CancelToken token;
	m_pendingGeneration[chunkPosition] = token;
	m_dispatcher.emit(ChunkRequestedEvent(chunkPosition));

	m_pool.submit(chunkPriority(ChunkJob::Generate, chunkPosition, m_priorityCenter),
				  [loader = &m_chunkLoader, chunkPosition, token, results = m_generated] {
					  if (token.isCancelled())
						  return;
					  results->push(
						  {.position = chunkPosition, .token = token, .chunk = loader->loadChunk(chunkPosition)});
				  });
}

void ChunkStreamer::requestRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x)
		for (int y = start.y; y <= end.y; ++y)
			for (int z = start.z; z <= end.z; ++z)
				requestChunk({x, y, z});
}

void ChunkStreamer::unloadChunk(const glm::ivec3 chunkPosition) {
	const bool wasPending = concurrency::cancelAndErase(m_pendingGeneration, chunkPosition);
	const bool wasLoaded  = m_chunks.erase(chunkPosition) > 0;

	if (wasPending || wasLoaded) {
		refreshStats();
		m_dispatcher.emit(ChunkUnloadedEvent(chunkPosition));
	}
}

void ChunkStreamer::unloadRange(const glm::ivec3 start, const glm::ivec3 end) {
	for (int x = start.x; x <= end.x; ++x)
		for (int y = start.y; y <= end.y; ++y)
			for (int z = start.z; z <= end.z; ++z)
				unloadChunk({x, y, z});
}

void ChunkStreamer::setBlock(const glm::ivec3 chunkPosition, const glm::ivec3 blockPosition, const Block& block) {
	const auto chunk = m_chunks.find(chunkPosition);
	DEBUG_ASSERT(chunk != m_chunks.end(), "block set in a chunk that is not loaded", chunkPosition.x, chunkPosition.y,
				 chunkPosition.z);

	chunk->second.setBlock(blockPosition.x, blockPosition.y, blockPosition.z, block);
	refreshStats();
	m_dispatcher.emit(ChunkChangedEvent(chunkPosition, blockPosition, chunk->second));
}

void ChunkStreamer::update() {
	std::vector<GeneratedChunk> generated = m_generated->drain();

	for (auto& [position, token, chunk]: generated) {
		if (token.isCancelled())
			continue;

		m_pendingGeneration.erase(position);
		const Chunk& stored = m_chunks.insert_or_assign(position, std::move(*chunk)).first->second;
		m_dispatcher.emit(ChunkLoadedEvent(position, stored));
	}

	if (!generated.empty())
		refreshStats();
	m_stats.pendingGeneration = m_pendingGeneration.size();
}

bool ChunkStreamer::isLoaded(const glm::ivec3 chunkPosition) const {
	return m_chunks.contains(chunkPosition);
}

bool ChunkStreamer::isIdle() const {
	return m_pendingGeneration.empty();
}

void ChunkStreamer::refreshStats() const {
	m_stats.loadedChunks   = m_chunks.size();
	m_stats.chunkDataBytes = 0;
	for (const Chunk& chunk: m_chunks | std::views::values)
		m_stats.chunkDataBytes += chunk.dataBytes();
}
