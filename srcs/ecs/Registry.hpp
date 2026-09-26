#pragma once

#include <entt/entity/registry.hpp>

#include "entity/Entity.hpp"

namespace ecs {
	using Registry = entt::basic_registry<Entity>;
}
