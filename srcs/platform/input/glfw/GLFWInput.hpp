#pragma once

#include "../../../render/input/InputTypes.hpp"

namespace platform::input::glfw {
	[[nodiscard]] render::input::InputAction glfwToInputAction(int glfwAction);
	[[nodiscard]] render::input::InputMods   glfwToInputMods(int glfwMods);

	[[nodiscard]] render::input::MouseButton glfwToMouseButton(int glfwButton);
}
