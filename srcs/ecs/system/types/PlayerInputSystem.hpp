#pragma once

#include "ecs/system/System.hpp"
#include "ecs/system/Dispatcher.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "ecs/component/Components.hpp"
#include "render/input/InputManager.hpp"

namespace ecs {
	class PlayerInputSystem : public System<component::Input> {
	private:
		render::input::InputManager& m_inputManager;
		Dispatcher*                  m_dispatcher{nullptr};

	public:
		explicit PlayerInputSystem(render::input::InputManager& inputManager);

		void onSimulate(const SimulateEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
