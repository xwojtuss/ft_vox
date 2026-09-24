#pragma once

namespace ecs {
	template<typename PanelType>
	void GuiSystem::registerPanel(const render::input::InputEvent toggleEvent, PanelType& panel) {
		auto typeIdx = std::type_index(typeid(PanelType));

		m_panels.emplace(typeIdx, std::make_unique<PanelType>(panel));

		if (m_eventToPanelType.find(toggleEvent) != m_eventToPanelType.end()) {
			m_eventToPanelType[toggleEvent].push_back(typeIdx);
			return;
		}
		std::vector<std::type_index> panelTypes;

		panelTypes.push_back(typeIdx);
		m_eventToPanelType[toggleEvent] = panelTypes;
	}

	template<typename PanelType>
	PanelType* GuiSystem::getPanel() {
		const auto typeIdx = std::type_index(typeid(PanelType));

		if (const auto it = m_panels.find(typeIdx); it != m_panels.end()) {
			return static_cast<PanelType*>(it->second.get());
		}

		return nullptr;
	}
}
