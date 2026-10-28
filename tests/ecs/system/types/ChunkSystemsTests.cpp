#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <thread>

#include "concurrency/IThreadPool.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/system/types/ChunkRenderSystem.hpp"
#include "ecs/system/types/ChunkStreamSystem.hpp"
#include "support/Blocks.hpp"
#include "support/FakeRenderer.hpp"
#include "support/ManualThreadPool.hpp"
#include "support/RenderArea.hpp"
#include "support/TestSeed.hpp"

namespace {
	constexpr glm::vec<3, unsigned short> spawnRenderDistance = {2, 4, 2};

	const size_t chunksAroundSpawn = test::renderAreaSize(spawnRenderDistance);

	constexpr render::chunks::ChunkRenderSettings noUploadLimit = {.meshUploadsPerFrame = 100000};

	struct WorldEnv {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ecs::Registry           registry{blockDatas};
		test::FakeRenderer      renderer;
		profiling::ServerStats  serverStats;
		profiling::ClientStats  clientStats;

		void addSystems(concurrency::IThreadPool& pool) {
			registry.createSystem<ecs::ChunkStreamSystem>(
				registry, pool, serverStats, test::seed,
				game::planet::ChunkStreamSettings{.renderDistance = spawnRenderDistance});
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

SCENARIO("The streaming and rendering systems build the world around the player together", "[ecs][chunk-systems]") {
	GIVEN("both systems on one pool and a registry without any player") {
		test::ManualThreadPool pool;
		WorldEnv               env;
		env.addSystems(pool);

		THEN("nothing is requested before the first simulation step") {
			env.registry.getSystemManager().onRegistryReady();
			REQUIRE(pool.pending() == 0);
		}

		WHEN("frames pass until the pool has nothing left to do") {
			constexpr int maxFrames = 100;
			for (int frame = 0; frame < maxFrames && !env.isSettled(pool); ++frame) {
				env.frame();
				pool.runAll();
			}

			THEN("every chunk around the origin is loaded and the chunks with visible blocks are drawn") {
				REQUIRE(env.isSettled(pool));
				REQUIRE(env.drawnChunkCount() > 0);
				REQUIRE(env.drawnChunkCount() <= static_cast<size_t>(chunksAroundSpawn));
			}
		}
	}
}

SCENARIO("The world is built around the player and grows as they walk", "[ecs][chunk-systems]") {
	GIVEN("both systems and a player standing in chunk (10, 0, 0)") {
		test::ManualThreadPool pool;
		WorldEnv               env;
		env.addSystems(pool);

		ecs::EntityHandle player = env.registry.createEntity();
		player.add(ecs::component::Transform{.position = glm::vec3((10 * 16) + 8, 8.0f, 8.0f)});
		player.add(ecs::component::Camera{});

		const auto settle = [&] {
			constexpr int maxFrames = 100;
			for (int frame = 0; frame < maxFrames; ++frame) {
				env.frame();
				pool.runAll();
				if (env.serverStats.pendingGeneration == 0 && env.clientStats.pendingMeshing == 0 &&
					pool.unfinishedTaskCount() == 0)
					return;
			}
		};

		WHEN("the world has been built") {
			settle();

			THEN("only chunks around the player were generated") {
				REQUIRE(env.serverStats.loadedChunks == static_cast<size_t>(chunksAroundSpawn));
				REQUIRE(env.drawnChunkCount() > 0);
				for (auto&& [entity, transform]: env.registry.query<const ecs::component::Transform>()) {
					if (entity == player.id())
						continue;
					REQUIRE(transform.position.x >= 9 * 16);
					REQUIRE(transform.position.x <= 11 * 16);
				}
			}

			AND_WHEN("the player walks ten chunks further") {
				player.get<ecs::component::Transform>().position.x += 10 * 16;
				settle();

				THEN("the chunks around the new place are generated and the old ones are gone") {
					REQUIRE(env.serverStats.loadedChunks == static_cast<size_t>(chunksAroundSpawn));
					for (auto&& [entity, transform]: env.registry.query<const ecs::component::Transform>()) {
						if (entity != player.id())
							REQUIRE(transform.position.x >= 19 * 16);
					}
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
		for (int frame = 0; frame < 100 && !reference.isSettled(pool); ++frame) {
			reference.frame();
			pool.runAll();
		}
		REQUIRE(reference.isSettled(pool));

		WHEN("the same world is built by a pool of four worker threads") {
			const auto pool4 = concurrency::createThreadPool(4);
			WorldEnv   threaded;
			threaded.addSystems(*pool4);

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
