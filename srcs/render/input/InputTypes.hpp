#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>

namespace render::input {
	using magic_enum::bitwise_operators::operator~;
	using magic_enum::bitwise_operators::operator|;
	using magic_enum::bitwise_operators::operator&;
	using magic_enum::bitwise_operators::operator|=;
	using magic_enum::bitwise_operators::operator&=;

	enum class InputEvent : uint32_t {
		None                       = 0,
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
		ClientPerformanceToggle    = 1U << 11U,
		ServerPerformanceToggle    = 1U << 12U,
		All                        = 1U << 31U
	};

	using InputEvents = InputEvent;

	enum class MouseButton : uint32_t {
		None         = 0,
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

	enum class InputAction : uint8_t { Press, Release, Repeat };

	enum class InputMod : uint32_t { None = 0, Shift = 1U << 0U, Control = 1U << 1U, Alt = 1U << 2U, Super = 1U << 3U };

	using InputMods = InputMod;

	[[nodiscard]] constexpr bool hasModifier(const InputMods mods, const InputMod mod) {
		return magic_enum::enum_flags_test(mods, mod);
	}

	[[nodiscard]] constexpr bool hasEvent(const InputEvents events, const InputEvent event) {
		return magic_enum::enum_flags_test(events, event);
	}

	[[nodiscard]] constexpr bool hasAnyEvent(const InputEvents events, const InputEvents other) {
		return magic_enum::enum_flags_test_any(events, other);
	}

	[[nodiscard]] constexpr bool hasAllEvents(const InputEvents events, const InputEvents other) {
		return magic_enum::enum_flags_test(events, other);
	}

	using Input = long;

	[[nodiscard]] constexpr Input createInput(const int scancode, const InputMods mods) {
		return (static_cast<Input>(scancode) << 8) | static_cast<Input>(std::to_underlying(mods));
	}

	[[nodiscard]] constexpr Input createMouseInput(const MouseButton button, const InputMods mods) {
		return (static_cast<Input>(std::to_underlying(button)) << 16) | static_cast<Input>(std::to_underlying(mods));
	}

	[[nodiscard]] constexpr int getScancode(const Input input) {
		return static_cast<int>(input >> 8);
	}

	[[nodiscard]] constexpr MouseButton getMouseButton(const Input input) {
		return static_cast<MouseButton>(input >> 16);
	}

	using InputEventBindings = std::unordered_map<Input, std::vector<InputEvent>>;
}
