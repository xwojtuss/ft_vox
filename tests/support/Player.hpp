#pragma once

#include "ecs/component/Components.hpp"
#include "support/TestEcs.hpp"

namespace test {
	inline ecs::EntityHandle createPlayer(TestWorld& testWorld, float mouseSensitivity = 1.0f) {
		ecs::EntityHandle player = testWorld.createEntity();

		player.add(ecs::component::Transform{});
		player.add(ecs::component::Velocity{.maxSpeed = 10.0f, .acceleration = 5.0f, .deceleration = 5.0f});
		player.add(ecs::component::Input{.mouseSensitivity = mouseSensitivity});
		return player;
	}

	inline ecs::InputEvent inputFrom(const ecs::EntityHandle& source, const render::input::InputCommand& command) {
		ecs::InputEvent event;
		event.source  = source.id();
		event.command = command;
		return event;
	}

	inline render::input::InputCommand pressing(render::input::InputEvents startedEvents) {
		render::input::InputCommand command = {};
		command.startedEvents               = startedEvents;
		return command;
	}
}
