#include "ecs/system/types/GuiSystem.hpp"
#include "render/IRenderer.hpp"
#include "ecs/Registry.hpp"
#include "render/gui/PlayerComponentsPanel.hpp"
#include "render/gui/EventsRuntimePanel.hpp"
#include "render/gui/ClientPerformancePanel.hpp"
#include "render/gui/ServerPerformancePanel.hpp"

using namespace ecs;

GuiSystem::GuiSystem(render::gui::IGui& gui, const profiling::ClientStats& clientStats,
					 const profiling::ServerStats& serverStats) :
	m_gui(gui), m_clientStats(clientStats), m_serverStats(serverStats) {
}

void GuiSystem::onRegistryReady([[maybe_unused]] const RegistryReadyEvent& event) {
	render::gui::PlayerComponentsPanel playerComponentsPanel(m_gui, *m_registry);
	registerPanel(render::input::InputEvent::PlayerComponentsMenuToggle, playerComponentsPanel);
	render::gui::EventsRuntimePanel eventsRuntimePanel(m_gui, m_registry->getSystemManager().getDispatcher());
	registerPanel(render::input::InputEvent::EventRuntimesMenuToggle, eventsRuntimePanel);
	render::gui::ClientPerformancePanel clientPerformancePanel(m_gui, m_clientStats);
	registerPanel(render::input::InputEvent::ClientPerformanceToggle, clientPerformancePanel);
	render::gui::ServerPerformancePanel serverPerformancePanel(m_gui, m_serverStats);
	registerPanel(render::input::InputEvent::ServerPerformanceToggle, serverPerformancePanel);
}

void GuiSystem::onInput(const InputEvent& event) {
	for (const auto& [inputEvent, panelTypes]: m_eventToPanelType) {
		if (!render::input::hasEvent(event.command.startedEvents, inputEvent))
			continue;

		for (const auto& panelType: panelTypes) {
			auto it = m_panels.find(panelType);
			if (it == m_panels.end())
				continue;
			it->second->setCaller(event.source);
			it->second->toggle();
		}
	}
}

void GuiSystem::onRendererFrame(const RendererFrameEvent& event) {
	m_gui.beginFrame();

	for (const auto& [type, panel]: m_panels) {
		if (panel && panel->isOpen())
			panel->display();
	}

	m_gui.endFrame();
	event.renderer->render(m_gui);
}

void GuiSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe<InputEvent>(this, &GuiSystem::onInput);
	dispatcher.subscribe<RendererFrameEvent>(this, &GuiSystem::onRendererFrame);
	dispatcher.subscribe<RegistryReadyEvent>(this, &GuiSystem::onRegistryReady);
}
