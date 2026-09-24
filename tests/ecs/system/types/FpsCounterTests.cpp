#include <catch2/catch_test_macros.hpp>

#include "ecs/system/types/FpsCounter.hpp"
#include "support/TestEcs.hpp"

using ecs::component::Text;
using ecs::component::Transform2D;

namespace {
ecs::EntityHandle	createFpsLabel(test::TestWorld& testWorld) {
	ecs::EntityHandle	label = testWorld.createEntity();
	Text				text;
	text.aligned = true;
	label.addComponent(Transform2D());
	label.addComponent(text);
	label.registerToSystem<ecs::FpsCounter>();
	return label;
}

void	renderFramesAt(test::TestWorld& testWorld, std::initializer_list<double> times) {
	for (double time : times)
		testWorld.world.getSystemManager().onRender(1.0f, time);
}
}

SCENARIO("The FPS label shows how many frames were drawn in the last second", "[ecs][fps]") {
	GIVEN("an FPS label") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::FpsCounter>();
		ecs::EntityHandle	label = createFpsLabel(testWorld);

		WHEN("frames are drawn every 0.25 s but a full second has not passed yet") {
			renderFramesAt(testWorld, {0.25, 0.5, 0.75});

			THEN("the label is not updated yet") {
				REQUIRE(label.getComponent<Text>()->text.empty());
			}
		}

		WHEN("frames are drawn every 0.25 s for a full second") {
			renderFramesAt(testWorld, {0.25, 0.5, 0.75, 1.0});

			THEN("it shows 4 FPS") {
				REQUIRE(label.getComponent<Text>()->text == "FPS: 4");
			}
			AND_THEN("the label is marked to be re-aligned, since its text changed") {
				REQUIRE_FALSE(label.getComponent<Text>()->aligned);
			}

			AND_WHEN("the next second only has 2 frames") {
				renderFramesAt(testWorld, {1.5, 2.0});

				THEN("counting started over, so it shows 2 FPS") {
					REQUIRE(label.getComponent<Text>()->text == "FPS: 2");
				}
			}
		}
	}
}

// TODO: make pass
SCENARIO("The first second is counted from the first frame", "[ecs][fps]") {
	GIVEN("an FPS label in a game whose first frame is drawn after 5 s of loading") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::FpsCounter>();
		ecs::EntityHandle	label = createFpsLabel(testWorld);

		WHEN("the first frame is drawn") {
			renderFramesAt(testWorld, {5.0});

			THEN("the label is not updated yet, since no second of frames has passed") {
				REQUIRE(label.getComponent<Text>()->text.empty());
			}
		}
	}
}
