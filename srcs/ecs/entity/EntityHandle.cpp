#include "EntityHandle.hpp"

using namespace ecs;

EntityHandle::EntityHandle(Registry& registry, const Entity entity) : m_registry(&registry), m_entity(entity) {
}

Entity EntityHandle::id() const {
	return m_entity;
}

bool EntityHandle::isAlive() const {
	return m_registry->valid(m_entity);
}

void EntityHandle::destroy() {
	if (isAlive())
		m_registry->destroy(m_entity);
}
