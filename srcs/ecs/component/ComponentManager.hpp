#pragma once

#include <array>
#include <unordered_map>

#include "IComponentManager.hpp"
#include "../entity/Entity.hpp"

namespace ecs {
	constexpr int maxComponents = 2048;

	template<typename ComponentType>
	class ComponentManager : public IComponentManager {
	private:
		std::array<ComponentType, maxComponents> m_components;
		std::array<Entity, maxComponents>        m_componentEntities{};
		std::unordered_map<Entity, size_t>       m_entityToComponentIndex;
		std::size_t                              m_componentCount{0};

	public:
		ComponentManager();

		void                         addComponent(const Entity& entity, const ComponentType& component);
		void                         removeComponent(const Entity& entity) override;
		[[nodiscard]] ComponentType* getComponent(const Entity& entity);
		void                         getComponent(const Entity& entity, IComponent*& component) override;
		[[nodiscard]] bool           hasComponent(const Entity& entity) const override;
		[[nodiscard]] size_t         getComponentCount() const;
		[[nodiscard]] ComponentType* getComponentAtIndex(size_t index);
	};
}

#include "ComponentManager.tpp"
