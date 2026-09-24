#include <catch2/catch_test_macros.hpp>

#include "game/world/generation/Perlin2DMap.hpp"

using game::world::Perlin2DMap;

SCENARIO("The height map is the same every time", "[generation][noise]") {
	GIVEN("two maps created with the same settings") {
		const Perlin2DMap first(16, 16, 4, 0.01f);
		const Perlin2DMap second(16, 16, 4, 0.01f);

		THEN("they return the same value everywhere, inside and outside the precomputed area") {
			for (int x = -40; x <= 40; x += 7)
				for (int y = -40; y <= 40; y += 7)
					REQUIRE(first.getValue(x, y) == second.getValue(x, y));
		}
	}
}

SCENARIO("Precomputed and on-the-fly values agree", "[generation][noise]") {
	GIVEN("a map that precomputes a 16 x 16 area and one that precomputes almost nothing") {
		const Perlin2DMap precomputed(16, 16, 4, 0.01f);
		const Perlin2DMap onTheFly(1, 1, 4, 0.01f);

		THEN("both give the same value for every point of the 16 x 16 area") {
			for (int x = 0; x < 16; ++x)
				for (int y = 0; y < 16; ++y)
					REQUIRE(precomputed.getValue(x, y) == onTheFly.getValue(x, y));
		}
	}
}

SCENARIO("Normalized heights always lie between 0 and 1", "[generation][noise]") {
	GIVEN("a map with a large scale so values vary a lot") {
		const Perlin2DMap map(16, 16, 4, 0.37f);

		THEN("every normalized value is in [0, 1], including at negative coordinates") {
			for (int x = -200; x <= 200; x += 3) {
				for (int y = -200; y <= 200; y += 3) {
					const float value = map.getNormalizedValue(x, y);
					REQUIRE(value >= 0.0f);
					REQUIRE(value <= 1.0f);
				}
			}
		}
	}

	GIVEN("a map sampled only on the noise grid, where Perlin noise is always 0") {
		const Perlin2DMap map(4, 4, 1, 1.0f);

		THEN("the raw value is 0 and the normalized value is the midpoint 0.5") {
			REQUIRE(map.getValue(2, 3) == 0.0f);
			REQUIRE(map.getNormalizedValue(2, 3) == 0.5f);
			REQUIRE(map.getNormalizedValue(-5, 9) == 0.5f);
		}
	}
}
