#include "StbTextureLoader.hpp"

using namespace assets;

assets::TextureData	StbTextureLoader::toTextureData(const char* path) {
	int width, height, channels;
	TextureData textureData;

	stbi_uc* data = stbi_load(resolvePath(path).c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!data) {
		throw std::runtime_error("failed to load texture image: " + std::string(path));
	}
	if (width <= 0 || height <= 0) {
		stbi_image_free(data);
		throw std::runtime_error("invalid texture dimensions!");
	}

	textureData.width = static_cast<unsigned int>(width);
	textureData.height = static_cast<unsigned int>(height);
	textureData.mipLevels = 1;
	textureData.pixels = data;
	textureData.freePixels = stbi_image_free;
	
	return textureData;
}

assets::TextureData	StbTextureLoader::toTextureData(std::string path) {
	return toTextureData(path.c_str());
}

StbTextureLoader::~StbTextureLoader() {
}
