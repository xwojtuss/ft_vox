#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "support/TestEcs.hpp"

SCENARIO("Every component type gets its own id", "[ecs][component]") {
	GIVEN("two different component types") {
		const int healthId = ecs::Component<test::Health>::getId();
		const int armorId = ecs::Component<test::Armor>::getId();

		THEN("their ids differ") {
			REQUIRE(healthId != armorId);
		}
		AND_THEN("a type keeps the same id every time it is asked") {
			REQUIRE(ecs::Component<test::Health>::getId() == healthId);
		}
		AND_THEN("ids start at 1 and fit in a system's 32-bit dependency mask") {
			REQUIRE(healthId >= 1);
			REQUIRE(armorId >= 1);
			REQUIRE(healthId < 32);
			REQUIRE(armorId < 32);
		}
	}
}

SCENARIO("A component knows its name", "[ecs][component]") {
	GIVEN("a Health component") {
		const test::Health health;

		THEN("its name is the one given by its type") {
			REQUIRE(health.getName() == "Health");
		}
		AND_THEN("without a custom description it prints as a generic component") {
			std::ostringstream out;
			out << static_cast<const ecs::IComponent&>(health);
			REQUIRE(out.str() == "Component");
		}
	}
}
