#pragma once

#include <array>
#include <optional>
#include <vector>

#include "Chunk.hpp"
#include "../block/BlockData.hpp"
#include "../../assets/Resources.hpp"

namespace game::world {
	class ChunkMesher {
	public:
		enum Face : unsigned char {
			PositiveX,
			NegativeX,
			PositiveY,
			NegativeY,
			PositiveZ,
			NegativeZ,
			FaceCount
		};

	private:
		struct ModelTriangle {
			std::array<render::Vertex, 3> vertices;
			std::optional<Face>           cullFace;
		};

		struct PreparedModel {
			std::vector<ModelTriangle> triangles;
			bool                       everyTriangleCullable = true;
		};

		using FaceOcclusion = std::array<bool, FaceCount>;

		block::BlockDatas& m_blockDatas;

		[[nodiscard]] static PreparedModel prepareModel(const assets::MeshData& model);
		[[nodiscard]] static std::optional<Face> boundaryFaceOf(const std::array<render::Vertex, 3>& triangle);
		[[nodiscard]] static FaceOcclusion occludedFaces(const Chunk& chunk, int x, int y, int z);
		[[nodiscard]] static BlockId getVoxelCheckBounds(const Chunk& chunk, int x, int y, int z);
		static void appendTriangle(assets::MeshData& meshData, const ModelTriangle& triangle,
									const glm::vec3& blockPosition);

	public:
		explicit ChunkMesher(block::BlockDatas& blockDatas);

		[[nodiscard]] assets::MeshData toMeshData(const Chunk& chunk) const;
	};
}
