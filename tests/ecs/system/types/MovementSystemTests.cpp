#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <glm/gtc/epsilon.hpp>

#include "scene/WorldInfo.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/system/types/MovementSystem.hpp"
#include "support/Player.hpp"

using Catch::Approx;
using ecs::component::Transform;
using ecs::component::Velocity;
namespace worldinfo = scene::worldinfo;

namespace {
	bool nearlyEqual(const glm::vec3& a, const glm::vec3& b) {
		return glm::all(glm::epsilonEqual(a, b, 1e-4f));
	}

	struct MoveRecorder {
		std::vector<ecs::PlayerMoveEvent> moves;

		void onPlayerMove(const ecs::PlayerMoveEvent& event) { moves.push_back(event); }
	};

	render::input::InputCommand moving(float forward, float right, float up) {
		render::input::InputCommand command = {};
		command.moveForward                 = forward;
		command.moveRight                   = right;
		command.moveUp                      = up;
		return command;
	}

	render::input::InputCommand looking(float up, float right) {
		render::input::InputCommand command = {};
		command.lookUp                      = up;
		command.lookRight                   = right;
		return command;
	}
}

SCENARIO("Movement input sets where the player wants to go, relative to where they look", "[ecs][movement]") {
	GIVEN("a player turned 90 degrees to the left") {
		test::TestWorld testWorld;
		testWorld.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player = test::createPlayer(testWorld);
		player.registerToSystem<ecs::MovementSystem>();
		player.getComponent<Transform>()->rotation = glm::angleAxis(glm::radians(90.0f), worldinfo::up);

		WHEN("forward is pressed") {
			testWorld.dispatcher().emit(test::inputFrom(player, moving(1.0f, 0.0f, 0.0f)));

			THEN("the player wants to go towards what used to be left") {
				REQUIRE(nearlyEqual(player.getComponent<Velocity>()->desiredVelocity, worldinfo::left));
			}
		}

		WHEN("right and up are pressed") {
			testWorld.dispatcher().emit(test::inputFrom(player, moving(0.0f, 1.0f, 1.0f)));

			THEN("the player wants to go to their own right, and straight up in the world") {
				REQUIRE(
					nearlyEqual(player.getComponent<Velocity>()->desiredVelocity, worldinfo::forward + worldinfo::up));
			}
		}

		WHEN("the input comes from an entity that is not in the movement system") {
			ecs::EntityHandle stranger = testWorld.createEntity();
			testWorld.dispatcher().emit(test::inputFrom(stranger, moving(1.0f, 0.0f, 0.0f)));

			THEN("the player is not affected") {
				REQUIRE(player.getComponent<Velocity>()->desiredVelocity == glm::vec3(0.0f));
			}
		}
	}
}

SCENARIO("Looking around turns the player, but never past straight up or down", "[ecs][movement]") {
	GIVEN("a player facing forward") {
		test::TestWorld testWorld;
		testWorld.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player = test::createPlayer(testWorld);
		player.registerToSystem<ecs::MovementSystem>();

		WHEN("they look up by 30 degrees") {
			testWorld.dispatcher().emit(test::inputFrom(player, looking(glm::radians(30.0f), 0.0f)));

			THEN("they face 30 degrees above the horizon") {
				REQUIRE(
					nearlyEqual(player.getComponent<Transform>()->forward(), {0.0f, std::sin(glm::radians(30.0f)), -std
						::cos(glm::radians(30.0f))}));
			}
		}

		WHEN("they look up by 170 degrees") {
			testWorld.dispatcher().emit(test::inputFrom(player, looking(glm::radians(170.0f), 0.0f)));

			THEN("they stop at the maximum pitch of 89 degrees instead of flipping over") {
				REQUIRE(
					player.getComponent<Transform>()->forward().y == Approx(std::sin(glm::radians(89.0f))).epsilon(1e-4
					));
				REQUIRE(player.getComponent<Transform>()->forward().z < 0.0f);
			}
		}

		WHEN("they look down by 170 degrees") {
			testWorld.dispatcher().emit(test::inputFrom(player, looking(glm::radians(-170.0f), 0.0f)));

			THEN("they stop at the minimum pitch of -89 degrees") {
				REQUIRE(
					player.getComponent<Transform>()->forward().y == Approx(-std::sin(glm::radians(89.0f))).epsilon(1e-4
					));
			}
		}

		WHEN("they cannot rotate and try to look around") {
			player.getComponent<Transform>()->canRotate = false;
			testWorld.dispatcher().emit(test::inputFrom(player, looking(1.0f, 1.0f)));

			THEN("they keep facing forward") {
				REQUIRE(nearlyEqual(player.getComponent<Transform>()->forward(), worldinfo::forward));
			}
		}
	}
}

SCENARIO("The player speeds up towards where they want to go, up to their top speed", "[ecs][movement]") {
	GIVEN("a standing player who wants to go forward, accelerating at 5 units/s² up to 10 units/s") {
		test::TestWorld testWorld;
		testWorld.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player = test::createPlayer(testWorld);
		player.registerToSystem<ecs::MovementSystem>();
		Velocity& velocity       = *player.getComponent<Velocity>();
		velocity.desiredVelocity = worldinfo::forward;

		WHEN("0.1 s passes") {
			testWorld.simulate(0.1f, 1.0f);

			THEN("they move forward at 0.5 units/s") {
				REQUIRE(nearlyEqual(velocity.velocity, 0.5f * worldinfo::forward));
			}
			AND_THEN("they have moved 0.05 units forward") {
				REQUIRE(nearlyEqual(player.getComponent<Transform>()->position, 0.05f * worldinfo::forward));
			}
		}

		WHEN("10 s pass") {
			for (int step = 0; step < 100; ++step)
				testWorld.simulate(0.1f, 1.0f);

			THEN("they never go faster than their top speed") {
				REQUIRE(glm::length(velocity.velocity) == Approx(10.0f));
			}
		}

		WHEN("they are not allowed to move") {
			velocity.canMove = false;
			testWorld.simulate(0.1f, 1.0f);

			THEN("they stay where they are") {
				REQUIRE(player.getComponent<Transform>()->position == glm::vec3(0.0f));
			}
		}
	}
}

SCENARIO("The player slows down to a stop when no direction is held", "[ecs][movement]") {
	GIVEN("a player moving forward at 1 unit/s, slowing down at 5 units/s²") {
		test::TestWorld testWorld;
		testWorld.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player = test::createPlayer(testWorld);
		player.registerToSystem<ecs::MovementSystem>();
		Velocity& velocity = *player.getComponent<Velocity>();
		velocity.velocity  = worldinfo::forward;

		WHEN("0.1 s passes") {
			testWorld.simulate(0.1f, 1.0f);

			THEN("they slow down to 0.5 units/s, still going forward") {
				REQUIRE(nearlyEqual(velocity.velocity, 0.5f * worldinfo::forward));
			}
		}

		WHEN("a whole second passes") {
			testWorld.simulate(1.0f, 1.0f);

			THEN("they come to a full stop instead of going backwards") {
				REQUIRE(velocity.velocity == glm::vec3(0.0f));
			}
		}
	}
}

SCENARIO("Every step the player moves is announced", "[ecs][movement]") {
	GIVEN("a player and something listening for player moves") {
		test::TestWorld testWorld;
		MoveRecorder    recorder;
		testWorld.addSystem<ecs::MovementSystem>();
		testWorld.dispatcher().subscribe(&recorder, &MoveRecorder::onPlayerMove);
		ecs::EntityHandle player = test::createPlayer(testWorld);
		player.registerToSystem<ecs::MovementSystem>();

		WHEN("the player moves") {
			player.getComponent<Velocity>()->velocity = worldinfo::right;
			testWorld.simulate(1.0f, 1.0f);

			THEN("one move is announced, from the old position to the new one") {
				REQUIRE(recorder.moves.size() == 1);
				REQUIRE(recorder.moves.front().previousPosition == glm::vec3(0.0f));
				REQUIRE(recorder.moves.front().currentPosition == player.getComponent<Transform>()->position);
			}
		}

		WHEN("the player stands still") {
			testWorld.simulate(1.0f, 1.0f);

			THEN("nothing is announced") {
				REQUIRE(recorder.moves.empty());
			}
		}
	}
}
