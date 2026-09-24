#include "Component.hpp"

using namespace ecs;

int ComponentId::id = 1;

const std::string& IComponent::getName() const {
	return m_name;
}
