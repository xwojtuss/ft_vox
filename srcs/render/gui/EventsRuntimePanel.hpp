#pragma once

#include "APanel.hpp"
#include "../../ecs/system/Dispatcher.hpp"

namespace render::gui {
	class EventsRuntimePanel : public APanel {
	private:
		ecs::Dispatcher& m_dispatcher;

	public:
		EventsRuntimePanel(IGui& gui, ecs::Dispatcher& dispatcher);

		void display() override;
	};
}
