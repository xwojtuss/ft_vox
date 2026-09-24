#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/epsilon.hpp>

#include "scene/WorldInfo.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/system/types/CameraSystem.hpp"
#include "support/FakeRenderer.hpp"
#include "support/TestEcs.hpp"

using ecs::component::Camera;
using ecs::component::Transform;
namespace worldinfo = scene::worldinfo;

namespace {
	bool nearlyEqual(const glm::vec3& a, const glm::vec3& b) {
		return glm::all(glm::epsilonEqual(a, b, 1e-4f));
	}

	glm::vec3 seenFromCamera(const Camera& camera, const glm::vec3& point) {
		return glm::vec3(camera.view * glm::vec4(point, 1.0f));
	}
}

SCENARIO("The camera sees the world from its entity's position and direction", "[ecs][camera]") {
	GIVEN("a camera with a 70 degree field of view at (1, 2, 3), turned 90 degrees to the left") {
		test::TestWorld testWorld;
		testWorld.addSystem<ecs::CameraSystem>();
		ecs::EntityHandle cameraEntity = testWorld.createEntity();
		Transform         transform;
		Camera            camera;
		transform.position = {1.0f, 2.0f, 3.0f};
		transform.rotation = glm::angleAxis(glm::radians(90.0f), worldinfo::up);
		camera.fov         = 70.0f;
		cameraEntity.addComponent(transform);
		cameraEntity.addComponent(camera);
		cameraEntity.registerToSystem<ecs::CameraSystem>();

		WHEN("a frame is rendered in a 16:9 window") {
			testWorld.world.getSystemManager().onRender(16.0f / 9.0f, 1.0);
			const Camera& updated = *cameraEntity.getComponent<Camera>();

			THEN("the projection uses its field of view and the window's aspect ratio") {
				REQUIRE(updated.projection == glm::perspective(glm::radians(70.0f), 16.0f / 9.0f, 0.1f, 1e10f));
			}
			AND_THEN("the camera's position is the center of the view") {
				REQUIRE(nearlyEqual(seenFromCamera(updated, transform.position), glm::vec3(0.0f)));
			}
			AND_THEN("what is in front of the entity is straight ahead in the view") {
				REQUIRE(
					nearlyEqual(seenFromCamera(updated, transform.position + worldinfo::left), {0.0f, 0.0f, -1.0f}));
			}

			AND_WHEN("the renderer finishes the frame") {
				test::FakeRenderer renderer;
				testWorld.world.getSystemManager().onRendererFrame(renderer);

				THEN("the renderer receives the camera's view") {
					REQUIRE(renderer.cameraViews.size() == 1);
					REQUIRE(renderer.cameraViews.front() == updated.view);
				}
			}
		}
	}
}
