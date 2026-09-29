#pragma once

#include <entt/entity/registry.hpp>

#include "ecs/entity/Entity.hpp"

namespace ecs {
	using Storage = entt::basic_registry<Entity>;
}
