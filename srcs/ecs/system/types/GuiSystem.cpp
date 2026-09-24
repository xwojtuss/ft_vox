#include "GuiSystem.hpp"
#include "../../../render/IRenderer.hpp"
#include "../../World.hpp"
#include "../../../render/gui/PlayerComponentsPanel.hpp"
#include "../../../render/gui/EventsRuntimePanel.hpp"

using namespace ecs;

GuiSystem::GuiSystem(render::gui::IGui& gui) : ASystem(Dependencies()), m_gui(gui) {
}

void GuiSystem::onWorldReady([[maybe_unused]] const WorldReadyEvent& event) {
	render::gui::PlayerComponentsPanel playerComponentsPanel(m_gui, *m_world);
	registerPanel(render::input::PlayerComponentsMenuToggle, playerComponentsPanel);
	render::gui::EventsRuntimePanel eventsRuntimePanel(m_gui, m_world->getSystemManager().getDispatcher());
	registerPanel(render::input::EventRuntimesMenuToggle, eventsRuntimePanel);
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
	dispatcher.subscribe<WorldReadyEvent>(this, &GuiSystem::onWorldReady);
}
