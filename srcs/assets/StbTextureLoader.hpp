#pragma once

#include <string>

#include "Resources.hpp"
#include "ITextureLoader.hpp"

namespace assets {
	class StbTextureLoader : public ITextureLoader {
	public:
		[[nodiscard]] TextureData toTextureData(const char* path) override;
		[[nodiscard]] TextureData toTextureData(const std::string& path) override;
	};
}
