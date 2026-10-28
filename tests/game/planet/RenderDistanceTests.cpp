#include <catch2/catch_test_macros.hpp>

#include "game/planet/RenderDistance.hpp"

using game::planet::isWithinRenderDistance;
using game::planet::RenderDistance;
using game::planet::renderDistanceReach;

SCENARIO("The render distance is an ellipsoid of chunks around the player", "[render-distance]") {
	GIVEN("the same render distance on every axis") {
		constexpr RenderDistance sphere = {6, 6, 6};

		THEN("the chunk the player stands in is inside") {
			REQUIRE(isWithinRenderDistance({0, 0, 0}, sphere));
		}
		AND_THEN("the chunks as far away as half the render distance are inside on every axis") {
			REQUIRE(isWithinRenderDistance({3, 0, 0}, sphere));
			REQUIRE(isWithinRenderDistance({0, -3, 0}, sphere));
			REQUIRE(isWithinRenderDistance({0, 0, 3}, sphere));
		}
		AND_THEN("chunks beyond that are outside") {
			REQUIRE_FALSE(isWithinRenderDistance({4, 0, 0}, sphere));
			REQUIRE_FALSE(isWithinRenderDistance({0, 0, -4}, sphere));
		}
		AND_THEN("the corners of the surrounding box are outside") {
			REQUIRE_FALSE(isWithinRenderDistance({3, 3, 3}, sphere));
			REQUIRE_FALSE(isWithinRenderDistance({3, 3, 0}, sphere));
		}
		AND_THEN("chunks diagonally inside the sphere are inside") {
			REQUIRE(isWithinRenderDistance({2, 2, 2}, sphere));
			REQUIRE(isWithinRenderDistance({-2, 2, -2}, sphere));
		}
	}

	GIVEN("a shorter render distance vertically than horizontally") {
		constexpr RenderDistance flattened = {10, 2, 10};

		THEN("the area reaches further sideways than up and down") {
			REQUIRE(isWithinRenderDistance({5, 0, 0}, flattened));
			REQUIRE(isWithinRenderDistance({0, 1, 0}, flattened));
			REQUIRE_FALSE(isWithinRenderDistance({0, 2, 0}, flattened));
			REQUIRE_FALSE(isWithinRenderDistance({5, 1, 0}, flattened));
		}
	}

	GIVEN("a render distance of 1 on every axis") {
		THEN("only the player's own chunk is inside") {
			REQUIRE(isWithinRenderDistance({0, 0, 0}, {1, 1, 1}));
			REQUIRE_FALSE(isWithinRenderDistance({1, 0, 0}, {1, 1, 1}));
		}
	}

	GIVEN("a render distance of 0 on one axis") {
		THEN("nothing is inside, not even the player's own chunk") {
			REQUIRE_FALSE(isWithinRenderDistance({0, 0, 0}, {0, 6, 6}));
			REQUIRE_FALSE(isWithinRenderDistance({0, 0, 0}, {6, 6, 0}));
		}
	}
}

SCENARIO("The reach of a render distance is the box that holds its ellipsoid", "[render-distance]") {
	GIVEN("render distances counted across") {
		THEN("the reach is half of them in every direction, rounded down") {
			REQUIRE(renderDistanceReach({6, 4, 2}) == glm::ivec3(3, 2, 1));
			REQUIRE(renderDistanceReach({7, 1, 0}) == glm::ivec3(3, 0, 0));
		}
	}
}
