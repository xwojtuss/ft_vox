#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <type_traits>

#include "render/input/InputManager.hpp"

namespace input = render::input;
using input::InputAction;
using input::InputEvent;

namespace {
	constexpr int forwardKey = 17;
}

SCENARIO("Each player's keyboard is independent", "[client][input][command][multiplayer]") {
	GIVEN("two players with their own input devices, both using W to move forward") {
		input::InputManager first;
		input::InputManager second;
		first.getKeyInputProcessor().bindEvent(input::createInput(forwardKey, 0), InputEvent::MoveForward);
		second.getKeyInputProcessor().bindEvent(input::createInput(forwardKey, 0), InputEvent::MoveForward);

		WHEN("only the first player presses W") {
			first.processKey(forwardKey, InputAction::Press, 0);

			THEN("only the first player's command moves forward") {
				REQUIRE(first.buildCommand().moveForward == 1.0f);
				REQUIRE(second.buildCommand().moveForward == 0.0f);
			}
		}
	}
}

SCENARIO("A command is plain data that can be sent over the network", "[client][input][command][multiplayer]") {
	THEN("it can be copied byte by byte") {
		STATIC_REQUIRE(std::is_trivially_copyable_v<input::InputCommand>);
	}

	GIVEN("a command where the player moves forward, turns and starts jumping") {
		input::InputCommand sent = {};
		sent.moveForward         = 1.0f;
		sent.lookRight           = 0.25f;
		sent.startedEvents       = InputEvent::Jump;

		WHEN("it is written into a packet and read back on the other side") {
			unsigned char       packet[sizeof(input::InputCommand)];
			input::InputCommand received;
			std::memcpy(packet, &sent, sizeof(packet));
			std::memcpy(&received, packet, sizeof(packet));

			THEN("the received command is the same") {
				REQUIRE(received.moveForward == sent.moveForward);
				REQUIRE(received.lookRight == sent.lookRight);
				REQUIRE(received.startedEvents == sent.startedEvents);
				REQUIRE(received.maxPitch == sent.maxPitch);
			}
		}
	}
}
