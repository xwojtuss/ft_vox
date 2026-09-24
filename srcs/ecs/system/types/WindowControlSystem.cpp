#include "WindowControlSystem.hpp"
#include "../../component/Components.hpp"
#include "../../../render/input/InputTypes.hpp"
#include "../../../render/input/InputManager.hpp"
#include "../../World.hpp"

using namespace ecs;

WindowControlSystem::WindowControlSystem(platform::window::IWindow& window,
										render::gui::IGui&          gui) : ASystem(Dependencies()), m_window(window),
																m_gui(gui) {
	m_dependencies.addDependency<component::Input>();
}

void WindowControlSystem::onInput([[maybe_unused]] const InputEvent& event) const {
	for (const Entity& entity: m_entities) {
		const component::Input* input = m_world->getComponentManager<component::Input>().getComponent(entity);

		if (!input)
			continue;

		if (!m_window.isMouseCursorVisible()
			&& render::input::hasEvent(input->command.startedEvents, render::input::InputEvent::ToggleCursor)) {
			m_window.setMouseCursorVisible(true);
			m_window.setMouseCursorPositionToCenter();

			if (component::Transform* transform = m_world->getComponentManager<component::Transform>().
															getComponent(entity))
				transform->canRotate = false;

			if (component::Velocity* velocity = m_world->getComponentManager<component::Velocity>().
														getComponent(entity))
				velocity->canMove = false;
		} else if (m_window.isMouseCursorVisible()
					&& !m_gui.wantsMouseCapture()
					&& render::input::hasAnyEvent(input->command.startedEvents,
												render::input::InputEvent::AnyMouseButton)) {
			m_window.setMouseCursorVisible(false);

			if (component::Velocity* velocity = m_world->getComponentManager<component::Velocity>().
														getComponent(entity))
				velocity->canMove = true;

			if (component::Transform* transform = m_world->getComponentManager<component::Transform>().
															getComponent(entity))
				transform->canRotate = true;
		}
	}
}

void WindowControlSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe<InputEvent>(this, &WindowControlSystem::onInput);
}
