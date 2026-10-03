#include <catch2/catch_test_macros.hpp>

#include "noise/INoise2D.hpp"

namespace {
	constexpr noise::FractalSettings settings = {.frequency = 0.01f, .octaves = 4};
}

SCENARIO("Noise depends only on its seed and the position", "[noise]") {
	GIVEN("two noise sources created with the same seed") {
		const auto first  = noise::createFractalPerlin2D(42, settings);
		const auto second = noise::createFractalPerlin2D(42, settings);

		THEN("they give the same value everywhere") {
			for (int x = -300; x <= 300; x += 37)
				for (int y = -300; y <= 300; y += 41)
					REQUIRE(first->sample(static_cast<float>(x), static_cast<float>(y)) ==
							second->sample(static_cast<float>(x), static_cast<float>(y)));
		}
		AND_THEN("asking again gives the same value, so sampling keeps no state") {
			const float once = first->sample(12.0f, -7.0f);
			static_cast<void>(first->sample(500.0f, 500.0f));
			REQUIRE(first->sample(12.0f, -7.0f) == once);
		}
	}

	GIVEN("two noise sources with different seeds") {
		const auto first  = noise::createFractalPerlin2D(1, settings);
		const auto second = noise::createFractalPerlin2D(2, settings);

		THEN("they give different values somewhere") {
			bool different = false;
			for (int x = 0; x < 200 && !different; x += 13)
				for (int y = 0; y < 200 && !different; y += 17)
					different = first->sample(static_cast<float>(x), static_cast<float>(y)) !=
								second->sample(static_cast<float>(x), static_cast<float>(y));
			REQUIRE(different);
		}
	}
}

SCENARIO("Noise values stay between -1 and 1", "[noise]") {
	GIVEN("a noise source with a large frequency so values vary a lot") {
		const auto source = noise::createFractalPerlin2D(9, {.frequency = 0.37f, .octaves = 4});

		THEN("every sample is inside the range, including at negative positions") {
			for (int x = -200; x <= 200; x += 3) {
				for (int y = -200; y <= 200; y += 3) {
					const float value = source->sample(static_cast<float>(x), static_cast<float>(y));
					REQUIRE(value >= -1.0f);
					REQUIRE(value <= 1.0f);
				}
			}
		}
	}
}
