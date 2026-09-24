#include "BlockData.hpp"

#include <utility>

using namespace game::block;

BlockDatas::BlockDatas(assets::MeshData defaultMeshData, assets::TextureData defaultTextureData) : m_defaultMeshData(
		std::move(defaultMeshData)), m_defaultTextureData(std::move(defaultTextureData)) {
	m_blockDatas[0] = {
		.prettyName  = "Air",
		.meshData    = assets::MeshData{},
		.textureData = assets::TextureData{}
	};

	m_blockDatas[1] = {
		.prettyName  = "Dirt",
		.meshData    = m_defaultMeshData,
		.textureData = m_defaultTextureData
	};
}

BlockData& BlockDatas::getBlockData(const BlockId id) {
	return m_blockDatas[id];
}
