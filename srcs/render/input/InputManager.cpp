#include "InputManager.hpp"

using namespace render::input;

float InputManager::axis(const InputEvents activeEvents, const InputEvent positive, const InputEvent negative) {
	const float forward  = hasEvent(activeEvents, positive) ? 1.0f : 0.0f;
	const float backward = hasEvent(activeEvents, negative) ? 1.0f : 0.0f;
	return forward - backward;
}

InputCommand InputManager::buildCommand() {
	InputCommand command = {};

	double deltaX = 0.0;
	double deltaY = 0.0;
	m_mouseProcessor.getMouseDelta(deltaX, deltaY);

	command.lookRight += static_cast<float>(deltaX);
	command.lookUp    += static_cast<float>(deltaY);

	InputEvents pressedEvents  = 0;
	InputEvents repeatedEvents = 0;
	InputEvents releasedEvents = 0;
	InputEvents activeEvents   = 0;
	m_keyInputProcessor.getKeyEvents(pressedEvents, repeatedEvents, releasedEvents, activeEvents);

	command.startedEvents  = pressedEvents;
	command.repeatedEvents = repeatedEvents;
	command.releasedEvents = releasedEvents;
	command.activeEvents   = activeEvents;

	command.moveForward = axis(activeEvents, MoveForward, MoveBackward);
	command.moveRight   = axis(activeEvents, MoveRight, MoveLeft);
	command.moveUp      = axis(activeEvents, Jump, Crouch);

	return command;
}

void InputManager::processMouseMove(const double xPos, const double yPos) {
	m_mouseProcessor.processMouseMove(-xPos, -yPos);
}

void InputManager::processMouseButton(int button, const InputAction action, const InputMods modifiers) {
	m_keyInputProcessor.processMouseButton(static_cast<MouseButton>(button), action, modifiers);
}

void InputManager::processKey(const int scancode, const InputAction action, const InputMods modifiers) {
	m_keyInputProcessor.processKey(scancode, action, modifiers);
}

KeyInputProcessor& InputManager::getKeyInputProcessor() {
	return m_keyInputProcessor;
}
