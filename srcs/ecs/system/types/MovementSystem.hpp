#pragma once

#include "ecs/system/System.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "ecs/component/Components.hpp"

namespace ecs {
	class MovementSystem : public System<component::Transform, component::Velocity, const component::Input> {
	public:
		void onInput(const InputEvent& event) const;
		void onSimulate(const SimulateEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
