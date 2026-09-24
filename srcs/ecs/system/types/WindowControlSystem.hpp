#pragma once

#include "../ASystem.hpp"
#include "../DispatcherEvents.hpp"
#include "../../../render/gui/IGui.hpp"
#include "../../../platform/window/IWindow.hpp"

namespace ecs {
	class WindowControlSystem : public ASystem {
	private:
		platform::window::IWindow& m_window;
		render::gui::IGui&         m_gui;

	public:
		WindowControlSystem(platform::window::IWindow& window, render::gui::IGui& gui);

		void onInput(const InputEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
