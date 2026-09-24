#include "ChunkMesher.hpp"

#include <glm/geometric.hpp>

using namespace game::world;

ChunkMesher::ChunkMesher(block::BlockDatas& blockDatas) : m_blockDatas(blockDatas) {
}

/**
 * A bit of room for improvement here.
 * Meshes a chunk into a single mesh, can be optimized by checking chunk borders.
 * @todo TODO: optimize and split
 */
assets::MeshData ChunkMesher::toMeshData(const Chunk& chunk) const {
	assets::MeshData meshData;
	meshData.vertices.reserve(chunkVolume * 24);
	meshData.indices.reserve(chunkVolume * 36);

	for (unsigned short x = 0; x < chunkXSize; ++x) {
		for (unsigned short y = 0; y < chunkYSize; ++y) {
			for (unsigned short z = 0; z < chunkZSize; ++z) {
				const Block& block = chunk.getBlock(x, y, z);
				if (block.id == 0)
					continue;

				const auto& [vertices, indices] = m_blockDatas.getBlockData(block.id).meshData;
				if (indices.size() < 3 || vertices.empty())
					continue;

				for (size_t i = 0; i + 2 < indices.size(); i += 3) {
					const uint32_t i0 = indices[i];
					const uint32_t i1 = indices[i + 1];
					const uint32_t i2 = indices[i + 2];

					if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
						continue;

					const auto& [posV0, colorV0, texCoordV0] = vertices[i0];
					const auto& [posV1, colorV1, texCoordV1] = vertices[i1];
					const auto& [posV2, colorV2, texCoordV2] = vertices[i2];

					const glm::vec3 edgeA  = posV1 - posV0;
					const glm::vec3 edgeB  = posV2 - posV0;
					const glm::vec3 normal = glm::cross(edgeA, edgeB);

					if (const float normalLength = glm::length(normal);
						normalLength <= std::numeric_limits<float>::epsilon())
						continue;

					const glm::vec3 n    = glm::normalize(normal);
					const glm::vec3 absN = glm::abs(n);

					glm::ivec3 offset(0);
					if (glm::max(absN.x, glm::max(absN.y, absN.z)) == absN.x)
						offset.x = n.x > 0.0f ? 1 : -1;
					else if (glm::max(absN.x, glm::max(absN.y, absN.z)) == absN.y)
						offset.y = n.y > 0.0f ? 1 : -1;
					else
						offset.z = n.z > 0.0f ? 1 : -1;

					if (getVoxelCheckBounds(chunk, x + offset.x, y + offset.y, z + offset.z) != 0)
						continue;

					meshData.vertices.push_back({
						.pos      = posV0 + glm::vec3(x, y, z),
						.color    = colorV0,
						.texCoord = texCoordV0
					});
					meshData.vertices.push_back({
						.pos      = posV1 + glm::vec3(x, y, z),
						.color    = colorV1,
						.texCoord = texCoordV1
					});
					meshData.vertices.push_back({
						.pos      = posV2 + glm::vec3(x, y, z),
						.color    = colorV2,
						.texCoord = texCoordV2
					});

					meshData.indices.push_back(static_cast<uint32_t>(meshData.vertices.size()) - 3);
					meshData.indices.push_back(static_cast<uint32_t>(meshData.vertices.size()) - 2);
					meshData.indices.push_back(static_cast<uint32_t>(meshData.vertices.size()) - 1);
				}
			}
		}
	}

	return meshData;
}

game::BlockId ChunkMesher::getVoxelCheckBounds(const Chunk& chunk, const unsigned short x, const unsigned short y,
												const unsigned short z) {
	if (x < 0 || x >= chunkXSize || y < 0 || y >= chunkYSize || z < 0 || z >= chunkZSize)
		return 0;
	return chunk.getBlock(x, y, z).id;
}
