#pragma once

#include "render/gui/APanel.hpp"
#include "ecs/Registry.hpp"

namespace render::gui {
	class PlayerComponentsPanel : public APanel {
	private:
		ecs::Registry& m_registry;

	public:
		PlayerComponentsPanel(IGui& gui, ecs::Registry& registry);

		void display() override;
	};
}
