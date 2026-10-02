#include "render/gui/ServerPerformancePanel.hpp"

#include <format>

using namespace render::gui;

namespace {
	constexpr double bytesPerMebibyte = 1024.0 * 1024.0;
}

ServerPerformancePanel::ServerPerformancePanel(IGui& gui, const profiling::ServerStats& stats) :
	APanel(gui), m_stats(stats) {
}

void ServerPerformancePanel::display() {
	if (!m_isOpen)
		return;

	if (m_gui.beginWindow("Server Performance", &m_isOpen)) {
		m_gui.text(std::format("Loaded chunks: {}", m_stats.loadedChunks));
		m_gui.text(
			std::format("Chunk data: {:.1f} MiB", static_cast<double>(m_stats.chunkDataBytes) / bytesPerMebibyte));
		m_gui.separator();
		m_gui.text(std::format("Chunks waiting to be generated: {}", m_stats.pendingGeneration));
	}
	m_gui.endWindow();
}
