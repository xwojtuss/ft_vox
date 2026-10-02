#include <catch2/catch_test_macros.hpp>

#include <array>
#include <glm/vec3.hpp>
#include <utility>

#include "game/planet/Chunk.hpp"
#include "support/Assertions.hpp"
#include "support/Blocks.hpp"

using game::Block;
using game::planet::Chunk;
using game::planet::chunkXSize;
using game::planet::chunkYSize;
using game::planet::chunkZSize;

namespace {
	game::BlockId uniqueIdFor(int x, int y, int z) {
		return 1 + (x * chunkYSize * chunkZSize) + (y * chunkZSize) + z;
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
			for (int x = 0; x < chunkXSize; ++x)
				for (int y = 0; y < chunkYSize; ++y)
					for (int z = 0; z < chunkZSize; ++z)
						REQUIRE(chunk.getBlock(x, y, z).id == test::air);
		}
	}
}

SCENARIO("Every position in a chunk stores its own block", "[chunk]") {
	GIVEN("a chunk where every position holds a different block type") {
		Chunk chunk;

		for (int x = 0; x < chunkXSize; ++x)
			for (int y = 0; y < chunkYSize; ++y)
				for (int z = 0; z < chunkZSize; ++z)
					chunk.setBlock(x, y, z, Block(uniqueIdFor(x, y, z)));

		THEN("reading any position returns exactly the block stored there") {
			const Chunk& readOnly = chunk;

			for (int x = 0; x < chunkXSize; ++x)
				for (int y = 0; y < chunkYSize; ++y)
					for (int z = 0; z < chunkZSize; ++z)
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
		block.entity = ecs::Entity{7};
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

SCENARIO("A chunk of only air stores no block data", "[chunk]") {
	GIVEN("a freshly created chunk") {
		Chunk chunk;

		THEN("it is empty and takes no block data") {
			REQUIRE(chunk.isEmpty());
			REQUIRE_FALSE(chunk.isFull());
			REQUIRE(chunk.dataBytes() == 0);
		}

		WHEN("air is placed into it") {
			chunk.setBlock(1, 2, 3, Block(test::air));

			THEN("it still takes no block data") {
				REQUIRE(chunk.isEmpty());
				REQUIRE(chunk.dataBytes() == 0);
			}
		}

		WHEN("a block is placed into it") {
			chunk.setBlock(1, 2, 3, Block(test::dirt));

			THEN("the chunk is no longer empty and holds block data") {
				REQUIRE_FALSE(chunk.isEmpty());
				REQUIRE(chunk.dataBytes() > 0);
				REQUIRE(chunk.getBlock(1, 2, 3).id == test::dirt);
				REQUIRE(chunk.getBlock(0, 0, 0).id == test::air);
			}

			AND_WHEN("that block is removed again") {
				chunk.removeBlock(1, 2, 3);

				THEN("the block data is given back") {
					REQUIRE(chunk.isEmpty());
					REQUIRE(chunk.dataBytes() == 0);
				}
			}

			AND_WHEN("it is replaced by another block") {
				chunk.setBlock(1, 2, 3, Block(test::blockWithoutModel));

				THEN("the chunk still counts one block") {
					chunk.removeBlock(1, 2, 3);
					REQUIRE(chunk.isEmpty());
				}
			}
		}
	}
}

SCENARIO("A chunk knows when it is completely filled", "[chunk]") {
	GIVEN("a chunk filled with dirt except for one block") {
		Chunk chunk;
		for (int x = 0; x < chunkXSize; ++x)
			for (int y = 0; y < chunkYSize; ++y)
				for (int z = 0; z < chunkZSize; ++z)
					chunk.setBlock(x, y, z, Block(test::dirt));
		chunk.removeBlock(5, 5, 5);

		THEN("it is not full") {
			REQUIRE_FALSE(chunk.isFull());
		}

		WHEN("the missing block is placed") {
			chunk.setBlock(5, 5, 5, Block(test::dirt));

			THEN("it is full") {
				REQUIRE(chunk.isFull());
			}
		}
	}
}

SCENARIO("A chunk knows whether one of its layers is solid", "[chunk]") {
	GIVEN("a chunk whose bottom layer (y = 0) is dirt") {
		Chunk chunk;
		for (int x = 0; x < chunkXSize; ++x)
			for (int z = 0; z < chunkZSize; ++z)
				chunk.setBlock(x, 0, z, Block(test::dirt));

		THEN("that layer is solid and the one above is not") {
			REQUIRE(chunk.isLayerSolid(1, 0));
			REQUIRE_FALSE(chunk.isLayerSolid(1, 1));
		}
		AND_THEN("the layers across it are not solid, since they hold only one row of dirt each") {
			REQUIRE_FALSE(chunk.isLayerSolid(0, 0));
			REQUIRE_FALSE(chunk.isLayerSolid(2, 3));
		}

		WHEN("one block of the bottom layer is removed") {
			chunk.removeBlock(7, 0, 9);

			THEN("the layer is no longer solid") {
				REQUIRE_FALSE(chunk.isLayerSolid(1, 0));
			}
		}
	}

	GIVEN("an empty chunk") {
		const Chunk chunk;

		THEN("no layer is solid") {
			REQUIRE_FALSE(chunk.isLayerSolid(0, 0));
			REQUIRE_FALSE(chunk.isLayerSolid(1, 5));
			REQUIRE_FALSE(chunk.isLayerSolid(2, chunkZSize - 1));
		}
	}
}

SCENARIO("A chunk knows which positions are inside it", "[chunk][bounds]") {
	THEN("its corners are inside") {
		STATIC_REQUIRE(Chunk::contains(0, 0, 0));
		STATIC_REQUIRE(Chunk::contains(chunkXSize - 1, chunkYSize - 1, chunkZSize - 1));
	}
	AND_THEN("one step past any face is outside") {
		STATIC_REQUIRE_FALSE(Chunk::contains(-1, 0, 0));
		STATIC_REQUIRE_FALSE(Chunk::contains(chunkXSize, 0, 0));
		STATIC_REQUIRE_FALSE(Chunk::contains(0, -1, 0));
		STATIC_REQUIRE_FALSE(Chunk::contains(0, chunkYSize, 0));
		STATIC_REQUIRE_FALSE(Chunk::contains(0, 0, -1));
		STATIC_REQUIRE_FALSE(Chunk::contains(0, 0, chunkZSize));
	}
}

SCENARIO("A block outside the chunk cannot be read or written", "[chunk][bounds][assert]") {
	GIVEN("a chunk and positions one step outside each of its faces") {
		Chunk            chunk;
		const std::array outside = {
			glm::ivec3(-1, 0, 0),         glm::ivec3(chunkXSize, 0, 0), glm::ivec3(0, -1, 0),
			glm::ivec3(0, chunkYSize, 0), glm::ivec3(0, 0, -1),         glm::ivec3(0, 0, chunkZSize),
		};

		THEN("reading, writing or removing any of them is caught as a bug") {
			for (const glm::ivec3& position: outside) {
				CAPTURE(position.x, position.y, position.z);
				REQUIRE_THROWS_AS(std::as_const(chunk).getBlock(position.x, position.y, position.z),
								  test::AssertionFailed);
				REQUIRE_THROWS_AS(chunk.setBlock(position.x, position.y, position.z, Block(test::dirt)),
								  test::AssertionFailed);
				REQUIRE_THROWS_AS(chunk.removeBlock(position.x, position.y, position.z), test::AssertionFailed);
			}
		}
	}
}
