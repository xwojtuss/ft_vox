#pragma once

#include <string>

#include "assets/Resources.hpp"
#include "assets/ITextureLoader.hpp"

namespace assets {
	class StbTextureLoader : public ITextureLoader {
	public:
		[[nodiscard]] TextureData toTextureData(const char* path) override;
		[[nodiscard]] TextureData toTextureData(const std::string& path) override;
	};
}
