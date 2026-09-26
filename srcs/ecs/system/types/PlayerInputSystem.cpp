#include "PlayerInputSystem.hpp"

using namespace ecs;

PlayerInputSystem::PlayerInputSystem(render::input::InputManager& inputManager) : m_inputManager(inputManager) {
}

void PlayerInputSystem::onSimulate(const SimulateEvent& event) const {
	const render::input::InputCommand command = m_inputManager.buildCommand();
	InputEvent                        inputEvent{};

	for (auto&& [entity, input]: entities()) {
		input.command      = command;
		inputEvent.source  = entity;
		inputEvent.command = command;
		break;
	}
	inputEvent.deltaTime = event.deltaTime;
	m_dispatcher->emit<InputEvent>(inputEvent);
}

void PlayerInputSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe<SimulateEvent>(this, &PlayerInputSystem::onSimulate);
	m_dispatcher = &dispatcher;
}
