#include "PlayerComponentsPanel.hpp"

#include "../../ecs/component/Components.hpp"

using namespace render::gui;

PlayerComponentsPanel::PlayerComponentsPanel(IGui& gui, ecs::World& world) : APanel(gui), m_world(world) {
}

void PlayerComponentsPanel::display() {
	if (!m_isOpen || !m_world.getEntity(m_caller).has<ecs::component::Transform>())
		return;

	const std::vector<ecs::ComponentDescription> components = m_world.describe(m_caller);

	if (m_gui.beginWindow("Player Components", &m_isOpen)) {
		for (const auto& [name, details]: components) {
			if (m_gui.beginSection(name, nullptr))
				m_gui.text(details);
			m_gui.endSection();
		}
	}
	m_gui.endWindow();
}
