#include <catch2/catch_test_macros.hpp>

#include "render/input/InputManager.hpp"

namespace input = render::input;
using input::InputAction;
using input::InputEvent;

namespace {
	enum Key {
		W       = 17,
		S       = 31,
		A       = 30,
		D       = 32,
		Space   = 57,
		Control = 29
	};

	struct Controls {
		input::InputManager manager;

		Controls() {
			bind(W, InputEvent::MoveForward);
			bind(S, InputEvent::MoveBackward);
			bind(A, InputEvent::MoveLeft);
			bind(D, InputEvent::MoveRight);
			bind(Space, InputEvent::Jump);
			bind(Control, InputEvent::Crouch);
			manager.getKeyInputProcessor().bindEvent(input::createMouseInput(input::MouseButton::LeftButton, 0),
													InputEvent::ActionButton);
		}

		void bind(Key key, InputEvent event) {
			manager.getKeyInputProcessor().bindEvent(input::createInput(key, 0), event);
		}

		void press(std::initializer_list<Key> keys) {
			for (Key key: keys)
				manager.processKey(key, InputAction::Press, 0);
		}

		void release(std::initializer_list<Key> keys) {
			for (Key key: keys)
				manager.processKey(key, InputAction::Release, 0);
		}
	};
}

SCENARIO("One command describes everything the player is doing at once", "[client][input][command]") {
	GIVEN("a player holding forward, right and jump") {
		Controls controls;
		controls.press({W, D, Space});

		WHEN("a command is built") {
			const input::InputCommand command = controls.manager.buildCommand();

			THEN("it moves forward, right and up at full strength") {
				REQUIRE(command.moveForward == 1.0f);
				REQUIRE(command.moveRight == 1.0f);
				REQUIRE(command.moveUp == 1.0f);
			}
			AND_THEN("it lists all three as just started and active") {
				const input::InputEvents all = InputEvent::MoveForward | InputEvent::MoveRight | InputEvent::Jump;
				REQUIRE(input::hasAllEvents(command.startedEvents, all));
				REQUIRE(input::hasAllEvents(command.activeEvents, all));
			}
		}
	}
}

SCENARIO("Opposite directions cancel out", "[client][input][command]") {
	GIVEN("a player") {
		Controls controls;

		THEN("holding forward and backward together does not move them") {
			controls.press({W, S});
			REQUIRE(controls.manager.buildCommand().moveForward == 0.0f);
		}
		AND_THEN("holding left and right together does not move them") {
			controls.press({A, D});
			REQUIRE(controls.manager.buildCommand().moveRight == 0.0f);
		}
		AND_THEN("holding jump and crouch together does not move them") {
			controls.press({Space, Control});
			REQUIRE(controls.manager.buildCommand().moveUp == 0.0f);
		}
		AND_THEN("backward, left and crouch alone move at full strength the other way") {
			controls.press({S, A, Control});
			const input::InputCommand command = controls.manager.buildCommand();
			REQUIRE(command.moveForward == -1.0f);
			REQUIRE(command.moveRight == -1.0f);
			REQUIRE(command.moveUp == -1.0f);
		}
	}
}

SCENARIO("A released direction stops moving the player", "[client][input][command]") {
	GIVEN("a player who held forward in the last command") {
		Controls controls;
		controls.press({W});
		[[maybe_unused]] const input::InputCommand heldForward = controls.manager.buildCommand();

		WHEN("they release it and a new command is built") {
			controls.release({W});
			const input::InputCommand command = controls.manager.buildCommand();

			THEN("the command no longer moves them and reports the release") {
				REQUIRE(command.moveForward == 0.0f);
				REQUIRE(input::hasEvent(command.releasedEvents, InputEvent::MoveForward));
			}
		}
	}
}

SCENARIO("Mouse movement between two commands is collected into one look", "[client][input][command][mouse]") {
	GIVEN("a mouse that has been seen at (100, 100)") {
		Controls controls;
		controls.manager.processMouseMove(100.0, 100.0);

		THEN("the first position alone does not turn the player") {
			const input::InputCommand command = controls.manager.buildCommand();
			REQUIRE(command.lookRight == 0.0f);
			REQUIRE(command.lookUp == 0.0f);
		}

		WHEN("it moves 10 pixels and then 15 more to the side before the next command") {
			controls.manager.processMouseMove(110.0, 100.0);
			controls.manager.processMouseMove(125.0, 100.0);
			const input::InputCommand command = controls.manager.buildCommand();

			THEN("the command turns by all 25 pixels and does not look up or down") {
				REQUIRE(std::abs(command.lookRight) == 25.0f);
				REQUIRE(command.lookUp == 0.0f);
			}
			AND_THEN("the next command does not turn again") {
				REQUIRE(controls.manager.buildCommand().lookRight == 0.0f);
			}
		}

		WHEN("it moves 20 pixels one way and back before the next command") {
			controls.manager.processMouseMove(120.0, 100.0);
			controls.manager.processMouseMove(100.0, 100.0);

			THEN("the movements cancel out") {
				REQUIRE(controls.manager.buildCommand().lookRight == 0.0f);
			}
		}
	}
}

SCENARIO("Mouse clicks become part of the command", "[client][input][command][mouse]") {
	GIVEN("the left mouse button bound to the action button") {
		Controls controls;

		WHEN("it is clicked while holding forward") {
			controls.press({W});
			controls.manager.processMouseButton(input::MouseButton::LeftButton, InputAction::Press, 0);
			const input::InputCommand command = controls.manager.buildCommand();

			THEN("the command moves forward and starts the action") {
				REQUIRE(command.moveForward == 1.0f);
				REQUIRE(input::hasEvent(command.startedEvents, InputEvent::ActionButton));
			}
		}
	}
}
