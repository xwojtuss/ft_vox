#include <catch2/catch_test_macros.hpp>

#include <format>
#include <string>
#include <vector>

#include "error/Exception.hpp"
#include "game/block/BlockData.hpp"
#include "game/block/FsBlockRepository.hpp"
#include "support/FakeTextureRepository.hpp"
#include "support/TemporaryDirectory.hpp"

namespace {
	std::string blockJson(const std::string& prettyName) {
		return R"({ "prettyName": ")" + prettyName + R"(", "isUnique": false })";
	}
}

SCENARIO("Every JSON file in a pack's blocks folder becomes a block", "[block][repository]") {
	GIVEN("a temporary pack with two blocks") {
		test::TemporaryDirectory    pack("blocks");
		test::FakeTextureRepository textures;
		FsBlockRepository           repository(textures);

		pack.write("blocks/base/dirt.json", blockJson("Dirt"));
		pack.write("blocks/custom/stone.json", blockJson("Stone"));

		WHEN("the blocks are loaded") {
			REQUIRE_NOTHROW(repository.loadBlocks(pack.path()));

			THEN("both files are registered under their pretty name") {
				REQUIRE(repository.names() == std::vector<std::string>{"Dirt", "Stone"});
			}
			AND_THEN("the block data comes from the file") {
				const game::block::BlockData& dirt = repository.get("Dirt");

				REQUIRE(dirt.prettyName == "Dirt");
				REQUIRE(dirt.textureId == "base:dirt");
				REQUIRE(dirt.packName == "base");
				REQUIRE_FALSE(dirt.isUnique);
			}
			AND_THEN("the block repository asks the texture repository for its texture") {
				REQUIRE_NOTHROW(repository.getBlockTexture("Dirt"));
			}
		}
	}
}

SCENARIO("A block's texture is its pack and lowercased pretty name", "[block][repository]") {
	GIVEN("a block whose file name differs from its pretty name") {
		const test::TemporaryDirectory pack("derived-texture");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/anything.json", blockJson("Gravel"));

		WHEN("the block is loaded") {
			repository.loadBlocks(pack.path());

			THEN("the texture id comes from the pack and the block's name") {
				REQUIRE(repository.get("Gravel").textureId == "base:gravel");
			}
		}
	}
}

SCENARIO("Two blocks cannot share a pretty name", "[block][repository]") {
	GIVEN("two files that define the same block") {
		const test::TemporaryDirectory pack("duplicate-block");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/first.json", blockJson("Gravel"));
		pack.write("blocks/base/second.json", blockJson("Gravel"));

		WHEN("the blocks are loaded") {
			THEN("the duplicate name is reported") {
				REQUIRE_THROWS_AS(repository.loadBlocks(pack.path()), error::AssetError);
			}
		}
	}
}

SCENARIO("The block repository has no fixed number of blocks", "[block][repository]") {
	GIVEN("a temporary pack with more than 256 block files") {
		test::TemporaryDirectory    pack("many-blocks");
		test::FakeTextureRepository textures;
		FsBlockRepository           repository(textures);

		for (unsigned int id = 0; id < 300; ++id)
			pack.write(std::format("blocks/base/block{}.json", id), blockJson(std::format("Block{}", id)));

		WHEN("the blocks are loaded") {
			repository.loadBlocks(pack.path());

			THEN("all of them are registered") {
				REQUIRE(repository.names().size() == 300);
			}
			AND_THEN("a runtime registry keeps all of them next to air") {
				game::block::BlockDatas blockDatas;
				for (const std::string& name: repository.names())
					blockDatas.add(repository.get(name));

				REQUIRE(blockDatas.size() == 301);
			}
		}
	}
}
