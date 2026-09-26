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
		ecs::SystemManager manager;
		test::FakeRenderer renderer;
		HealthSystem&      system = manager.addSystem<HealthSystem>();
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
				REQUIRE(system.receivedEvents == std::vector<std::string>{"RendererDrawEvent", "TextDrawEvent",
						"RendererFrameEvent"});
				REQUIRE(system.lastRenderer == &renderer);
			}
		}
	}
}
