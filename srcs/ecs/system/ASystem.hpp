#pragma once

#include "Dependencies.hpp"
#include "Dispatcher.hpp"
#include "../entity/Entity.hpp"

namespace ecs {
	class World;

	class ASystem {
	protected:
		Dependencies        m_dependencies;
		std::vector<Entity> m_entities;
		World*              m_world{nullptr};

		explicit ASystem(const Dependencies& dependencies);

	public:
		virtual ~ASystem() = default;

		virtual void bindEvents(Dispatcher& dispatcher) = 0;
		void         registerEntity(const Entity& entity);
		void         unregisterEntity(const Entity& entity);
		void         registerWorld(World* world);

		[[nodiscard]] Dependencies getDependencies() const;
		[[nodiscard]] virtual bool canRegister(const Entity& entity) const;
		[[nodiscard]] bool         hasEntity(const Entity& entity) const;
	};
}
