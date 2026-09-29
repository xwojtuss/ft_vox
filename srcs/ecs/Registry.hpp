#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ecs/Storage.hpp"
#include "ecs/Query.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "ecs/component/ComponentDescriber.hpp"
#include "ecs/system/SystemManager.hpp"
#include "game/block/BlockData.hpp"

namespace ecs {
	class Registry {
	private:
		Storage                 m_storage{};
		ComponentDescriber      m_componentDescriber;
		SystemManager           m_systemManager;
		game::block::BlockDatas m_blockDatas;

	public:
		explicit Registry(game::block::BlockDatas blockDatas);
		Registry(const Registry&)            = delete;
		Registry& operator=(const Registry&) = delete;
		Registry(Registry&&)                 = delete;
		Registry& operator=(Registry&&)      = delete;
		~Registry()                          = default;

		[[nodiscard]] EntityHandle createEntity();
		[[nodiscard]] EntityHandle getEntity(Entity entity);
		void                       destroyEntity(Entity entity);

		template<typename... Components>
		[[nodiscard]] Query<Components...> query();

		template<typename ComponentType>
		void describeComponentAs(std::string_view name);

		[[nodiscard]] std::vector<ComponentDescription> describe(Entity entity) const;

		template<typename SystemType, typename... Args>
		SystemType& createSystem(Args&&... args);

		[[nodiscard]] SystemManager&           getSystemManager();
		[[nodiscard]] game::block::BlockDatas& getBlockDatas();
	};
}

#include "ecs/Registry.tpp"
