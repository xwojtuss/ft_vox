#pragma once

#include "../System.hpp"
#include "../DispatcherEvents.hpp"
#include "../../component/Components.hpp"
#include "../../../render/gui/IGui.hpp"
#include "../../../platform/window/IWindow.hpp"

namespace ecs {
	class WindowControlSystem : public System<const component::Input> {
	private:
		platform::window::IWindow& m_window;
		render::gui::IGui&         m_gui;

		void releaseCursor(EntityHandle player) const;
		void captureCursor(EntityHandle player) const;

	public:
		WindowControlSystem(platform::window::IWindow& window, render::gui::IGui& gui);

		void onInput(const InputEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
