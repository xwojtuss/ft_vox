#include "MouseInputProcessor.hpp"

using namespace render::input;

void MouseInputProcessor::processMouseMove(const double xPos, const double yPos) {
	if (!m_hasLastMousePosition) {
		m_lastMouseX           = xPos;
		m_lastMouseY           = yPos;
		m_hasLastMousePosition = true;
		return;
	}

	const double deltaX = xPos - m_lastMouseX;
	const double deltaY = yPos - m_lastMouseY;

	m_accumulatedMouseX += deltaX;
	m_accumulatedMouseY += deltaY;
	m_lastMouseX        = xPos;
	m_lastMouseY        = yPos;
}

void MouseInputProcessor::getMouseDelta(double& deltaX, double& deltaY) {
	deltaX              = m_accumulatedMouseX;
	deltaY              = m_accumulatedMouseY;
	m_accumulatedMouseX = 0.0;
	m_accumulatedMouseY = 0.0;
}
