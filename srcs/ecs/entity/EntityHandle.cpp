#include "ecs/entity/EntityHandle.hpp"

using namespace ecs;

EntityHandle::EntityHandle(Storage& storage, const Entity entity) : m_storage(&storage), m_entity(entity) {
}

Entity EntityHandle::id() const {
	return m_entity;
}

bool EntityHandle::isAlive() const {
	return m_storage->valid(m_entity);
}

void EntityHandle::destroy() {
	if (isAlive())
		m_storage->destroy(m_entity);
}
