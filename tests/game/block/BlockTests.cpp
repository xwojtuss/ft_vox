#include <catch2/catch_test_macros.hpp>

#include "game/block/Block.hpp"
#include "support/Blocks.hpp"

using game::Block;

SCENARIO("A default block is air that belongs to no entity", "[block]") {
	GIVEN("a default constructed block") {
		const Block block;

		THEN("it is air") {
			REQUIRE(block.id == test::air);
		}
		AND_THEN("it is not linked to an entity") {
			REQUIRE_FALSE(block.hasEntity());
		}
	}
}

SCENARIO("A block created with a type keeps that type", "[block]") {
	GIVEN("a dirt block") {
		Block block(test::dirt);

		THEN("its type is dirt and it has no entity yet") {
			REQUIRE(block.id == test::dirt);
			REQUIRE_FALSE(block.hasEntity());
		}

		WHEN("it is linked to an entity") {
			block.entity = 42;

			THEN("it reports having an entity") {
				REQUIRE(block.hasEntity());
			}
		}
	}
}
