#pragma once

#include <format>
#include <utility>
#include <entt/core/type_info.hpp>

#include "../../error/Exception.hpp"

namespace ecs {
	template<typename ComponentType>
	decltype(auto) EntityHandle::add(ComponentType component) {
		if (!isAlive())
			throw error::EcsError(std::format("cannot add {} to entity {}, it does not exist",
											entt::type_name<ComponentType>::value(), std::to_underlying(m_entity)));
		return m_registry->emplace_or_replace<ComponentType>(m_entity, std::move(component));
	}

	template<typename ComponentType>
	void EntityHandle::remove() {
		m_registry->remove<ComponentType>(m_entity);
	}

	template<typename ComponentType>
	bool EntityHandle::has() const {
		return m_registry->all_of<ComponentType>(m_entity);
	}

	template<typename ComponentType>
	ComponentType& EntityHandle::get() {
		return const_cast<ComponentType&>(std::as_const(*this).get<ComponentType>());
	}

	template<typename ComponentType>
	const ComponentType& EntityHandle::get() const {
		const ComponentType* component = tryGet<ComponentType>();
		if (component == nullptr)
			throw error::EcsError(std::format("entity {} has no {}", std::to_underlying(m_entity),
											entt::type_name<ComponentType>::value()));
		return *component;
	}

	template<typename ComponentType>
	ComponentType* EntityHandle::tryGet() {
		return m_registry->try_get<ComponentType>(m_entity);
	}

	template<typename ComponentType>
	const ComponentType* EntityHandle::tryGet() const {
		return std::as_const(*m_registry).try_get<ComponentType>(m_entity);
	}
}
