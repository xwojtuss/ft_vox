#include <catch2/catch_test_macros.hpp>

#include "scene/WorldInfo.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "ecs/system/types/RenderSystem.hpp"
#include "game/world/ChunkManager.hpp"
#include "support/Blocks.hpp"
#include "support/FakeRenderer.hpp"

using game::world::ChunkManager;
using game::world::chunkXSize;
using game::world::chunkYSize;
using game::world::chunkZSize;
namespace worldinfo = scene::worldinfo;

namespace {
	constexpr int chunksAroundSpawn = (worldinfo::maxHorizontalRenderDistance + 1)
									* (worldinfo::maxHorizontalRenderDistance + 1)
									* worldinfo::maxVerticalRenderDistance;

	constexpr size_t trianglesInSolidChunk = 6 * chunkXSize * chunkZSize * 2;

	constexpr ecs::Entity firstEntity = 1;

	struct SpawnedWorld {
		int                           dirtPixels = 0;
		game::block::BlockDatas       blockDatas = test::makeBlockDatas(&dirtPixels);
		ecs::World                    world{blockDatas};
		test::FakeRenderer            renderer;
		std::unique_ptr<ChunkManager> manager;
		size_t                        meshesAtSpawn;

		SpawnedWorld() {
			world.createSystem<ecs::RenderSystem>();
			manager       = std::make_unique<ChunkManager>(blockDatas, world, renderer);
			meshesAtSpawn = renderer.createdMeshes.size();
		}

		size_t meshCount() const {
			return renderer.createdMeshes.size();
		}

		ecs::EntityHandle entityOfMesh(size_t meshIndex) {
			return ecs::EntityHandle{firstEntity + static_cast<ecs::Entity>(meshIndex), &world};
		}
	};

	SpawnedWorld& sharedSpawnedWorld() {
		static SpawnedWorld instance;
		return instance;
	}
}

SCENARIO("Starting the world loads every chunk around spawn", "[chunk-manager]") {
	GIVEN("a world whose chunk manager has just been created") {
		SpawnedWorld& env = sharedSpawnedWorld();

		THEN("exactly one texture is created, from the dirt block's texture") {
			REQUIRE(env.renderer.createdTextures.size() == 1);
			REQUIRE(env.renderer.createdTexturePixels.front() == &env.dirtPixels);
		}
		AND_THEN("every chunk in the render distance with visible blocks gets one mesh, and no more") {
			REQUIRE(env.meshesAtSpawn > 0);
			REQUIRE(env.meshesAtSpawn <= static_cast<size_t>(chunksAroundSpawn));
			for (size_t i = 0; i < env.meshesAtSpawn; ++i)
				REQUIRE(env.renderer.createdMeshTriangleCounts[i] > 0);
		}
		AND_THEN("each chunk mesh becomes a textured entity drawn by the render system") {
			ecs::RenderSystem* renderSystem = env.world.getSystemManager().getSystem<ecs::RenderSystem>();

			for (size_t i = 0; i < env.meshesAtSpawn; ++i) {
				ecs::EntityHandle           entity = env.entityOfMesh(i);
				const ecs::component::Mesh* mesh   = entity.getComponent<ecs::component::Mesh>();

				REQUIRE(mesh != nullptr);
				REQUIRE(mesh->mesh.id == env.renderer.createdMeshes[i].id);
				REQUIRE(mesh->pipelineType == assets::PipelineType::Textured);
				REQUIRE(
					entity.getComponent<ecs::component::Texture>()->texture.id == env.renderer.createdTextures.front().
					id);
				REQUIRE(entity.hasComponent<ecs::component::Transform>());
				REQUIRE(renderSystem->hasEntity(entity.entity));
			}
		}
		AND_THEN("every chunk entity sits on the 16-block chunk grid, inside the render distance") {
			const glm::ivec3 chunkSize(chunkXSize, chunkYSize, chunkZSize);
			const int        halfDistance = worldinfo::maxHorizontalRenderDistance / 2;

			for (size_t i = 0; i < env.meshesAtSpawn; ++i) {
				const glm::vec3  position = env.entityOfMesh(i).getComponent<ecs::component::Transform>()->position;
				const glm::ivec3 chunk    = glm::ivec3(position) / chunkSize;

				REQUIRE(glm::vec3(chunk * chunkSize) == position);
				REQUIRE(std::abs(chunk.x) <= halfDistance);
				REQUIRE(std::abs(chunk.z) <= halfDistance);
				REQUIRE(chunk.y >= 0);
				REQUIRE(chunk.y < worldinfo::maxVerticalRenderDistance);
			}
		}
	}
}

// TODO: make pass
SCENARIO("Loading a chunk makes it visible", "[chunk-manager]") {
	GIVEN("a running world") {
		SpawnedWorld& env          = sharedSpawnedWorld();
		const size_t  meshesBefore = env.meshCount();

		WHEN("an underground chunk (y = -1) is loaded") {
			env.manager->loadChunk(glm::ivec3(40, -1, -40));

			THEN("it gets exactly one new mesh: the outer shell of a solid chunk") {
				REQUIRE(env.meshCount() == meshesBefore + 1);
				REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesInSolidChunk);
			}
			AND_THEN("its entity is placed at that chunk's world position") {
				const glm::vec3 position = env.entityOfMesh(meshesBefore).getComponent<ecs::component::Transform>()->
						position;
				REQUIRE(position == glm::vec3(40 * chunkXSize, -1 * chunkYSize, -40 * chunkZSize));
			}
		}

		WHEN("a chunk above the maximum terrain height is loaded") {
			env.manager->loadChunk(0, worldinfo::terrainMaxHeightBlocks / chunkYSize, 0);

			THEN("it is empty, so no mesh or entity is created") {
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}

		WHEN("a 2 x 1 x 3 range of underground chunks is loaded") {
			env.manager->loadRange({50, -2, 50}, {51, -2, 52});

			THEN("every chunk in the range, both ends included, gets a mesh") {
				REQUIRE(env.meshCount() == meshesBefore + 2 * 1 * 3);
			}
		}

		WHEN("an already visible chunk is made renderable again") {
			env.manager->loadChunk(glm::ivec3(45, -1, 45));
			env.manager->makeChunkRenderable(env.world, env.renderer, {45, -1, 45});

			THEN("it is not duplicated: it still has exactly one mesh") {
				REQUIRE(env.meshCount() == meshesBefore + 1);
			}
		}
	}
}

// TODO: make pass
SCENARIO("Unloading a chunk forgets it", "[chunk-manager]") {
	GIVEN("a running world") {
		SpawnedWorld& env = sharedSpawnedWorld();

		WHEN("a loaded chunk is unloaded") {
			env.manager->loadChunk(60, -1, 60);
			env.manager->unloadChunk(60, -1, 60);
			const size_t meshesBefore = env.meshCount();

			THEN("it can no longer be made renderable") {
				env.manager->makeChunkRenderable(env.world, env.renderer, {60, -1, 60});
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}

		WHEN("a visible chunk is unloaded") {
			const size_t meshIndex = env.meshCount();
			env.manager->loadChunk(65, -1, 65);
			ecs::EntityHandle chunkEntity = env.entityOfMesh(meshIndex);
			REQUIRE(chunkEntity.hasComponent<ecs::component::Mesh>());

			env.manager->unloadChunk(65, -1, 65);

			THEN("it disappears from the world: it is no longer drawn") {
				ecs::RenderSystem* renderSystem = env.world.getSystemManager().getSystem<ecs::RenderSystem>();

				REQUIRE_FALSE(renderSystem->hasEntity(chunkEntity.entity));
				REQUIRE_FALSE(chunkEntity.hasComponent<ecs::component::Mesh>());
			}
		}

		WHEN("a range of loaded chunks is unloaded") {
			env.manager->loadRange({70, -1, 70}, {71, -1, 71});
			env.manager->unloadRange({70, -1, 70}, {71, -1, 71});
			const size_t meshesBefore = env.meshCount();

			THEN("none of them can be made renderable any more") {
				for (int x = 70; x <= 71; ++x)
					for (int z = 70; z <= 71; ++z)
						env.manager->makeChunkRenderable(env.world, env.renderer, {x, -1, z});
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}

		WHEN("a chunk that was never loaded is unloaded") {
			const size_t meshesBefore = env.meshCount();

			THEN("nothing happens") {
				REQUIRE_NOTHROW(env.manager->unloadChunk(glm::ivec3(999, 999, 999)));
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}
	}
}
