#include <catch2/catch_test_macros.hpp>

#include "support/FakeRenderer.hpp"
#include "support/TestEcs.hpp"

using test::HealthSystem;

SCENARIO("Systems are stored by type", "[ecs][systems]") {
	GIVEN("a system manager") {
		ecs::SystemManager manager;

		THEN("asking for a system that was never added gives nothing") {
			REQUIRE(manager.getSystem<HealthSystem>() == nullptr);
		}

		WHEN("a health system is added") {
			HealthSystem& added = manager.addSystem<HealthSystem>();

			THEN("asking for it returns that same system") {
				REQUIRE(manager.getSystem<HealthSystem>() == &added);
			}
		}
	}
}

SCENARIO("The system manager turns engine phases into events", "[ecs][systems]") {
	GIVEN("a system manager with a health system listening to every event") {
		ecs::SystemManager	manager;
		test::FakeRenderer	renderer;
		HealthSystem&		system = manager.addSystem<HealthSystem>();
		system.bindEvents(manager.getDispatcher());

		WHEN("the world becomes ready") {
			manager.onWorldReady();

			THEN("a world-ready event is sent") {
				REQUIRE(system.receivedEvents == std::vector<std::string>{"WorldReadyEvent"});
			}
		}

		WHEN("the simulation advances by 0.016 s") {
			manager.onSimulate(0.016f, 3.0f);

			THEN("a simulate event carrying the time step is sent") {
				REQUIRE(system.receivedEvents == std::vector<std::string>{"SimulateEvent"});
				REQUIRE(system.lastDeltaTime == 0.016f);
			}
		}

		WHEN("a frame is rendered with a 16:9 window") {
			manager.onRender(16.0f / 9.0f, 1.0);

			THEN("a render event carrying the aspect ratio is sent") {
				REQUIRE(system.receivedEvents == std::vector<std::string>{"RenderEvent"});
				REQUIRE(system.lastAspectRatio == 16.0f / 9.0f);
			}
		}

		WHEN("the renderer draws the scene, the text and finishes the frame") {
			manager.onRendererDraw(renderer);
			manager.onTextDraw(renderer);
			manager.onRendererFrame(renderer);

			THEN("one event per step is sent, each carrying the renderer") {
				REQUIRE(system.receivedEvents == std::vector<std::string>{"RendererDrawEvent", "TextDrawEvent", "RendererFrameEvent"});
				REQUIRE(system.lastRenderer == &renderer);
			}
		}
	}
}

SCENARIO("A system only accepts entities that have the components it needs", "[ecs][systems]") {
	GIVEN("a world with a health system and three entities") {
		game::block::BlockDatas	blockDatas(assets::MeshData{}, assets::TextureData{});
		ecs::World				world(blockDatas);
		world.createSystem<HealthSystem>();
		HealthSystem&			system = *world.getSystemManager().getSystem<HealthSystem>();

		ecs::EntityHandle	withHealth = world.createEntity();
		ecs::EntityHandle	withArmorOnly = world.createEntity();
		ecs::EntityHandle	withNothing = world.createEntity();
		withHealth.addComponent(test::Health(10));
		withArmorOnly.addComponent(test::Armor(5));

		THEN("the system knows what it needs") {
			REQUIRE(system.getDependencies().includes<test::Health>());
		}

		WHEN("all three are registered") {
			system.registerEntity(withHealth.entity);
			system.registerEntity(withArmorOnly.entity);
			system.registerEntity(withNothing.entity);

			THEN("only the entity with Health is accepted") {
				REQUIRE(system.hasEntity(withHealth.entity));
				REQUIRE_FALSE(system.hasEntity(withArmorOnly.entity));
				REQUIRE_FALSE(system.hasEntity(withNothing.entity));
				REQUIRE(system.entities().size() == 1);
			}

			AND_WHEN("the accepted entity is unregistered") {
				system.unregisterEntity(withHealth.entity);

				THEN("the system no longer has it") {
					REQUIRE_FALSE(system.hasEntity(withHealth.entity));
					REQUIRE(system.entities().empty());
				}
			}
		}

		WHEN("an entity that was never registered is unregistered") {
			system.registerEntity(withHealth.entity);
			system.unregisterEntity(withNothing.entity);

			THEN("nothing changes") {
				REQUIRE(system.entities().size() == 1);
			}
		}
	}
}

SCENARIO("A system outside any world accepts no entities", "[ecs][systems]") {
	GIVEN("a health system that was never added to a world") {
		HealthSystem system;

		WHEN("an entity is registered") {
			system.registerEntity(1);

			THEN("it is refused, because there is no world to check its components in") {
				REQUIRE_FALSE(system.hasEntity(1));
			}
		}
	}
}

// TODO: make pass
SCENARIO("Registering the same entity twice keeps it once", "[ecs][systems]") {
	GIVEN("a world with a health system and an entity with Health") {
		game::block::BlockDatas	blockDatas(assets::MeshData{}, assets::TextureData{});
		ecs::World				world(blockDatas);
		world.createSystem<HealthSystem>();
		HealthSystem&			system = *world.getSystemManager().getSystem<HealthSystem>();
		ecs::EntityHandle		entity = world.createEntity();
		entity.addComponent(test::Health());

		WHEN("the entity is registered twice") {
			system.registerEntity(entity.entity);
			system.registerEntity(entity.entity);

			THEN("the system holds it only once") {
				REQUIRE(system.entities().size() == 1);
			}
		}
	}
}
