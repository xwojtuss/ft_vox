#pragma once

#include "MouseInputProcessor.hpp"
#include "KeyInputProcessor.hpp"
#include "InputTypes.hpp"

namespace render::input {
	class InputManager {
	private:
		MouseInputProcessor m_mouseProcessor;
		KeyInputProcessor   m_keyInputProcessor;

		static float axis(InputEvents activeEvents, InputEvent positive, InputEvent negative);

	public:
		[[nodiscard]] InputCommand       buildCommand();
		void                             processMouseMove(double xPos, double yPos);
		void                             processMouseButton(int button, InputAction action, InputMods modifiers);
		void                             processKey(int scancode, InputAction action, InputMods modifiers);
		[[nodiscard]] KeyInputProcessor& getKeyInputProcessor();
	};
}
