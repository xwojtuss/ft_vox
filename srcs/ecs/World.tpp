#pragma once

#include <utility>

namespace ecs {
	template<typename... Components>
	Query<Components...> World::query() {
		return Query<Components...>(m_registry);
	}

	template<typename ComponentType>
	void World::describeComponentAs(const std::string_view name) {
		m_componentDescriber.add<ComponentType>(name);
	}

	template<typename SystemType, typename... Args>
	SystemType& World::createSystem(Args&&... args) {
		auto& system = m_systemManager.addSystem<SystemType>(std::forward<Args>(args)...);
		system.registerWorld(this);
		system.bindEvents(m_systemManager.getDispatcher());
		return system;
	}
}
