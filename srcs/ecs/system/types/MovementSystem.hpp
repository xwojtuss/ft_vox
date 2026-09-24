#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../ASystem.hpp"
#include "../DispatcherEvents.hpp"

namespace ecs {
	class World;

	class MovementSystem : public ASystem {
	public:
		MovementSystem();

		void onInput(const InputEvent& event) const;
		void onSimulate(const SimulateEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
