#pragma once

#include <array>
#include <string>

#include "Block.hpp"
#include "../../assets/Resources.hpp"

namespace game::block {
	struct BlockData {
		std::string         prettyName;
		assets::MeshData    meshData;
		assets::TextureData textureData;
	};

	class BlockDatas {
	public:
		constexpr static unsigned int maxBlockTypes = 256;

	private:
		std::array<BlockData, maxBlockTypes> m_blockDatas;
		assets::MeshData                     m_defaultMeshData;
		assets::TextureData                  m_defaultTextureData;

	public:
		BlockDatas(assets::MeshData defaultMeshData, assets::TextureData defaultTextureData);

		[[nodiscard]] BlockData& getBlockData(BlockId id);
	};
}
