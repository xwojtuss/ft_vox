#pragma once

#include "render/input/InputTypes.hpp"
#include "platform/input/glfw/GLFWDefaultKeybinds.hpp"

namespace render::input {
	class KeyInputProcessor {
	public:
		constexpr static size_t maxEventActions = 64;

	private:
		platform::input::glfw::GLFWDefaultKeybinds m_defaultBindings;
		InputEventBindings                         m_bindings;
		InputEvents                                m_pressedEvents  = InputEvent::None;
		InputEvents                                m_repeatedEvents = InputEvent::None;
		InputEvents                                m_releasedEvents = InputEvent::None;
		InputEvents                                m_activeEvents   = InputEvent::None;

		void switchEvent(InputEvent event, InputAction action);

	public:
		void processKey(int scancode, InputAction action, InputMods modifiers);
		void getKeyEvents(InputEvents& pressedEvents, InputEvents& repeatedEvents, InputEvents& releasedEvents,
						  InputEvents& activeEvents);
		void processMouseButton(MouseButton button, InputAction action, InputMods modifiers);
		void bindEvent(Input input, InputEvent event);
		void resetBindings();
	};
}
