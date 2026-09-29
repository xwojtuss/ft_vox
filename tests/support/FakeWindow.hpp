#pragma once

#include "platform/window/IWindow.hpp"

namespace test {
	class FakeWindow : public platform::window::IWindow {
	public:
		bool                        cursorVisible    = false;
		int                         cursorCenterings = 0;
		render::input::InputManager inputManager;

		void setMouseCursorVisible(bool visible) override {
			cursorVisible = visible;
		}
		[[nodiscard]] bool isMouseCursorVisible() const override {
			return cursorVisible;
		}
		void setMouseCursorPositionToCenter() override {
			++cursorCenterings;
		}
		render::input::InputManager& getInputManager() override {
			return inputManager;
		}

		[[nodiscard]] uint32_t getWidth() const override {
			return platform::window::winWidth;
		}
		[[nodiscard]] uint32_t getHeight() const override {
			return platform::window::winHeight;
		}
		[[nodiscard]] float getAspectRatio() const override {
			return platform::window::aspectRatio;
		}

		void getFramebufferSize(uint32_t* width, uint32_t* height) const override {
			*width  = getWidth();
			*height = getHeight();
		}

		void waitUntilNotMinimized() override {
		}

		[[nodiscard]] bool shouldClose() const override {
			return false;
		}

		void pollEvents() override {
		}

		[[nodiscard]] bool wasResized() const override {
			return false;
		}
		[[nodiscard]] void* getHandle() const override {
			return nullptr;
		}

		const char** getExtensions(uint32_t* count) const override {
			*count = 0;
			return nullptr;
		}

		[[nodiscard]] double getTime() const override {
			return 0.0;
		}

		void setMouseCursorPosition(double, double) override {
		}
	};
}
