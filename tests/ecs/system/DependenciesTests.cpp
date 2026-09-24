#include <catch2/catch_test_macros.hpp>

#include "support/TestEcs.hpp"

SCENARIO("A system's dependencies list the components it needs", "[ecs][dependencies]") {
	GIVEN("no dependencies") {
		ecs::Dependencies dependencies;

		THEN("nothing is required") {
			REQUIRE(dependencies.mask.none());
			REQUIRE_FALSE(dependencies.includes<test::Health>());
		}

		WHEN("Health is added") {
			dependencies.addDependency<test::Health>();

			THEN("Health is required, Armor is not") {
				REQUIRE(dependencies.includes<test::Health>());
				REQUIRE_FALSE(dependencies.includes<test::Armor>());
			}

			AND_WHEN("Health is removed again") {
				dependencies.removeDependency<test::Health>();

				THEN("nothing is required") {
					REQUIRE(dependencies.mask.none());
				}
			}
		}
	}
}

SCENARIO("Dependencies are satisfied only when every required component is present", "[ecs][dependencies]") {
	GIVEN("a system that needs Health and Armor") {
		ecs::Dependencies required;
		required.addDependency<test::Health>();
		required.addDependency<test::Armor>();

		ecs::Dependencies healthOnly;
		healthOnly.addDependency<test::Health>();

		ecs::Dependencies both = healthOnly;
		both.addDependency<test::Armor>();

		THEN("something with only Health does not satisfy it") {
			REQUIRE_FALSE(required.matches(healthOnly));
		}
		AND_THEN("something with Health and Armor does") {
			REQUIRE(required.matches(both));
		}
		AND_THEN("needing nothing is satisfied by anything") {
			REQUIRE(ecs::Dependencies().matches(healthOnly));
			REQUIRE(ecs::Dependencies().matches(ecs::Dependencies()));
		}
	}
}
