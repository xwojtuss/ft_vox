#include <catch2/catch_test_macros.hpp>

#include "ecs/system/types/ToggleShaderSystem.hpp"
#include "support/Player.hpp"

using assets::PipelineType;
using ecs::component::Input;
using ecs::component::Mesh;
namespace input = render::input;

namespace {
ecs::EntityHandle	createMesh(test::TestWorld& testWorld, PipelineType pipeline) {
	ecs::EntityHandle	entity = testWorld.createEntity();
	Mesh				mesh;
	mesh.pipelineType = pipeline;
	entity.addComponent(mesh);
	return entity;
}
}

SCENARIO("The shader toggle switches every mesh between textures and vertex colors", "[ecs][shader-toggle]") {
	GIVEN("a player, a textured mesh and a vertex-colored mesh") {
		test::TestWorld		testWorld;
		testWorld.addSystem<ecs::ToggleShaderSystem>();
		ecs::EntityHandle	player = test::createPlayer(testWorld);
		player.registerToSystem<ecs::ToggleShaderSystem>();
		ecs::EntityHandle	textured = createMesh(testWorld, PipelineType::Textured);
		ecs::EntityHandle	colored = createMesh(testWorld, PipelineType::VertexColor);

		WHEN("the player presses the shader toggle") {
			player.getComponent<Input>()->command = test::pressing(input::InputEvent::ShaderToggle);
			testWorld.dispatcher().emit(test::inputFrom(player, player.getComponent<Input>()->command));

			THEN("each mesh switches to the other look") {
				REQUIRE(textured.getComponent<Mesh>()->pipelineType == PipelineType::VertexColor);
				REQUIRE(colored.getComponent<Mesh>()->pipelineType == PipelineType::Textured);
			}

			AND_WHEN("they press it again") {
				testWorld.dispatcher().emit(test::inputFrom(player, player.getComponent<Input>()->command));

				THEN("every mesh is back to how it was") {
					REQUIRE(textured.getComponent<Mesh>()->pipelineType == PipelineType::Textured);
					REQUIRE(colored.getComponent<Mesh>()->pipelineType == PipelineType::VertexColor);
				}
			}
		}

		WHEN("the player presses something else") {
			player.getComponent<Input>()->command = test::pressing(input::InputEvent::Jump);
			testWorld.dispatcher().emit(test::inputFrom(player, player.getComponent<Input>()->command));

			THEN("nothing changes") {
				REQUIRE(textured.getComponent<Mesh>()->pipelineType == PipelineType::Textured);
				REQUIRE(colored.getComponent<Mesh>()->pipelineType == PipelineType::VertexColor);
			}
		}
	}
}
