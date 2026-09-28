#pragma once

#include "render/gui/APanel.hpp"
#include "ecs/World.hpp"

namespace render::gui {
	class PlayerComponentsPanel : public APanel {
	private:
		ecs::World& m_world;

	public:
		PlayerComponentsPanel(IGui& gui, ecs::World& world);

		void display() override;
	};
}
