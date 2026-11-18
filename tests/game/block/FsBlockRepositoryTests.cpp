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
		const test::TemporaryDirectory pack("blocks");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/dirt.json", blockJson("dirt"));
		pack.write("blocks/custom/stone.json", blockJson("stone"));

		WHEN("the blocks are loaded") {
			REQUIRE_NOTHROW(repository.loadBlocks(pack.path()));

			THEN("both files are registered under their pack-qualified name") {
				REQUIRE(repository.names() == std::vector<std::string>{"base:dirt", "custom:stone"});
			}
			AND_THEN("the block data comes from the file") {
				const game::block::BlockData& dirt = repository.get("base:dirt");

				REQUIRE(dirt.blockName == "dirt");
				REQUIRE(dirt.prettyName == "dirt");
				REQUIRE(dirt.textureId == "base:dirt");
				REQUIRE(dirt.packName == "base");
				REQUIRE_FALSE(dirt.isUnique);
			}
			AND_THEN("the block repository asks the texture repository for its texture") {
				REQUIRE_NOTHROW(repository.getBlockTexture("base:dirt"));
			}
		}
	}
}

SCENARIO("A block's pretty name is normalized into the name used by the program", "[block][repository]") {
	GIVEN("a block whose pretty name contains uppercase letters and spaces") {
		const test::TemporaryDirectory pack("pretty-name");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/super-dirt.json", blockJson("Super Dirt"));

		WHEN("the block is loaded") {
			repository.loadBlocks(pack.path());

			THEN("the pretty name is kept and the internal name is normalized") {
				REQUIRE(repository.names() == std::vector<std::string>{"base:super_dirt"});

				const game::block::BlockData& block = repository.get("base:super_dirt");

				REQUIRE(block.blockName == "super_dirt");
				REQUIRE(block.prettyName == "Super Dirt");
				REQUIRE(block.textureId == "base:super_dirt");
			}
		}
	}
}

SCENARIO("A block's texture is its pack and lowercased pretty name", "[block][repository]") {
	GIVEN("a block whose file name differs from its pretty name") {
		const test::TemporaryDirectory pack("derived-texture");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/anything.json", blockJson("gravel"));

		WHEN("the block is loaded") {
			repository.loadBlocks(pack.path());

			THEN("the texture id comes from the pack and the block's name") {
				REQUIRE(repository.get("base:gravel").textureId == "base:gravel");
			}
		}
	}
}

SCENARIO("Two blocks in the same pack cannot share a name", "[block][repository]") {
	GIVEN("two files that define the same block") {
		const test::TemporaryDirectory pack("duplicate-block");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/first.json", blockJson("gravel"));
		pack.write("blocks/base/second.json", blockJson("gravel"));

		WHEN("the blocks are loaded") {
			THEN("the duplicate name is reported") {
				REQUIRE_THROWS_AS(repository.loadBlocks(pack.path()), error::AssetError);
			}
		}
	}
}

SCENARIO("Two packs can define a block with the same pretty name", "[block][repository]") {
	GIVEN("two packs that define a block with the same name") {
		const test::TemporaryDirectory pack("shared-name");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/gravel.json", blockJson("gravel"));
		pack.write("blocks/custom/gravel.json", blockJson("gravel"));

		WHEN("the blocks are loaded") {
			repository.loadBlocks(pack.path());

			THEN("both blocks are registered separately") {
				REQUIRE(repository.names() == std::vector<std::string>{"base:gravel", "custom:gravel"});
				REQUIRE(repository.get("base:gravel").textureId == "base:gravel");
				REQUIRE(repository.get("custom:gravel").textureId == "custom:gravel");
			}
		}
	}
}

SCENARIO("A block outside a pack folder is registered under its bare name", "[block][repository]") {
	GIVEN("a block file directly in the blocks folder") {
		const test::TemporaryDirectory pack("no-pack");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/stone.json", blockJson("stone"));

		WHEN("the block is loaded") {
			repository.loadBlocks(pack.path());

			THEN("the name and the texture id have no pack prefix") {
				REQUIRE(repository.names() == std::vector<std::string>{"stone"});
				REQUIRE(repository.get("stone").textureId == "stone");
			}
		}
	}
}

SCENARIO("Block names must be legal identifiers", "[block][repository]") {
	GIVEN("a temporary pack") {
		const test::TemporaryDirectory pack("illegal-name");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		WHEN("a block name contains an unsupported character") {
			pack.write("blocks/base/punctuation.json", blockJson("stone!"));

			THEN("the illegal name is reported") {
				REQUIRE_THROWS_AS(repository.loadBlocks(pack.path()), error::AssetError);
			}
		}
		WHEN("a block name is empty") {
			pack.write("blocks/base/empty.json", blockJson(""));

			THEN("the illegal name is reported") {
				REQUIRE_THROWS_AS(repository.loadBlocks(pack.path()), error::AssetError);
			}
		}
		WHEN("a block name is reserved") {
			pack.write("blocks/base/air.json", blockJson("air"));

			THEN("the illegal name is reported") {
				REQUIRE_THROWS_AS(repository.loadBlocks(pack.path()), error::AssetError);
			}
		}
		WHEN("a block name becomes reserved after normalization") {
			pack.write("blocks/base/upper-air.json", blockJson("Air"));

			THEN("the illegal name is reported") {
				REQUIRE_THROWS_AS(repository.loadBlocks(pack.path()), error::AssetError);
			}
		}
	}
}

SCENARIO("A malformed block file is reported as an asset error", "[block][repository]") {
	GIVEN("a block file that is not valid JSON") {
		const test::TemporaryDirectory pack("malformed-block");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		pack.write("blocks/base/broken.json", "{ this is not json");

		WHEN("the blocks are loaded") {
			THEN("the parse error is wrapped in an asset error") {
				REQUIRE_THROWS_AS(repository.loadBlocks(pack.path()), error::AssetError);
			}
		}
	}
}

SCENARIO("The block repository has no fixed number of blocks", "[block][repository]") {
	GIVEN("a temporary pack with more than 256 block files") {
		const test::TemporaryDirectory pack("many-blocks");
		test::FakeTextureRepository    textures;
		FsBlockRepository              repository(textures);

		for (unsigned int id = 0; id < 300; ++id)
			pack.write(std::format("blocks/base/block{}.json", id), blockJson(std::format("block{}", id)));

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
