#include "EntityManager.hpp"

using namespace ecs;

Entity EntityManager::createEntity() {
	return m_lastEntity++;
}
