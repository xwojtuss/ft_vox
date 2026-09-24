#pragma once

#include <string>

#include "Resources.hpp"

namespace assets {
	class ITextureLoader {
	public:
		virtual ~ITextureLoader() = default;

		[[nodiscard]] virtual TextureData toTextureData(const char* path) = 0;
		[[nodiscard]] virtual TextureData toTextureData(const std::string& path) = 0;
	};
}
