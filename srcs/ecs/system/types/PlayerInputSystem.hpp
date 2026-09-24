#pragma once

#include "../ASystem.hpp"
#include "../Dispatcher.hpp"
#include "../DispatcherEvents.hpp"
#include "../../../render/input/InputManager.hpp"

namespace ecs {
	class PlayerInputSystem : public ASystem {
	private:
		render::input::InputManager& m_inputManager;
		Dispatcher*                  m_dispatcher{nullptr};

	public:
		explicit PlayerInputSystem(render::input::InputManager& inputManager);

		void onSimulate(const SimulateEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
