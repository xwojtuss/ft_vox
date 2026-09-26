#pragma once

#include "ASystem.hpp"
#include "../Query.hpp"

namespace ecs {
	/**
	 * A system that processes every entity having all of Components.
	 * There is no registration to a System, it chooses which entities to process.
	 */
	template<typename... Components>
	class System : public ASystem {
	public:
		[[nodiscard]] Query<Components...> entities() const;
		[[nodiscard]] bool                 processes(Entity entity) const;
	};
}

#include "System.tpp"
