#pragma once

#include <string_view>

#include "assets/ITextureRepository.hpp"

namespace test {
	class FakeTextureRepository : public assets::ITextureRepository {
	public:
		assets::TextureData texture{};

		FakeTextureRepository() {
			texture.width     = 1;
			texture.height    = 1;
			texture.mipLevels = 1;
			texture.pixels    = {0, 0, 0, 255};
		}

		void loadPacks(const std::filesystem::path&) override {
		}

		[[nodiscard]] const assets::TextureData& get(std::string_view) const override {
			return texture;
		}
	};
}
