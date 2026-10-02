#pragma once

#include "ecs/system/ASystem.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "profiling/ClientStats.hpp"
#include "profiling/ServerStats.hpp"
#include "render/gui/IGui.hpp"
#include "render/gui/APanel.hpp"
#include "render/input/InputTypes.hpp"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <vector>

namespace ecs {
	class GuiSystem : public ASystem {
	private:
		render::gui::IGui&                                                          m_gui;
		const profiling::ClientStats&                                               m_clientStats;
		const profiling::ServerStats&                                               m_serverStats;
		std::unordered_map<render::input::InputEvent, std::vector<std::type_index>> m_eventToPanelType;
		std::unordered_map<std::type_index, std::unique_ptr<render::gui::APanel>>   m_panels;

		template<typename PanelType>
		void registerPanel(render::input::InputEvent toggleEvent, PanelType& panel);

		template<typename PanelType>
		PanelType* getPanel();

	public:
		GuiSystem(render::gui::IGui& gui, const profiling::ClientStats& clientStats,
				  const profiling::ServerStats& serverStats);

		void onRegistryReady(const RegistryReadyEvent& event);
		void onInput(const InputEvent& event);
		void onRendererFrame(const RendererFrameEvent& event);
		void bindEvents(Dispatcher& dispatcher) override;
	};
}

#include "ecs/system/types/GuiSystem.tpp"
