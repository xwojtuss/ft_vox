#include "FsTextureRepository.hpp"

#include <algorithm>
#include <string>
#include <format>
#include <ranges>

#include "StbTextureLoader.hpp"
#include "error/Exception.hpp"
#include "platform/filesystem/filesystemUtils.hpp"
#include "platform/filesystem/resolvePath.hpp"

using namespace assets;

FsTextureRepository::FsTextureRepository() : m_textureLoader(std::make_unique<StbTextureLoader>()) {
}

void FsTextureRepository::loadPacks(const std::filesystem::path& path) {
	namespace fs = std::filesystem;

	const fs::path texDir = fs::path(resolvePath(path)) / TEXTURES_DIR_NAME;
	if (!fs::is_directory(texDir)) {
		throw error::AssetError(texDir.string(), "texture directory not found");
	}

	for (const auto& entry: fs::recursive_directory_iterator(texDir, fs::directory_options::skip_permission_denied)) {
		if (!fs::is_regular_file(entry) || !hasAllowedExtension(entry.path().filename(), {".png", ".jpg", ".jpeg"})) {
			continue;
		}

		TextureData tex = this->m_textureLoader->toTextureData(entry.path().string());

		const std::string texId = textureId(texDir, entry.path());

		if (auto [it, inserted] = this->m_loadedTextures.emplace(texId, std::move(tex)); !inserted) {
			throw error::AssetError(texId, "duplicate texture loading attempt");
		}
	}
}

const TextureData& FsTextureRepository::get(const std::string_view id) const {
	const auto it = this->m_loadedTextures.find(std::string(id));
	if (it == this->m_loadedTextures.end()) {
		throw error::AssetError(static_cast<std::string>(id), "texture id does not exist");
	}
	return it->second;
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
