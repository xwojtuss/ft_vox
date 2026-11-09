#include <catch2/catch_test_macros.hpp>

#include "error/Exception.hpp"
#include "game/block/BlockData.hpp"
#include "support/Blocks.hpp"

SCENARIO("The block registry knows air and dirt", "[block][registry]") {
	GIVEN("a registry built from a cube model and a texture") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();

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
			REQUIRE(dirt.textureId == "base:dirt");
			REQUIRE(dirt.packName == "base");
		}
		AND_THEN("a block type without a model is registered but has no geometry") {
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

SCENARIO("The block registry grows without a fixed limit", "[block][registry]") {
	GIVEN("a registry") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();

		WHEN("more block types are added") {
			for (unsigned int id = 0; id < 300; ++id) {
				game::block::BlockData stone;
				stone.prettyName = "Stone";
				blockDatas.add(stone);
			}

			THEN("every block is stored") {
				REQUIRE(blockDatas.size() == 303);
			}
		}
	}
}

SCENARIO("New blocks get the next numeric id", "[block][registry]") {
	GIVEN("a registry") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();

		THEN("air is always id zero") {
			REQUIRE(game::block::BlockDatas::airId == test::air);
		}
		WHEN("a block is added") {
			game::block::BlockData stone;
			stone.prettyName          = "Stone";
			const game::BlockId newId = blockDatas.add(stone);

			THEN("it gets the next id and can be looked up by it") {
				REQUIRE(newId == static_cast<game::BlockId>(blockDatas.size() - 1));
				REQUIRE(blockDatas.getBlockData(newId).prettyName == "Stone");
			}
		}
		AND_THEN("an unknown numeric id is reported") {
			REQUIRE_THROWS_AS(blockDatas.getBlockData(999), error::EcsError);
		}
	}
}
