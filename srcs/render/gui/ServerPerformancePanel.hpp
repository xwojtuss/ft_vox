#pragma once

#include "profiling/ServerStats.hpp"
#include "render/gui/APanel.hpp"

namespace render::gui {
	class ServerPerformancePanel : public APanel {
	private:
		const profiling::ServerStats& m_stats;

	public:
		ServerPerformancePanel(IGui& gui, const profiling::ServerStats& stats);

		void display() override;
	};
}
