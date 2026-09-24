#pragma once

#include "../render/input/InputManager.hpp"

namespace platform::window {
	constexpr uint32_t winWidth    = 800;
	constexpr uint32_t winHeight   = 600;
	constexpr float    aspectRatio = static_cast<float>(winWidth) / static_cast<float>(winHeight);

	class IWindow {
	public:
		virtual ~IWindow() = default;

		[[nodiscard]] virtual uint32_t     getWidth() const = 0;
		[[nodiscard]] virtual uint32_t     getHeight() const = 0;
		[[nodiscard]] virtual float        getAspectRatio() const = 0;
		virtual void                       getFramebufferSize(uint32_t* width, uint32_t* height) const = 0;
		virtual void                       waitUntilNotMinimized() = 0;
		[[nodiscard]] virtual bool         shouldClose() const = 0;
		virtual void                       pollEvents() = 0;
		[[nodiscard]] virtual bool         wasResized() const = 0;
		[[nodiscard]] virtual void*        getHandle() const = 0;
		[[nodiscard]] virtual const char** getExtensions(uint32_t* count) const = 0;
		[[nodiscard]] virtual double       getTime() const = 0;
		virtual void                       setMouseCursorVisible(bool visible) = 0;
		virtual void                       setMouseCursorPosition(double x, double y) = 0;
		virtual void                       setMouseCursorPositionToCenter() = 0;
		[[nodiscard]] virtual bool         isMouseCursorVisible() const = 0;

		[[nodiscard]] virtual render::input::InputManager& getInputManager() = 0;
	};
}
