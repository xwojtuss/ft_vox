#include <catch2/catch_test_macros.hpp>

#include "error/Exception.hpp"
#include "support/Blocks.hpp"
#include "support/TestEcs.hpp"

using test::Armor;
using test::Health;
using test::HealthSystem;

namespace {
	std::vector<std::string> componentNames(const ecs::World& world, const ecs::Entity entity) {
		std::vector<std::string> names;
		for (const ecs::ComponentDescription& description: world.describe(entity))
			names.push_back(description.name);
		return names;
	}
}

SCENARIO("Every new entity gets its own id", "[ecs][world]") {
	GIVEN("a world") {
		test::TestWorld testWorld;

		WHEN("three entities are created") {
			const ecs::EntityHandle first  = testWorld.createEntity();
			const ecs::EntityHandle second = testWorld.createEntity();
			const ecs::EntityHandle third  = testWorld.createEntity();

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

SCENARIO("Components are attached to entities through their handle", "[ecs][world][entity]") {
	GIVEN("an entity without components") {
		test::TestWorld   testWorld;
		ecs::EntityHandle entity = testWorld.createEntity();

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
			AND_THEN("the world describes both components for that entity, in the order they were described") {
				REQUIRE(componentNames(testWorld.world, entity.id()) == std::vector<std::string>{"Health", "Armor"});
			}
			AND_THEN("other entities are not affected") {
				REQUIRE(componentNames(testWorld.world, testWorld.createEntity().id()).empty());
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

SCENARIO("A component describes itself for the debug panel", "[ecs][world]") {
	GIVEN("an entity with Health 40") {
		test::TestWorld   testWorld;
		ecs::EntityHandle entity = testWorld.createEntity();
		entity.add(Health{.points = 40});

		THEN("its description has the component's name and formatted value") {
			const std::vector<ecs::ComponentDescription> descriptions = testWorld.world.describe(entity.id());

			REQUIRE(descriptions.size() == 1);
			REQUIRE(descriptions.front().name == "Health");
			REQUIRE(descriptions.front().details == "Points: 40");
		}
	}

	GIVEN("the null entity") {
		test::TestWorld testWorld;

		THEN("it has nothing to describe") {
			REQUIRE(testWorld.world.describe(ecs::nullEntity).empty());
		}
	}
}

SCENARIO("A system processes every entity that has the components it needs", "[ecs][world][systems]") {
	GIVEN("a health system and entities with Health, with Armor only and with nothing") {
		test::TestWorld   testWorld;
		HealthSystem&     system        = testWorld.addSystem<HealthSystem>();
		ecs::EntityHandle withHealth    = testWorld.createEntity();
		ecs::EntityHandle withArmorOnly = testWorld.createEntity();
		ecs::EntityHandle withNothing   = testWorld.createEntity();
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

SCENARIO("A system outside any world processes no entities", "[ecs][world][systems]") {
	GIVEN("a health system that was never added to a world") {
		const HealthSystem system;

		THEN("it processes nothing") {
			REQUIRE_FALSE(system.processes(ecs::nullEntity));
		}
		AND_THEN("asking for its entities is rejected") {
			REQUIRE_THROWS_AS(system.entities(), error::EcsError);
		}
	}
}

SCENARIO("Systems created by the world are connected to it", "[ecs][world]") {
	GIVEN("a world") {
		test::TestWorld testWorld;

		WHEN("a health system is created through the world") {
			HealthSystem& system = testWorld.addSystem<HealthSystem>();

			THEN("it receives the world's events") {
				testWorld.world.getSystemManager().onWorldReady();
				REQUIRE(system.receivedEvents == std::vector<std::string>{"WorldReadyEvent"});
			}
		}
	}
}

SCENARIO("The world keeps the block definitions it was created with", "[ecs][world]") {
	GIVEN("a world created from a block registry") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ecs::World              world(blockDatas);

		THEN("it knows the same blocks") {
			REQUIRE(world.getBlockDatas().getBlockData(test::dirt).prettyName == "Dirt");
			REQUIRE(
				world.getBlockDatas().getBlockData(test::dirt).meshData.indices == blockDatas.getBlockData(test::dirt).
				meshData.indices);
		}
	}
}

SCENARIO("Destroying an entity removes it and all of its components", "[ecs][world]") {
	GIVEN("an entity with Health and Armor next to another entity with Health, and a health system") {
		test::TestWorld   testWorld;
		HealthSystem&     system   = testWorld.addSystem<HealthSystem>();
		ecs::EntityHandle doomed   = testWorld.createEntity();
		ecs::EntityHandle survivor = testWorld.createEntity();
		doomed.add(Health{.points = 1});
		doomed.add(Armor{.rating = 1});
		survivor.add(Health{.points = 2});

		WHEN("the first entity is destroyed") {
			testWorld.world.destroyEntity(doomed.id());

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
				REQUIRE(componentNames(testWorld.world, survivor.id()) == std::vector<std::string>{"Health"});
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
