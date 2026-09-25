#include "ChunkMesher.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <glm/geometric.hpp>

#include "../../scene/WorldInfo.hpp"

using namespace game::world;

namespace {
	constexpr float boundaryTolerance = 1e-5f;

	namespace worldinfo = scene::worldinfo;

	constexpr std::array<glm::ivec3, ChunkMesher::FaceCount> faceOffsets = {
		glm::ivec3(worldinfo::right),
		glm::ivec3(worldinfo::left),
		glm::ivec3(worldinfo::up),
		glm::ivec3(worldinfo::down),
		glm::ivec3(worldinfo::backward),
		glm::ivec3(worldinfo::forward),
	};

	ChunkMesher::Face dominantFace(const glm::vec3& normal) {
		const glm::vec3 absNormal = glm::abs(normal);

		if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z)
			return normal.x > 0.0f ? ChunkMesher::PositiveX : ChunkMesher::NegativeX;
		if (absNormal.y >= absNormal.z)
			return normal.y > 0.0f ? ChunkMesher::PositiveY : ChunkMesher::NegativeY;
		return normal.z > 0.0f ? ChunkMesher::PositiveZ : ChunkMesher::NegativeZ;
	}

	bool liesOnPlane(const std::array<render::Vertex, 3>& triangle, const int axis, const float plane) {
		for (const render::Vertex& vertex: triangle) {
			if (std::abs(vertex.pos[axis] - plane) > boundaryTolerance)
				return false;
		}
		return true;
	}
}

ChunkMesher::ChunkMesher(block::BlockDatas& blockDatas) : m_blockDatas(blockDatas) {
}

// TODO: refactor
assets::MeshData ChunkMesher::toMeshData(const Chunk& chunk) const {
	assets::MeshData                           meshData;
	std::unordered_map<BlockId, PreparedModel> preparedModels;

	for (unsigned short x = 0; x < chunkXSize; ++x) {
		for (unsigned short y = 0; y < chunkYSize; ++y) {
			for (unsigned short z = 0; z < chunkZSize; ++z) {
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
				const bool          enclosed = std::all_of(occluded.begin(), occluded.end(),
												[](const bool face) { return face; });
				if (enclosed && everyTriangleCullable)
					continue;

				const glm::vec3 blockPosition(x, y, z);
				for (const ModelTriangle& triangle: triangles) {
					if (triangle.cullFace && occluded[*triangle.cullFace])
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
	const auto&   [vertices, indices] = model;

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size())
			continue;

		const std::array<render::Vertex, 3> triangle = {
			vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]
		};
		const glm::vec3 normal = glm::cross(triangle[1].pos - triangle[0].pos, triangle[2].pos - triangle[0].pos);

		if (glm::length(normal) <= std::numeric_limits<float>::epsilon())
			continue;

		std::optional<Face> cullFace = boundaryFaceOf(triangle);
		if (cullFace != dominantFace(normal)) {
			cullFace.reset();
			prepared.everyTriangleCullable = false;
		}

		prepared.triangles.push_back({.vertices = triangle, .cullFace = cullFace});
	}
	return prepared;
}

std::optional<ChunkMesher::Face> ChunkMesher::boundaryFaceOf(const std::array<render::Vertex, 3>& triangle) {
	for (int face = 0; face < FaceCount; ++face) {
		const int axis = face / 2;

		if (const float plane = face % 2 == 0 ? 1.0f : 0.0f; liesOnPlane(triangle, axis, plane))
			return static_cast<Face>(face);
	}
	return std::nullopt;
}

ChunkMesher::FaceOcclusion ChunkMesher::occludedFaces(const Chunk& chunk, const int x, const int y, const int z) {
	FaceOcclusion occluded{};

	for (int face = 0; face < FaceCount; ++face) {
		const glm::ivec3& offset = faceOffsets[face];
		occluded[face]           = getVoxelCheckBounds(chunk, x + offset.x, y + offset.y, z + offset.z) != 0;
	}
	return occluded;
}

game::BlockId ChunkMesher::getVoxelCheckBounds(const Chunk& chunk, const int x, const int y, const int z) {
	if (x < 0 || x >= chunkXSize || y < 0 || y >= chunkYSize || z < 0 || z >= chunkZSize)
		return 0;
	return chunk.getBlock(static_cast<unsigned short>(x), static_cast<unsigned short>(y),
						static_cast<unsigned short>(z)).id;
}

void ChunkMesher::appendTriangle(assets::MeshData& meshData, const ModelTriangle& triangle,
								const glm::vec3&   blockPosition) {
	for (const auto& [pos, color, texCoord]: triangle.vertices) {
		meshData.vertices.push_back({
			.pos      = pos + blockPosition,
			.color    = color,
			.texCoord = texCoord
		});
		meshData.indices.push_back(static_cast<uint32_t>(meshData.vertices.size()) - 1);
	}
}
