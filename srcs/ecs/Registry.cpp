#include "ecs/Registry.hpp"

#include <utility>

#include "ecs/component/Components.hpp"

using namespace ecs;

static_assert(nullEntity == entt::null);

Registry::Registry(game::block::BlockDatas blockDatas) : m_blockDatas(std::move(blockDatas)) {
	describeComponentAs<component::Transform>("Transform");
	describeComponentAs<component::Velocity>("Velocity");
	describeComponentAs<component::Camera>("Camera");
	describeComponentAs<component::Mesh>("Mesh");
	describeComponentAs<component::Texture>("Texture");
	describeComponentAs<component::Input>("Input");
}

EntityHandle Registry::createEntity() {
	return {m_storage, m_storage.create()};
}

EntityHandle Registry::getEntity(const Entity entity) {
	return {m_storage, entity};
}

void Registry::destroyEntity(const Entity entity) {
	getEntity(entity).destroy();
}

std::vector<ComponentDescription> Registry::describe(const Entity entity) const {
	return m_componentDescriber.describe(m_storage, entity);
}

SystemManager& Registry::getSystemManager() {
	return m_systemManager;
}

game::block::BlockDatas& Registry::getBlockDatas() {
	return m_blockDatas;
}
