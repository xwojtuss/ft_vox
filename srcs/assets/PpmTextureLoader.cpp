#include "PpmTextureLoader.hpp"

using namespace assets;

TextureData	PpmTextureLoader::toTextureData(const char* path) {
	std::ifstream file(resolvePath(path));
	TextureData textureData;

	if (!file.is_open())
		throw std::runtime_error("Failed to open file: " + std::string(path));

	std::string version;
	file >> version;
	if (version != "P6")
		throw std::runtime_error("Unsupported PPM format: " + version);

	file >> textureData.width >> textureData.height;
	int maxColorValue;
	file >> maxColorValue;

	if (maxColorValue != 255)
		throw std::runtime_error("Unsupported max color value in PPM: " + std::to_string(maxColorValue));

	if (file.get() != '\n')
		throw std::runtime_error("Expected newline after max color value in PPM");
	
	textureData.mipLevels = 1;
	textureData.pixels = new unsigned char[textureData.width * textureData.height * 4];
	textureData.freePixels = freePixels;
	loadBinaryData(file, textureData.pixels, textureData.width * textureData.height * 4);
	return textureData;
}

void	PpmTextureLoader::loadBinaryData(std::ifstream& file, void* buffer, size_t size) {
	unsigned char* byteBuffer = static_cast<unsigned char*>(buffer);
	
	for (size_t i = 0; i < size / 4; ++i) {
		unsigned char r, g, b;
		file.read(reinterpret_cast<char*>(&r), 1);
		file.read(reinterpret_cast<char*>(&g), 1);
		file.read(reinterpret_cast<char*>(&b), 1);

		byteBuffer[i * 4 + 0] = r;
		byteBuffer[i * 4 + 1] = g;
		byteBuffer[i * 4 + 2] = b;
		byteBuffer[i * 4 + 3] = 255;
	}
}

TextureData	PpmTextureLoader::toTextureData(std::string path) {
	return toTextureData(path.c_str());
}

void	PpmTextureLoader::freePixels(void* pixels) {
	delete[] static_cast<unsigned char*>(pixels);
}
