#pragma once

#include "IGui.hpp"
#include "../../ecs/entity/Entity.hpp"

namespace render::gui {
	constexpr size_t maxPanels = 32;

	class APanel {
	protected:
		IGui&       m_gui;
		ecs::Entity m_caller{};
		bool        m_isOpen = false;

		explicit APanel(IGui& gui);

	public:
		virtual ~APanel() = default;

		virtual void       open();
		virtual void       display() = 0;
		virtual void       close();
		void               toggle();
		[[nodiscard]] bool isOpen() const;
		void               setCaller(ecs::Entity caller);
	};
}
