#pragma once

#include "Entity.hpp"

namespace ecs {
	class World;

	struct EntityHandle {
		Entity entity;
		World* world;

		template<typename ComponentType>
		[[nodiscard]] ComponentType* getComponent();

		template<typename ComponentType>
		[[nodiscard]] bool hasComponent() const;

		template<typename ComponentType>
		void addComponent(const ComponentType& component);

		template<typename ComponentType>
		void removeComponent();

		template<typename SystemType>
		void registerToSystem();

		template<typename SystemType>
		void unregisterFromSystem();
	};
}

#include "EntityHandle.tpp"
