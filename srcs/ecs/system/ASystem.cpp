#include <algorithm>

#include "../World.hpp"
#include "../component/IComponentManager.hpp"
#include "ASystem.hpp"

using namespace ecs;

ASystem::ASystem(const Dependencies& dependencies) : m_dependencies(dependencies) {
}

Dependencies ASystem::getDependencies() const {
	return m_dependencies;
}

void ASystem::registerEntity(const Entity& entity) {
	if (hasEntity(entity) || !canRegister(entity))
		return;

	m_entities.push_back(entity);
}

void ASystem::unregisterEntity(const Entity& entity) {
	if (const auto it = std::find(m_entities.begin(), m_entities.end(), entity); it != m_entities.end())
		m_entities.erase(it);
}

void ASystem::registerWorld(World* world) {
	m_world = world;
}

bool ASystem::canRegister(const Entity& entity) const {
	if (!m_world)
		return false;

	const IComponentManager* manager = nullptr;

	for (int i = 0; i < static_cast<int>(m_dependencies.mask.size()); ++i) {
		if (!m_dependencies.mask.test(i))
			continue;

		manager = m_world->getComponentManager(i);

		if (!manager || !manager->hasComponent(entity))
			return false;
	}
	return true;
}

bool ASystem::hasEntity(const Entity& entity) const {
	return std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
}
