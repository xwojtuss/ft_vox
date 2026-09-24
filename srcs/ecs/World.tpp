#pragma once

namespace ecs {
	template<typename ComponentType>
	ComponentManager<ComponentType>& World::getComponentManager() {
		const int componentId = Component<ComponentType>::getId();

		if (m_componentManagers.find(componentId) == m_componentManagers.end()) {
			m_componentManagers[componentId] = std::make_unique<ComponentManager<ComponentType>>();
		}

		return *static_cast<ComponentManager<ComponentType>*>(m_componentManagers[componentId].get());
	}

	template<typename SystemType, typename... Args>
	void World::createSystem(Args&&... args) {
		auto& system = m_systemManager.addSystem<SystemType>(std::forward<Args>(args)...);
		system.registerWorld(this);
		system.bindEvents(m_systemManager.getDispatcher());
	}
}
