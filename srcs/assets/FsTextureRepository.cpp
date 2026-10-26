#include "FsTextureRepository.hpp"

#include <algorithm>
#include <string>
#include <format>
#include <ranges>

#include "ITextureRepository.hpp"
#include "StbTextureLoader.hpp"
#include "error/Exception.hpp"

using namespace assets;

FsTextureRepository::FsTextureRepository() : texLoader_(std::make_unique<StbTextureLoader>()) {
}

void FsTextureRepository::loadPacks(const std::filesystem::path& path) {
	namespace fs = std::filesystem;

	const fs::path texDir = path / TEXTURES_DIR_NAME;
	if (!fs::is_directory(texDir)) {
		throw error::AssetError(texDir.string(), "texture directory not found");
	}

	for (const auto& entry: std::filesystem::recursive_directory_iterator(texDir)) {
		if (!fs::is_regular_file(entry)) {
			continue;
		}

		TextureData tex = this->texLoader_->toTextureData(entry.path().string());

		const std::string texId = textureId(texDir, entry.path());

		if (auto [it, inserted] = this->loadedTex_.emplace(texId, std::move(tex)); !inserted) {
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

std::string FsTextureRepository::textureId(const std::filesystem::path& texDir, const std::filesystem::path& file) {
	namespace fs = std::filesystem;

	const fs::path rel    = file.lexically_relative(texDir);
	const fs::path parent = rel.parent_path();
	std::string    dir    = parent.generic_string();
	std::string    name   = rel.stem().string();

	if (dir.empty() || dir == ".")
		return name;

	std::ranges::replace(dir, '/', ':');

	return std::format("{}:{}", dir, name);
}
