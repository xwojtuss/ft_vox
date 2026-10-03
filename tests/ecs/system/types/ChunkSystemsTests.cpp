#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <thread>

#include "concurrency/IThreadPool.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/system/types/ChunkRenderSystem.hpp"
#include "ecs/system/types/ChunkStreamSystem.hpp"
#include "scene/PlanetInfo.hpp"
#include "support/Blocks.hpp"
#include "support/FakeRenderer.hpp"
#include "support/ManualThreadPool.hpp"
#include "support/TestSeed.hpp"

namespace planetinfo = scene::planetinfo;

namespace {
	constexpr glm::vec<3, unsigned short> spawnRenderDistance = {2, planetinfo::maxVerticalRenderDistance, 2};

	constexpr int chunksAroundSpawn = (spawnRenderDistance.x + 1) * (spawnRenderDistance.z + 1) * spawnRenderDistance.y;

	constexpr render::chunks::ChunkRenderSettings noUploadLimit = {.meshUploadsPerFrame = 100000};

	struct WorldEnv {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ecs::Registry           registry{blockDatas};
		test::FakeRenderer      renderer;
		profiling::ServerStats  serverStats;
		profiling::ClientStats  clientStats;

		void addSystems(concurrency::IThreadPool& pool) {
			registry.createSystem<ecs::ChunkStreamSystem>(registry, pool, serverStats, test::seed,
														  game::planet::ChunkStreamSettings{spawnRenderDistance});
			registry.createSystem<ecs::ChunkRenderSystem>(registry, renderer, pool, clientStats, noUploadLimit);
		}

		void frame() {
			registry.getSystemManager().onSimulate(0.016f, 0.0f);
			registry.getSystemManager().onRender(1.0f, 0.0);
		}

		[[nodiscard]] bool isSettled(const concurrency::IThreadPool& pool) const {
			return serverStats.loadedChunks == static_cast<size_t>(chunksAroundSpawn) &&
				   serverStats.pendingGeneration == 0 && clientStats.pendingMeshing == 0 &&
				   pool.unfinishedTaskCount() == 0;
		}

		[[nodiscard]] size_t drawnChunkCount() {
			size_t count = 0;
			for (auto&& drawn: registry.query<const ecs::component::Mesh>()) {
				static_cast<void>(drawn);
				++count;
			}
			return count;
		}
	};
}

SCENARIO("The streaming and rendering systems build the world around spawn together", "[ecs][chunk-systems]") {
	GIVEN("both systems on one pool, before the registry is ready") {
		test::ManualThreadPool pool;
		WorldEnv               env;
		env.addSystems(pool);

		THEN("nothing is requested yet") {
			REQUIRE(pool.pending() == 0);
		}

		WHEN("the registry becomes ready") {
			env.registry.getSystemManager().onRegistryReady();

			THEN("the area around spawn is queued") {
				REQUIRE(pool.pending() == static_cast<size_t>(chunksAroundSpawn));
			}

			AND_WHEN("frames pass until the pool has nothing left to do") {
				constexpr int maxFrames = 100;
				for (int frame = 0; frame < maxFrames && !env.isSettled(pool); ++frame) {
					pool.runAll();
					env.frame();
				}

				THEN("every chunk is loaded and the chunks with visible blocks are drawn") {
					REQUIRE(env.isSettled(pool));
					REQUIRE(env.drawnChunkCount() > 0);
					REQUIRE(env.drawnChunkCount() <= static_cast<size_t>(chunksAroundSpawn));
				}
			}
		}
	}
}

SCENARIO("Worker threads build the same world as running the jobs one by one", "[ecs][chunk-systems][threads]") {
	GIVEN("the world built with jobs run one by one") {
		test::ManualThreadPool pool;
		WorldEnv               reference;
		reference.addSystems(pool);
		reference.registry.getSystemManager().onRegistryReady();
		for (int frame = 0; frame < 100 && !reference.isSettled(pool); ++frame) {
			pool.runAll();
			reference.frame();
		}
		REQUIRE(reference.isSettled(pool));

		WHEN("the same world is built by a pool of four worker threads") {
			const auto pool4 = concurrency::createThreadPool(4);
			WorldEnv   threaded;
			threaded.addSystems(*pool4);
			threaded.registry.getSystemManager().onRegistryReady();

			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
			while (!threaded.isSettled(*pool4) && std::chrono::steady_clock::now() < deadline) {
				threaded.frame();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			REQUIRE(threaded.isSettled(*pool4));

			THEN("the same chunks are loaded and the same meshes are built") {
				REQUIRE(threaded.serverStats.chunkDataBytes == reference.serverStats.chunkDataBytes);
				REQUIRE(threaded.drawnChunkCount() == reference.drawnChunkCount());

				auto threadedCounts  = threaded.renderer.createdMeshTriangleCounts;
				auto referenceCounts = reference.renderer.createdMeshTriangleCounts;
				std::ranges::sort(threadedCounts);
				std::ranges::sort(referenceCounts);
				REQUIRE(threadedCounts == referenceCounts);
			}
		}
	}
}
