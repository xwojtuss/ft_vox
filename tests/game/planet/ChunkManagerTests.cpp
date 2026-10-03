#include <catch2/catch_test_macros.hpp>

#include <utility>

#include "scene/PlanetInfo.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "ecs/system/types/RenderSystem.hpp"
#include "game/planet/ChunkManager.hpp"
#include "support/Blocks.hpp"
#include "support/FakeRenderer.hpp"
#include "support/TestSeed.hpp"

using game::planet::ChunkManager;
using game::planet::chunkXSize;
using game::planet::chunkYSize;
using game::planet::chunkZSize;
namespace planetinfo = scene::planetinfo;

namespace {
	constexpr glm::vec<3, unsigned short> spawnRenderDistance = {2, planetinfo::maxVerticalRenderDistance, 2};

	constexpr int chunksAroundSpawn = (spawnRenderDistance.x + 1) * (spawnRenderDistance.z + 1) * spawnRenderDistance.y;

	constexpr size_t trianglesOnTop = 2uz * chunkXSize * chunkZSize;

	struct SpawnedPlanet {
		std::vector<unsigned char>    dirtPixels = {10, 20, 30, 255};
		game::block::BlockDatas       blockDatas = test::makeBlockDatas(dirtPixels);
		ecs::Registry                 registry{blockDatas};
		test::FakeRenderer            renderer;
		profiling::ServerStats        stats;
		std::unique_ptr<ChunkManager> manager       = createManager();
		size_t                        meshesAtSpawn = renderer.createdMeshes.size();

		std::unique_ptr<ChunkManager> createManager() {
			registry.createSystem<ecs::RenderSystem>();
			return std::make_unique<ChunkManager>(blockDatas, registry, renderer, stats, test::seed,
												  spawnRenderDistance);
		}

		[[nodiscard]] size_t meshCount() const {
			return renderer.createdMeshes.size();
		}

		[[nodiscard]] size_t drawnChunkCount() {
			size_t count = 0;
			for (auto&& drawn: registry.query<const ecs::component::Mesh>()) {
				static_cast<void>(drawn);
				++count;
			}
			return count;
		}

		ecs::EntityHandle entityOfMesh(const size_t meshIndex) {
			const uint64_t meshId = renderer.createdMeshes.at(meshIndex).id;

			for (auto&& [entity, mesh]: registry.query<const ecs::component::Mesh>()) {
				if (mesh.mesh.id == meshId)
					return registry.getEntity(entity);
			}
			return registry.getEntity(ecs::nullEntity);
		}
	};

	SpawnedPlanet& sharedSpawnedPlanet() {
		static SpawnedPlanet instance;
		return instance;
	}
}

SCENARIO("Starting a planet loads every chunk around spawn", "[chunk-manager]") {
	GIVEN("a planet whose chunk manager has just been created") {
		SpawnedPlanet& env = sharedSpawnedPlanet();

		THEN("exactly one texture is created, from the dirt block's texture") {
			REQUIRE(env.renderer.createdTextures.size() == 1);
			REQUIRE(env.renderer.createdTexturePixels.front() == env.dirtPixels);
		}
		AND_THEN("every chunk in the render distance with visible blocks gets one mesh, and no more") {
			REQUIRE(env.meshesAtSpawn > 0);
			REQUIRE(env.meshesAtSpawn <= static_cast<size_t>(chunksAroundSpawn));
			for (size_t i = 0; i < env.meshesAtSpawn; ++i)
				REQUIRE(env.renderer.createdMeshTriangleCounts[i] > 0);
		}
		AND_THEN("each chunk mesh becomes a textured entity drawn by the render system") {
			const auto* renderSystem = env.registry.getSystemManager().getSystem<ecs::RenderSystem>();

			for (size_t i = 0; i < env.meshesAtSpawn; ++i) {
				ecs::EntityHandle           entity = env.entityOfMesh(i);
				const ecs::component::Mesh* mesh   = entity.tryGet<ecs::component::Mesh>();

				REQUIRE(mesh != nullptr);
				REQUIRE(mesh->mesh.id == env.renderer.createdMeshes[i].id);
				REQUIRE(mesh->pipelineType == assets::PipelineType::Chunk);
				REQUIRE(entity.get<ecs::component::Texture>().texture.id == env.renderer.createdTextures.front().id);
				REQUIRE(entity.has<ecs::component::Transform>());
				REQUIRE(renderSystem->processes(entity.id()));
			}
		}
		AND_THEN("every chunk entity sits on the 16-block chunk grid, inside the render distance") {
			const glm::ivec3 chunkSize(chunkXSize, chunkYSize, chunkZSize);
			const int        halfDistance = spawnRenderDistance.x / 2;

			for (size_t i = 0; i < env.meshesAtSpawn; ++i) {
				const glm::vec3  position = env.entityOfMesh(i).get<ecs::component::Transform>().position;
				const glm::ivec3 chunk    = glm::ivec3(position) / chunkSize;

				REQUIRE(glm::vec3(chunk * chunkSize) == position);
				REQUIRE(std::abs(chunk.x) <= halfDistance);
				REQUIRE(std::abs(chunk.z) <= halfDistance);
				REQUIRE(chunk.y >= 0);
				REQUIRE(std::cmp_less(chunk.y, spawnRenderDistance.y));
			}
		}
	}
}

SCENARIO("Loading a chunk makes it visible", "[chunk-manager]") {
	GIVEN("a planet with its spawn area loaded") {
		SpawnedPlanet& env          = sharedSpawnedPlanet();
		const size_t   meshesBefore = env.meshCount();

		WHEN("an underground chunk (y = -1) is loaded") {
			env.manager->loadChunk(glm::ivec3(40, -1, -40));

			THEN("it gets exactly one new mesh: only the top of a solid chunk, as no neighbour is loaded") {
				REQUIRE(env.meshCount() == meshesBefore + 1);
				REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop);
			}
			AND_THEN("its entity is placed at that chunk's position on the planet") {
				const glm::vec3 position = env.entityOfMesh(meshesBefore).get<ecs::component::Transform>().position;
				REQUIRE(position == glm::vec3(40 * chunkXSize, -1 * chunkYSize, -40 * chunkZSize));
			}
		}

		WHEN("a chunk above the maximum terrain height is loaded") {
			env.manager->loadChunk(0, (planetinfo::terrainMaxHeightBlocks / chunkYSize) + 5, 0);

			THEN("it is empty, so no mesh or entity is created") {
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}

		WHEN("a 2 x 1 x 3 range of underground chunks is loaded") {
			env.manager->loadRange({50, -2, 50}, {51, -2, 52});

			THEN("every chunk in the range, both ends included, gets a mesh") {
				REQUIRE(env.meshCount() == meshesBefore + (2uz * 1 * 3));
			}
		}

		WHEN("an already visible chunk is made renderable again") {
			env.manager->loadChunk(glm::ivec3(45, -1, 45));
			env.manager->makeChunkRenderable(env.registry, env.renderer, {45, -1, 45});

			THEN("it is not duplicated: it still has exactly one mesh") {
				REQUIRE(env.meshCount() == meshesBefore + 1);
			}
		}
	}
}

SCENARIO("Unloading a chunk forgets it", "[chunk-manager]") {
	GIVEN("a planet with its spawn area loaded") {
		SpawnedPlanet& env = sharedSpawnedPlanet();

		WHEN("a loaded chunk is unloaded") {
			env.manager->loadChunk(60, -1, 60);
			env.manager->unloadChunk(60, -1, 60);
			const size_t meshesBefore = env.meshCount();

			THEN("it can no longer be made renderable") {
				env.manager->makeChunkRenderable(env.registry, env.renderer, {60, -1, 60});
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}

		WHEN("a visible chunk is unloaded") {
			const size_t meshIndex       = env.meshCount();
			const size_t destroyedBefore = env.renderer.destroyedMeshes.size();
			env.manager->loadChunk(65, -1, 65);
			ecs::EntityHandle const chunkEntity = env.entityOfMesh(meshIndex);
			REQUIRE(chunkEntity.has<ecs::component::Mesh>());

			env.manager->unloadChunk(65, -1, 65);

			THEN("it disappears from the planet: it is no longer drawn") {
				const auto* renderSystem = env.registry.getSystemManager().getSystem<ecs::RenderSystem>();

				REQUIRE_FALSE(renderSystem->processes(chunkEntity.id()));
				REQUIRE_FALSE(chunkEntity.has<ecs::component::Mesh>());
			}
			AND_THEN("its mesh is handed back to the renderer so the GPU memory can be reused") {
				REQUIRE(env.renderer.destroyedMeshes.size() == destroyedBefore + 1);
				REQUIRE(env.renderer.destroyedMeshes.back().id == env.renderer.createdMeshes.at(meshIndex).id);
			}
		}

		WHEN("a range of loaded chunks is unloaded") {
			env.manager->loadRange({70, -1, 70}, {71, -1, 71});
			env.manager->unloadRange({70, -1, 70}, {71, -1, 71});
			const size_t meshesBefore = env.meshCount();

			THEN("none of them can be made renderable any more") {
				for (int x = 70; x <= 71; ++x)
					for (int z = 70; z <= 71; ++z)
						env.manager->makeChunkRenderable(env.registry, env.renderer, {x, -1, z});
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

SCENARIO("The chunk manager reports how many chunks are loaded", "[chunk-manager][stats]") {
	GIVEN("a planet with its spawn area loaded") {
		SpawnedPlanet env;

		THEN("the loaded chunk count matches the spawn area") {
			REQUIRE(env.stats.loadedChunks == static_cast<size_t>(chunksAroundSpawn));
		}
		AND_THEN("the chunk data is made of whole chunks, and chunks of only air take none") {
			constexpr size_t bytesPerChunk = game::planet::chunkVolume * sizeof(game::Block);

			REQUIRE(env.stats.chunkDataBytes > 0);
			REQUIRE(env.stats.chunkDataBytes % bytesPerChunk == 0);
			REQUIRE(env.stats.chunkDataBytes < env.stats.loadedChunks * bytesPerChunk);
		}
		AND_THEN("nothing is waiting to be generated, because generation is immediate") {
			REQUIRE(env.stats.pendingGeneration == 0);
		}

		WHEN("another chunk is loaded") {
			const size_t chunksBefore = env.stats.loadedChunks;
			env.manager->loadChunk(glm::ivec3(60, -1, 60));

			THEN("the count grows by one") {
				REQUIRE(env.stats.loadedChunks == chunksBefore + 1);
			}

			AND_WHEN("it is unloaded again") {
				env.manager->unloadChunk(glm::ivec3(60, -1, 60));

				THEN("the count drops back") {
					REQUIRE(env.stats.loadedChunks == chunksBefore);
				}
			}
		}
	}
}

SCENARIO("Chunks that hold only air are never meshed and store no blocks", "[chunk-manager][skip]") {
	GIVEN("a planet with its spawn area loaded") {
		const SpawnedPlanet env;
		const size_t        meshesBefore = env.meshCount();
		const size_t        bytesBefore  = env.stats.chunkDataBytes;
		const size_t        chunksBefore = env.stats.loadedChunks;

		WHEN("a chunk high above the terrain is loaded") {
			env.manager->loadChunk(glm::ivec3(0, 10, 0));

			THEN("it counts as loaded") {
				REQUIRE(env.stats.loadedChunks == chunksBefore + 1);
			}
			AND_THEN("it stores no block data") {
				REQUIRE(env.stats.chunkDataBytes == bytesBefore);
			}
			AND_THEN("no mesh is made for it") {
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}
	}
}

SCENARIO("A chunk buried inside solid terrain is not drawn", "[chunk-manager][skip]") {
	constexpr glm::ivec3 center(100, -2, 100);

	GIVEN("a planet with its spawn area loaded") {
		SpawnedPlanet env;
		const size_t  drawnBefore = env.drawnChunkCount();

		WHEN("a 3 x 3 x 3 block of underground chunks is loaded together") {
			env.manager->loadRange(center - 1, center + 1);

			THEN("only the nine chunks on top are drawn, with the tops that have no chunk above them") {
				REQUIRE(env.drawnChunkCount() == drawnBefore + 9);
			}
		}

		WHEN("the same chunks are loaded but the one above the middle chunk is missing") {
			env.manager->loadRange(center - 1, center + 1);
			env.manager->unloadChunk(center + glm::ivec3(0, 1, 0));

			THEN("the middle chunk is drawn, because its top is exposed") {
				REQUIRE(env.drawnChunkCount() == drawnBefore + 8 + 1);
			}
		}
	}
}

SCENARIO("Loading and unloading chunks updates the faces on their borders", "[chunk-manager][border]") {
	constexpr glm::ivec3 lower(300, -3, 300);
	constexpr glm::ivec3 upper = lower + glm::ivec3(0, 1, 0);

	GIVEN("a planet with its spawn area loaded") {
		SpawnedPlanet env;
		const size_t  drawnBefore = env.drawnChunkCount();

		WHEN("a solid chunk is loaded without neighbours") {
			env.manager->loadChunk(lower);

			THEN("only its top is drawn") {
				REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop);
				REQUIRE(env.drawnChunkCount() == drawnBefore + 1);
			}

			AND_WHEN("a solid chunk is loaded on top of it") {
				env.manager->loadChunk(upper);

				THEN("the lower chunk is meshed again: its top is covered, so nothing of it is drawn") {
					REQUIRE(env.drawnChunkCount() == drawnBefore + 1);
				}
				AND_THEN("the upper chunk draws its own top") {
					REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop);
				}

				AND_WHEN("the upper chunk is unloaded again") {
					env.manager->unloadChunk(upper);

					THEN("the lower chunk shows its top again") {
						REQUIRE(env.drawnChunkCount() == drawnBefore + 1);
						REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop);
					}
				}
			}
		}

		WHEN("two solid chunks side by side are loaded together") {
			env.manager->loadRange(lower, lower + glm::ivec3(1, 0, 0));

			THEN("each draws only its top: the sides facing each other are hidden and the others have no neighbour") {
				const auto& counts = env.renderer.createdMeshTriangleCounts;
				REQUIRE(counts[counts.size() - 1] == trianglesOnTop);
				REQUIRE(counts[counts.size() - 2] == trianglesOnTop);
			}
		}
	}
}

SCENARIO("A chunk is meshed again when a block next to its border changes", "[chunk-manager][border]") {
	constexpr glm::ivec3 left(310, -1, 310);
	constexpr glm::ivec3 right = left + glm::ivec3(1, 0, 0);

	GIVEN("two solid chunks side by side") {
		SpawnedPlanet env;
		env.manager->loadRange(left, right);
		const size_t meshesBefore = env.meshCount();

		WHEN("a block on the border of the right chunk is removed") {
			env.manager->setBlock(right, {0, 5, 5}, game::Block());

			THEN("both chunks are meshed again") {
				REQUIRE(env.meshCount() == meshesBefore + 2);
			}
			AND_THEN("the left chunk now shows the one face that is no longer covered, next to its top") {
				REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop + 2);
			}
		}

		WHEN("a block in the middle of the right chunk is removed") {
			env.manager->setBlock(right, {8, 8, 8}, game::Block());

			THEN("only the right chunk is meshed again") {
				REQUIRE(env.meshCount() == meshesBefore + 1);
			}
		}
	}
}
