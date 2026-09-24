#include "KeyInputProcessor.hpp"
#include <unordered_map>
#include "../../platform/input/glfw/GLFWInput.hpp"

#include "../../error/Exception.hpp"

using namespace render::input;

void KeyInputProcessor::processKey(const int scancode, const InputAction action, const InputMods modifiers) {
	const Input input       = createInput(scancode, modifiers);
	const Input singleInput = createInput(scancode, 0);

	auto it = m_bindings.find(input);
	if (it == m_bindings.end())
		it = m_bindings.find(singleInput);
	if (it == m_bindings.end())
		return;

	for (const auto& bindingInputEvent: it->second) {
		switchEvent(bindingInputEvent, action);
	}
}

void KeyInputProcessor::getKeyEvents(InputEvents& pressedEvents, InputEvents&  repeatedEvents,
									InputEvents&  releasedEvents, InputEvents& activeEvents) {
	pressedEvents    = m_pressedEvents;
	repeatedEvents   = m_repeatedEvents;
	releasedEvents   = m_releasedEvents;
	activeEvents     = m_activeEvents;
	m_pressedEvents  = 0;
	m_repeatedEvents = 0;
	m_releasedEvents = 0;
}

void KeyInputProcessor::processMouseButton(const MouseButton button, const InputAction action,
											const InputMods  modifiers) {
	const Input input       = createMouseInput(button, modifiers);
	const Input singleInput = createMouseInput(button, 0);

	auto it = m_bindings.find(input);
	if (it == m_bindings.end())
		it = m_bindings.find(singleInput);
	if (it == m_bindings.end())
		return;

	for (const auto& bindingInputEvent: it->second) {
		switchEvent(bindingInputEvent, action);
	}
}

void KeyInputProcessor::switchEvent(const InputEvent event, const InputAction action) {
	switch (action) {
		case Press:
			m_pressedEvents |= event;
			m_activeEvents |= event;
			break;
		case Release:
			m_releasedEvents |= event;
			m_activeEvents &= ~event;
			break;
		case Repeat:
			m_repeatedEvents |= event;
			break;
		default:
			break;
	}
}

void KeyInputProcessor::bindEvent(const Input input, const InputEvent event) {
	m_bindings[input].push_back(event);
}

void KeyInputProcessor::resetBindings() {
	m_bindings = m_defaultBindings.getDefaultBindings();

	if (m_bindings.empty()) {
		throw error::InputError("default key bindings were requested before GLFW was initialized");
	}
}
