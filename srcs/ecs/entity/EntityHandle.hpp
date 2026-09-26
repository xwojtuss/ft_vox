#pragma once

#include "Entity.hpp"
#include "../Registry.hpp"

namespace ecs {
	class EntityHandle {
	private:
		Registry* m_registry{};
		Entity    m_entity;

	public:
		EntityHandle(Registry& registry, Entity entity);

		[[nodiscard]] Entity id() const;
		[[nodiscard]] bool   isAlive() const;
		void                 destroy();

		template<typename ComponentType>
		decltype(auto) add(ComponentType component);

		template<typename ComponentType>
		void remove();

		template<typename ComponentType>
		[[nodiscard]] bool has() const;

		template<typename ComponentType>
		[[nodiscard]] ComponentType& get();

		template<typename ComponentType>
		[[nodiscard]] const ComponentType& get() const;

		template<typename ComponentType>
		[[nodiscard]] ComponentType* tryGet();

		template<typename ComponentType>
		[[nodiscard]] const ComponentType* tryGet() const;
	};
}

#include "EntityHandle.tpp"
