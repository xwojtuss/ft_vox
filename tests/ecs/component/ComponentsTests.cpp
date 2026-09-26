#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <format>
#include <glm/gtc/epsilon.hpp>

#include "scene/WorldInfo.hpp"
#include "ecs/component/Components.hpp"

using Catch::Matchers::ContainsSubstring;
using namespace ecs::component;
namespace worldinfo = scene::worldinfo;

namespace {
	bool nearlyEqual(const glm::vec3& a, const glm::vec3& b) {
		return glm::all(glm::epsilonEqual(a, b, 1e-5f));
	}

	template<typename ComponentType>
	std::string describe(const ComponentType& component) {
		return std::format("{}", component);
	}

	glm::quat turnLeft90() {
		return glm::angleAxis(glm::radians(90.0f), worldinfo::up);
	}
}

SCENARIO("An unrotated transform faces the world's forward direction", "[ecs][transform]") {
	GIVEN("a default transform") {
		const Transform transform;

		THEN("it sits at the origin with scale 1") {
			REQUIRE(transform.position == glm::vec3(0.0f));
			REQUIRE(transform.scale == glm::vec3(1.0f));
		}
		AND_THEN("its directions are the world's directions") {
			REQUIRE(nearlyEqual(transform.forward(), worldinfo::forward));
			REQUIRE(nearlyEqual(transform.right(), worldinfo::right));
			REQUIRE(nearlyEqual(transform.left(), worldinfo::left));
			REQUIRE(nearlyEqual(transform.up(), worldinfo::up));
			REQUIRE(nearlyEqual(transform.down(), worldinfo::down));
		}
	}
}

SCENARIO("Rotating a transform turns its directions with it", "[ecs][transform]") {
	GIVEN("a transform turned 90 degrees to the left around the up axis") {
		Transform transform;
		transform.rotation = turnLeft90();

		THEN("it now faces what used to be left") {
			REQUIRE(nearlyEqual(transform.forward(), worldinfo::left));
		}
		AND_THEN("its right is what used to be forward") {
			REQUIRE(nearlyEqual(transform.right(), worldinfo::forward));
			REQUIRE(nearlyEqual(transform.left(), worldinfo::backward));
		}
		AND_THEN("up and down do not change") {
			REQUIRE(nearlyEqual(transform.up(), worldinfo::up));
			REQUIRE(nearlyEqual(transform.down(), worldinfo::down));
		}
	}
}

SCENARIO("The model matrix scales, then rotates, then moves", "[ecs][transform]") {
	GIVEN("a transform at (1, 2, 3) scaled by 2") {
		Transform transform;
		transform.position = {1.0f, 2.0f, 3.0f};
		transform.scale    = glm::vec3(2.0f);

		THEN("the model's corner (1, 1, 1) ends up at (3, 4, 5)") {
			const glm::vec4 corner = transform.toModelMatrix() * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			REQUIRE(nearlyEqual(glm::vec3(corner), {3.0f, 4.0f, 5.0f}));
		}

		WHEN("it is also turned 90 degrees to the left") {
			transform.rotation = turnLeft90();

			THEN("a point 1 unit in front of the model ends up 2 units to the world's left of the position") {
				const glm::vec4 front = transform.toModelMatrix() * glm::vec4(worldinfo::forward, 1.0f);
				REQUIRE(nearlyEqual(glm::vec3(front), transform.position + 2.0f * worldinfo::left));
			}
		}
	}
}

SCENARIO("Components describe themselves for the debug panel", "[ecs][component]") {
	GIVEN("a transform at (1, 2, 3)") {
		Transform transform;
		transform.position = {1.0f, 2.0f, 3.0f};

		THEN("its description shows position, rotation and scale with 2 decimals") {
			REQUIRE_THAT(describe(transform), ContainsSubstring("Position: (X:1.00, Y:2.00, Z:3.00)"));
			REQUIRE_THAT(describe(transform), ContainsSubstring("Rotation: (X:0.00, Y:0.00, Z:0.00, W:1.00)"));
			REQUIRE_THAT(describe(transform), ContainsSubstring("Scale: (X:1.00, Y:1.00, Z:1.00)"));
		}
	}

	GIVEN("a velocity that cannot move") {
		Velocity velocity;
		velocity.velocity        = {1.0f, 0.0f, 0.0f};
		velocity.desiredVelocity = {0.0f, 2.0f, 0.0f};
		velocity.maxSpeed        = 10.0f;
		velocity.acceleration    = 3.0f;
		velocity.deceleration    = 4.0f;
		velocity.canMove         = false;

		THEN("its description shows every movement setting") {
			const std::string text = describe(velocity);
			REQUIRE_THAT(text, ContainsSubstring("Velocity: (X:1.00, Y:0.00, Z:0.00)"));
			REQUIRE_THAT(text, ContainsSubstring("Desired Velocity: (X:0.00, Y:2.00, Z:0.00)"));
			REQUIRE_THAT(text, ContainsSubstring("Max Speed: 10.00"));
			REQUIRE_THAT(text, ContainsSubstring("Acceleration: 3.00"));
			REQUIRE_THAT(text, ContainsSubstring("Deceleration: 4.00"));
			REQUIRE_THAT(text, ContainsSubstring("Can Move: false"));
		}
	}

	GIVEN("a camera with a 70 degree field of view") {
		Camera camera;
		camera.fov = 70.0f;

		THEN("its description shows the field of view") {
			REQUIRE_THAT(describe(camera), ContainsSubstring("FOV: 70.00"));
		}
	}

	GIVEN("a textured mesh") {
		Mesh mesh;
		mesh.pipelineType = assets::PipelineType::Textured;

		THEN("its description shows its handle and pipeline by name") {
			REQUIRE_THAT(describe(mesh), ContainsSubstring("Mesh Handle: " + std::to_string(mesh.mesh.id)));
			REQUIRE_THAT(describe(mesh), ContainsSubstring("Pipeline Type: Textured"));
		}
	}

	GIVEN("a texture") {
		const Texture texture;

		THEN("its description shows its handle") {
			REQUIRE_THAT(describe(texture), ContainsSubstring("Texture Handle: " + std::to_string(texture.texture.id)));
		}
	}

	GIVEN("an input with a mouse sensitivity of 0.5") {
		Input input;
		input.mouseSensitivity    = 0.5f;
		input.command             = {};
		input.command.moveForward = 1.0f;

		THEN("its description shows the sensitivity and the current command") {
			REQUIRE_THAT(describe(input), ContainsSubstring("Mouse Sensitivity: 0.50"));
			REQUIRE_THAT(describe(input), ContainsSubstring("Move Forward: 1.00"));
		}
	}
}
