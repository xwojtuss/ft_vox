#include "EventsRuntimePanel.hpp"
#include <sstream>
#include <iomanip>

using namespace render::gui;

EventsRuntimePanel::EventsRuntimePanel(IGui& gui, ecs::Dispatcher& dispatcher) : APanel(gui), m_dispatcher(dispatcher) {
}

void EventsRuntimePanel::display() {
	if (!m_isOpen)
		return;

	auto runtimes = m_dispatcher.getEventRuntimes();

	if (m_gui.beginWindow("EventsRuntime", &m_isOpen)) {
		bool              first = true;
		std::stringstream ss;

		for (const auto& [eventName, runtime]: runtimes) {
			if (!first)
				m_gui.separator();

			first = false;

			ss << std::fixed << std::setprecision(7);
			ss << eventName << " Runtime: " << runtime.count() << " ms";
			m_gui.text(ss.str());
			ss.str("");
		}
	}
	m_gui.endWindow();
}
