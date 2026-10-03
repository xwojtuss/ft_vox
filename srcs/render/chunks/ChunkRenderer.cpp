#include "render/chunks/ChunkRenderer.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>
#include <utility>

#include "ecs/component/Components.hpp"
#include "game/planet/ChunkPriority.hpp"
#include "profiling/Profiler.hpp"
#include "render/IRenderer.hpp"

using namespace render::chunks;
using namespace game::planet;

namespace {
	ChunkMesher::Neighbours viewsOf(const std::array<std::optional<Chunk>, chunkFaceCount>& snapshots) {
		ChunkMesher::Neighbours views{};
		for (std::size_t face = 0; face < views.size(); ++face) {
			if (const std::optional<Chunk>& snapshot = snapshots[face]; snapshot.has_value())
				views[face] = &*snapshot;
		}
		return views;
	}
}

ChunkRenderer::ChunkRenderer(game::block::BlockDatas& blockDatas, ecs::Registry& registry, IRenderer& renderer,
							 concurrency::IThreadPool& pool, profiling::ClientStats& stats,
							 const ChunkRenderSettings settings) :
	m_chunkMesher(blockDatas), m_registry(registry), m_renderer(renderer), m_pool(pool), m_stats(stats),
	m_settings(settings) {
	m_chunkTexture = renderer.createTexture(blockDatas.getBlockData(1).textureData);
}

ChunkRenderer::~ChunkRenderer() {
	for (const auto& token: m_meshJobs | std::views::values)
		token.cancel();
	m_pool.waitUntilIdle();
}

void ChunkRenderer::onChunkRequested(const ChunkRequestedEvent& event) {
	m_expected.insert(event.position);
}

void ChunkRenderer::onChunkLoaded(const ChunkLoadedEvent& event) {
	m_expected.erase(event.position);
	m_chunks.insert_or_assign(event.position, event.chunk);
	markDirtyWithNeighbours(event.position);
}

void ChunkRenderer::onChunkUnloaded(const ChunkUnloadedEvent& event) {
	m_expected.erase(event.position);
	concurrency::cancelAndErase(m_meshJobs, event.position);
	m_dirty.erase(event.position);
	removeChunkEntity(event.position);

	if (m_chunks.erase(event.position) > 0)
		markDirtyWithNeighbours(event.position);
}

void ChunkRenderer::onChunkChanged(const ChunkChangedEvent& event) {
	m_chunks.insert_or_assign(event.position, event.chunk);

	markDirty(event.position);
	for (std::size_t index = 0; index < chunkFaceCount; ++index) {
		if (isBlockOnChunkFace(event.blockPosition, static_cast<ChunkFace>(index)))
			markDirty(event.position + chunkFaceOffsets[index]);
	}
}

void ChunkRenderer::update() {
	FT_PROFILE_FUNCTION();
	startReadyMeshJobs();
	integrateMeshes();

	m_stats.pendingMeshing = m_dirty.size() + m_meshJobs.size();
}

bool ChunkRenderer::isIdle() const {
	return m_expected.empty() && m_dirty.empty() && m_meshJobs.empty() && m_readyMeshes.empty();
}

void ChunkRenderer::markDirty(const glm::ivec3 chunkPosition) {
	if (m_chunks.contains(chunkPosition))
		m_dirty.insert(chunkPosition);
}

void ChunkRenderer::markDirtyWithNeighbours(const glm::ivec3 chunkPosition) {
	markDirty(chunkPosition);
	for (const glm::ivec3& offset: chunkFaceOffsets)
		markDirty(chunkPosition + offset);
}

bool ChunkRenderer::isReadyToMesh(const glm::ivec3 chunkPosition) const {
	if (!m_chunks.contains(chunkPosition) || m_expected.contains(chunkPosition))
		return false;

	return std::ranges::none_of(chunkFaceOffsets, [this, chunkPosition](const glm::ivec3& offset) {
		return m_expected.contains(chunkPosition + offset);
	});
}

void ChunkRenderer::startReadyMeshJobs() {
	for (auto dirty = m_dirty.begin(); dirty != m_dirty.end();) {
		if (!isReadyToMesh(*dirty)) {
			++dirty;
			continue;
		}

		const glm::ivec3 position = *dirty;
		dirty                     = m_dirty.erase(dirty);
		startMeshing(position);
	}
}

void ChunkRenderer::startMeshing(const glm::ivec3 chunkPosition) {
	concurrency::cancelAndErase(m_meshJobs, chunkPosition);

	if (m_chunks.at(chunkPosition).isEmpty() || isBuried(chunkPosition)) {
		removeChunkEntity(chunkPosition);
		return;
	}
	submitMeshJob(chunkPosition);
}

void ChunkRenderer::submitMeshJob(const glm::ivec3 chunkPosition) {
	const concurrency::CancelToken token;
	m_meshJobs[chunkPosition] = token;

	m_pool.submit(chunkPriority(ChunkJob::Mesh, chunkPosition, m_priorityCenter), makeMeshJob(chunkPosition, token));
}

ChunkRenderer::NeighbourSnapshots ChunkRenderer::snapshotNeighbours(const glm::ivec3 chunkPosition) const {
	NeighbourSnapshots snapshots;

	for (std::size_t face = 0; face < snapshots.size(); ++face) {
		if (const auto neighbour = m_chunks.find(chunkPosition + chunkFaceOffsets[face]); neighbour != m_chunks.end())
			snapshots[face] = neighbour->second;
	}
	return snapshots;
}

concurrency::Task ChunkRenderer::makeMeshJob(const glm::ivec3                chunkPosition,
											 const concurrency::CancelToken& token) const {
	return [mesher = &m_chunkMesher, chunkPosition, token, chunk = m_chunks.at(chunkPosition),
			neighbours = snapshotNeighbours(chunkPosition), results = m_meshed] {
		if (token.isCancelled())
			return;

		results->push(
			{.position = chunkPosition, .token = token, .mesh = mesher->toMeshData(chunk, viewsOf(neighbours))});
	};
}

void ChunkRenderer::integrateMeshes() {
	for (MeshedChunk& meshed: m_meshed->drain())
		m_readyMeshes.push_back(std::move(meshed));

	std::size_t uploads = 0;
	while (!m_readyMeshes.empty() && uploads < m_settings.meshUploadsPerFrame) {
		const MeshedChunk meshed = std::move(m_readyMeshes.front());
		m_readyMeshes.pop_front();

		if (meshed.token.isCancelled())
			continue;

		applyMesh(meshed);
		++uploads;
	}
}

void ChunkRenderer::applyMesh(const MeshedChunk& meshed) {
	m_meshJobs.erase(meshed.position);
	removeChunkEntity(meshed.position);

	if (meshed.mesh.vertices.empty() || meshed.mesh.indices.empty())
		return;

	ecs::EntityHandle chunkEntity = m_registry.createEntity();

	chunkEntity.add(
		ecs::component::Mesh{.mesh = m_renderer.createMesh(meshed.mesh), .pipelineType = assets::PipelineType::Chunk});
	chunkEntity.add(ecs::component::Texture{.texture = m_chunkTexture});
	chunkEntity.add(ecs::component::Transform{
		.position = glm::vec3(meshed.position * glm::ivec3(chunkXSize, chunkYSize, chunkZSize))});
	m_chunkEntities[meshed.position] = chunkEntity.id();
}

void ChunkRenderer::removeChunkEntity(const glm::ivec3 chunkPosition) {
	const auto it = m_chunkEntities.find(chunkPosition);
	if (it == m_chunkEntities.end())
		return;

	ecs::EntityHandle chunkEntity = m_registry.getEntity(it->second);
	if (const auto* mesh = chunkEntity.tryGet<ecs::component::Mesh>(); mesh != nullptr)
		m_renderer.destroyMesh(mesh->mesh);
	m_registry.destroyEntity(it->second);
	m_chunkEntities.erase(it);
}

bool ChunkRenderer::isBuried(const glm::ivec3 chunkPosition) const {
	if (!m_chunks.at(chunkPosition).isFull())
		return false;

	for (std::size_t index = 0; index < chunkFaceCount; ++index) {
		const auto neighbour = m_chunks.find(chunkPosition + chunkFaceOffsets[index]);
		if (neighbour == m_chunks.end() || !neighbour->second.isFaceSolid(opposite(static_cast<ChunkFace>(index))))
			return false;
	}
	return true;
}
