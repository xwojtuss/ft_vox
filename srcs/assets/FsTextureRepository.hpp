#pragma once

#include "ITextureLoader.hpp"
#include "ITextureRepository.hpp"

namespace assets {

	class FsTextureRepository : public ITextureRepository {
	public:
		explicit FsTextureRepository();

		void loadPacks(const std::filesystem::path& path) override;

		[[nodiscard]] const TextureData& get(std::string_view id) override;

	protected:
		[[nodiscard]] bool exists(std::string_view id) override;

	private:
		std::unique_ptr<ITextureLoader> texLoader_;
		std::unordered_map<std::string, TextureData> loadedTex_;

		static std::string textureId(const std::filesystem::path& path);
	};

}