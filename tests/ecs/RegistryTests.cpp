#include <catch2/catch_test_macros.hpp>

#include "error/Exception.hpp"
#include "support/Blocks.hpp"
#include "support/TestEcs.hpp"

using test::Armor;
using test::Health;
using test::HealthSystem;

namespace {
	std::vector<std::string> componentNames(const ecs::Registry& registry, const ecs::Entity entity) {
		std::vector<std::string> names;
		for (const ecs::ComponentDescription& description: registry.describe(entity))
			names.push_back(description.name);
		return names;
	}
}

SCENARIO("Every new entity gets its own id", "[ecs][registry]") {
	GIVEN("a registry") {
		test::TestRegistry testRegistry;

		WHEN("three entities are created") {
			const ecs::EntityHandle first  = testRegistry.createEntity();
			const ecs::EntityHandle second = testRegistry.createEntity();
			const ecs::EntityHandle third  = testRegistry.createEntity();

			THEN("their ids are distinct and none of them is the null entity") {
				REQUIRE(first.id() != second.id());
				REQUIRE(second.id() != third.id());
				REQUIRE(first.id() != third.id());
				REQUIRE(first.id() != ecs::nullEntity);
			}
			AND_THEN("they are alive") {
				REQUIRE(first.isAlive());
				REQUIRE(third.isAlive());
			}
		}
	}
}

SCENARIO("Components are attached to entities through their handle", "[ecs][registry][entity]") {
	GIVEN("an entity without components") {
		test::TestRegistry testRegistry;
		ecs::EntityHandle  entity = testRegistry.createEntity();

		THEN("it has no Health") {
			REQUIRE_FALSE(entity.has<Health>());
			REQUIRE(entity.tryGet<Health>() == nullptr);
		}
		AND_THEN("asking for its Health is rejected") {
			REQUIRE_THROWS_AS(entity.get<Health>(), error::EcsError);
		}

		WHEN("it gets Health 40 and Armor 3") {
			entity.add(Health{.points = 40});
			entity.add(Armor{.rating = 3});

			THEN("both can be read back") {
				REQUIRE(entity.has<Health>());
				REQUIRE(entity.get<Health>().points == 40);
				REQUIRE(entity.get<Armor>().rating == 3);
			}
			AND_THEN("the registry describes both components for that entity, in the order they were described") {
				REQUIRE(componentNames(testRegistry.registry, entity.id()) ==
						std::vector<std::string>{"Health", "Armor"});
			}
			AND_THEN("other entities are not affected") {
				REQUIRE(componentNames(testRegistry.registry, testRegistry.createEntity().id()).empty());
			}

			AND_WHEN("it gets Health again") {
				entity.add(Health{.points = 7});

				THEN("the new value replaces the old one") {
					REQUIRE(entity.get<Health>().points == 7);
				}
			}

			AND_WHEN("the Armor is removed") {
				entity.remove<Armor>();

				THEN("only Health remains") {
					REQUIRE_FALSE(entity.has<Armor>());
					REQUIRE(entity.has<Health>());
				}
			}
		}

		WHEN("a component it does not have is removed") {
			THEN("nothing happens") {
				REQUIRE_NOTHROW(entity.remove<Armor>());
				REQUIRE_FALSE(entity.has<Armor>());
			}
		}
	}
}

SCENARIO("A component describes itself for the debug panel", "[ecs][registry]") {
	GIVEN("an entity with Health 40") {
		test::TestRegistry testRegistry;
		ecs::EntityHandle  entity = testRegistry.createEntity();
		entity.add(Health{.points = 40});

		THEN("its description has the component's name and formatted value") {
			const std::vector<ecs::ComponentDescription> descriptions = testRegistry.registry.describe(entity.id());

			REQUIRE(descriptions.size() == 1);
			REQUIRE(descriptions.front().name == "Health");
			REQUIRE(descriptions.front().details == "Points: 40");
		}
	}

	GIVEN("the null entity") {
		test::TestRegistry const testRegistry;

		THEN("it has nothing to describe") {
			REQUIRE(testRegistry.registry.describe(ecs::nullEntity).empty());
		}
	}
}

SCENARIO("A system processes every entity that has the components it needs", "[ecs][registry][systems]") {
	GIVEN("a health system and entities with Health, with Armor only and with nothing") {
		test::TestRegistry testRegistry;
		auto&              system        = testRegistry.addSystem<HealthSystem>();
		ecs::EntityHandle  withHealth    = testRegistry.createEntity();
		ecs::EntityHandle  withArmorOnly = testRegistry.createEntity();
		ecs::EntityHandle  withNothing   = testRegistry.createEntity();
		withHealth.add(Health{});
		withArmorOnly.add(Armor{});

		THEN("only the entity with Health is processed, without registering it") {
			REQUIRE(system.processes(withHealth.id()));
			REQUIRE_FALSE(system.processes(withArmorOnly.id()));
			REQUIRE_FALSE(system.processes(withNothing.id()));
			REQUIRE(system.entities().count() == 1);
		}

		WHEN("the entity loses its Health") {
			withHealth.remove<Health>();

			THEN("the system stops processing it") {
				REQUIRE_FALSE(system.processes(withHealth.id()));
				REQUIRE(system.entities().empty());
			}
		}

		WHEN("another entity gains Health") {
			withNothing.add(Health{});

			THEN("the system processes it too") {
				REQUIRE(system.processes(withNothing.id()));
				REQUIRE(system.entities().count() == 2);
			}
		}
	}
}

SCENARIO("A system outside any registry processes no entities", "[ecs][registry][systems]") {
	GIVEN("a health system that was never added to a registry") {
		const HealthSystem system;

		THEN("it processes nothing") {
			REQUIRE_FALSE(system.processes(ecs::nullEntity));
		}
		AND_THEN("asking for its entities is rejected") {
			REQUIRE_THROWS_AS(system.entities(), error::EcsError);
		}
	}
}

SCENARIO("Systems created by the registry are connected to it", "[ecs][registry]") {
	GIVEN("a registry") {
		test::TestRegistry testRegistry;

		WHEN("a health system is created through the registry") {
			auto& system = testRegistry.addSystem<HealthSystem>();

			THEN("it receives the registry's events") {
				testRegistry.registry.getSystemManager().onRegistryReady();
				REQUIRE(system.receivedEvents == std::vector<std::string>{"RegistryReadyEvent"});
			}
		}
	}
}

SCENARIO("The registry keeps the block definitions it was created with", "[ecs][registry]") {
	GIVEN("a registry created with block definitions") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ecs::Registry           registry(blockDatas);

		THEN("it knows the same blocks") {
			REQUIRE(registry.getBlockDatas().getBlockData(test::dirt).prettyName == "Dirt");
			REQUIRE(registry.getBlockDatas().getBlockData(test::dirt).meshData.indices ==
					blockDatas.getBlockData(test::dirt).meshData.indices);
		}
	}
}

SCENARIO("Destroying an entity removes it and all of its components", "[ecs][registry]") {
	GIVEN("an entity with Health and Armor next to another entity with Health, and a health system") {
		test::TestRegistry testRegistry;
		auto&              system   = testRegistry.addSystem<HealthSystem>();
		ecs::EntityHandle  doomed   = testRegistry.createEntity();
		ecs::EntityHandle  survivor = testRegistry.createEntity();
		doomed.add(Health{.points = 1});
		doomed.add(Armor{.rating = 1});
		survivor.add(Health{.points = 2});

		WHEN("the first entity is destroyed") {
			testRegistry.registry.destroyEntity(doomed.id());

			THEN("it is no longer alive and has no components left") {
				REQUIRE_FALSE(doomed.isAlive());
				REQUIRE_FALSE(doomed.has<Health>());
				REQUIRE_FALSE(doomed.has<Armor>());
			}
			AND_THEN("the system no longer processes it") {
				REQUIRE_FALSE(system.processes(doomed.id()));
				REQUIRE(system.entities().count() == 1);
			}
			AND_THEN("the other entity keeps its own components") {
				REQUIRE(survivor.get<Health>().points == 2);
				REQUIRE(componentNames(testRegistry.registry, survivor.id()) == std::vector<std::string>{"Health"});
			}
			AND_THEN("adding a component to the destroyed entity is rejected") {
				REQUIRE_THROWS_AS(doomed.add(Health{}), error::EcsError);
			}
			AND_THEN("destroying it again does nothing") {
				REQUIRE_NOTHROW(doomed.destroy());
			}
		}
	}
}
