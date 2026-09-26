#include "WindowControlSystem.hpp"
#include "../../../render/input/InputTypes.hpp"

using namespace ecs;

WindowControlSystem::WindowControlSystem(platform::window::IWindow& window, render::gui::IGui& gui) : m_window(window),
	m_gui(gui) {
}

void WindowControlSystem::onInput([[maybe_unused]] const InputEvent& event) const {
	for (auto&& [entity, input]: entities()) {
		const render::input::InputEvents started = input.command.startedEvents;

		if (!m_window.isMouseCursorVisible() && hasEvent(started, render::input::InputEvent::ToggleCursor))
			releaseCursor(m_world->getEntity(entity));
		else if (m_window.isMouseCursorVisible() && !m_gui.wantsMouseCapture()
				&& hasAnyEvent(started, render::input::InputEvent::AnyMouseButton))
			captureCursor(m_world->getEntity(entity));
	}
}

void WindowControlSystem::releaseCursor(EntityHandle player) const {
	m_window.setMouseCursorVisible(true);
	m_window.setMouseCursorPositionToCenter();

	if (auto* transform = player.tryGet<component::Transform>())
		transform->canRotate = false;
	if (auto* velocity = player.tryGet<component::Velocity>())
		velocity->canMove = false;
}

void WindowControlSystem::captureCursor(EntityHandle player) const {
	m_window.setMouseCursorVisible(false);

	if (auto* velocity = player.tryGet<component::Velocity>())
		velocity->canMove = true;
	if (auto* transform = player.tryGet<component::Transform>())
		transform->canRotate = true;
}

void WindowControlSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe<InputEvent>(this, &WindowControlSystem::onInput);
}
