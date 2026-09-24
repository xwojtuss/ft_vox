#pragma once

#include "../../../render/input/InputTypes.hpp"

namespace platform::input::glfw {
	class GLFWDefaultKeybinds {
	private:
		render::input::InputEventBindings m_bindings;
		bool                              m_initialized = false;

		void addBinding(int key, render::input::InputMods mods, render::input::InputEvent event);
		void addMouseBinding(render::input::MouseButton button, render::input::InputMods mods,
							render::input::InputEvent   event);

	public:
		GLFWDefaultKeybinds();

		void                              init();
		render::input::InputEventBindings getDefaultBindings();
	};
}
