#pragma once

#include "ecs/component/Components.hpp"
#include "support/TestEcs.hpp"

namespace test {
	inline ecs::EntityHandle createPlayer(TestWorld& testWorld, float mouseSensitivity = 1.0f) {
		ecs::EntityHandle        player = testWorld.createEntity();
		ecs::component::Velocity velocity;
		ecs::component::Input    input;

		velocity.velocity        = glm::vec3(0.0f);
		velocity.desiredVelocity = glm::vec3(0.0f);
		velocity.maxSpeed        = 10.0f;
		velocity.acceleration    = 5.0f;
		velocity.deceleration    = 5.0f;
		input.command            = {};
		input.mouseSensitivity   = mouseSensitivity;

		player.addComponent(ecs::component::Transform());
		player.addComponent(velocity);
		player.addComponent(input);
		return player;
	}

	inline ecs::InputEvent inputFrom(const ecs::EntityHandle& source, const render::input::InputCommand& command) {
		ecs::InputEvent event;
		event.source  = source.entity;
		event.command = command;
		return event;
	}

	inline render::input::InputCommand pressing(render::input::InputEvents startedEvents) {
		render::input::InputCommand command = {};
		command.startedEvents               = startedEvents;
		return command;
	}
}
