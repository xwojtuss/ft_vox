#pragma once

#include "Entity.hpp"

namespace ecs {
	class EntityManager {
	private:
		Entity m_lastEntity{1};

	public:
		Entity createEntity();
	};
}
