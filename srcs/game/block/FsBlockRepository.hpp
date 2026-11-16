#pragma once

#include <array>
#include <istream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "IBlockRepository.hpp"
#include "nlohmann/json.hpp"

class FsBlockRepository : public IBlockRepository {
private:
	std::unordered_map<std::string, game::block::BlockData> m_blocks;
	std::vector<std::string>                                m_names;
	assets::MeshData                                        m_defaultMeshData;

	static game::block::BlockData jsonToBlock(std::istream& stream, const std::string& path);
	static void                   assertLegalBlockName(std::string_view name);

	static constexpr auto       BLOCK_DIR_NAME = "blocks";
	static constexpr std::array EXCLUDED_BLOCK_NAMES{std::string_view{"air"}};

public:
	explicit FsBlockRepository(assets::ITextureRepository& textureRepository, assets::MeshData defaultMeshData = {});

	void loadBlocks(const std::filesystem::path& path) override;

	[[nodiscard]] const game::block::BlockData&   get(std::string_view name) const override;
	[[nodiscard]] const assets::TextureData&      getBlockTexture(std::string_view name) const override;
	[[nodiscard]] const std::vector<std::string>& names() const;
};
