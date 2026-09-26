#pragma once

#include "../World.hpp"
#include "../../error/Exception.hpp"

namespace ecs {
	template<typename... Components>
	Query<Components...> System<Components...>::entities() const {
		if (m_world == nullptr)
			throw error::EcsError("a system needs a world before it can look at entities");
		return m_world->query<Components...>();
	}

	template<typename... Components>
	bool System<Components...>::processes(const Entity entity) const {
		return m_world != nullptr && entities().contains(entity);
	}
}
