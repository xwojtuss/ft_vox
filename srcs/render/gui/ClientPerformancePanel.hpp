#pragma once

#include "profiling/ClientStats.hpp"
#include "render/gui/APanel.hpp"

namespace render::gui {
	class ClientPerformancePanel : public APanel {
	private:
		const profiling::ClientStats& m_stats;

	public:
		ClientPerformancePanel(IGui& gui, const profiling::ClientStats& stats);

		void display() override;
	};
}
