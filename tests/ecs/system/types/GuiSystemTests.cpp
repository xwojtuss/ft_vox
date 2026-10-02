#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <chrono>
#include <thread>

#include "ecs/system/types/GuiSystem.hpp"
#include "support/FakeGui.hpp"
#include "support/FakeRenderer.hpp"
#include "support/Player.hpp"

using Catch::Matchers::ContainsSubstring;
namespace input = render::input;

namespace {
	constexpr const char* playerPanel = "Player Components";
	constexpr const char* eventsPanel = "EventsRuntime";
	constexpr const char* clientPanel = "Client Performance";
	constexpr const char* serverPanel = "Server Performance";

	struct SlowSystem {
		std::chrono::milliseconds delay{20};

		void onSimulate(const ecs::SimulateEvent&) const {
			std::this_thread::sleep_for(delay);
		}
	};

	struct GuiRegistry {
		test::TestRegistry     testRegistry;
		test::FakeGui          gui;
		test::FakeRenderer     renderer;
		ecs::EntityHandle      player = test::createPlayer(testRegistry);
		profiling::ClientStats clientStats;
		profiling::ServerStats serverStats;

		GuiRegistry() {
			testRegistry.addSystem<ecs::GuiSystem>(gui, clientStats, serverStats);
		}

		void registryReady() {
			testRegistry.registry.getSystemManager().onRegistryReady();
		}

		void press(input::InputEvents events) {
			testRegistry.dispatcher().emit(test::inputFrom(player, test::pressing(events)));
		}

		void renderFrame() {
			gui.openedWindows.clear();
			gui.texts.clear();
			testRegistry.registry.getSystemManager().onRendererFrame(renderer);
		}

		[[nodiscard]] bool isShown(const std::string& window) const {
			return std::ranges::find(gui.openedWindows, window) != gui.openedWindows.end();
		}

		[[nodiscard]] std::string textContaining(const std::string& part) const {
			for (const std::string& text: gui.texts)
				if (text.contains(part))
					return text;
			return "";
		}
	};
}

SCENARIO("The GUI is drawn as its own step at the end of every frame", "[ecs][gui]") {
	GIVEN("a GUI with no open panels") {
		GuiRegistry env;
		env.registryReady();

		WHEN("a frame is rendered") {
			env.renderFrame();

			THEN("the GUI frame is opened, closed and handed to the renderer once") {
				REQUIRE(env.gui.calls == std::vector<std::string>{"beginFrame", "endFrame"});
				REQUIRE(env.renderer.guiRenders == 1);
			}
			AND_THEN("no panel is shown") {
				REQUIRE(env.gui.openedWindows.empty());
			}
		}
	}
}

SCENARIO("The player components panel is toggled from the keyboard", "[ecs][gui]") {
	GIVEN("a ready registry with a player") {
		GuiRegistry env;
		env.registryReady();

		WHEN("the player presses the player components toggle") {
			env.press(input::InputEvent::PlayerComponentsMenuToggle);
			env.renderFrame();

			THEN("the panel is shown") {
				REQUIRE(env.isShown(playerPanel));
			}
			AND_THEN("it describes the player's components") {
				REQUIRE_THAT(env.textContaining("Position"), ContainsSubstring("Position: (X:0.00, Y:0.00, Z:0.00)"));
			}

			AND_WHEN("they press it again") {
				env.press(input::InputEvent::PlayerComponentsMenuToggle);
				env.renderFrame();

				THEN("the panel is hidden") {
					REQUIRE_FALSE(env.isShown(playerPanel));
				}
			}
		}

		WHEN("the player presses an unrelated key") {
			env.press(input::InputEvent::Jump);
			env.renderFrame();

			THEN("no panel is shown") {
				REQUIRE(env.gui.openedWindows.empty());
			}
		}
	}

	GIVEN("a registry that is not ready yet") {
		GuiRegistry env;

		WHEN("the player presses the player components toggle") {
			env.press(input::InputEvent::PlayerComponentsMenuToggle);
			env.renderFrame();

			THEN("no panel exists yet, so nothing is shown") {
				REQUIRE(env.gui.openedWindows.empty());
			}
		}
	}
}

SCENARIO("The events panel lists how long each event took", "[ecs][gui]") {
	GIVEN("a ready registry where a simulation step takes about 20 ms") {
		GuiRegistry      env;
		const SlowSystem slowSystem;
		env.testRegistry.dispatcher().subscribe(&slowSystem, &SlowSystem::onSimulate);
		env.registryReady();

		WHEN("the events panel is opened after a simulation step") {
			env.press(input::InputEvent::EventRuntimesMenuToggle);
			env.testRegistry.simulate(0.016f, 1.0f);
			env.renderFrame();

			THEN("the panel is shown and lists the simulate event") {
				REQUIRE(env.isShown(eventsPanel));
				REQUIRE_THAT(env.textContaining("SimulateEvent"), ContainsSubstring("SimulateEvent Runtime: "));
			}
		}
	}
}

SCENARIO("Event runtimes are shown in milliseconds", "[ecs][gui]") {
	GIVEN("a ready registry where a simulation step takes about 20 ms") {
		GuiRegistry      env;
		const SlowSystem slowSystem;
		env.testRegistry.dispatcher().subscribe(&slowSystem, &SlowSystem::onSimulate);
		env.registryReady();

		WHEN("the events panel is opened after a simulation step") {
			env.press(input::InputEvent::EventRuntimesMenuToggle);
			env.testRegistry.simulate(0.016f, 1.0f);
			env.renderFrame();

			THEN("the simulate event is shown as taking at least 20 ms") {
				const std::string line              = env.textContaining("SimulateEvent Runtime: ");
				const float       shownMilliseconds = std::stof(line.substr(line.find(": ") + 2));
				REQUIRE(shownMilliseconds >= 20.0f);
			}
		}
	}
}

SCENARIO("The client performance panel is toggled with its own key", "[ecs][gui][stats]") {
	GIVEN("a ready registry whose client has drawn a frame") {
		GuiRegistry env;
		env.clientStats.frameTimes.record(1.0, 0.010f);
		env.clientStats.frameTimes.record(1.1, 0.030f);
		env.clientStats.drawnMeshes = 12;
		env.clientStats.drawCalls   = 2;
		env.clientStats.triangles   = 3456;
		env.registryReady();

		WHEN("the player presses the client performance toggle") {
			env.press(input::InputEvent::ClientPerformanceToggle);
			env.renderFrame();

			THEN("only the client panel is shown") {
				REQUIRE(env.isShown(clientPanel));
				REQUIRE_FALSE(env.isShown(serverPanel));
			}
			AND_THEN("it shows the average and the worst frame time") {
				REQUIRE_THAT(env.textContaining("Frame time avg"), ContainsSubstring("20.00 ms"));
				REQUIRE_THAT(env.textContaining("Frame time worst"), ContainsSubstring("30.00 ms"));
			}
			AND_THEN("it shows what was drawn") {
				REQUIRE_THAT(env.textContaining("Drawn meshes"), ContainsSubstring("12"));
				REQUIRE_THAT(env.textContaining("Draw calls"), ContainsSubstring("2"));
				REQUIRE_THAT(env.textContaining("Triangles"), ContainsSubstring("3456"));
			}
		}

		WHEN("the player has not pressed the toggle") {
			env.renderFrame();

			THEN("the panel is hidden") {
				REQUIRE_FALSE(env.isShown(clientPanel));
			}
		}
	}
}

SCENARIO("The server performance panel is toggled with its own key", "[ecs][gui][stats]") {
	GIVEN("a ready registry whose server has loaded chunks") {
		GuiRegistry env;
		env.serverStats.loadedChunks      = 125;
		env.serverStats.pendingGeneration = 7;
		env.registryReady();

		WHEN("the player presses the server performance toggle") {
			env.press(input::InputEvent::ServerPerformanceToggle);
			env.renderFrame();

			THEN("only the server panel is shown") {
				REQUIRE(env.isShown(serverPanel));
				REQUIRE_FALSE(env.isShown(clientPanel));
			}
			AND_THEN("it shows the chunk counts") {
				REQUIRE_THAT(env.textContaining("Loaded chunks"), ContainsSubstring("125"));
				REQUIRE_THAT(env.textContaining("waiting to be generated"), ContainsSubstring("7"));
			}
		}
	}
}
