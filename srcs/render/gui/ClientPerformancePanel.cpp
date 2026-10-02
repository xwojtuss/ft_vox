#include "render/gui/ClientPerformancePanel.hpp"

#include <format>

using namespace render::gui;

namespace {
	constexpr float  millisecondsPerSecond = 1000.0f;
	constexpr double bytesPerMebibyte      = 1024.0 * 1024.0;
	constexpr float  plotScaleMilliseconds = 33.3f;

	std::string formatMebibytes(const uint64_t bytes) {
		return std::format("{:.1f} MiB", static_cast<double>(bytes) / bytesPerMebibyte);
	}
}

ClientPerformancePanel::ClientPerformancePanel(IGui& gui, const profiling::ClientStats& stats) :
	APanel(gui), m_stats(stats) {
}

void ClientPerformancePanel::display() {
	if (!m_isOpen)
		return;

	if (m_gui.beginWindow("Client Performance", &m_isOpen)) {
		const profiling::FrameTimes& frameTimes = m_stats.frameTimes;

		m_gui.text(std::format("Frame time avg: {:.2f} ms", frameTimes.average() * millisecondsPerSecond));
		m_gui.text(std::format("Frame time worst: {:.2f} ms", frameTimes.worst() * millisecondsPerSecond));
		std::vector<float> milliseconds = frameTimes.values();
		for (float& value: milliseconds)
			value *= millisecondsPerSecond;
		m_gui.plotLines("##frameTimes", milliseconds, plotScaleMilliseconds);
		m_gui.separator();
		m_gui.text(std::format("Drawn meshes: {}", m_stats.drawnMeshes));
		m_gui.text(std::format("Draw calls: {}", m_stats.drawCalls));
		m_gui.text(std::format("Triangles: {}", m_stats.triangles));
		m_gui.separator();
		m_gui.text(std::format("GPU memory used: {}", formatMebibytes(m_stats.gpuMemoryUsedBytes)));
		m_gui.text(std::format("GPU memory reserved: {}", formatMebibytes(m_stats.gpuMemoryReservedBytes)));
		m_gui.text(std::format("CPU memory (draw lists): {}", formatMebibytes(m_stats.cpuMemoryBytes)));
		m_gui.separator();
		m_gui.text(std::format("Chunks waiting to be meshed: {}", m_stats.pendingMeshing));
	}
	m_gui.endWindow();
}
