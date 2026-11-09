#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "IBlockRepository.hpp"
#include "nlohmann/json.hpp"

class FsBlockRepository : public IBlockRepository {
private:
	std::unordered_map<std::string, game::block::BlockData> m_blocks;
	std::vector<std::string>                                m_names;
	assets::MeshData                                        m_defaultMeshData;

	static game::block::BlockData jsonToBlock(const nlohmann::json& json);

public:
	static constexpr auto BLOCK_DIR_NAME = "blocks";

	explicit FsBlockRepository(assets::ITextureRepository& textureRepository, assets::MeshData defaultMeshData = {});

	void loadBlocks(const std::filesystem::path& path) override;

	[[nodiscard]] const game::block::BlockData&   get(std::string_view name) const override;
	[[nodiscard]] const assets::TextureData&      getBlockTexture(std::string_view name) const override;
	[[nodiscard]] const std::vector<std::string>& names() const;
};
