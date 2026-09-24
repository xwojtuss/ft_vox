#include "World.hpp"

#include <utility>

#include "entity/EntityHandle.hpp"

using namespace ecs;

World::World(game::block::BlockDatas blockDatas) : m_blockDatas(std::move(blockDatas)) {
}

EntityHandle World::createEntity() {
	const Entity entity = m_entityManager.createEntity();
	return EntityHandle{.entity = entity, .world = this};
}

void World::destroyEntity(const Entity& entity) const {
	m_systemManager.unregisterEntity(entity);
	for (const auto& [componentId, manager]: m_componentManagers) {
		if (manager->hasComponent(entity)) {
			manager->removeComponent(entity);
		}
	}
}

IComponentManager* World::getComponentManager(int componentId) {
	const auto it = m_componentManagers.find(componentId);
	if (it == m_componentManagers.end()) {
		return nullptr;
	}
	return it->second.get();
}

std::vector<IComponent*> World::getAllComponents(const Entity& entity) const {
	std::vector<IComponent*> components;
	IComponent*              component = nullptr;

	for (const auto& [id, manager]: m_componentManagers) {
		if (manager->hasComponent(entity)) {
			manager->getComponent(entity, component);
			components.push_back(component);
		}
	}

	return components;
}

SystemManager& World::getSystemManager() {
	return m_systemManager;
}

game::block::BlockDatas& World::getBlockDatas() {
	return m_blockDatas;
}
