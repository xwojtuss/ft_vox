#pragma once

#include "BlockData.hpp"
#include "assets/ITextureRepository.hpp"

#include <filesystem>
#include <string_view>

class IBlockRepository {
protected:
	assets::ITextureRepository& m_textureRepository;

public:
	explicit IBlockRepository(assets::ITextureRepository& textureRepository) : m_textureRepository(textureRepository) {
	}
	virtual ~IBlockRepository() = default;

	virtual void                                        loadBlocks(const std::filesystem::path& path) = 0;
	[[nodiscard]] virtual const game::block::BlockData& get(std::string_view name) const              = 0;
	[[nodiscard]] virtual const assets::TextureData&    getBlockTexture(std::string_view name) const  = 0;
};
