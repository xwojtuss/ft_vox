#include "GLFWInput.hpp"
#include <GLFW/glfw3.h>

namespace platform::input::glfw {
	render::input::InputAction glfwToInputAction(const int glfwAction) {
		switch (glfwAction) {
			case GLFW_PRESS:
				return render::input::InputAction::Press;
			case GLFW_RELEASE:
				return render::input::InputAction::Release;
			case GLFW_REPEAT:
				return render::input::InputAction::Repeat;
			default:
				return render::input::InputAction::Release;
		}
	}

	render::input::InputMods glfwToInputMods(const int glfwMods) {
		render::input::InputMods mods = 0;

		if (glfwMods & GLFW_MOD_SHIFT)
			mods = mods | render::input::InputMod::Shift;
		if (glfwMods & GLFW_MOD_CONTROL)
			mods = mods | render::input::InputMod::Control;
		if (glfwMods & GLFW_MOD_ALT)
			mods = mods | render::input::InputMod::Alt;
		if (glfwMods & GLFW_MOD_SUPER)
			mods = mods | render::input::InputMod::Super;

		return mods;
	}

	render::input::MouseButton glfwToMouseButton(const int glfwButton) {
		return static_cast<render::input::MouseButton>(1U << glfwButton);
	}
}
