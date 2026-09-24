#include "PlayerInputSystem.hpp"
#include "../../component/Components.hpp"
#include "../../../render/input/InputTypes.hpp"
#include "../../World.hpp"

using namespace ecs;

PlayerInputSystem::PlayerInputSystem(render::input::InputManager& inputManager) : ASystem(Dependencies()),
	m_inputManager(inputManager) {
	m_dependencies.addDependency<component::Input>();
}

void PlayerInputSystem::onSimulate(const SimulateEvent& event) const {
	const render::input::InputCommand command = m_inputManager.buildCommand();
	InputEvent                        inputEvent{};

	for (const Entity& entity: m_entities) {
		component::Input* input = m_world->getComponentManager<component::Input>().getComponent(entity);

		if (!input)
			continue;

		input->command     = command;
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
