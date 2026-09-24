#pragma once

namespace render::input {
	class MouseInputProcessor {
	private:
		bool   m_hasLastMousePosition{false};
		double m_lastMouseX{0.0};
		double m_lastMouseY{0.0};
		double m_accumulatedMouseX{0.0};
		double m_accumulatedMouseY{0.0};

	public:
		void processMouseMove(double xPos, double yPos);
		void getMouseDelta(double& deltaX, double& deltaY);
	};
}
