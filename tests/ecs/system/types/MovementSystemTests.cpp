#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <glm/gtc/epsilon.hpp>

#include "scene/PlanetInfo.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/system/types/MovementSystem.hpp"
#include "support/Player.hpp"

using Catch::Approx;
using ecs::component::Transform;
using ecs::component::Velocity;
namespace planetinfo = scene::planetinfo;

namespace {
	bool nearlyEqual(const glm::vec3& a, const glm::vec3& b) {
		return glm::all(glm::epsilonEqual(a, b, 1e-4f));
	}

	struct MoveRecorder {
		std::vector<ecs::PlayerMoveEvent> moves;

		void onPlayerMove(const ecs::PlayerMoveEvent& event) {
			moves.push_back(event);
		}
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
		test::TestRegistry testRegistry;
		testRegistry.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player         = test::createPlayer(testRegistry);
		player.get<Transform>().rotation = glm::angleAxis(glm::radians(90.0f), planetinfo::up);

		WHEN("forward is pressed") {
			testRegistry.dispatcher().emit(test::inputFrom(player, moving(1.0f, 0.0f, 0.0f)));

			THEN("the player wants to go towards what used to be left") {
				REQUIRE(nearlyEqual(player.get<Velocity>().desiredVelocity, planetinfo::left));
			}
		}

		WHEN("right and up are pressed") {
			testRegistry.dispatcher().emit(test::inputFrom(player, moving(0.0f, 1.0f, 1.0f)));

			THEN("the player wants to go to their own right, and straight up on the planet") {
				REQUIRE(nearlyEqual(player.get<Velocity>().desiredVelocity, planetinfo::forward + planetinfo::up));
			}
		}

		WHEN("the input comes from an entity that is not in the movement system") {
			ecs::EntityHandle const stranger = testRegistry.createEntity();
			testRegistry.dispatcher().emit(test::inputFrom(stranger, moving(1.0f, 0.0f, 0.0f)));

			THEN("the player is not affected") {
				REQUIRE(player.get<Velocity>().desiredVelocity == glm::vec3(0.0f));
			}
		}
	}
}

SCENARIO("Looking around turns the player, but never past straight up or down", "[ecs][movement]") {
	GIVEN("a player facing forward") {
		test::TestRegistry testRegistry;
		testRegistry.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player = test::createPlayer(testRegistry);

		WHEN("they look up by 30 degrees") {
			testRegistry.dispatcher().emit(test::inputFrom(player, looking(glm::radians(30.0f), 0.0f)));

			THEN("they face 30 degrees above the horizon") {
				REQUIRE(nearlyEqual(player.get<Transform>().forward(),
									{0.0f, std::sin(glm::radians(30.0f)), -std ::cos(glm::radians(30.0f))}));
			}
		}

		WHEN("they look up by 170 degrees") {
			testRegistry.dispatcher().emit(test::inputFrom(player, looking(glm::radians(170.0f), 0.0f)));

			THEN("they stop at the maximum pitch of 89 degrees instead of flipping over") {
				REQUIRE(player.get<Transform>().forward().y == Approx(std::sin(glm::radians(89.0f))).epsilon(1e-4));
				REQUIRE(player.get<Transform>().forward().z < 0.0f);
			}
		}

		WHEN("they look down by 170 degrees") {
			testRegistry.dispatcher().emit(test::inputFrom(player, looking(glm::radians(-170.0f), 0.0f)));

			THEN("they stop at the minimum pitch of -89 degrees") {
				REQUIRE(player.get<Transform>().forward().y == Approx(-std::sin(glm::radians(89.0f))).epsilon(1e-4));
			}
		}

		WHEN("they cannot rotate and try to look around") {
			player.get<Transform>().canRotate = false;
			testRegistry.dispatcher().emit(test::inputFrom(player, looking(1.0f, 1.0f)));

			THEN("they keep facing forward") {
				REQUIRE(nearlyEqual(player.get<Transform>().forward(), planetinfo::forward));
			}
		}
	}
}

SCENARIO("The player speeds up towards where they want to go, up to their top speed", "[ecs][movement]") {
	GIVEN("a standing player who wants to go forward, accelerating at 5 units/s² up to 10 units/s") {
		test::TestRegistry testRegistry;
		testRegistry.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player   = test::createPlayer(testRegistry);
		auto&             velocity = player.get<Velocity>();
		velocity.desiredVelocity   = planetinfo::forward;

		WHEN("0.1 s passes") {
			testRegistry.simulate(0.1f, 1.0f);

			THEN("they move forward at 0.5 units/s") {
				REQUIRE(nearlyEqual(velocity.velocity, 0.5f * planetinfo::forward));
			}
			AND_THEN("they have moved 0.05 units forward") {
				REQUIRE(nearlyEqual(player.get<Transform>().position, 0.05f * planetinfo::forward));
			}
		}

		WHEN("10 s pass") {
			for (int step = 0; step < 100; ++step)
				testRegistry.simulate(0.1f, 1.0f);

			THEN("they never go faster than their top speed") {
				REQUIRE(glm::length(velocity.velocity) == Approx(10.0f));
			}
		}

		WHEN("they are not allowed to move") {
			velocity.canMove = false;
			testRegistry.simulate(0.1f, 1.0f);

			THEN("they stay where they are") {
				REQUIRE(player.get<Transform>().position == glm::vec3(0.0f));
			}
		}
	}
}

SCENARIO("The player slows down to a stop when no direction is held", "[ecs][movement]") {
	GIVEN("a player moving forward at 1 unit/s, slowing down at 5 units/s²") {
		test::TestRegistry testRegistry;
		testRegistry.addSystem<ecs::MovementSystem>();
		ecs::EntityHandle player   = test::createPlayer(testRegistry);
		auto&             velocity = player.get<Velocity>();
		velocity.velocity          = planetinfo::forward;

		WHEN("0.1 s passes") {
			testRegistry.simulate(0.1f, 1.0f);

			THEN("they slow down to 0.5 units/s, still going forward") {
				REQUIRE(nearlyEqual(velocity.velocity, 0.5f * planetinfo::forward));
			}
		}

		WHEN("a whole second passes") {
			testRegistry.simulate(1.0f, 1.0f);

			THEN("they come to a full stop instead of going backwards") {
				REQUIRE(velocity.velocity == glm::vec3(0.0f));
			}
		}
	}
}

SCENARIO("Every step the player moves is announced", "[ecs][movement]") {
	GIVEN("a player and something listening for player moves") {
		test::TestRegistry testRegistry;
		MoveRecorder       recorder;
		testRegistry.addSystem<ecs::MovementSystem>();
		testRegistry.dispatcher().subscribe(&recorder, &MoveRecorder::onPlayerMove);
		ecs::EntityHandle player = test::createPlayer(testRegistry);

		WHEN("the player moves") {
			player.get<Velocity>().velocity = planetinfo::right;
			testRegistry.simulate(1.0f, 1.0f);

			THEN("one move is announced, from the old position to the new one") {
				REQUIRE(recorder.moves.size() == 1);
				REQUIRE(recorder.moves.front().previousPosition == glm::vec3(0.0f));
				REQUIRE(recorder.moves.front().currentPosition == player.get<Transform>().position);
			}
		}

		WHEN("the player stands still") {
			testRegistry.simulate(1.0f, 1.0f);

			THEN("nothing is announced") {
				REQUIRE(recorder.moves.empty());
			}
		}
	}
}
