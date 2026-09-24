#include <catch2/catch_test_macros.hpp>

#include "ecs/component/Components.hpp"
#include "ecs/system/types/WindowControlSystem.hpp"
#include "support/FakeGui.hpp"
#include "support/FakeWindow.hpp"
#include "support/Player.hpp"

using ecs::component::Input;
using ecs::component::Transform;
using ecs::component::Velocity;
namespace input = render::input;

namespace {
	void press(test::TestWorld& testWorld, ecs::EntityHandle& player, render::input::InputEvents events) {
		player.getComponent<Input>()->command = test::pressing(events);
		testWorld.dispatcher().emit(test::inputFrom(player, player.getComponent<Input>()->command));
	}

	bool isFrozen(ecs::EntityHandle& player) {
		return !player.getComponent<Transform>()->canRotate && !player.getComponent<Velocity>()->canMove;
	}

	bool isFree(ecs::EntityHandle& player) {
		return player.getComponent<Transform>()->canRotate && player.getComponent<Velocity>()->canMove;
	}
}

SCENARIO("Releasing the cursor freezes the player until they click back into the game", "[ecs][window-control]") {
	GIVEN("a player playing with the cursor captured") {
		test::TestWorld  testWorld;
		test::FakeWindow window;
		test::FakeGui    gui;
		testWorld.addSystem<ecs::WindowControlSystem>(window, gui);
		ecs::EntityHandle player = test::createPlayer(testWorld);
		player.registerToSystem<ecs::WindowControlSystem>();

		WHEN("they press the cursor toggle") {
			press(testWorld, player, input::InputEvent::ToggleCursor);

			THEN("the cursor is shown in the middle of the window") {
				REQUIRE(window.cursorVisible);
				REQUIRE(window.cursorCenterings == 1);
			}
			AND_THEN("the player can neither move nor look around") {
				REQUIRE(isFrozen(player));
			}

			AND_WHEN("they click into the game") {
				press(testWorld, player, input::InputEvent::ActionButton);

				THEN("the cursor is captured again and the player can move and look around") {
					REQUIRE_FALSE(window.cursorVisible);
					REQUIRE(isFree(player));
				}
			}

			AND_WHEN("they click on a menu") {
				gui.capturingMouse = true;
				press(testWorld, player, input::InputEvent::SecondaryButton);

				THEN("the cursor stays visible and the player stays frozen") {
					REQUIRE(window.cursorVisible);
					REQUIRE(isFrozen(player));
				}
			}
		}

		WHEN("they click while the cursor is already captured") {
			press(testWorld, player, input::InputEvent::ActionButton);

			THEN("nothing changes") {
				REQUIRE_FALSE(window.cursorVisible);
				REQUIRE(isFree(player));
			}
		}
	}

	GIVEN("an entity with only an Input component") {
		test::TestWorld  testWorld;
		test::FakeWindow window;
		test::FakeGui    gui;
		testWorld.addSystem<ecs::WindowControlSystem>(window, gui);
		ecs::EntityHandle controller = testWorld.createEntity();
		Input             input;
		input.command = test::pressing(input::InputEvent::ToggleCursor);
		controller.addComponent(input);
		controller.registerToSystem<ecs::WindowControlSystem>();

		THEN("it can still toggle the cursor both ways") {
			testWorld.dispatcher().emit(test::inputFrom(controller, input.command));
			REQUIRE(window.cursorVisible);

			controller.getComponent<Input>()->command = test::pressing(input::InputEvent::ActionButton);
			testWorld.dispatcher().emit(test::inputFrom(controller, input.command));
			REQUIRE_FALSE(window.cursorVisible);
		}
	}
}
