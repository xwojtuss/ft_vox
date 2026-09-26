#include "StbTextureLoader.hpp"

#include "stb_image.h"
#include "../platform/filesystem/resolvePath.hpp"

#include "../error/Exception.hpp"

using namespace assets;

TextureData StbTextureLoader::toTextureData(const char* path) {
	int width    = 0;
	int height   = 0;
	int channels = 0;

	TextureData textureData;

	stbi_uc* data = stbi_load(resolvePath(path).c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!data) {
		throw error::AssetError(path, stbi_failure_reason());
	}
	if (width <= 0 || height <= 0) {
		stbi_image_free(data);
		throw error::AssetError(path, "image has no pixels");
	}

	textureData.width     = static_cast<unsigned int>(width);
	textureData.height    = static_cast<unsigned int>(height);
	textureData.mipLevels = 1;
	textureData.pixels.assign(data, data + static_cast<size_t>(width) * static_cast<size_t>(height) * STBI_rgb_alpha);
	stbi_image_free(data);

	return textureData;
}

TextureData StbTextureLoader::toTextureData(const std::string& path) {
	return toTextureData(path.c_str());
}
