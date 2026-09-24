#pragma once

#include <memory>
#include <unordered_map>

#include "component/IComponentManager.hpp"
#include "component/ComponentManager.hpp"
#include "component/Component.hpp"
#include "entity/EntityManager.hpp"
#include "system/SystemManager.hpp"
#include "../game/block/BlockData.hpp"

namespace ecs {
	struct EntityHandle;

	class World {
	private:
		SystemManager                                               m_systemManager;
		std::unordered_map<int, std::unique_ptr<IComponentManager>> m_componentManagers;
		EntityManager                                               m_entityManager;
		game::block::BlockDatas                                     m_blockDatas;

	public:
		explicit World(game::block::BlockDatas blockDatas);

		template<typename ComponentType>
		[[nodiscard]] ComponentManager<ComponentType>& getComponentManager();
		[[nodiscard]] IComponentManager*               getComponentManager(int componentId);
		EntityHandle                                   createEntity();
		void                                           destroyEntity(const Entity& entity) const;

		// This is expensive, only call for debug
		[[nodiscard]] std::vector<IComponent*> getAllComponents(const Entity& entity) const;

		template<typename SystemType, typename... Args>
		void createSystem(Args&&... args);

		[[nodiscard]] SystemManager&           getSystemManager();
		[[nodiscard]] game::block::BlockDatas& getBlockDatas();
	};
}

#include "World.tpp"
