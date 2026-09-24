#include <catch2/catch_test_macros.hpp>

#include "game/world/Chunk.hpp"

using game::Block;
using game::world::Chunk;

SCENARIO("Removing a block leaves air behind", "[chunk]") {
	GIVEN("a chunk with a stone block at (3, 4, 5)") {
		constexpr game::BlockId	stone = 1;
		Chunk					chunk;
		chunk.setBlock(3, 4, 5, Block(stone));

		REQUIRE(chunk.getBlock(3, 4, 5).id == stone);

		WHEN("that block is removed") {
			chunk.removeBlock(3, 4, 5);

			THEN("the position holds air") {
				REQUIRE(chunk.getBlock(3, 4, 5).id == Block().id);
			}
		}
	}
}
