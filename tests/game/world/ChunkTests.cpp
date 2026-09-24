#include <catch2/catch_test_macros.hpp>

#include "game/world/Chunk.hpp"
#include "support/Blocks.hpp"

using game::Block;
using game::world::Chunk;
using game::world::chunkXSize;
using game::world::chunkYSize;
using game::world::chunkZSize;

namespace {
	game::BlockId uniqueIdFor(unsigned short x, unsigned short y, unsigned short z) {
		return 1 + x * chunkYSize * chunkZSize + y * chunkZSize + z;
	}
}

SCENARIO("A chunk is a 16 x 16 x 16 cube of blocks", "[chunk]") {
	THEN("its size is 16 blocks on every axis") {
		STATIC_REQUIRE(chunkXSize == 16);
		STATIC_REQUIRE(chunkYSize == 16);
		STATIC_REQUIRE(chunkZSize == 16);
	}
}

SCENARIO("A new chunk is filled with air", "[chunk]") {
	GIVEN("a freshly created chunk") {
		const Chunk chunk;

		THEN("every block in it is air") {
			for (unsigned short x = 0; x < chunkXSize; ++x)
				for (unsigned short y = 0; y < chunkYSize; ++y)
					for (unsigned short z = 0; z < chunkZSize; ++z)
						REQUIRE(chunk.getBlock(x, y, z).id == test::air);
		}
	}
}

SCENARIO("Every position in a chunk stores its own block", "[chunk]") {
	GIVEN("a chunk where every position holds a different block type") {
		Chunk chunk;

		for (unsigned short x = 0; x < chunkXSize; ++x)
			for (unsigned short y = 0; y < chunkYSize; ++y)
				for (unsigned short z = 0; z < chunkZSize; ++z)
					chunk.setBlock(x, y, z, Block(uniqueIdFor(x, y, z)));

		THEN("reading any position returns exactly the block stored there") {
			const Chunk& readOnly = chunk;

			for (unsigned short x = 0; x < chunkXSize; ++x)
				for (unsigned short y = 0; y < chunkYSize; ++y)
					for (unsigned short z = 0; z < chunkZSize; ++z)
						REQUIRE(readOnly.getBlock(x, y, z).id == uniqueIdFor(x, y, z));
		}
	}
}

SCENARIO("Removing a block leaves air behind", "[chunk]") {
	GIVEN("a chunk with dirt at (3, 4, 5) and at its neighbour (3, 4, 6)") {
		Chunk chunk;
		chunk.setBlock(3, 4, 5, Block(test::dirt));
		chunk.setBlock(3, 4, 6, Block(test::dirt));

		REQUIRE(chunk.getBlock(3, 4, 5).id == test::dirt);

		WHEN("the block at (3, 4, 5) is removed") {
			chunk.removeBlock(3, 4, 5);

			THEN("that position holds air") {
				REQUIRE(chunk.getBlock(3, 4, 5).id == test::air);
			}
			AND_THEN("the neighbouring block is untouched") {
				REQUIRE(chunk.getBlock(3, 4, 6).id == test::dirt);
			}
		}
	}
}

SCENARIO("A removed block forgets its entity", "[chunk]") {
	GIVEN("a block linked to an entity") {
		Chunk chunk;
		Block block(test::dirt);
		block.entity = 7;
		chunk.setBlock(0, 0, 0, block);

		REQUIRE(chunk.getBlock(0, 0, 0).hasEntity());

		WHEN("it is removed") {
			chunk.removeBlock(0, 0, 0);

			THEN("the position is no longer linked to an entity") {
				REQUIRE_FALSE(chunk.getBlock(0, 0, 0).hasEntity());
			}
		}
	}
}

SCENARIO("Blocks can be edited in place", "[chunk]") {
	GIVEN("a chunk") {
		Chunk chunk;

		WHEN("a block is changed through the reference returned by getBlock") {
			chunk.getBlock(15, 15, 15).id = test::dirt;

			THEN("the chunk stores the change") {
				REQUIRE(chunk.getBlock(15, 15, 15).id == test::dirt);
			}
		}
	}
}
