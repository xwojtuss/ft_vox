#include "platform/input/glfw/GLFWDefaultKeybinds.hpp"
#include <GLFW/glfw3.h>

using namespace platform::input::glfw;

GLFWDefaultKeybinds::GLFWDefaultKeybinds() = default;

void GLFWDefaultKeybinds::init() {
	addBinding(GLFW_KEY_W, render::input::InputMod::None, render::input::InputEvent::MoveForward);
	addBinding(GLFW_KEY_S, render::input::InputMod::None, render::input::InputEvent::MoveBackward);
	addBinding(GLFW_KEY_D, render::input::InputMod::None, render::input::InputEvent::MoveRight);
	addBinding(GLFW_KEY_A, render::input::InputMod::None, render::input::InputEvent::MoveLeft);
	addBinding(GLFW_KEY_SPACE, render::input::InputMod::None, render::input::InputEvent::Jump);
	addBinding(GLFW_KEY_LEFT_CONTROL, render::input::InputMod::None, render::input::InputEvent::Crouch);
	addBinding(GLFW_KEY_ESCAPE, render::input::InputMod::None, render::input::InputEvent::ToggleCursor);
	addBinding(GLFW_KEY_F3, render::input::InputMod::None, render::input::InputEvent::PlayerComponentsMenuToggle);
	addBinding(GLFW_KEY_F3, render::input::InputMod::None, render::input::InputEvent::EventRuntimesMenuToggle);
	addBinding(GLFW_KEY_F4, render::input::InputMod::None, render::input::InputEvent::ClientPerformanceToggle);
	addBinding(GLFW_KEY_F5, render::input::InputMod::None, render::input::InputEvent::ServerPerformanceToggle);

	addMouseBinding(render::input::MouseButton::LeftButton, render::input::InputMod::None,
					render::input::InputEvent::SecondaryButton);
	addMouseBinding(render::input::MouseButton::RightButton, render::input::InputMod::None,
					render::input::InputEvent::ActionButton);

	if (!m_bindings.empty())
		m_initialized = true;
}

void GLFWDefaultKeybinds::addBinding(int key, render::input::InputMods mods, render::input::InputEvent event) {
	m_bindings[render::input::createInput(glfwGetKeyScancode(key), mods)].push_back(event);
}

void GLFWDefaultKeybinds::addMouseBinding(render::input::MouseButton button, render::input::InputMods mods,
										  render::input::InputEvent event) {
	m_bindings[render::input::createMouseInput(button, mods)].push_back(event);
}

render::input::InputEventBindings GLFWDefaultKeybinds::getDefaultBindings() {
	init();
	return m_bindings;
}
