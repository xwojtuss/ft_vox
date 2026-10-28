#pragma once

#include "ITextureLoader.hpp"
#include "ITextureRepository.hpp"

namespace assets {

	class FsTextureRepository : public ITextureRepository {
	public:
		explicit FsTextureRepository();

		void loadPacks(const std::filesystem::path& path) override;

		[[nodiscard]] const TextureData& get(std::string_view id) const override;

	private:
		static constexpr auto TEXTURES_DIR_NAME = "textures";

		std::unique_ptr<ITextureLoader>              m_textureLoader;
		std::unordered_map<std::string, TextureData> m_loadedTextures;

		static std::string textureId(const std::filesystem::path& texDir, const std::filesystem::path& file);
	};

}
