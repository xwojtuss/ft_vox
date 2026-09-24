#include <catch2/catch_test_macros.hpp>

#include "render/input/KeyInputProcessor.hpp"

namespace input = render::input;
using input::InputAction;
using input::InputEvent;
using input::InputMod;
using input::MouseButton;

namespace {
	constexpr int keyW       = 17;
	constexpr int keyD       = 32;
	constexpr int keySpace   = 57;
	constexpr int keyF3      = 61;
	constexpr int keyUnbound = 99;

	struct Frame {
		input::InputEvents pressed;
		input::InputEvents repeated;
		input::InputEvents released;
		input::InputEvents active;

		bool isPressed(InputEvent event) const { return input::hasEvent(pressed, event); }
		bool isRepeated(InputEvent event) const { return input::hasEvent(repeated, event); }
		bool isReleased(InputEvent event) const { return input::hasEvent(released, event); }
		bool isActive(InputEvent event) const { return input::hasEvent(active, event); }
	};

	struct Keyboard {
		input::KeyInputProcessor processor;

		void bind(int key, InputEvent event, input::InputMods mods = 0) {
			processor.bindEvent(input::createInput(key, mods), event);
		}

		void bindMouse(MouseButton button, InputEvent event) {
			processor.bindEvent(input::createMouseInput(button, 0), event);
		}

		void press(int key, input::InputMods mods = 0) { processor.processKey(key, InputAction::Press, mods); }
		void hold(int key, input::InputMods mods = 0) { processor.processKey(key, InputAction::Repeat, mods); }
		void release(int key, input::InputMods mods = 0) { processor.processKey(key, InputAction::Release, mods); }

		Frame nextFrame() {
			Frame frame{};
			processor.getKeyEvents(frame.pressed, frame.repeated, frame.released, frame.active);
			return frame;
		}
	};

	Keyboard wasdKeyboard() {
		Keyboard keyboard;
		keyboard.bind(keyW, InputEvent::MoveForward);
		keyboard.bind(keyD, InputEvent::MoveRight);
		keyboard.bind(keySpace, InputEvent::Jump);
		return keyboard;
	}
}

SCENARIO("Several actions can be held at the same time", "[client][input][keys]") {
	GIVEN("keys bound to moving forward, moving right and jumping") {
		Keyboard keyboard = wasdKeyboard();

		WHEN("all three are pressed in the same frame") {
			keyboard.press(keyW);
			keyboard.press(keyD);
			keyboard.press(keySpace);
			const Frame frame = keyboard.nextFrame();

			THEN("all three actions start and are active together") {
				REQUIRE(frame.isPressed(InputEvent::MoveForward));
				REQUIRE(frame.isPressed(InputEvent::MoveRight));
				REQUIRE(frame.isPressed(InputEvent::Jump));
				REQUIRE(
					input::hasAllEvents(frame.active, InputEvent::MoveForward | InputEvent::MoveRight | InputEvent::Jump
					));
			}

			AND_WHEN("only the jump key is released") {
				keyboard.release(keySpace);
				const Frame later = keyboard.nextFrame();

				THEN("jumping stops while moving forward and right continue") {
					REQUIRE(later.isReleased(InputEvent::Jump));
					REQUIRE_FALSE(later.isActive(InputEvent::Jump));
					REQUIRE(later.isActive(InputEvent::MoveForward));
					REQUIRE(later.isActive(InputEvent::MoveRight));
				}
			}
		}
	}
}

SCENARIO("Starting and stopping an action are reported for one frame, holding it for as long as it lasts",
		"[client][input][keys]") {
	GIVEN("a key bound to moving forward") {
		Keyboard keyboard = wasdKeyboard();

		WHEN("it is pressed") {
			keyboard.press(keyW);
			const Frame first = keyboard.nextFrame();

			THEN("the first frame reports that moving forward started") {
				REQUIRE(first.isPressed(InputEvent::MoveForward));
				REQUIRE(first.isActive(InputEvent::MoveForward));
			}

			AND_WHEN("it is held through the next frame, with the keyboard repeating it") {
				keyboard.hold(keyW);
				const Frame second = keyboard.nextFrame();

				THEN("it is still active and repeating, but no longer reported as just started") {
					REQUIRE_FALSE(second.isPressed(InputEvent::MoveForward));
					REQUIRE(second.isRepeated(InputEvent::MoveForward));
					REQUIRE(second.isActive(InputEvent::MoveForward));
				}
			}

			AND_WHEN("it is released") {
				keyboard.release(keyW);
				const Frame second = keyboard.nextFrame();
				const Frame third  = keyboard.nextFrame();

				THEN("the stop is reported once and the action is no longer active") {
					REQUIRE(second.isReleased(InputEvent::MoveForward));
					REQUIRE_FALSE(second.isActive(InputEvent::MoveForward));
					REQUIRE_FALSE(third.isReleased(InputEvent::MoveForward));
				}
			}
		}
	}
}

SCENARIO("A tap shorter than a frame is not lost", "[client][input][keys]") {
	GIVEN("a key bound to jumping") {
		Keyboard keyboard = wasdKeyboard();

		WHEN("it is pressed and released before the next frame") {
			keyboard.press(keySpace);
			keyboard.release(keySpace);
			const Frame frame = keyboard.nextFrame();

			THEN("the frame sees both the press and the release") {
				REQUIRE(frame.isPressed(InputEvent::Jump));
				REQUIRE(frame.isReleased(InputEvent::Jump));
			}
			AND_THEN("jumping is not left active") {
				REQUIRE_FALSE(frame.isActive(InputEvent::Jump));
			}
		}
	}
}

SCENARIO("One key can trigger several actions", "[client][input][keys]") {
	GIVEN("F3 bound to both debug panels") {
		Keyboard keyboard;
		keyboard.bind(keyF3, InputEvent::PlayerComponentsMenuToggle);
		keyboard.bind(keyF3, InputEvent::EventRuntimesMenuToggle);

		WHEN("F3 is pressed") {
			keyboard.press(keyF3);
			const Frame frame = keyboard.nextFrame();

			THEN("both panel toggles start") {
				REQUIRE(frame.isPressed(InputEvent::PlayerComponentsMenuToggle));
				REQUIRE(frame.isPressed(InputEvent::EventRuntimesMenuToggle));
			}
		}
	}
}

SCENARIO("Keys that are not bound do nothing", "[client][input][keys]") {
	GIVEN("a keyboard with bindings") {
		Keyboard keyboard = wasdKeyboard();

		WHEN("an unbound key is pressed") {
			keyboard.press(keyUnbound);
			const Frame frame = keyboard.nextFrame();

			THEN("no action starts") {
				REQUIRE(frame.pressed == 0);
				REQUIRE(frame.active == 0);
			}
		}
	}
}

SCENARIO("Modifier keys select a different action", "[client][input][keys][modifiers]") {
	GIVEN("W bound to moving forward and Shift+W bound to jumping") {
		Keyboard keyboard = wasdKeyboard();
		keyboard.bind(keyW, InputEvent::Jump, InputMod::Shift);

		WHEN("W is pressed with Shift held") {
			keyboard.press(keyW, InputMod::Shift);
			const Frame frame = keyboard.nextFrame();

			THEN("only the Shift+W action starts") {
				REQUIRE(frame.isPressed(InputEvent::Jump));
				REQUIRE_FALSE(frame.isPressed(InputEvent::MoveForward));
			}
		}

		WHEN("W is pressed on its own") {
			keyboard.press(keyW);
			const Frame frame = keyboard.nextFrame();

			THEN("only the plain W action starts") {
				REQUIRE(frame.isPressed(InputEvent::MoveForward));
				REQUIRE_FALSE(frame.isPressed(InputEvent::Jump));
			}
		}

		WHEN("W is pressed with Control, a combination that is not bound") {
			keyboard.press(keyW, InputMod::Control);
			const Frame frame = keyboard.nextFrame();

			THEN("the plain W action is used") {
				REQUIRE(frame.isPressed(InputEvent::MoveForward));
			}
		}
	}
}

SCENARIO("Mouse buttons and keys can be used together", "[client][input][keys][mouse]") {
	GIVEN("the left mouse button bound to the action button, and W to moving forward") {
		Keyboard keyboard = wasdKeyboard();
		keyboard.bindMouse(MouseButton::LeftButton, InputEvent::ActionButton);

		WHEN("the player runs forward while clicking") {
			keyboard.press(keyW);
			keyboard.processor.processMouseButton(MouseButton::LeftButton, InputAction::Press, 0);
			const Frame frame = keyboard.nextFrame();

			THEN("both actions are active") {
				REQUIRE(frame.isActive(InputEvent::MoveForward));
				REQUIRE(frame.isActive(InputEvent::ActionButton));
			}

			AND_WHEN("the button is released with Shift held, a combination that is not bound") {
				keyboard.processor.processMouseButton(MouseButton::LeftButton, InputAction::Release, InputMod::Shift);
				const Frame later = keyboard.nextFrame();

				THEN("the plain button is released and running continues") {
					REQUIRE_FALSE(later.isActive(InputEvent::ActionButton));
					REQUIRE(later.isActive(InputEvent::MoveForward));
				}
			}
		}

		WHEN("an unbound mouse button is pressed") {
			keyboard.processor.processMouseButton(MouseButton::MiddleButton, InputAction::Press, 0);

			THEN("nothing happens") {
				REQUIRE(keyboard.nextFrame().active == 0);
			}
		}
	}
}
