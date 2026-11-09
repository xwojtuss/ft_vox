#include "FsBlockRepository.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <utility>

#include "error/Exception.hpp"
#include "platform/filesystem/filesystemUtils.hpp"
#include "platform/filesystem/resolvePath.hpp"

namespace {
	std::string packNameFromPath(const std::filesystem::path& blockDir, const std::filesystem::path& file) {
		const std::filesystem::path relative = file.lexically_relative(blockDir);
		const auto                  first    = relative.begin();

		if (first == relative.end() || std::next(first) == relative.end())
			return {};
		return first->string();
	}

	std::string lowercased(const std::string_view value) {
		std::string result(value);
		std::ranges::transform(result, result.begin(), [](const unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		return result;
	}

	std::string textureIdFor(const std::string_view packName, const std::string_view prettyName) {
		if (packName.empty()) {
			return lowercased(prettyName);
		}
		return std::format("{}:{}", packName, lowercased(prettyName));
	}
}

FsBlockRepository::FsBlockRepository(assets::ITextureRepository& textureRepository, assets::MeshData defaultMeshData) :
	IBlockRepository(textureRepository), m_defaultMeshData(std::move(defaultMeshData)) {
}

void FsBlockRepository::loadBlocks(const std::filesystem::path& path) {
	namespace fs = std::filesystem;

	const fs::path blockDir = fs::path(resolvePath(path)) / BLOCK_DIR_NAME;
	if (!fs::is_directory(blockDir)) {
		throw error::AssetError(blockDir.string(), "block directory not found");
	}

	std::vector<fs::path> blockFiles;
	for (const auto& entry: fs::recursive_directory_iterator(blockDir, fs::directory_options::skip_permission_denied)) {
		if (fs::is_regular_file(entry) && hasAllowedExtension(entry.path().filename(), {".json"}))
			blockFiles.push_back(entry.path());
	}
	std::ranges::sort(blockFiles);

	for (const fs::path& file: blockFiles) {
		std::ifstream stream(file);
		if (!stream.is_open())
			throw error::AssetError(file.string(), "could not be opened");

		const nlohmann::json   parsed    = nlohmann::json::parse(stream);
		game::block::BlockData blockData = jsonToBlock(parsed);

		blockData.meshData  = m_defaultMeshData;
		blockData.packName  = packNameFromPath(blockDir, file);
		blockData.textureId = textureIdFor(blockData.packName, blockData.prettyName);

		static_cast<void>(m_textureRepository.get(blockData.textureId));

		std::string name = blockData.prettyName;
		if (auto [it, inserted] = m_blocks.emplace(name, std::move(blockData)); !inserted)
			throw error::AssetError(it->first, "duplicate block name loading attempt");

		m_names.push_back(std::move(name));
	}
}

const game::block::BlockData& FsBlockRepository::get(const std::string_view name) const {
	const auto it = m_blocks.find(std::string(name));
	if (it == m_blocks.end()) {
		throw error::EcsError(std::format("block name={} does not exist", name));
	}
	return it->second;
}

const assets::TextureData& FsBlockRepository::getBlockTexture(const std::string_view name) const {
	return m_textureRepository.get(get(name).textureId);
}

const std::vector<std::string>& FsBlockRepository::names() const {
	return m_names;
}

game::block::BlockData FsBlockRepository::jsonToBlock(const nlohmann::json& json) {
	game::block::BlockData block;
	block.prettyName = json.at("prettyName").get<std::string>();
	block.isUnique   = json.value("isUnique", false);
	return block;
}
