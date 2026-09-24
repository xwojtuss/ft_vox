#pragma once

#include "ComponentManager.hpp"
#include "error/Exception.hpp"

namespace ecs {
	template<typename ComponentType>
	ComponentManager<ComponentType>::ComponentManager() : m_components() {
	}

	template<typename ComponentType>
	void ComponentManager<ComponentType>::addComponent(const Entity& entity, const ComponentType& component) {
		if (m_componentCount >= maxComponents) {
			throw error::EcsError(
				"cannot add a component to entity " + std::to_string(entity) + ": all " + std::to_string(maxComponents)
				+ " slots are in use");
		}

		const size_t componentIndex         = m_componentCount;
		m_components[componentIndex]        = component;
		m_componentEntities[componentIndex] = entity;
		m_entityToComponentIndex[entity]    = componentIndex;
		m_componentCount++;
	}

	template<typename ComponentType>
	ComponentType* ComponentManager<ComponentType>::getComponent(const Entity& entity) {
		auto it = m_entityToComponentIndex.find(entity);
		if (it == m_entityToComponentIndex.end())
			return nullptr;
		return &m_components[it->second];
	}

	template<typename ComponentType>
	void ComponentManager<ComponentType>::getComponent(const Entity& entity, IComponent*& component) {
		auto it = m_entityToComponentIndex.find(entity);
		if (it == m_entityToComponentIndex.end()) {
			component = nullptr;
			return;
		}
		component = static_cast<IComponent*>(static_cast<Component<ComponentType>*>(&m_components[it->second]));
	}

	template<typename ComponentType>
	bool ComponentManager<ComponentType>::hasComponent(const Entity& entity) const {
		return m_entityToComponentIndex.find(entity) != m_entityToComponentIndex.end();
	}

	template<typename ComponentType>
	void ComponentManager<ComponentType>::removeComponent(const Entity& entity) {
		auto it = m_entityToComponentIndex.find(entity);
		if (it == m_entityToComponentIndex.end()) {
			throw error::EcsError("entity " + std::to_string(entity) + " has no component of this type");
		}

		const size_t componentIndex = it->second;
		const size_t lastIndex      = m_componentCount - 1;

		m_entityToComponentIndex.erase(it);
		if (componentIndex != lastIndex) {
			const Entity movedEntity = m_componentEntities[lastIndex];

			m_components[componentIndex]          = m_components[lastIndex];
			m_componentEntities[componentIndex]   = movedEntity;
			m_entityToComponentIndex[movedEntity] = componentIndex;
		}
		m_componentCount--;
	}

	template<typename ComponentType>
	size_t ComponentManager<ComponentType>::getComponentCount() const {
		return m_componentCount;
	}

	template<typename ComponentType>
	ComponentType* ComponentManager<ComponentType>::getComponentAtIndex(size_t index) {
		if (index >= m_componentCount) {
			throw error::EcsError(
				"component index " + std::to_string(index) + " is out of bounds, there are " +
				std::to_string(m_componentCount) + " components");
		}
		return &m_components[index];
	}
}
