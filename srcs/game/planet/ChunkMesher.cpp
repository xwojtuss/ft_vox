#include "game/planet/ChunkMesher.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <utility>
#include <glm/geometric.hpp>
#include <magic_enum/magic_enum.hpp>

#include "profiling/Profiler.hpp"
#include "scene/PlanetInfo.hpp"

using namespace game::planet;

namespace {
	constexpr float boundaryTolerance = 1e-5f;

	namespace planetinfo = scene::planetinfo;

	using Face = ChunkMesher::Face;

	static_assert(magic_enum::enum_count<Face>() == ChunkMesher::faceCount);

	constexpr std::array faceOffsets = {
		glm::ivec3(planetinfo::right), glm::ivec3(planetinfo::left),     glm::ivec3(planetinfo::up),
		glm::ivec3(planetinfo::down),  glm::ivec3(planetinfo::backward), glm::ivec3(planetinfo::forward),
	};

	Face dominantFace(const glm::vec3& normal) {
		const glm::vec3 absNormal = glm::abs(normal);

		if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z)
			return normal.x > 0.0f ? Face::PositiveX : Face::NegativeX;
		if (absNormal.y >= absNormal.z)
			return normal.y > 0.0f ? Face::PositiveY : Face::NegativeY;
		return normal.z > 0.0f ? Face::PositiveZ : Face::NegativeZ;
	}

	bool liesOnPlane(const std::array<render::Vertex, 3>& triangle, const int axis, const float plane) {
		return std::ranges::all_of(triangle, [axis, plane](const render::Vertex& vertex) {
			return std::abs(vertex.pos[axis] - plane) <= boundaryTolerance;
		});
	}
}

ChunkMesher::ChunkMesher(block::BlockDatas& blockDatas) : m_blockDatas(blockDatas) {
}

// TODO: refactor
assets::MeshData ChunkMesher::toMeshData(const Chunk& chunk) const {
	FT_PROFILE_FUNCTION();
	assets::MeshData                           meshData;
	std::unordered_map<BlockId, PreparedModel> preparedModels;

	for (int x = 0; x < chunkXSize; ++x) {
		for (int y = 0; y < chunkYSize; ++y) {
			for (int z = 0; z < chunkZSize; ++z) {
				const BlockId id = chunk.getBlock(x, y, z).id;
				if (id == 0)
					continue;

				auto model = preparedModels.find(id);
				if (model == preparedModels.end())
					model = preparedModels.try_emplace(id, prepareModel(m_blockDatas.getBlockData(id).meshData)).first;

				const auto& [triangles, everyTriangleCullable] = model->second;
				if (triangles.empty())
					continue;

				const FaceOcclusion occluded = occludedFaces(chunk, x, y, z);
				const bool          enclosed = std::ranges::all_of(occluded, [](const bool face) { return face; });
				if (enclosed && everyTriangleCullable)
					continue;

				const glm::vec3 blockPosition(x, y, z);
				for (const ModelTriangle& triangle: triangles) {
					if (triangle.cullable && occluded[std::to_underlying(triangle.face)])
						continue;
					appendTriangle(meshData, triangle, blockPosition);
				}
			}
		}
	}

	return meshData;
}

ChunkMesher::PreparedModel ChunkMesher::prepareModel(const assets::MeshData& model) {
	PreparedModel prepared;
	const auto& [vertices, indices] = model;

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size())
			continue;

		const std::array<render::Vertex, 3> triangle = {vertices[indices[i]], vertices[indices[i + 1]],
														vertices[indices[i + 2]]};
		const glm::vec3 normal = glm::cross(triangle[1].pos - triangle[0].pos, triangle[2].pos - triangle[0].pos);

		if (glm::length(normal) <= std::numeric_limits<float>::epsilon())
			continue;

		const Face face     = dominantFace(normal);
		const bool cullable = boundaryFaceOf(triangle) == face;
		if (!cullable)
			prepared.everyTriangleCullable = false;

		prepared.triangles.push_back({.vertices = triangle, .face = face, .cullable = cullable});
	}
	return prepared;
}

std::optional<ChunkMesher::Face> ChunkMesher::boundaryFaceOf(const std::array<render::Vertex, 3>& triangle) {
	for (const Face face: magic_enum::enum_values<Face>()) {
		const int index = std::to_underlying(face);
		const int axis  = index / 2;

		if (const float plane = index % 2 == 0 ? 1.0f : 0.0f; liesOnPlane(triangle, axis, plane))
			return face;
	}
	return std::nullopt;
}

ChunkMesher::FaceOcclusion ChunkMesher::occludedFaces(const Chunk& chunk, const int x, const int y, const int z) {
	FaceOcclusion occluded{};

	for (const Face face: magic_enum::enum_values<Face>()) {
		const std::size_t index  = std::to_underlying(face);
		const glm::ivec3& offset = faceOffsets[index];
		occluded[index]          = neighbourAt(chunk, x + offset.x, y + offset.y, z + offset.z) != 0;
	}
	return occluded;
}

game::BlockId ChunkMesher::neighbourAt(const Chunk& chunk, const int x, const int y, const int z) {
	if (!Chunk::contains(x, y, z))
		return 0;
	return chunk.getBlock(x, y, z).id;
}

void ChunkMesher::appendTriangle(assets::MeshData& meshData, const ModelTriangle& triangle,
								 const glm::vec3& blockPosition) {
	for (const auto& [pos, color, texCoord]: triangle.vertices) {
		meshData.vertices.push_back({.pos = pos + blockPosition, .color = color, .texCoord = texCoord});
		meshData.indices.push_back(static_cast<uint32_t>(meshData.vertices.size()) - 1);
	}
}
