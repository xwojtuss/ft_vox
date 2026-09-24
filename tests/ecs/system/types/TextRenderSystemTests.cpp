#include <catch2/catch_test_macros.hpp>

#include "ecs/system/types/TextRenderSystem.hpp"
#include "support/FakeRenderer.hpp"
#include "support/TestEcs.hpp"

using ecs::component::Color;
using ecs::component::HAlignment;
using ecs::component::Text;
using ecs::component::Texture;
using ecs::component::Transform2D;
using ecs::component::VAlignment;

namespace {
ecs::EntityHandle	createLabel(test::TestWorld& testWorld, const std::string& content, glm::vec2 position, glm::vec2 characterSize = {1.0f, 1.0f}) {
	ecs::EntityHandle	label = testWorld.createEntity();
	Transform2D			transform;
	Text				text;
	transform.position = position;
	transform.scale = characterSize;
	text.text = content;
	label.addComponent(transform);
	label.addComponent(text);
	label.registerToSystem<ecs::TextRenderSystem>();
	return label;
}

void	drawText(test::TestWorld& testWorld, test::FakeRenderer& renderer) {
	testWorld.world.getSystemManager().onTextDraw(renderer);
}
}

SCENARIO("Texts are drawn one after another from a shared character buffer", "[ecs][text]") {
	GIVEN("the labels \"FPS\", an empty one and \"hello\"") {
		test::TestWorld		testWorld;
		test::FakeRenderer	renderer;
		testWorld.addSystem<ecs::TextRenderSystem>();
		createLabel(testWorld, "FPS", {10.0f, 20.0f});
		createLabel(testWorld, "", {0.0f, 0.0f});
		createLabel(testWorld, "hello", {30.0f, 40.0f});

		WHEN("text is drawn") {
			drawText(testWorld, renderer);

			THEN("the empty label is skipped") {
				REQUIRE(renderer.drawnTexts.size() == 2);
			}
			AND_THEN("each text starts in the buffer right after the previous one") {
				REQUIRE(renderer.drawnTexts[0].text == "FPS");
				REQUIRE(renderer.drawnTexts[0].offset == 0);
				REQUIRE(renderer.drawnTexts[1].text == "hello");
				REQUIRE(renderer.drawnTexts[1].offset == 3);
			}
			AND_THEN("left and top aligned text is drawn exactly at its position") {
				REQUIRE(renderer.drawnTexts[0].position == glm::vec2(10.0f, 20.0f));
			}
			AND_THEN("without a color or texture, none is passed") {
				REQUIRE(renderer.drawnTexts[0].color == nullptr);
				REQUIRE(renderer.drawnTexts[0].texture == nullptr);
			}
		}
	}
}

SCENARIO("Aligned text is shifted by its size, once", "[ecs][text]") {
	GIVEN("a 4 character label at (100, 50) whose characters are 2 wide and 3 tall") {
		test::TestWorld		testWorld;
		test::FakeRenderer	renderer;
		testWorld.addSystem<ecs::TextRenderSystem>();
		ecs::EntityHandle	label = createLabel(testWorld, "abcd", {100.0f, 50.0f}, {2.0f, 3.0f});
		Text&				text = *label.getComponent<Text>();

		WHEN("it is centered horizontally and vertically and drawn") {
			text.horizontalAlignment = HAlignment::Center;
			text.verticalAlignment = VAlignment::Middle;
			drawText(testWorld, renderer);

			THEN("it moves left by half its width and up by half its height") {
				REQUIRE(renderer.drawnTexts[0].position == glm::vec2(100.0f - 4.0f, 50.0f - 1.5f));
				REQUIRE(text.aligned);
			}

			AND_WHEN("it is drawn again") {
				drawText(testWorld, renderer);

				THEN("it is not shifted a second time") {
					REQUIRE(renderer.drawnTexts[1].position == renderer.drawnTexts[0].position);
				}
			}
		}

		WHEN("it is aligned right and bottom and drawn") {
			text.horizontalAlignment = HAlignment::Right;
			text.verticalAlignment = VAlignment::Bottom;
			drawText(testWorld, renderer);

			THEN("it moves left by its full width and up by its full height") {
				REQUIRE(renderer.drawnTexts[0].position == glm::vec2(100.0f - 8.0f, 50.0f - 3.0f));
			}
		}
	}
}

SCENARIO("Text is drawn with its color and font texture when it has them", "[ecs][text]") {
	GIVEN("a colored label with a font texture") {
		test::TestWorld		testWorld;
		test::FakeRenderer	renderer;
		testWorld.addSystem<ecs::TextRenderSystem>();
		ecs::EntityHandle	label = createLabel(testWorld, "hi", {0.0f, 0.0f});
		label.addComponent(Color());
		label.addComponent(Texture());

		WHEN("text is drawn") {
			drawText(testWorld, renderer);

			THEN("its color and texture are passed to the renderer") {
				REQUIRE(renderer.drawnTexts[0].color == label.getComponent<Color>());
				REQUIRE(renderer.drawnTexts[0].texture == label.getComponent<Texture>());
			}
		}
	}
}
