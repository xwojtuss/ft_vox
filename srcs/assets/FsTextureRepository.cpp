#include "FsTextureRepository.hpp"

#include <string>
#include <format>

#include "ITextureRepository.hpp"
#include "StbTextureLoader.hpp"
#include "error/Exception.hpp"

using namespace assets;

FsTextureRepository::FsTextureRepository() : texLoader_(std::make_unique<StbTextureLoader>()) {
}

void FsTextureRepository::loadPacks(const std::filesystem::path& path) {
	namespace fs = std::filesystem;

	const fs::path texDir = path / "textures";
	if (!fs::is_directory(texDir)) {
		throw error::AssetError(texDir.string(), "texture directory not found");
	}

	for (const auto& entry: std::filesystem::recursive_directory_iterator(texDir)) {
		if (!fs::is_regular_file(entry)) {
			continue;
		}

		TextureData tx = this->texLoader_->toTextureData(entry.path().string());

		fs::path filename = entry.path().lexically_relative(entry);
		filename.replace_extension();

		const std::string texId = textureId(entry.path());

		if (auto [it, inserted] = this->loadedTex_.emplace(texId, std::move(tx)); !inserted) {
			// to verify: is this err a kill-program candidate?
			throw error::AssetError(texId, "duplicate texture loading attempt");
		}
	}
}

const TextureData& FsTextureRepository::get(const std::string_view id) {
	if (!exists(id)) {
		throw error::AssetError(static_cast<std::string>(id), "texture id does not exist");
	}
	return this->loadedTex_.at(std::string(id));
}

bool FsTextureRepository::exists(const std::string_view id) {
	return this->loadedTex_.contains(std::string(id));
}

std::string FsTextureRepository::textureId(const std::filesystem::path& path) {
	return std::format("{}:{}", path.parent_path().filename().generic_string(), path.stem().generic_string());
}
