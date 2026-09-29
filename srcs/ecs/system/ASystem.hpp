#pragma once

#include "ecs/system/Dispatcher.hpp"

namespace ecs {
	class Registry;

	class ASystem {
	protected:
		Registry* m_registry{nullptr};

	public:
		virtual ~ASystem() = default;

		virtual void bindEvents(Dispatcher& dispatcher) = 0;

		void setRegistry(Registry* registry) {
			m_registry = registry;
		}
	};
}
