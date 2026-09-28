#pragma once

#include <string>
#include <vector>

#include "ecs/Registry.hpp"
#include "ecs/Query.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "ecs/component/ComponentDescriber.hpp"
#include "ecs/system/SystemManager.hpp"
#include "game/block/BlockData.hpp"

namespace ecs {
	class World {
	private:
		Registry                m_registry{};
		ComponentDescriber      m_componentDescriber;
		SystemManager           m_systemManager;
		game::block::BlockDatas m_blockDatas;

	public:
		explicit World(game::block::BlockDatas blockDatas);
		World(const World&)            = delete;
		World& operator=(const World&) = delete;

		[[nodiscard]] EntityHandle createEntity();
		[[nodiscard]] EntityHandle getEntity(Entity entity);
		void                       destroyEntity(Entity entity);

		template<typename... Components>
		[[nodiscard]] Query<Components...> query();

		template<typename ComponentType>
		void describeComponentAs(std::string name);

		[[nodiscard]] std::vector<ComponentDescription> describe(Entity entity) const;

		template<typename SystemType, typename... Args>
		SystemType& createSystem(Args&&... args);

		[[nodiscard]] SystemManager&           getSystemManager();
		[[nodiscard]] game::block::BlockDatas& getBlockDatas();
	};
}

#include "ecs/World.tpp"
