#pragma once

#include <array>
#include <cstddef>
#include <cstdint> // for std::uint32_t
#include <optional>
#include <vector>

#include "game/planet/Chunk.hpp"
#include "game/planet/ChunkFace.hpp"
#include "game/block/BlockData.hpp"
#include "assets/Resources.hpp"

namespace game::planet {
	class ChunkMesher {
	public:
		/**
		 * The chunks touching this one, indexed by ChunkFace. A missing neighbour (nullptr) is a chunk that is not
		 * loaded: its side is not drawn, except the top, which stays visible.
		 */
		using Neighbours = std::array<const Chunk*, chunkFaceCount>;

	private:
		struct ModelTriangle {
			std::array<render::Vertex, 3> vertices;
			ChunkFace                     face{};
			bool                          cullable{};
		};

		struct PreparedModel {
			std::vector<ModelTriangle> triangles;
			bool                       everyTriangleCullable = true;
		};

		using FaceOcclusion = std::array<bool, chunkFaceCount>;

		block::BlockDatas& m_blockDatas;

		[[nodiscard]] static PreparedModel            prepareModel(const assets::MeshData& model);
		[[nodiscard]] static std::optional<ChunkFace> boundaryFaceOf(const std::array<render::Vertex, 3>& triangle);
		[[nodiscard]] static FaceOcclusion occludedFaces(const Chunk& chunk, const Neighbours& neighbours, int x, int y,
														 int z);
		[[nodiscard]] static BlockId neighbourAt(const Chunk& chunk, const Neighbours& neighbours, int x, int y, int z);
		static void                  appendTriangle(assets::ChunkMeshData& meshData, const ModelTriangle& triangle,
													const glm::vec3& blockPosition);

	public:
		explicit ChunkMesher(block::BlockDatas& blockDatas);

		[[nodiscard]] assets::ChunkMeshData toMeshData(const Chunk& chunk, const Neighbours& neighbours = {}) const;
	};
}
