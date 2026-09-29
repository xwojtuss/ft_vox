#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <glm/gtc/epsilon.hpp>

#include "scene/PlanetInfo.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/system/types/MovementSystem.hpp"
#include "ecs/system/types/PlayerInputSystem.hpp"
#include "support/Player.hpp"

using Catch::Approx;
using ecs::component::Input;
using ecs::component::Transform;
using render::input::InputAction;
namespace input      = render::input;
namespace planetinfo = scene::planetinfo;

namespace {
	constexpr int forwardKey = 17;

	struct InputRecorder {
		std::vector<ecs::InputEvent> events;

		void onInput(const ecs::InputEvent& event) {
			events.push_back(event);
		}
	};

	bool nearlyEqual(const glm::vec3& a, const glm::vec3& b) {
		return glm::all(glm::epsilonEqual(a, b, 1e-4f));
	}
}

SCENARIO("Pressed keys become the player's input command", "[ecs][player-input]") {
	GIVEN("a player, with the forward key bound to moving forward") {
		test::TestRegistry          testRegistry;
		render::input::InputManager inputManager;
		InputRecorder               recorder;
		testRegistry.addSystem<ecs::PlayerInputSystem>(inputManager);
		testRegistry.dispatcher().subscribe(&recorder, &InputRecorder::onInput);
		ecs::EntityHandle player = test::createPlayer(testRegistry);
		inputManager.getKeyInputProcessor().bindEvent(render::input::createInput(forwardKey, input::InputMod::None),
													  input::InputEvent::MoveForward);

		WHEN("the forward key is pressed and a simulation step runs") {
			inputManager.processKey(forwardKey, InputAction::Press, input::InputMod::None);
			testRegistry.simulate(0.016f, 1.0f);

			THEN("the player's command says they started moving forward") {
				const render::input::InputCommand& command = player.get<Input>().command;
				REQUIRE(command.moveForward == 1.0f);
				REQUIRE(render::input::hasEvent(command.startedEvents, input::InputEvent::MoveForward));
			}
			AND_THEN("one input event from the player, with that command and the time step, is sent") {
				REQUIRE(recorder.events.size() == 1);
				REQUIRE(recorder.events.front().source == player.id());
				REQUIRE(recorder.events.front().command.moveForward == 1.0f);
				REQUIRE(recorder.events.front().deltaTime == 0.016f);
			}

			AND_WHEN("the key is held through another step") {
				testRegistry.simulate(0.016f, 1.016f);

				THEN("the player still moves forward, but it no longer counts as just started") {
					const render::input::InputCommand& command = player.get<Input>().command;
					REQUIRE(command.moveForward == 1.0f);
					REQUIRE(render::input::hasEvent(command.activeEvents, input::InputEvent::MoveForward));
					REQUIRE_FALSE(render::input::hasEvent(command.startedEvents, input::InputEvent::MoveForward));
				}
			}
		}
	}
}

// TODO: make pass
SCENARIO("Without a player, no input event is sent", "[ecs][player-input][!mayfail]") {
	GIVEN("a registry with the player input system but no player") {
		test::TestRegistry          testRegistry;
		render::input::InputManager inputManager;
		InputRecorder               recorder;
		testRegistry.addSystem<ecs::PlayerInputSystem>(inputManager);
		testRegistry.dispatcher().subscribe(&recorder, &InputRecorder::onInput);

		WHEN("a simulation step runs") {
			testRegistry.simulate(0.016f, 1.0f);

			THEN("nothing is sent") {
				REQUIRE(recorder.events.empty());
			}
		}
	}
}

SCENARIO("The mouse turns the player the way it moves", "[ecs][player-input][movement]") {
	GIVEN("a player facing forward, turning 1 degree per pixel of mouse movement") {
		test::TestRegistry          testRegistry;
		render::input::InputManager inputManager;
		testRegistry.addSystem<ecs::PlayerInputSystem>(inputManager);
		testRegistry.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player = test::createPlayer(testRegistry, glm::radians(1.0f));
		inputManager.processMouseMove(0.0, 0.0);

		WHEN("the mouse moves 90 pixels to the right") {
			inputManager.processMouseMove(90.0, 0.0);
			testRegistry.simulate(0.016f, 1.0f);

			THEN("the player turns 90 degrees to the right") {
				REQUIRE(nearlyEqual(player.get<Transform>().forward(), planetinfo::right));
			}
		}

		WHEN("the mouse moves 30 pixels up the screen") {
			inputManager.processMouseMove(0.0, -30.0);
			testRegistry.simulate(0.016f, 1.0f);

			THEN("the player looks 30 degrees up") {
				REQUIRE(player.get<Transform>().forward().y == Approx(std::sin(glm::radians(30.0f))).epsilon(1e-4));
			}
		}
	}
}
