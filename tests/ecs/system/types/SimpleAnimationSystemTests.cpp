#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>

#include "ecs/system/types/SimpleAnimationSystem.hpp"
#include "support/TestEcs.hpp"

using Catch::Approx;
using ecs::component::Animation;
using ecs::component::AnimationType;
using ecs::component::Transform;
namespace worldinfo = scene::worldinfo;

namespace {
constexpr float	pi = glm::pi<float>();

ecs::EntityHandle	createAnimated(test::TestWorld& testWorld, AnimationType type, float speed = 1.0f, float intensity = 1.0f, glm::vec3 position = glm::vec3(0.0f)) {
	ecs::EntityHandle	entity = testWorld.createEntity();
	Transform			transform;
	Animation			animation;
	transform.position = position;
	animation.type = type;
	animation.speed = speed;
	animation.intensity = intensity;
	entity.addComponent(transform);
	entity.addComponent(animation);
	entity.registerToSystem<ecs::SimpleAnimationSystem>();
	return entity;
}

bool	nearlyEqual(const glm::vec3& a, const glm::vec3& b) {
	return glm::all(glm::epsilonEqual(a, b, 1e-4f));
}

bool	eachAxisMovedAtMost(const glm::vec3& before, const glm::vec3& after, float limit) {
	return glm::all(glm::lessThanEqual(glm::abs(after - before), glm::vec3(limit)));
}
}

SCENARIO("Nothing animates before the clock starts", "[ecs][animation]") {
	GIVEN("a spinning entity") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::SimpleAnimationSystem>();
		ecs::EntityHandle	entity = createAnimated(testWorld, AnimationType::Spin);

		WHEN("a step runs at time 0") {
			testWorld.simulate(1.0f, 0.0f);

			THEN("it has not turned") {
				REQUIRE(nearlyEqual(entity.getComponent<Transform>()->forward(), worldinfo::forward));
			}
		}
	}
}

SCENARIO("A spinning entity turns around the up axis at 90 degrees per second times its speed", "[ecs][animation]") {
	GIVEN("a spinning entity with speed 1") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::SimpleAnimationSystem>();
		ecs::EntityHandle	entity = createAnimated(testWorld, AnimationType::Spin);

		WHEN("one second passes") {
			testWorld.simulate(1.0f, 1.0f);

			THEN("it has turned a quarter turn to the left") {
				REQUIRE(nearlyEqual(entity.getComponent<Transform>()->forward(), worldinfo::left));
			}
		}
	}
}

SCENARIO("A bouncing entity moves up and down over time", "[ecs][animation]") {
	GIVEN("a bouncing entity with intensity 2") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::SimpleAnimationSystem>();
		ecs::EntityHandle	entity = createAnimated(testWorld, AnimationType::Bounce, 1.0f, 2.0f);

		WHEN("a 0.1 s step runs while the bounce is rising fastest") {
			testWorld.simulate(0.1f, pi / 4.0f);

			THEN("it moves up by 0.1") {
				REQUIRE(entity.getComponent<Transform>()->position.y == Approx(0.1f));
			}
		}

		WHEN("a 0.1 s step runs while the bounce is falling fastest") {
			testWorld.simulate(0.1f, 3.0f * pi / 4.0f);

			THEN("it moves down by 0.1") {
				REQUIRE(entity.getComponent<Transform>()->position.y == Approx(-0.1f));
			}
		}
	}
}

SCENARIO("A pulsing entity grows and shrinks between nothing and twice its intensity", "[ecs][animation]") {
	GIVEN("a pulsing entity with intensity 1.5") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::SimpleAnimationSystem>();
		ecs::EntityHandle	entity = createAnimated(testWorld, AnimationType::Pulse, 1.0f, 1.5f);

		THEN("at the peak of the pulse it is 3 times its size") {
			testWorld.simulate(0.1f, pi / 2.0f);
			REQUIRE(nearlyEqual(entity.getComponent<Transform>()->scale, glm::vec3(3.0f)));
		}
		AND_THEN("at the bottom of the pulse it shrinks to nothing") {
			testWorld.simulate(0.1f, 3.0f * pi / 2.0f);
			REQUIRE(nearlyEqual(entity.getComponent<Transform>()->scale, glm::vec3(0.0f)));
		}
	}
}

SCENARIO("Jittering and random entities only move a little each step", "[ecs][animation]") {
	GIVEN("a jittering entity with speed 2 and intensity 3") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::SimpleAnimationSystem>();
		ecs::EntityHandle	entity = createAnimated(testWorld, AnimationType::Jitter, 2.0f, 3.0f);

		THEN("in 0.1 s each axis moves at most 0.3 in any direction") {
			for (int step = 0; step < 50; ++step) {
				const glm::vec3 before = entity.getComponent<Transform>()->position;
				testWorld.simulate(0.1f, 1.0f);
				REQUIRE(eachAxisMovedAtMost(before, entity.getComponent<Transform>()->position, 0.3f));
			}
		}
	}

	GIVEN("a randomly moving entity with intensity 2") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::SimpleAnimationSystem>();
		ecs::EntityHandle	entity = createAnimated(testWorld, AnimationType::Random, 1.0f, 2.0f);

		THEN("in 0.1 s each axis moves at most 0.2 and its size changes by at most 0.1") {
			for (int step = 0; step < 50; ++step) {
				const Transform before = *entity.getComponent<Transform>();
				testWorld.simulate(0.1f, 1.0f);
				REQUIRE(eachAxisMovedAtMost(before.position, entity.getComponent<Transform>()->position, 0.2f));
				REQUIRE(eachAxisMovedAtMost(before.scale, entity.getComponent<Transform>()->scale, 0.1f));
			}
		}
	}
}

// TODO: make pass
SCENARIO("A circling entity circles around where it was placed", "[ecs][animation]") {
	GIVEN("a circling entity with intensity 1 placed at (10, 0, 10)") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::SimpleAnimationSystem>();
		ecs::EntityHandle	entity = createAnimated(testWorld, AnimationType::Circle, 1.0f, 1.0f, {10.0f, 0.0f, 10.0f});

		WHEN("a step runs") {
			testWorld.simulate(0.1f, 1.0f);

			THEN("it stays within its 2 unit circle around (10, 0, 10)") {
				REQUIRE(glm::distance(entity.getComponent<Transform>()->position, glm::vec3(10.0f, 0.0f, 10.0f)) <= 2.0f + 1e-4f);
			}
		}
	}
}
