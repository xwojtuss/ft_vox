#pragma once
#include <filesystem>
#include <string_view>

#include "assets/Resources.hpp"

namespace assets {
	class ITextureRepository {
	public:
		virtual ~ITextureRepository() = default;

		virtual void                             loadPacks(const std::filesystem::path& path) = 0;
		[[nodiscard]] virtual const TextureData& get(std::string_view id) const               = 0;
	};
}
