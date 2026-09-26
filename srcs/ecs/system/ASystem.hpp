#pragma once

#include "Dispatcher.hpp"

namespace ecs {
	class World;

	class ASystem {
	protected:
		World* m_world{nullptr};

	public:
		virtual ~ASystem() = default;

		virtual void bindEvents(Dispatcher& dispatcher) = 0;

		void registerWorld(World* world) { m_world = world; }
	};
}
