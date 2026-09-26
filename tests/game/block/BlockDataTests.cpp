#include <catch2/catch_test_macros.hpp>

#include "game/block/BlockData.hpp"
#include "support/Blocks.hpp"

SCENARIO("The block registry knows air and dirt", "[block][registry]") {
	GIVEN("a registry built from a cube model and a texture") {
		const std::vector<unsigned char> texturePixels = {10, 20, 30, 255};
		game::block::BlockDatas          blockDatas    = test::makeBlockDatas(texturePixels);

		THEN("block 0 is air and has nothing to draw") {
			const game::block::BlockData& air = blockDatas.getBlockData(test::air);

			REQUIRE(air.prettyName == "Air");
			REQUIRE(air.meshData.vertices.empty());
			REQUIRE(air.meshData.indices.empty());
		}
		AND_THEN("block 1 is dirt and uses the default model and texture") {
			const game::block::BlockData& dirt = blockDatas.getBlockData(test::dirt);

			REQUIRE(dirt.prettyName == "Dirt");
			REQUIRE(dirt.meshData.vertices == test::makeCubeMesh().vertices);
			REQUIRE(dirt.meshData.indices == test::makeCubeMesh().indices);
			REQUIRE(dirt.textureData.pixels == texturePixels);
		}
		AND_THEN("unregistered block types have no name and no model") {
			const game::block::BlockData& unknown = blockDatas.getBlockData(test::blockWithoutModel);

			REQUIRE(unknown.prettyName.empty());
			REQUIRE(unknown.meshData.vertices.empty());
		}
	}
}

SCENARIO("Block definitions can be changed through the registry", "[block][registry]") {
	GIVEN("a registry") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();

		WHEN("a new block type is defined") {
			blockDatas.getBlockData(test::blockWithoutModel).prettyName = "Stone";

			THEN("later lookups see the new definition") {
				REQUIRE(blockDatas.getBlockData(test::blockWithoutModel).prettyName == "Stone");
			}
		}
	}
}
