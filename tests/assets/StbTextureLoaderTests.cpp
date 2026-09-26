#include <catch2/catch_test_macros.hpp>

#include "error/Exception.hpp"
#include "assets/StbTextureLoader.hpp"
#include "support/TemporaryFile.hpp"

namespace {
	constexpr const char* twoByTwoPng = "tests/fixtures/2x2.png";
}

SCENARIO("A PNG image is loaded as RGBA pixels", "[assets][stb]") {
	GIVEN("a 2 x 2 PNG with red, green, blue and half transparent white pixels") {
		const assets::TextureData texture = assets::StbTextureLoader().toTextureData(twoByTwoPng);

		THEN("its size is read from the file") {
			REQUIRE(texture.width == 2);
			REQUIRE(texture.height == 2);
			REQUIRE(texture.mipLevels == 1);
		}
		AND_THEN("its pixels are read row by row, keeping transparency") {
			REQUIRE(texture.pixels == std::vector<unsigned char>{
					255, 0, 0, 255, 0, 255, 0, 255,
					0, 0, 255, 255, 255, 255, 255, 128
					});
		}
	}
}

SCENARIO("Images that cannot be read are rejected", "[assets][stb]") {
	GIVEN("a texture loader") {
		assets::StbTextureLoader loader;

		THEN("a file that does not exist is rejected") {
			REQUIRE_THROWS_AS(loader.toTextureData("textures/does_not_exist.png"), error::AssetError);
		}
		AND_THEN("a file that is not an image is rejected") {
			const test::TemporaryFile notAnImage("not_an_image.png", "hello");
			REQUIRE_THROWS_AS(loader.toTextureData(notAnImage.path().c_str()), error::AssetError);
		}
	}
}
