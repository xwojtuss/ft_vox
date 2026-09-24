#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "support/TestEcs.hpp"

using HealthManager = ecs::ComponentManager<test::Health>;

namespace {
// Managers hold room for every component up front, so keep them off the stack
std::unique_ptr<HealthManager>	makeManager() {
	return std::make_unique<HealthManager>();
}
}

SCENARIO("An entity's component can be added, read and checked", "[ecs][component-manager]") {
	GIVEN("an empty component manager") {
		auto manager = makeManager();

		THEN("it holds nothing") {
			REQUIRE(manager->getComponentCount() == 0);
			REQUIRE_FALSE(manager->hasComponent(1));
			REQUIRE(manager->getComponent(1) == nullptr);
		}

		WHEN("entity 1 gets a Health component with 50 points") {
			manager->addComponent(1, test::Health(50));

			THEN("the entity has it and its data can be read back") {
				REQUIRE(manager->hasComponent(1));
				REQUIRE(manager->getComponent(1)->points == 50);
				REQUIRE(manager->getComponentCount() == 1);
			}
			AND_THEN("it can be read as a generic component too") {
				ecs::IComponent* component = nullptr;
				manager->getComponent(1, component);

				REQUIRE(component != nullptr);
				REQUIRE(component->getName() == "Health");
			}
			AND_THEN("other entities still have nothing") {
				ecs::IComponent* component = reinterpret_cast<ecs::IComponent*>(0x1);
				manager->getComponent(2, component);

				REQUIRE_FALSE(manager->hasComponent(2));
				REQUIRE(component == nullptr);
			}
			AND_THEN("changes made through the returned pointer are kept") {
				manager->getComponent(1)->points = 10;
				REQUIRE(manager->getComponent(1)->points == 10);
			}
		}
	}
}

SCENARIO("Components can be walked by index", "[ecs][component-manager]") {
	GIVEN("a manager with components for entities 1, 2 and 3") {
		auto manager = makeManager();
		manager->addComponent(1, test::Health(10));
		manager->addComponent(2, test::Health(20));
		manager->addComponent(3, test::Health(30));

		THEN("each index up to the count returns a component, in insertion order") {
			REQUIRE(manager->getComponentCount() == 3);
			REQUIRE(manager->getComponentAtIndex(0)->points == 10);
			REQUIRE(manager->getComponentAtIndex(1)->points == 20);
			REQUIRE(manager->getComponentAtIndex(2)->points == 30);
		}
		AND_THEN("an index past the count is rejected") {
			REQUIRE_THROWS_AS(manager->getComponentAtIndex(3), std::runtime_error);
		}
	}
}

SCENARIO("Removing a component", "[ecs][component-manager]") {
	GIVEN("entities 1 and 2 with 10 and 20 health") {
		auto manager = makeManager();
		manager->addComponent(1, test::Health(10));
		manager->addComponent(2, test::Health(20));

		WHEN("entity 1's component is removed") {
			manager->removeComponent(1);

			THEN("entity 1 no longer has one") {
				REQUIRE_FALSE(manager->hasComponent(1));
				REQUIRE(manager->getComponent(1) == nullptr);
			}
			AND_THEN("entity 2 keeps its own") {
				REQUIRE(manager->hasComponent(2));
				REQUIRE(manager->getComponent(2)->points == 20);
			}
			AND_THEN("the count goes down and the remaining component is packed to the front") {
				REQUIRE(manager->getComponentCount() == 1);
				REQUIRE(manager->getComponentAtIndex(0)->points == 20);
			}
		}

		WHEN("a component is removed from an entity that has none") {
			THEN("it is rejected") {
				REQUIRE_THROWS_AS(manager->removeComponent(99), std::runtime_error);
			}
		}
	}
}

// TODO: make pass
SCENARIO("Removing one entity's component never changes another entity's data", "[ecs][component-manager]") {
	GIVEN("entities 1 and 2 with 10 and 20 health") {
		auto manager = makeManager();
		manager->addComponent(1, test::Health(10));
		manager->addComponent(2, test::Health(20));

		WHEN("entity 1's component is removed and entity 3 then gets 30 health") {
			manager->removeComponent(1);
			manager->addComponent(3, test::Health(30));

			THEN("entity 2 still has 20 health") {
				REQUIRE(manager->getComponent(2)->points == 20);
			}
		}
	}
}

SCENARIO("A component manager has a fixed capacity", "[ecs][component-manager]") {
	GIVEN("a manager filled up to its capacity") {
		auto manager = makeManager();
		for (ecs::Entity entity = 0; entity < ecs::maxComponents; ++entity)
			manager->addComponent(entity, test::Health(entity));

		THEN("every component was stored") {
			REQUIRE(manager->getComponentCount() == static_cast<size_t>(ecs::maxComponents));
		}
		AND_THEN("adding one more is rejected") {
			REQUIRE_THROWS_AS(manager->addComponent(ecs::maxComponents, test::Health()), std::runtime_error);
		}
	}
}

SCENARIO("A component manager can be copied", "[ecs][component-manager]") {
	GIVEN("a manager with entity 1 at 10 health") {
		auto original = makeManager();
		original->addComponent(1, test::Health(10));

		WHEN("it is assigned to another manager") {
			auto copy = makeManager();
			*copy = *original;

			THEN("the copy holds the same components") {
				REQUIRE(copy->getComponentCount() == 1);
				REQUIRE(copy->getComponent(1)->points == 10);
			}
			AND_THEN("changing the copy leaves the original alone") {
				copy->getComponent(1)->points = 99;
				REQUIRE(original->getComponent(1)->points == 10);
			}
		}

		WHEN("it is assigned to itself") {
			HealthManager& same = *original;
			*original = same;

			THEN("nothing changes") {
				REQUIRE(original->getComponentCount() == 1);
				REQUIRE(original->getComponent(1)->points == 10);
			}
		}
	}
}
