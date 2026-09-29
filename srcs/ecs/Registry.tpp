#pragma once

#include <utility>

namespace ecs {
	template<typename... Components>
	Query<Components...> Registry::query() {
		return Query<Components...>(m_storage);
	}

	template<typename ComponentType>
	void Registry::describeComponentAs(const std::string_view name) {
		m_componentDescriber.add<ComponentType>(name);
	}

	template<typename SystemType, typename... Args>
	SystemType& Registry::createSystem(Args&&... args) {
		auto& system = m_systemManager.addSystem<SystemType>(std::forward<Args>(args)...);
		system.setRegistry(this);
		system.bindEvents(m_systemManager.getDispatcher());
		return system;
	}
}
