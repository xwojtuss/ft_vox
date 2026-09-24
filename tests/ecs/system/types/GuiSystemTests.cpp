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
constexpr const char*	playerPanel = "Player Components";
constexpr const char*	eventsPanel = "EventsRuntime";

struct SlowSystem {
	void	onSimulate(const ecs::SimulateEvent&) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
};

struct GuiWorld {
	test::TestWorld		testWorld;
	test::FakeGui		gui;
	test::FakeRenderer	renderer;
	ecs::EntityHandle	player = test::createPlayer(testWorld);

	GuiWorld() {
		testWorld.addSystem<ecs::GuiSystem>(gui);
	}

	void	worldReady() {
		testWorld.world.getSystemManager().onWorldReady();
	}

	void	press(render::input::InputEvents events) {
		testWorld.dispatcher().emit(test::inputFrom(player, test::pressing(events)));
	}

	void	renderFrame() {
		gui.openedWindows.clear();
		gui.texts.clear();
		testWorld.world.getSystemManager().onRendererFrame(renderer);
	}

	bool	isShown(const std::string& window) const {
		return std::find(gui.openedWindows.begin(), gui.openedWindows.end(), window) != gui.openedWindows.end();
	}

	std::string	textContaining(const std::string& part) const {
		for (const std::string& text : gui.texts)
			if (text.find(part) != std::string::npos)
				return text;
		return "";
	}
};
}

SCENARIO("The GUI is drawn as its own step at the end of every frame", "[ecs][gui]") {
	GIVEN("a GUI with no open panels") {
		GuiWorld env;
		env.worldReady();

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
	GIVEN("a ready world with a player") {
		GuiWorld env;
		env.worldReady();

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

	GIVEN("a world that is not ready yet") {
		GuiWorld env;

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
	GIVEN("a ready world where a simulation step takes about 20 ms") {
		GuiWorld	env;
		SlowSystem	slowSystem;
		env.testWorld.dispatcher().subscribe(&slowSystem, &SlowSystem::onSimulate);
		env.worldReady();

		WHEN("the events panel is opened after a simulation step") {
			env.press(input::InputEvent::EventRuntimesMenuToggle);
			env.testWorld.simulate(0.016f, 1.0f);
			env.renderFrame();

			THEN("the panel is shown and lists the simulate event") {
				REQUIRE(env.isShown(eventsPanel));
				REQUIRE_THAT(env.textContaining("SimulateEvent"), ContainsSubstring("SimulateEvent Runtime: "));
			}
		}
	}
}

// TODO: make pass
SCENARIO("Event runtimes are shown in milliseconds", "[ecs][gui]") {
	GIVEN("a ready world where a simulation step takes about 20 ms") {
		GuiWorld	env;
		SlowSystem	slowSystem;
		env.testWorld.dispatcher().subscribe(&slowSystem, &SlowSystem::onSimulate);
		env.worldReady();

		WHEN("the events panel is opened after a simulation step") {
			env.press(input::InputEvent::EventRuntimesMenuToggle);
			env.testWorld.simulate(0.016f, 1.0f);
			env.renderFrame();

			THEN("the simulate event is shown as taking at least 20 ms") {
				const std::string	line = env.textContaining("SimulateEvent Runtime: ");
				const float			shownMilliseconds = std::stof(line.substr(line.find(": ") + 2));
				REQUIRE(shownMilliseconds >= 20.0f);
			}
		}
	}
}
