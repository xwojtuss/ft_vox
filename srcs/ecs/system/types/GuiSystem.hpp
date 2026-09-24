#pragma once

#include "../ASystem.hpp"
#include "../DispatcherEvents.hpp"
#include "../../../render/gui/IGui.hpp"
#include "../../../render/gui/APanel.hpp"
#include "../../../render/input/InputTypes.hpp"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <vector>

namespace ecs {
	class GuiSystem : public ASystem {
	private:
		render::gui::IGui&                                                          m_gui;
		std::unordered_map<render::input::InputEvent, std::vector<std::type_index>> m_eventToPanelType;
		std::unordered_map<std::type_index, std::unique_ptr<render::gui::APanel>>   m_panels;

		template<typename PanelType>
		void registerPanel(render::input::InputEvent toggleEvent, PanelType& panel);

		template<typename PanelType>
		PanelType* getPanel();

	public:
		explicit GuiSystem(render::gui::IGui& gui);

		void onWorldReady(const WorldReadyEvent& event);
		void onInput(const InputEvent& event);
		void onRendererFrame(const RendererFrameEvent& event);
		void bindEvents(Dispatcher& dispatcher) override;
	};
}

#include "GuiSystem.tpp"
