#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

#include "game/planet/ChunkOrder.hpp"

using game::planet::chunkContaining;
using game::planet::chunkOrderCost;
using game::planet::mostUrgentChunks;
using game::planet::Viewer;

namespace {
	const Viewer centreOfOrigin = {.position = {8.0f, 8.0f, 8.0f}, .forward = {0.0f, 0.0f, -1.0f}};
}

SCENARIO("The chunk a position lies in is found for any coordinate", "[chunk-order]") {
	GIVEN("positions on both sides of zero") {
		THEN("each belongs to the chunk it lies in") {
			REQUIRE(chunkContaining({0.0f, 0.0f, 0.0f}) == glm::ivec3(0, 0, 0));
			REQUIRE(chunkContaining({15.9f, 15.9f, 15.9f}) == glm::ivec3(0, 0, 0));
			REQUIRE(chunkContaining({16.0f, 0.0f, 0.0f}) == glm::ivec3(1, 0, 0));
			REQUIRE(chunkContaining({-0.1f, -16.0f, -16.1f}) == glm::ivec3(-1, -1, -2));
		}
	}
}

SCENARIO("Chunks nearer to the player are handled first", "[chunk-order]") {
	GIVEN("a player in the middle of chunk (0, 0, 0)") {
		THEN("the chunk they stand in costs the least") {
			REQUIRE(chunkOrderCost({0, 0, 0}, centreOfOrigin) < chunkOrderCost({1, 0, 0}, centreOfOrigin));
			REQUIRE(chunkOrderCost({0, 0, 0}, centreOfOrigin) < chunkOrderCost({0, 0, -1}, centreOfOrigin));
		}
		AND_THEN("a chunk costs more the farther away it is, in a straight line ahead") {
			REQUIRE(chunkOrderCost({0, 0, -1}, centreOfOrigin) < chunkOrderCost({0, 0, -2}, centreOfOrigin));
			REQUIRE(chunkOrderCost({0, 0, -2}, centreOfOrigin) < chunkOrderCost({0, 0, -5}, centreOfOrigin));
		}
		AND_THEN("height counts as distance too") {
			REQUIRE(chunkOrderCost({0, 1, 0}, centreOfOrigin) < chunkOrderCost({0, 3, 0}, centreOfOrigin));
		}
	}
}

SCENARIO("Chunks in front of the player come before chunks behind them", "[chunk-order]") {
	GIVEN("a player in the middle of chunk (0, 0, 0) looking towards negative z") {
		THEN("a chunk ahead costs less than the equally distant chunk behind") {
			REQUIRE(chunkOrderCost({0, 0, -2}, centreOfOrigin) < chunkOrderCost({0, 0, 2}, centreOfOrigin));
			REQUIRE(chunkOrderCost({-2, 0, -2}, centreOfOrigin) < chunkOrderCost({-2, 0, 2}, centreOfOrigin));
		}
		AND_THEN("a chunk to the side is between the two") {
			const float ahead  = chunkOrderCost({0, 0, -2}, centreOfOrigin);
			const float side   = chunkOrderCost({2, 0, 0}, centreOfOrigin);
			const float behind = chunkOrderCost({0, 0, 2}, centreOfOrigin);
			REQUIRE(ahead < side);
			REQUIRE(side < behind);
		}
		AND_THEN("the chunks around the player stay close to the front even behind them") {
			REQUIRE(chunkOrderCost({0, 0, 1}, centreOfOrigin) < chunkOrderCost({0, 0, -4}, centreOfOrigin));
		}
	}

	GIVEN("the same player after turning around") {
		const Viewer turned = {.position = centreOfOrigin.position, .forward = {0.0f, 0.0f, 1.0f}};

		THEN("the chunk that was behind now costs less than the one ahead") {
			REQUIRE(chunkOrderCost({0, 0, 2}, turned) < chunkOrderCost({0, 0, -2}, turned));
		}
	}

	GIVEN("a player looking straight down") {
		const Viewer lookingDown = {.position = centreOfOrigin.position, .forward = {0.0f, -1.0f, 0.0f}};

		THEN("the chunk below costs less than the equally distant chunk above") {
			REQUIRE(chunkOrderCost({0, -3, 0}, lookingDown) < chunkOrderCost({0, 3, 0}, lookingDown));
		}
		AND_THEN("a chunk to the side is between the two") {
			REQUIRE(chunkOrderCost({0, -3, 0}, lookingDown) < chunkOrderCost({3, 0, 0}, lookingDown));
			REQUIRE(chunkOrderCost({3, 0, 0}, lookingDown) < chunkOrderCost({0, 3, 0}, lookingDown));
		}
	}

	GIVEN("a player looking straight up") {
		const Viewer lookingUp = {.position = centreOfOrigin.position, .forward = {0.0f, 1.0f, 0.0f}};

		THEN("the chunk above costs less than the equally distant chunk below") {
			REQUIRE(chunkOrderCost({0, 3, 0}, lookingUp) < chunkOrderCost({0, -3, 0}, lookingUp));
		}
	}

	GIVEN("a player looking towards negative z") {
		THEN("a chunk straight above or below counts as being to the side") {
			REQUIRE(chunkOrderCost({0, 3, 0}, centreOfOrigin) == chunkOrderCost({0, -3, 0}, centreOfOrigin));
			REQUIRE(chunkOrderCost({0, 3, 0}, centreOfOrigin) == chunkOrderCost({3, 0, 0}, centreOfOrigin));
		}
	}
}

SCENARIO("The most urgent chunks are picked in order", "[chunk-order]") {
	GIVEN("a few chunks around a player looking towards negative z") {
		const std::vector<glm::ivec3> chunks = {{0, 0, 3}, {0, 0, -1}, {0, 0, 0}, {0, 0, -3}, {2, 0, 0}};

		WHEN("the three most urgent are asked for") {
			const std::vector<glm::ivec3> urgent = mostUrgentChunks(chunks, 3, centreOfOrigin);

			THEN("they are the cheapest ones, cheapest first: the chunk ahead beats the one to the side") {
				REQUIRE(urgent == std::vector<glm::ivec3>{{0, 0, 0}, {0, 0, -1}, {0, 0, -3}});
			}
		}

		WHEN("more are asked for than there are") {
			THEN("all of them come back, best first") {
				const std::vector<glm::ivec3> urgent = mostUrgentChunks(chunks, 10, centreOfOrigin);
				REQUIRE(urgent.size() == chunks.size());
				REQUIRE(urgent.front() == glm::ivec3(0, 0, 0));
				REQUIRE(urgent.back() == glm::ivec3(0, 0, 3));
			}
		}

		WHEN("none are asked for") {
			THEN("nothing comes back") {
				REQUIRE(mostUrgentChunks(chunks, 0, centreOfOrigin).empty());
			}
		}
	}

	GIVEN("two chunks that cost exactly the same") {
		THEN("the result does not depend on the order they were given in") {
			const std::vector<glm::ivec3> first  = mostUrgentChunks({{3, 0, 0}, {-3, 0, 0}}, 2, centreOfOrigin);
			const std::vector<glm::ivec3> second = mostUrgentChunks({{-3, 0, 0}, {3, 0, 0}}, 2, centreOfOrigin);
			REQUIRE(first == second);
		}
	}
}
