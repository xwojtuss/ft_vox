#pragma once

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace render::input {
	using InputEvents = uint32_t;

	enum InputEvent : uint32_t {
		MoveForward                = 1U << 0U,
		MoveBackward               = 1U << 1U,
		MoveRight                  = 1U << 2U,
		MoveLeft                   = 1U << 3U,
		Jump                       = 1U << 4U,
		Crouch                     = 1U << 5U,
		AnyMove                    = MoveForward | MoveBackward | MoveRight | MoveLeft | Jump | Crouch,
		ToggleCursor               = 1U << 6U,
		ActionButton               = 1U << 7U,
		SecondaryButton            = 1U << 8U,
		AnyMouseButton             = ActionButton | SecondaryButton,
		PlayerComponentsMenuToggle = 1U << 9U,
		EventRuntimesMenuToggle    = 1U << 10U,
		All                        = 1U << ((sizeof(InputEvents) * 8U) - 1U)
	};

	enum MouseButton : uint32_t {
		LeftButton   = 1U << 0U,
		RightButton  = 1U << 1U,
		MiddleButton = 1U << 2U,
		Button4      = 1U << 3U,
		Button5      = 1U << 4U,
		Button6      = 1U << 5U,
		Button7      = 1U << 6U,
		Button8      = 1U << 7U,
		AnyButton    = LeftButton | RightButton | MiddleButton | Button4 | Button5 | Button6 | Button7 | Button8
	};

	struct InputCommand {
		float       moveForward{};
		float       moveRight{};
		float       moveUp{};
		float       lookUp{};
		float       lookRight{};
		InputEvents startedEvents{};
		InputEvents repeatedEvents{};
		InputEvents releasedEvents{};
		InputEvents activeEvents{};

		float maxPitch = glm::radians(89.0f);
	};

	enum InputAction {
		Press,
		Release,
		Repeat
	};

	using InputMods = uint32_t;

	enum InputMod : uint32_t {
		Shift   = 1U << 0U,
		Control = 1U << 1U,
		Alt     = 1U << 2U,
		Super   = 1U << 3U
	};

	[[nodiscard]] constexpr bool hasModifier(const InputMods mods, const InputMod mod) {
		return (mods & mod) != 0;
	}

	[[nodiscard]] constexpr bool hasEvent(const InputEvents events, const InputEvent event) {
		return (events & event) != 0;
	}

	[[nodiscard]] constexpr bool hasAnyEvent(const InputEvents events, const InputEvents other) {
		return (events & other) != 0;
	}

	[[nodiscard]] constexpr bool hasAllEvents(const InputEvents events, const InputEvents other) {
		return (events & other) == other;
	}

	using Input = long;

	[[nodiscard]] constexpr Input createInput(const int scancode, const InputMods mods) {
		return (static_cast<Input>(scancode) << 8) | static_cast<Input>(mods);
	}

	[[nodiscard]] constexpr Input createMouseInput(const MouseButton button, const InputMods mods) {
		return (static_cast<Input>(button) << 16) | static_cast<Input>(mods);
	}

	[[nodiscard]] constexpr int getScancode(const Input input) {
		return static_cast<int>(input >> 8);
	}

	[[nodiscard]] constexpr MouseButton getMouseButton(const Input input) {
		return static_cast<MouseButton>(input >> 16);
	}

	using InputEventBindings = std::unordered_map<Input, std::vector<InputEvent>>;
}
