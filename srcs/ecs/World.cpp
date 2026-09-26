#include "World.hpp"

#include <utility>

#include "component/Components.hpp"

using namespace ecs;

static_assert(nullEntity == entt::null);

World::World(game::block::BlockDatas blockDatas) : m_blockDatas(std::move(blockDatas)) {
	describeComponentAs<component::Transform>("Transform");
	describeComponentAs<component::Velocity>("Velocity");
	describeComponentAs<component::Camera>("Camera");
	describeComponentAs<component::Mesh>("Mesh");
	describeComponentAs<component::Texture>("Texture");
	describeComponentAs<component::Input>("Input");
}

EntityHandle World::createEntity() {
	return {m_registry, m_registry.create()};
}

EntityHandle World::getEntity(const Entity entity) {
	return {m_registry, entity};
}

void World::destroyEntity(const Entity entity) {
	getEntity(entity).destroy();
}

std::vector<ComponentDescription> World::describe(const Entity entity) const {
	return m_componentDescriber.describe(m_registry, entity);
}

SystemManager& World::getSystemManager() {
	return m_systemManager;
}

game::block::BlockDatas& World::getBlockDatas() {
	return m_blockDatas;
}
