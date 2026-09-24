#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "support/Blocks.hpp"
#include "support/TestEcs.hpp"

using test::Armor;
using test::Health;
using test::HealthSystem;

namespace {
std::vector<std::string>	componentNames(ecs::World& world, ecs::Entity entity) {
	std::vector<std::string> names;
	for (ecs::IComponent* component : world.getAllComponents(entity))
		names.push_back(component->getName());
	std::sort(names.begin(), names.end());
	return names;
}
}

SCENARIO("Every new entity gets its own id", "[ecs][world]") {
	GIVEN("a world") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);

		WHEN("three entities are created") {
			const ecs::EntityHandle first = world.createEntity();
			const ecs::EntityHandle second = world.createEntity();
			const ecs::EntityHandle third = world.createEntity();

			THEN("they get increasing, distinct ids starting at 1") {
				REQUIRE(first.entity == 1);
				REQUIRE(second.entity == 2);
				REQUIRE(third.entity == 3);
			}
			AND_THEN("each handle belongs to this world") {
				REQUIRE(first.world == &world);
			}
		}
	}
}

SCENARIO("A world creates one component manager per component type, on demand", "[ecs][world]") {
	GIVEN("a world where no Health component was used yet") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);
		const int				healthId = ecs::Component<Health>::getId();

		THEN("there is no Health manager yet") {
			REQUIRE(world.getComponentManager(healthId) == nullptr);
		}

		WHEN("the Health manager is asked for") {
			ecs::ComponentManager<Health>& manager = world.getComponentManager<Health>();

			THEN("it is created, and asking again returns the same manager") {
				REQUIRE(&world.getComponentManager<Health>() == &manager);
				REQUIRE(world.getComponentManager(healthId) == &manager);
			}
		}
	}
}

SCENARIO("Components are attached to entities through their handle", "[ecs][world][entity]") {
	GIVEN("an entity without components") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);
		ecs::EntityHandle		entity = world.createEntity();

		THEN("it has no Health") {
			REQUIRE_FALSE(entity.hasComponent<Health>());
			REQUIRE(entity.getComponent<Health>() == nullptr);
		}

		WHEN("it gets Health 40 and Armor 3") {
			entity.addComponent(Health(40));
			entity.addComponent(Armor(3));

			THEN("both can be read back") {
				REQUIRE(entity.hasComponent<Health>());
				REQUIRE(entity.getComponent<Health>()->points == 40);
				REQUIRE(entity.getComponent<Armor>()->rating == 3);
			}
			AND_THEN("the world lists both components for that entity") {
				REQUIRE(componentNames(world, entity.entity) == std::vector<std::string>{"Armor", "Health"});
			}
			AND_THEN("other entities are not affected") {
				REQUIRE(componentNames(world, world.createEntity().entity).empty());
			}

			AND_WHEN("the Armor is removed") {
				entity.removeComponent<Armor>();

				THEN("only Health remains") {
					REQUIRE_FALSE(entity.hasComponent<Armor>());
					REQUIRE(entity.hasComponent<Health>());
				}
			}
		}

		WHEN("a component it does not have is removed") {
			THEN("it is rejected") {
				REQUIRE_THROWS_AS(entity.removeComponent<Armor>(), std::runtime_error);
			}
		}
	}
}

SCENARIO("Entities join and leave systems through their handle", "[ecs][world][entity]") {
	GIVEN("a world with a health system and an entity with Health") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);
		world.createSystem<HealthSystem>();
		HealthSystem&			system = *world.getSystemManager().getSystem<HealthSystem>();
		ecs::EntityHandle		entity = world.createEntity();
		entity.addComponent(Health());

		WHEN("it is registered to the health system") {
			entity.registerToSystem<HealthSystem>();

			THEN("the system has it") {
				REQUIRE(system.hasEntity(entity.entity));
			}

			AND_WHEN("it is unregistered") {
				entity.unregisterFromSystem<HealthSystem>();

				THEN("the system no longer has it") {
					REQUIRE_FALSE(system.hasEntity(entity.entity));
				}
			}
		}
	}

	GIVEN("a world without a health system") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);
		ecs::EntityHandle		entity = world.createEntity();
		entity.addComponent(Health());

		THEN("registering and unregistering do nothing and do not fail") {
			REQUIRE_NOTHROW(entity.registerToSystem<HealthSystem>());
			REQUIRE_NOTHROW(entity.unregisterFromSystem<HealthSystem>());
		}
	}
}

SCENARIO("Systems created by the world are connected to it", "[ecs][world]") {
	GIVEN("a world") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);

		WHEN("a health system is created through the world") {
			world.createSystem<HealthSystem>();
			HealthSystem* system = world.getSystemManager().getSystem<HealthSystem>();

			THEN("it receives the world's events") {
				world.getSystemManager().onWorldReady();
				REQUIRE(system->receivedEvents == std::vector<std::string>{"WorldReadyEvent"});
			}
		}
	}
}

SCENARIO("The world keeps the block definitions it was created with", "[ecs][world]") {
	GIVEN("a world created from a block registry") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);

		THEN("it knows the same blocks") {
			REQUIRE(world.getBlockDatas().getBlockData(test::dirt).prettyName == "Dirt");
			REQUIRE(world.getBlockDatas().getBlockData(test::dirt).meshData.indices == blockDatas.getBlockData(test::dirt).meshData.indices);
		}
	}
}

SCENARIO("Destroying an entity removes all of its components", "[ecs][world]") {
	GIVEN("an entity with Health and Armor next to another entity with Health") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);
		ecs::EntityHandle		doomed = world.createEntity();
		ecs::EntityHandle		survivor = world.createEntity();
		doomed.addComponent(Health(1));
		doomed.addComponent(Armor(1));
		survivor.addComponent(Health(2));

		WHEN("the first entity is destroyed") {
			world.destroyEntity(doomed.entity);

			THEN("it has no components left") {
				REQUIRE_FALSE(doomed.hasComponent<Health>());
				REQUIRE_FALSE(doomed.hasComponent<Armor>());
			}
			AND_THEN("the other entity keeps its own") {
				REQUIRE(survivor.getComponent<Health>()->points == 2);
			}
		}
	}
}

// TODO: make pass
SCENARIO("A destroyed entity leaves every system", "[ecs][world]") {
	GIVEN("an entity with Health registered to the health system") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);
		world.createSystem<HealthSystem>();
		HealthSystem&			system = *world.getSystemManager().getSystem<HealthSystem>();
		ecs::EntityHandle		entity = world.createEntity();
		entity.addComponent(Health());
		entity.registerToSystem<HealthSystem>();

		WHEN("the entity is destroyed") {
			world.destroyEntity(entity.entity);

			THEN("the system no longer has it") {
				REQUIRE_FALSE(system.hasEntity(entity.entity));
			}
		}
	}
}

// TODO: make pass
SCENARIO("The world keeps working after an entity is destroyed", "[ecs][world][crash]") {
	GIVEN("two entities with Health") {
		game::block::BlockDatas	blockDatas = test::makeBlockDatas();
		ecs::World				world(blockDatas);
		ecs::EntityHandle		doomed = world.createEntity();
		ecs::EntityHandle		survivor = world.createEntity();
		doomed.addComponent(Health(1));
		survivor.addComponent(Health(2));

		WHEN("one of them is destroyed") {
			world.destroyEntity(doomed.entity);

			THEN("the other one's components can still be listed") {
				REQUIRE(componentNames(world, survivor.entity) == std::vector<std::string>{"Health"});
			}
		}
	}
}
