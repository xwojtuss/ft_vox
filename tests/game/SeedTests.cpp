#include <catch2/catch_test_macros.hpp>

#include "game/Seed.hpp"

using game::Seed;

SCENARIO("A seed can be a number", "[seed]") {
	GIVEN("the number 12345") {
		const Seed seed(12345);

		THEN("its value is that number") {
			REQUIRE(seed.value() == 12345);
		}
	}

	GIVEN("two seeds made from the same number") {
		THEN("they are equal") {
			REQUIRE(Seed(7) == Seed(7));
			REQUIRE_FALSE(Seed(7) == Seed(8));
		}
	}
}

SCENARIO("A seed can be any text", "[seed]") {
	GIVEN("the same text twice") {
		THEN("both give the same seed") {
			REQUIRE(Seed::fromText("hello world") == Seed::fromText("hello world"));
		}
	}

	GIVEN("two different texts") {
		THEN("they give different seeds") {
			REQUIRE_FALSE(Seed::fromText("hello") == Seed::fromText("Hello"));
			REQUIRE_FALSE(Seed::fromText("") == Seed::fromText(" "));
		}
	}

	GIVEN("a text seed") {
		THEN("its value is fixed, so it is the same on every platform and compiler") {
			REQUIRE(Seed::fromText("hello").value() == 0xa430d84680aabd0bULL);
			REQUIRE(Seed::fromText("").value() == 14695981039346656037ULL);
		}
	}
}

SCENARIO("Text that is a number gives the same seed as that number", "[seed]") {
	GIVEN("the text \"12345\"") {
		THEN("it equals the number 12345") {
			REQUIRE(Seed::fromText("12345") == Seed(12345));
		}
	}

	GIVEN("a negative number as text") {
		THEN("it equals that number") {
			REQUIRE(Seed::fromText("-1") == Seed(static_cast<std::uint64_t>(-1)));
		}
	}

	GIVEN("text that only starts like a number") {
		THEN("it is treated as text") {
			REQUIRE_FALSE(Seed::fromText("12345abc") == Seed(12345));
			REQUIRE_FALSE(Seed::fromText(" 12345") == Seed(12345));
		}
	}

	GIVEN("a number too large for a 64-bit integer") {
		THEN("it is treated as text instead of failing") {
			REQUIRE(Seed::fromText("99999999999999999999") == Seed::fromText("99999999999999999999"));
		}
	}
}

SCENARIO("A random seed is different every time", "[seed]") {
	GIVEN("two random seeds") {
		THEN("they differ") {
			REQUIRE_FALSE(Seed::random() == Seed::random());
		}
	}
}
