#pragma once

#include "../entity/Entity.hpp"
#include "Component.hpp"

namespace ecs {
	class IComponentManager {
	public:
		virtual ~IComponentManager() = default;

		virtual void               removeComponent(const Entity& entity) = 0;
		[[nodiscard]] virtual bool hasComponent(const Entity& entity) const = 0;
		virtual void               getComponent(const Entity& entity, IComponent*& component) = 0;
	};
}
