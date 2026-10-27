#include "assets/FsTextureRepository.hpp"
#include "error/Exception.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
	constexpr const char* TEST_VALID_TEXTURE_PATH        = TEST_FIXTURES_DIR "/textures_valid";
	constexpr const char* TEST_NON_EXISTENT_TEXTURE_PATH = "foo/bar";
	constexpr const char* TEST_INVALID_TEXTURE_PATH      = TEST_FIXTURES_DIR "/textures_invalid";
}

SCENARIO("repo can load all textures from a specified directory on the filesystem recursively", "[assets][textures]") {
	GIVEN("an instance of FsTextureRepository and a texture location") {
		auto        subject         = assets::FsTextureRepository{};
		const auto* textureLocation = TEST_VALID_TEXTURE_PATH;

		WHEN("the packs are loaded") {
			REQUIRE_NOTHROW(subject.loadPacks(textureLocation));

			THEN("the textures are available") {
				REQUIRE_NOTHROW(subject.get("valid:default"));
				REQUIRE_NOTHROW(subject.get("valid:nested:default"));
				REQUIRE_NOTHROW(subject.get("valid2:default"));
			}
		}
	}
}

SCENARIO("loadPacks throws when specified directory not found", "[assets][textures]") {
	GIVEN("an instance of FsTextureRepository and a texture location") {
		auto subject = assets::FsTextureRepository{};

		WHEN("an exception is thrown") {
			REQUIRE_THROWS_AS(subject.loadPacks(TEST_NON_EXISTENT_TEXTURE_PATH), error::AssetError);
		}
	}
}

SCENARIO("fetching a non-existent texture results in a throw", "[assets][textures]") {
	GIVEN("an instance of FsTextureRepository and a texture location") {
		auto subject = assets::FsTextureRepository{};

		THEN("packs are loaded") {
			REQUIRE_NOTHROW(subject.loadPacks(TEST_VALID_TEXTURE_PATH));
			AND_THEN("exception is thrown") {
				REQUIRE_THROWS_AS(subject.get("non-existent:default"), error::AssetError);
			}
		}
	}
}

SCENARIO("loading skips non-allowed extensions", "[assets][textures]") {
	GIVEN("an instance of FsTextureRepository and a texture with an invalid extension") {
		auto subject = assets::FsTextureRepository{};

		WHEN("the packs are loaded") {
			REQUIRE_NOTHROW(subject.loadPacks(TEST_INVALID_TEXTURE_PATH));

			THEN("the texture is not available") {
				REQUIRE_THROWS_AS(subject.get("should_be_skipped:certainly_not_a_texture_file"), error::AssetError);
				REQUIRE_NOTHROW(subject.get("should_not_be_skipped:valid_alongside_invalid"));
			}
		}
	}
}
