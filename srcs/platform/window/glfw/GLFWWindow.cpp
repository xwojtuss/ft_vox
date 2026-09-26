#include "GLFWWindow.hpp"
#include "../app/ApplicationInfo.hpp"
#include "../../input/glfw/GLFWInput.hpp"
#include "../../../error/Exception.hpp"

using namespace platform::window::glfw;

GLFWWindow::GLFWWindow() {
	if (glfwInit() != GLFW_TRUE)
		throw error::WindowError("GLFW could not be initialized, is a display available?");
	if (glfwVulkanSupported() != GLFW_TRUE) {
		glfwTerminate();
		throw error::WindowError("no Vulkan loader or driver was found on this system");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(winWidth, winHeight, app::appName, nullptr, nullptr);
	if (m_window == nullptr) {
		glfwTerminate();
		throw error::WindowError("the window could not be created");
	}
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	setMouseCursorVisible(false);
	setMouseCursorPositionToCenter();
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_FALSE);

	glfwSetCursorPosCallback(m_window, cursorPositionCallback);
	glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
	glfwSetKeyCallback(m_window, keyCallback);
}

void GLFWWindow::cursorPositionCallback(GLFWwindow* rawWindow, const double xPos, const double yPos) {
	auto* window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(rawWindow));

	window->getInputManager().processMouseMove(xPos, yPos);
}

void GLFWWindow::mouseButtonCallback(GLFWwindow* rawWindow, const int button, const int action, const int mods) {
	auto* window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(rawWindow));

	window->getInputManager().processMouseButton(input::glfw::glfwToMouseButton(button),
												input::glfw::glfwToInputAction(action),
												input::glfw::glfwToInputMods(mods));
}

void GLFWWindow::keyCallback(GLFWwindow* rawWindow, [[maybe_unused]] int key, const int scancode, const int action,
							const int    mods) {
	auto* window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(rawWindow));

	window->getInputManager().processKey(scancode, input::glfw::glfwToInputAction(action),
										input::glfw::glfwToInputMods(mods));
}

void GLFWWindow::framebufferResizeCallback(GLFWwindow*           rawWindow, [[maybe_unused]] int width,
											[[maybe_unused]] int height) {
	auto* window         = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(rawWindow));
	window->m_wasResized = true;
}

uint32_t GLFWWindow::getWidth() const {
	int width = 0;
	glfwGetFramebufferSize(m_window, &width, nullptr);
	return static_cast<uint32_t>(width);
}

uint32_t GLFWWindow::getHeight() const {
	int height = 0;
	glfwGetFramebufferSize(m_window, nullptr, &height);
	return static_cast<uint32_t>(height);
}

void GLFWWindow::waitUntilNotMinimized() {
	int width  = 0;
	int height = 0;

	glfwGetFramebufferSize(m_window, &width, &height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	m_wasResized = false;
}

void GLFWWindow::getFramebufferSize(uint32_t* width, uint32_t* height) const {
	int w = 0;
	int h = 0;
	glfwGetFramebufferSize(m_window, &w, &h);
	*width  = static_cast<uint32_t>(w);
	*height = static_cast<uint32_t>(h);
}

bool GLFWWindow::shouldClose() const {
	return glfwWindowShouldClose(m_window);
}

void GLFWWindow::pollEvents() {
	glfwPollEvents();
}

bool GLFWWindow::wasResized() const {
	return m_wasResized;
}

void* GLFWWindow::getHandle() const {
	return m_window;
}

double GLFWWindow::getTime() const {
	return glfwGetTime();
}

const char** GLFWWindow::getExtensions(uint32_t* count) const {
	return glfwGetRequiredInstanceExtensions(count);
}

float GLFWWindow::getAspectRatio() const {
	int width  = 0;
	int height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	if (width == 0 || height == 0)
		return aspectRatio;
	return static_cast<float>(width) / static_cast<float>(height);
}

void GLFWWindow::setMouseCursorVisible(const bool visible) {
	glfwSetInputMode(m_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void GLFWWindow::setMouseCursorPosition(const double x, const double y) {
	glfwSetCursorPos(m_window, x, y);
}

void GLFWWindow::setMouseCursorPositionToCenter() {
	int width  = 0;
	int height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	GLFWWindow::setMouseCursorPosition(width / 2.0, height / 2.0);
}

bool GLFWWindow::isMouseCursorVisible() const {
	return glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
}

render::input::InputManager& GLFWWindow::getInputManager() {
	return m_inputManager;
}

GLFWWindow::~GLFWWindow() {
	glfwDestroyWindow(m_window);
	glfwTerminate();
}
