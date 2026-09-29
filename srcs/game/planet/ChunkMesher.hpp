#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "game/planet/Chunk.hpp"
#include "game/block/BlockData.hpp"
#include "assets/Resources.hpp"

namespace game::planet {
	class ChunkMesher {
	public:
		enum class Face : std::uint8_t { PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ };

		static constexpr std::size_t faceCount = 6;

	private:
		struct ModelTriangle {
			std::array<render::Vertex, 3> vertices;
			Face                          face{};
			bool                          cullable{};
		};

		struct PreparedModel {
			std::vector<ModelTriangle> triangles;
			bool                       everyTriangleCullable = true;
		};

		using FaceOcclusion = std::array<bool, faceCount>;

		block::BlockDatas& m_blockDatas;

		[[nodiscard]] static PreparedModel       prepareModel(const assets::MeshData& model);
		[[nodiscard]] static std::optional<Face> boundaryFaceOf(const std::array<render::Vertex, 3>& triangle);
		[[nodiscard]] static FaceOcclusion       occludedFaces(const Chunk& chunk, int x, int y, int z);
		[[nodiscard]] static BlockId             neighbourAt(const Chunk& chunk, int x, int y, int z);
		static void appendTriangle(assets::MeshData& meshData, const ModelTriangle& triangle,
								   const glm::vec3& blockPosition);

	public:
		explicit ChunkMesher(block::BlockDatas& blockDatas);

		[[nodiscard]] assets::MeshData toMeshData(const Chunk& chunk) const;
	};
}
