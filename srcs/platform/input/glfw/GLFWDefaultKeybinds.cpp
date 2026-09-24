#include "GLFWDefaultKeybinds.hpp"
#include <GLFW/glfw3.h>

using namespace platform::input::glfw;

GLFWDefaultKeybinds::GLFWDefaultKeybinds() = default;

void GLFWDefaultKeybinds::init() {
	addBinding(GLFW_KEY_W, 0, render::input::InputEvent::MoveForward);
	addBinding(GLFW_KEY_S, 0, render::input::InputEvent::MoveBackward);
	addBinding(GLFW_KEY_D, 0, render::input::InputEvent::MoveRight);
	addBinding(GLFW_KEY_A, 0, render::input::InputEvent::MoveLeft);
	addBinding(GLFW_KEY_SPACE, 0, render::input::InputEvent::Jump);
	addBinding(GLFW_KEY_LEFT_CONTROL, 0, render::input::InputEvent::Crouch);
	addBinding(GLFW_KEY_ESCAPE, 0, render::input::InputEvent::ToggleCursor);
	addBinding(GLFW_KEY_F3, 0, render::input::InputEvent::PlayerComponentsMenuToggle);
	addBinding(GLFW_KEY_F3, 0, render::input::InputEvent::EventRuntimesMenuToggle);

	addMouseBinding(render::input::MouseButton::LeftButton, 0, render::input::InputEvent::SecondaryButton);
	addMouseBinding(render::input::MouseButton::RightButton, 0, render::input::InputEvent::ActionButton);

	if (!m_bindings.empty())
		m_initialized = true;
}

void GLFWDefaultKeybinds::addBinding(int key, render::input::InputMods mods, render::input::InputEvent event) {
	m_bindings[render::input::createInput(glfwGetKeyScancode(key), mods)].push_back(event);
}

void GLFWDefaultKeybinds::addMouseBinding(render::input::MouseButton button, render::input::InputMods mods,
										render::input::InputEvent    event) {
	m_bindings[render::input::createMouseInput(button, mods)].push_back(event);
}

render::input::InputEventBindings GLFWDefaultKeybinds::getDefaultBindings() {
	init();
	return m_bindings;
}
