#include <catch2/catch_test_macros.hpp>

#include "ecs/component/Components.hpp"
#include "ecs/system/types/RenderSystem.hpp"
#include "support/FakeRenderer.hpp"
#include "support/TestEcs.hpp"

using ecs::component::Mesh;
using ecs::component::Texture;
using ecs::component::Transform;

namespace {
	ecs::EntityHandle createDrawable(test::TestWorld& testWorld, const glm::vec3 position) {
		ecs::EntityHandle entity = testWorld.createEntity();
		entity.add(Transform{.position = position});
		entity.add(Mesh{});
		return entity;
	}

	const test::DrawnMesh* findDrawn(const test::FakeRenderer& renderer, const ecs::EntityHandle& entity) {
		const uint64_t meshId = entity.get<Mesh>().mesh.id;

		for (const test::DrawnMesh& drawn: renderer.drawnMeshes) {
			if (drawn.meshId == meshId)
				return &drawn;
		}
		return nullptr;
	}
}

SCENARIO("Every entity with a mesh and a position is drawn each frame", "[ecs][render]") {
	GIVEN("a textured mesh, an untextured mesh, and a position without a mesh") {
		test::TestWorld    testWorld;
		test::FakeRenderer renderer;
		testWorld.addSystem<ecs::RenderSystem>();

		ecs::EntityHandle textured = createDrawable(testWorld, {1.0f, 0.0f, 0.0f});
		textured.add(Texture{});
		ecs::EntityHandle untextured = createDrawable(testWorld, {2.0f, 0.0f, 0.0f});
		testWorld.createEntity().add(Transform{.position = {3.0f, 0.0f, 0.0f}});

		WHEN("the renderer draws the scene") {
			testWorld.world.getSystemManager().onRendererDraw(renderer);

			THEN("only the two meshes are drawn") {
				REQUIRE(renderer.drawnMeshes.size() == 2);
			}
			AND_THEN("each is drawn with its own mesh at its own position") {
				REQUIRE(findDrawn(renderer, textured) != nullptr);
				REQUIRE(findDrawn(renderer, textured)->position == glm::vec3(1.0f, 0.0f, 0.0f));
				REQUIRE(findDrawn(renderer, untextured) != nullptr);
				REQUIRE(findDrawn(renderer, untextured)->position == glm::vec3(2.0f, 0.0f, 0.0f));
			}
			AND_THEN("the texture is passed only for the textured mesh") {
				REQUIRE(findDrawn(renderer, textured)->texture == textured.tryGet<Texture>());
				REQUIRE(findDrawn(renderer, untextured)->texture == nullptr);
			}
		}
	}
}
