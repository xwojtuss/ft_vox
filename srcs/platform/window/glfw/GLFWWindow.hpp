#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#ifdef _WIN32
# define VK_USE_PLATFORM_WIN32_KHR
# define GLFW_EXPOSE_NATIVE_WIN32
# include <GLFW/glfw3native.h>
#endif

#include "../IWindow.hpp"
#include "../../../render/input/InputManager.hpp"

namespace platform::window::glfw {
	class GLFWWindow final : public IWindow {
	private:
		GLFWwindow*                 m_window;
		render::input::InputManager m_inputManager;
		bool                        m_wasResized = false;

		static void framebufferResizeCallback(GLFWwindow* rawWindow, int width, int height);
		static void cursorPositionCallback(GLFWwindow* rawWindow, double xPos, double yPos);
		static void mouseButtonCallback(GLFWwindow* rawWindow, int button, int action, int mods);
		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	public:
		GLFWWindow();
		GLFWWindow(const GLFWWindow&)            = delete;
		GLFWWindow& operator=(const GLFWWindow&) = delete;
		~GLFWWindow() override;

		[[nodiscard]] uint32_t getWidth() const override;
		[[nodiscard]] uint32_t getHeight() const override;
		void                   getFramebufferSize(uint32_t* width, uint32_t* height) const override;
		[[nodiscard]] float    getAspectRatio() const override;
		void                   waitUntilNotMinimized() override;
		[[nodiscard]] bool     shouldClose() const override;
		void                   pollEvents() override;
		[[nodiscard]] bool     wasResized() const override;
		[[nodiscard]] void*    getHandle() const override;
		const char**           getExtensions(uint32_t* count) const override;
		[[nodiscard]] double   getTime() const override;
		void                   setMouseCursorVisible(bool visible) override;
		void                   setMouseCursorPosition(double x, double y) override;
		void                   setMouseCursorPositionToCenter() override;
		[[nodiscard]] bool     isMouseCursorVisible() const override;

		render::input::InputManager& getInputManager() override;
	};
}
