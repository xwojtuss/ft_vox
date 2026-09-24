#pragma once

#include "Chunk.hpp"
#include "../block/BlockData.hpp"
#include "../../assets/Resources.hpp"

namespace game::world {
	class ChunkMesher {
	private:
		block::BlockDatas& m_blockDatas;

		static BlockId getVoxelCheckBounds(const Chunk& chunk, unsigned short x, unsigned short y, unsigned short z);

	public:
		explicit ChunkMesher(block::BlockDatas& blockDatas);

		[[nodiscard]] assets::MeshData toMeshData(const Chunk& chunk) const;
	};
}
