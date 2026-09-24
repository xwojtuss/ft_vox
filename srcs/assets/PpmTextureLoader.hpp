#pragma once

#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include <functional>
#include <stdexcept>

#include "Resources.hpp"
#include "ITextureLoader.hpp"
#include "../platform/filesystem/resolvePath.hpp"

namespace assets {
class PpmTextureLoader : public ITextureLoader {
public:
	PpmTextureLoader() = default;
	virtual ~PpmTextureLoader() = default;

	virtual TextureData	toTextureData(const char* path) override;
	virtual TextureData	toTextureData(std::string path) override;
	void				loadBinaryData(std::ifstream& file, void* buffer, size_t size);

	static void			freePixels(void* pixels);
};
}