#include "PlayerComponentsPanel.hpp"

#include <sstream>

#include "component/Components.hpp"

using namespace render::gui;

PlayerComponentsPanel::PlayerComponentsPanel(IGui& gui, ecs::World& world) : APanel(gui), m_world(world) {
}

void PlayerComponentsPanel::display() {
	if (!m_isOpen)
		return;

	if (const ecs::component::Transform* transform = m_world.getComponentManager<ecs::component::Transform>().
															getComponent(m_caller); !transform)
		return;

	const std::vector<ecs::IComponent*> components = m_world.getAllComponents(m_caller);
	if (m_gui.beginWindow("Player Components", &m_isOpen)) {
		std::stringstream ss;

		for (const auto& component: components) {
			if (m_gui.beginSection(component->getName(), nullptr)) {
				ss << *component;
				m_gui.text(ss.str());
				ss.str("");
			}
			m_gui.endSection();
		}
	}
	m_gui.endWindow();
}
