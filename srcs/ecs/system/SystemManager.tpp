#pragma once

namespace ecs {
	template<typename SystemType, typename... Args>
	SystemType& SystemManager::addSystem(Args&&... args) {
		auto        system    = std::make_unique<SystemType>(std::forward<Args>(args)...);
		SystemType* systemPtr = system.get();

		m_systems[std::type_index(typeid(SystemType))] = std::move(system);

		return *systemPtr;
	}

	template<typename SystemType>
	SystemType* SystemManager::getSystem() {
		auto it = m_systems.find(std::type_index(typeid(SystemType)));
		if (it != m_systems.end()) {
			return static_cast<SystemType*>(it->second.get());
		}
		return nullptr;
	}
}
