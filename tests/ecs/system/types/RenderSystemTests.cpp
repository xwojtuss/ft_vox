#include <catch2/catch_test_macros.hpp>

#include "ecs/component/Components.hpp"
#include "ecs/system/types/RenderSystem.hpp"
#include "support/FakeRenderer.hpp"
#include "support/TestEcs.hpp"

using ecs::component::Mesh;
using ecs::component::Texture;
using ecs::component::Transform;

namespace {
	ecs::EntityHandle createDrawable(test::TestWorld& testWorld, glm::vec3 position) {
		ecs::EntityHandle entity = testWorld.createEntity();
		Transform         transform;
		transform.position = position;
		entity.addComponent(transform);
		entity.addComponent(Mesh());
		return entity;
	}
}

SCENARIO("Every mesh in the render system is drawn each frame", "[ecs][render]") {
	GIVEN("a textured mesh and an untextured mesh in the render system, and a mesh outside it") {
		test::TestWorld    testWorld;
		test::FakeRenderer renderer;
		testWorld.addSystem<ecs::RenderSystem>();

		ecs::EntityHandle textured = createDrawable(testWorld, {1.0f, 0.0f, 0.0f});
		textured.addComponent(Texture());
		textured.registerToSystem<ecs::RenderSystem>();
		ecs::EntityHandle untextured = createDrawable(testWorld, {2.0f, 0.0f, 0.0f});
		untextured.registerToSystem<ecs::RenderSystem>();
		createDrawable(testWorld, {3.0f, 0.0f, 0.0f});

		WHEN("the renderer draws the scene") {
			testWorld.world.getSystemManager().onRendererDraw(renderer);

			THEN("only the two meshes in the system are drawn") {
				REQUIRE(renderer.drawnMeshes.size() == 2);
			}
			AND_THEN("each is drawn with its own mesh at its own position") {
				REQUIRE(renderer.drawnMeshes[0].meshId == textured.getComponent<Mesh>()->mesh.id);
				REQUIRE(renderer.drawnMeshes[0].position == glm::vec3(1.0f, 0.0f, 0.0f));
				REQUIRE(renderer.drawnMeshes[1].meshId == untextured.getComponent<Mesh>()->mesh.id);
				REQUIRE(renderer.drawnMeshes[1].position == glm::vec3(2.0f, 0.0f, 0.0f));
			}
			AND_THEN("the texture is passed only for the textured mesh") {
				REQUIRE(renderer.drawnMeshes[0].texture == textured.getComponent<Texture>());
				REQUIRE(renderer.drawnMeshes[1].texture == nullptr);
			}
		}
	}
}
