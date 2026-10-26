#include "assets/FsTextureRepository.hpp"
#include "error/Exception.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
	constexpr const char* TEST_VALID_TEXTURE_PATH        = "tests/fixtures/textures_valid";
	constexpr const char* TEST_NON_EXISTENT_TEXTURE_PATH = "foo/bar";
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
