#pragma once

#include <string>
#include <stdexcept>
#include <limits>

#include "stb_image.h"
#include "Resources.hpp"
#include "ITextureLoader.hpp"
#include "../platform/filesystem/resolvePath.hpp"

namespace assets {
class StbTextureLoader : public ITextureLoader {
public:
	StbTextureLoader() = default;
	virtual ~StbTextureLoader();

	virtual TextureData	toTextureData(const char* path);
	virtual TextureData	toTextureData(std::string path);
};
}