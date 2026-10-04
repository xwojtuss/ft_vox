#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <thread>
#include <tuple>
#include <vector>

#include "concurrency/IThreadPool.hpp"
#include "game/planet/ChunkOrder.hpp"
#include "game/planet/ChunkStreamer.hpp"
#include "support/Blocks.hpp"
#include "support/ChunkEventRecorder.hpp"
#include "support/ManualThreadPool.hpp"
#include "support/RenderArea.hpp"
#include "support/TestSeed.hpp"

using game::planet::chunkOrderCost;
using game::planet::ChunkStreamer;
using game::planet::ChunkStreamSettings;
using game::planet::RenderDistance;
using game::planet::Viewer;

namespace {
	constexpr RenderDistance noStreaming         = {0, 0, 0};
	constexpr RenderDistance spawnRenderDistance = {2, 4, 2};
	constexpr RenderDistance wideRenderDistance  = {6, 2, 6};

	const size_t chunksAroundSpawn = test::renderAreaSize(spawnRenderDistance);
	const size_t chunksInWideArea  = test::renderAreaSize(wideRenderDistance);

	const Viewer atSpawnLookingNorth = {.position = {8.0f, 8.0f, 8.0f}, .forward = {0.0f, 0.0f, -1.0f}};
	const Viewer atSpawnLookingSouth = {.position = {8.0f, 8.0f, 8.0f}, .forward = {0.0f, 0.0f, 1.0f}};

	struct StreamerEnv {
		ecs::Dispatcher          dispatcher;
		test::ChunkEventRecorder recorder;
		test::ManualThreadPool   pool;
		profiling::ServerStats   stats;
		ChunkStreamer            streamer;

		explicit StreamerEnv(const RenderDistance renderDistance      = noStreaming,
							 const std::size_t    queuedJobsPerWorker = game::planet::defaultQueuedJobsPerWorker) :
			streamer(
				dispatcher, pool, stats, test::seed,
				ChunkStreamSettings{.renderDistance = renderDistance, .queuedJobsPerWorker = queuedJobsPerWorker}) {
			recorder.listenTo(dispatcher);
			streamer.setViewer(atSpawnLookingNorth);
		}

		void settle() {
			constexpr int maxRounds = 200;
			for (int round = 0; round < maxRounds; ++round) {
				streamer.update();
				if (streamer.isIdle() && pool.pending() == 0)
					return;
				pool.runAll();
			}
		}

		void generateOneAtATime(const size_t count) {
			for (size_t i = 0; i < count; ++i) {
				streamer.update();
				pool.runNext();
			}
			streamer.update();
		}

		[[nodiscard]] std::vector<glm::ivec3> loadedOrder() const {
			std::vector<glm::ivec3> order;
			order.reserve(recorder.loaded.size());
			for (const auto& [position, chunk]: recorder.loaded)
				order.push_back(position);
			return order;
		}
	};

	size_t indexOf(const std::vector<glm::ivec3>& order, const glm::ivec3& chunk) {
		return static_cast<size_t>(std::ranges::find(order, chunk) - order.begin());
	}

	bool isOrderedBy(const std::vector<glm::ivec3>& order, const Viewer& viewer) {
		return std::ranges::is_sorted(order, {},
									  [&viewer](const glm::ivec3& chunk) { return chunkOrderCost(chunk, viewer); });
	}
}

SCENARIO("The chunks around the player are announced at once and delivered later", "[chunk-streamer]") {
	constexpr size_t poolLimit = 8;

	GIVEN("a streamer whose player stands in the middle of the first chunk") {
		StreamerEnv env(spawnRenderDistance, poolLimit);

		WHEN("the first update runs") {
			env.streamer.update();

			THEN("every chunk of the area around the player is announced, and none has been delivered") {
				REQUIRE(env.recorder.requested.size() == static_cast<size_t>(chunksAroundSpawn));
				REQUIRE(env.recorder.loaded.empty());
				REQUIRE(env.stats.loadedChunks == 0);
			}
			AND_THEN("only a limited number of generation jobs is handed to the pool, the rest waits") {
				REQUIRE(env.pool.pending() == poolLimit);
				REQUIRE(env.stats.pendingGeneration == static_cast<size_t>(chunksAroundSpawn));
			}
		}

		WHEN("the pool finishes one job and the world is updated") {
			env.streamer.update();
			env.pool.runNext();
			env.streamer.update();

			THEN("exactly one chunk is delivered, the one the player stands in") {
				REQUIRE(env.recorder.loaded.size() == 1);
				REQUIRE(env.recorder.loaded.front().first == glm::ivec3(0, 0, 0));
				REQUIRE(env.stats.loadedChunks == 1);
			}
			AND_THEN("the free place in the pool is filled with the next chunk") {
				REQUIRE(env.pool.pending() == poolLimit);
			}
		}

		WHEN("all the work is done") {
			env.settle();

			THEN("every chunk was delivered once and nothing is waiting") {
				REQUIRE(env.recorder.loaded.size() == static_cast<size_t>(chunksAroundSpawn));
				REQUIRE(env.stats.loadedChunks == static_cast<size_t>(chunksAroundSpawn));
				REQUIRE(env.stats.pendingGeneration == 0);
				REQUIRE(env.streamer.isIdle());
			}
		}
	}

	GIVEN("a streamer whose area reaches high above the terrain") {
		constexpr RenderDistance tall = {2, 12, 2};
		StreamerEnv              env(tall);

		WHEN("all the work is done") {
			env.settle();

			THEN("chunks that hold only air take no memory") {
				constexpr size_t bytesPerChunk = game::planet::chunkVolume * sizeof(game::Block);

				REQUIRE(env.stats.chunkDataBytes > 0);
				REQUIRE(env.stats.chunkDataBytes % bytesPerChunk == 0);
				REQUIRE(env.stats.chunkDataBytes < env.stats.loadedChunks * bytesPerChunk);
			}
		}
	}
}

SCENARIO("A player who looks down or up gets the chunks in that direction first", "[chunk-streamer][order]") {
	GIVEN("a streamer that hands one job at a time to the pool, with the player looking straight down") {
		StreamerEnv env(wideRenderDistance, 1);
		env.streamer.setViewer({.position = atSpawnLookingNorth.position, .forward = {0.0f, -1.0f, 0.0f}});
		env.generateOneAtATime(chunksInWideArea);
		const std::vector<glm::ivec3> order = env.loadedOrder();

		THEN("the chunk below was generated before the equally distant chunk above") {
			REQUIRE(indexOf(order, {0, -1, 0}) < indexOf(order, {0, 1, 0}));
		}
	}

	GIVEN("a streamer with the player looking straight up") {
		StreamerEnv env(wideRenderDistance, 1);
		env.streamer.setViewer({.position = atSpawnLookingNorth.position, .forward = {0.0f, 1.0f, 0.0f}});
		env.generateOneAtATime(chunksInWideArea);
		const std::vector<glm::ivec3> order = env.loadedOrder();

		THEN("the chunk above was generated before the equally distant chunk below") {
			REQUIRE(indexOf(order, {0, 1, 0}) < indexOf(order, {0, -1, 0}));
		}
	}
}

SCENARIO("A player who stands still gets the nearest chunks first, and those in front before those behind",
		 "[chunk-streamer][order]") {
	GIVEN("a streamer that hands one job at a time to the pool, with the player looking towards negative z") {
		StreamerEnv env(wideRenderDistance, 1);

		WHEN("all chunks around the player are generated one by one") {
			env.generateOneAtATime(chunksInWideArea);
			const std::vector<glm::ivec3> order = env.loadedOrder();

			THEN("every chunk of the area was generated") {
				REQUIRE(order.size() == static_cast<size_t>(chunksInWideArea));
			}
			AND_THEN("the chunk the player stands in came first") {
				REQUIRE(order.front() == glm::ivec3(0, 0, 0));
			}
			AND_THEN("they came in order of how soon the player needs them") {
				REQUIRE(isOrderedBy(order, atSpawnLookingNorth));
			}
			AND_THEN("a chunk in front of the player came before the equally distant chunk behind") {
				REQUIRE(indexOf(order, {0, 0, -3}) < indexOf(order, {0, 0, 3}));
				REQUIRE(indexOf(order, {-2, 0, -2}) < indexOf(order, {-2, 0, 2}));
			}
		}
	}
}

SCENARIO("The order follows a player who turns around", "[chunk-streamer][order]") {
	GIVEN("a streamer that hands one job at a time to the pool, after ten chunks were generated") {
		StreamerEnv env(wideRenderDistance, 1);
		env.generateOneAtATime(10);
		const size_t generatedBeforeTurning = env.recorder.loaded.size();
		REQUIRE(generatedBeforeTurning == 10);
		REQUIRE_FALSE(env.streamer.isLoaded({0, 0, 3}));
		REQUIRE_FALSE(env.streamer.isLoaded({0, 0, -3}));

		WHEN("the player turns around and the rest is generated") {
			env.streamer.setViewer(atSpawnLookingSouth);
			env.generateOneAtATime(chunksInWideArea);
			const std::vector<glm::ivec3> order = env.loadedOrder();
			const std::vector<glm::ivec3> afterTurning(
				order.begin() + static_cast<std::ptrdiff_t>(generatedBeforeTurning) + 1, order.end());

			THEN("the chunk that was behind the player now comes before the one in front") {
				REQUIRE(indexOf(order, {0, 0, 3}) < indexOf(order, {0, 0, -3}));
			}
			AND_THEN("the chunks generated after turning came in order of the new view, once the job already in the "
					 "pool was done") {
				REQUIRE(isOrderedBy(afterTurning, atSpawnLookingSouth));
			}
			AND_THEN("the chunks generated before turning were ordered for the old view") {
				const std::vector<glm::ivec3> beforeTurning(
					order.begin(), order.begin() + static_cast<std::ptrdiff_t>(generatedBeforeTurning));
				REQUIRE(isOrderedBy(beforeTurning, atSpawnLookingNorth));
			}
		}
	}
}

SCENARIO("Chunks are requested around the player's chunk as they move", "[chunk-streamer][order]") {
	GIVEN("a streamer that has generated the chunks around the first chunk") {
		StreamerEnv env(spawnRenderDistance);
		env.settle();
		REQUIRE(env.recorder.requested.size() == static_cast<size_t>(chunksAroundSpawn));

		WHEN("the player walks within the same chunk") {
			env.streamer.setViewer({.position = {12.0f, 8.0f, 3.0f}, .forward = {1.0f, 0.0f, 0.0f}});
			env.settle();

			THEN("nothing new is requested") {
				REQUIRE(env.recorder.requested.size() == static_cast<size_t>(chunksAroundSpawn));
			}
		}

		WHEN("the player walks into the next chunk") {
			env.streamer.setViewer({.position = {16.0f + 8.0f, 8.0f, 8.0f}, .forward = {1.0f, 0.0f, 0.0f}});
			env.streamer.update();

			THEN("only the chunks of the new area that were not in the old one are requested") {
				const test::ChunkSet wanted = test::unionOf(test::renderAreaAround({0, 0, 0}, spawnRenderDistance),
															test::renderAreaAround({1, 0, 0}, spawnRenderDistance));
				REQUIRE(env.recorder.requested.size() == wanted.size());
				REQUIRE(test::toSet(env.recorder.requested) == wanted);
				REQUIRE(wanted.size() > chunksAroundSpawn);
			}
			AND_THEN("they are generated and the old chunks are still there") {
				env.settle();
				REQUIRE(env.streamer.isLoaded({-1, 0, 0}));
				REQUIRE(env.streamer.isLoaded({2, 0, 0}));
				REQUIRE(env.stats.loadedChunks == env.recorder.requested.size());
				REQUIRE(env.recorder.unloaded.empty());
			}
		}

		WHEN("the player climbs one chunk") {
			env.streamer.setViewer({.position = {8.0f, 16.0f + 8.0f, 8.0f}, .forward = {0.0f, 0.0f, -1.0f}});
			env.streamer.update();

			THEN("only the chunks of the new area that were not in the old one are requested") {
				const test::ChunkSet wanted = test::unionOf(test::renderAreaAround({0, 0, 0}, spawnRenderDistance),
															test::renderAreaAround({0, 1, 0}, spawnRenderDistance));
				REQUIRE(test::toSet(env.recorder.requested) == wanted);
				REQUIRE(env.recorder.requested.size() == wanted.size());
			}
			AND_THEN("the chunks above are generated and nothing is unloaded") {
				env.settle();
				REQUIRE(env.streamer.isLoaded({0, 3, 0}));
				REQUIRE(env.streamer.isLoaded({0, -2, 0}));
				REQUIRE(env.recorder.unloaded.empty());
			}
		}

		WHEN("the player digs down below the first chunk") {
			env.streamer.setViewer({.position = {8.0f, (-16.0f * 5) + 8.0f, 8.0f}, .forward = {0.0f, -1.0f, 0.0f}});
			env.settle();

			THEN("chunks far below are generated too, as the range applies to every axis") {
				REQUIRE(env.streamer.isLoaded({0, -5, 0}));
				REQUIRE(env.streamer.isLoaded({0, -7, 0}));
				REQUIRE_FALSE(env.streamer.isLoaded({0, -8, 0}));
			}
		}

		WHEN("the player walks far away") {
			env.streamer.setViewer({.position = {(16.0f * 20) + 8.0f, 8.0f, 8.0f}, .forward = {0.0f, 0.0f, -1.0f}});
			env.settle();

			THEN("a whole new area is generated around them") {
				REQUIRE(env.streamer.isLoaded({20, 0, 0}));
				REQUIRE(env.stats.loadedChunks == chunksAroundSpawn);
			}
			AND_THEN("every chunk of the old area is unloaded") {
				REQUIRE_FALSE(env.streamer.isLoaded({0, 0, 0}));
				REQUIRE(test::toSet(env.recorder.unloaded) == test::renderAreaAround({0, 0, 0}, spawnRenderDistance));
			}
		}
	}
}

SCENARIO("Chunks the player left behind are unloaded, with some margin", "[chunk-streamer][unload]") {
	GIVEN("a streamer that has generated the chunks around the first chunk") {
		StreamerEnv env(spawnRenderDistance);
		env.settle();

		WHEN("the player walks one chunk away") {
			env.streamer.setViewer({.position = {16.0f + 8.0f, 8.0f, 8.0f}, .forward = {1.0f, 0.0f, 0.0f}});
			env.settle();

			THEN("chunks just outside the render distance are kept, so walking back and forth does not reload them") {
				REQUIRE(env.streamer.isLoaded({-1, 0, 0}));
				REQUIRE(env.recorder.unloaded.empty());
			}
		}

		WHEN("the player walks three chunks away") {
			env.streamer.setViewer({.position = {(16.0f * 3) + 8.0f, 8.0f, 8.0f}, .forward = {1.0f, 0.0f, 0.0f}});
			env.settle();

			THEN("chunks beyond the margin are unloaded and announced") {
				REQUIRE_FALSE(env.streamer.isLoaded({-1, 0, 0}));
				REQUIRE(std::ranges::find(env.recorder.unloaded, glm::ivec3(-1, 0, 0)) != env.recorder.unloaded.end());
			}
			AND_THEN("the chunks around the player stay loaded") {
				REQUIRE(env.streamer.isLoaded({3, 0, 0}));
				REQUIRE(env.streamer.isLoaded({2, 0, 0}));
			}
			AND_THEN("they no longer count as loaded") {
				REQUIRE(env.stats.loadedChunks == env.recorder.requested.size() - env.recorder.unloaded.size());
			}
		}

		WHEN("the player walks one chunk away and back") {
			env.streamer.setViewer({.position = {16.0f + 8.0f, 8.0f, 8.0f}, .forward = {1.0f, 0.0f, 0.0f}});
			env.settle();
			env.streamer.setViewer(atSpawnLookingNorth);
			env.settle();

			THEN("nothing is unloaded and the chunks of the first area are not requested again") {
				REQUIRE(env.recorder.unloaded.empty());
				const test::ChunkSet bothAreas = test::unionOf(test::renderAreaAround({0, 0, 0}, spawnRenderDistance),
															   test::renderAreaAround({1, 0, 0}, spawnRenderDistance));
				REQUIRE(env.recorder.requested.size() == bothAreas.size());
			}
		}
	}

	GIVEN("a streamer that still waits for most of the chunks around the player") {
		StreamerEnv env(spawnRenderDistance, 1);
		env.streamer.update();
		REQUIRE(env.stats.pendingGeneration == chunksAroundSpawn);

		WHEN("the player walks far away before they are generated") {
			env.streamer.setViewer({.position = {(16.0f * 20) + 8.0f, 8.0f, 8.0f}, .forward = {0.0f, 0.0f, -1.0f}});
			env.settle();

			THEN("the chunks that were never generated are unloaded too and never arrive") {
				REQUIRE(env.recorder.unloaded.size() == chunksAroundSpawn);
				for (const auto& [position, chunk]: env.recorder.loaded)
					REQUIRE(position.x >= 18);
			}
		}
	}

	GIVEN("a streamer that does not stream around the player") {
		StreamerEnv env;
		env.streamer.requestChunk({40, -1, 40});
		env.settle();

		WHEN("the player moves to another chunk") {
			env.streamer.setViewer({.position = {16.0f * 5, 8.0f, 8.0f}, .forward = {1.0f, 0.0f, 0.0f}});
			env.settle();

			THEN("the chunk that was requested by hand stays loaded") {
				REQUIRE(env.streamer.isLoaded({40, -1, 40}));
				REQUIRE(env.recorder.unloaded.empty());
			}
		}
	}
}

SCENARIO("What is generated is what the terrain makes there", "[chunk-streamer]") {
	GIVEN("one chunk below the planet's floor and one far above the terrain") {
		StreamerEnv env;
		env.streamer.requestChunk({40, -1, 40});
		env.streamer.requestChunk({40, 12, 40});
		env.settle();

		THEN("the underground chunk is completely solid") {
			REQUIRE(env.recorder.loaded.size() == 2);
			const auto underground = std::ranges::find(env.recorder.loaded, glm::ivec3(40, -1, 40),
													   &std::pair<glm::ivec3, game::planet::Chunk>::first);
			REQUIRE(underground != env.recorder.loaded.end());
			REQUIRE(underground->second.isFull());
		}
		AND_THEN("the chunk above the terrain is empty and stores nothing") {
			const auto sky = std::ranges::find(env.recorder.loaded, glm::ivec3(40, 12, 40),
											   &std::pair<glm::ivec3, game::planet::Chunk>::first);
			REQUIRE(sky != env.recorder.loaded.end());
			REQUIRE(sky->second.isEmpty());
			REQUIRE(env.stats.chunkDataBytes == game::planet::chunkVolume * sizeof(game::Block));
		}
	}
}

SCENARIO("A chunk that is unloaded before it is generated never arrives", "[chunk-streamer]") {
	constexpr glm::ivec3 chunk(300, -1, 300);

	GIVEN("a chunk that was requested but not handed to the pool yet") {
		StreamerEnv env;
		env.streamer.requestChunk(chunk);

		WHEN("it is unloaded") {
			env.streamer.unloadChunk(chunk);
			env.settle();

			THEN("the unload is announced, the chunk never arrives, and nothing is left waiting") {
				REQUIRE(env.recorder.unloaded == std::vector{chunk});
				REQUIRE(env.recorder.loaded.empty());
				REQUIRE_FALSE(env.streamer.isLoaded(chunk));
				REQUIRE(env.streamer.isIdle());
			}
		}
	}

	GIVEN("a chunk whose generation job is in the pool but has not run yet") {
		StreamerEnv env;
		env.streamer.requestChunk(chunk);
		env.streamer.update();
		REQUIRE(env.pool.pending() == 1);

		WHEN("it is unloaded before the pool gets to it") {
			env.streamer.unloadChunk(chunk);
			env.settle();

			THEN("the chunk never arrives and the pool is free again") {
				REQUIRE(env.recorder.loaded.empty());
				REQUIRE_FALSE(env.streamer.isLoaded(chunk));
				REQUIRE(env.streamer.isIdle());
				REQUIRE(env.pool.pending() == 0);
			}
		}
	}

	GIVEN("a chunk that was generated") {
		StreamerEnv env;
		env.streamer.requestChunk(chunk);
		env.settle();
		REQUIRE(env.streamer.isLoaded(chunk));

		WHEN("it is unloaded") {
			env.streamer.unloadChunk(chunk);

			THEN("the unload is announced and the statistics drop") {
				REQUIRE(env.recorder.unloaded == std::vector{chunk});
				REQUIRE_FALSE(env.streamer.isLoaded(chunk));
				REQUIRE(env.stats.loadedChunks == 0);
				REQUIRE(env.stats.chunkDataBytes == 0);
			}
		}
	}

	GIVEN("a chunk that was never requested") {
		StreamerEnv env;

		THEN("unloading it announces nothing") {
			env.streamer.unloadChunk(chunk);
			REQUIRE(env.recorder.unloaded.empty());
		}
	}

	GIVEN("a range of loaded chunks") {
		StreamerEnv env;
		env.streamer.requestRange({70, -1, 70}, {71, -1, 71});
		env.settle();

		WHEN("the range is unloaded") {
			env.streamer.unloadRange({70, -1, 70}, {71, -1, 71});

			THEN("every chunk of it is announced as unloaded") {
				REQUIRE(env.recorder.unloaded.size() == 4);
				REQUIRE(env.stats.loadedChunks == 0);
			}
		}
	}
}

SCENARIO("Changing a block announces the changed chunk as a snapshot", "[chunk-streamer]") {
	constexpr glm::ivec3 chunk(310, -1, 310);

	GIVEN("a loaded solid chunk") {
		StreamerEnv env;
		env.streamer.requestChunk(chunk);
		env.settle();
		const size_t bytesBefore = env.stats.chunkDataBytes;

		WHEN("a block of it is removed") {
			env.streamer.setBlock(chunk, {5, 6, 7}, game::Block());

			THEN("the change is announced with the block's position and the chunk without that block") {
				REQUIRE(env.recorder.changed.size() == 1);
				REQUIRE(env.recorder.changed.front().position == chunk);
				REQUIRE(env.recorder.changed.front().blockPosition == glm::ivec3(5, 6, 7));
				REQUIRE(env.recorder.changed.front().chunk.getBlock(5, 6, 7).id == 0);
			}
			AND_THEN("the chunk that was announced when it loaded still has the block") {
				REQUIRE(env.recorder.loaded.front().second.getBlock(5, 6, 7).id != 0);
			}
			AND_THEN("the memory use is unchanged, as the chunk still holds blocks") {
				REQUIRE(env.stats.chunkDataBytes == bytesBefore);
			}
		}
	}
}

SCENARIO("The memory use of the loaded chunks follows what is loaded", "[chunk-streamer]") {
	constexpr size_t bytesPerChunk = game::planet::chunkVolume * sizeof(game::Block);

	GIVEN("a loaded chunk far above the terrain, which holds only air") {
		constexpr glm::ivec3 airChunk(310, 12, 310);
		StreamerEnv          env;
		env.streamer.requestChunk(airChunk);
		env.settle();
		REQUIRE(env.stats.chunkDataBytes == 0);

		WHEN("a block is placed in it") {
			env.streamer.setBlock(airChunk, {1, 1, 1}, game::Block(test::dirt));

			THEN("it takes the memory of one chunk") {
				REQUIRE(env.stats.chunkDataBytes == bytesPerChunk);
			}
		}
	}

	GIVEN("several loaded chunks that hold blocks") {
		StreamerEnv env;
		env.streamer.requestRange({70, -1, 70}, {71, -1, 71});
		env.settle();
		REQUIRE(env.stats.chunkDataBytes == 4 * bytesPerChunk);

		WHEN("one is unloaded") {
			env.streamer.unloadChunk({70, -1, 70});

			THEN("its memory is released") {
				REQUIRE(env.stats.chunkDataBytes == 3 * bytesPerChunk);
			}
		}

		WHEN("one is requested and generated again") {
			env.streamer.requestChunk({70, -1, 70});
			env.settle();

			THEN("it is not counted twice") {
				REQUIRE(env.stats.chunkDataBytes == 4 * bytesPerChunk);
			}
		}

		WHEN("all of them are unloaded") {
			env.streamer.unloadRange({70, -1, 70}, {71, -1, 71});

			THEN("no memory is used") {
				REQUIRE(env.stats.chunkDataBytes == 0);
			}
		}
	}
}

SCENARIO("Asking for a chunk again generates it again", "[chunk-streamer]") {
	GIVEN("a loaded chunk") {
		StreamerEnv env;
		env.streamer.requestChunk({1, 0, 1});
		env.settle();

		WHEN("it is requested a second time") {
			env.streamer.requestChunk({1, 0, 1});
			env.settle();

			THEN("both requests were announced and the chunk was delivered twice, but it is stored once") {
				REQUIRE(env.recorder.requested.size() == 2);
				REQUIRE(env.recorder.loaded.size() == 2);
				REQUIRE(env.stats.loadedChunks == 1);
			}
		}
	}
}

SCENARIO("Worker threads deliver the same chunks as running the jobs one by one", "[chunk-streamer][threads]") {
	GIVEN("the area around the player generated with jobs run one by one") {
		StreamerEnv reference(spawnRenderDistance);
		reference.settle();

		WHEN("the same area is generated by a pool of four worker threads") {
			ecs::Dispatcher          dispatcher;
			test::ChunkEventRecorder recorder;
			recorder.listenTo(dispatcher);
			profiling::ServerStats stats;
			const auto             pool = concurrency::createThreadPool(4);
			{
				ChunkStreamer streamer(dispatcher, *pool, stats, test::seed,
									   ChunkStreamSettings{.renderDistance = spawnRenderDistance});
				streamer.setViewer(atSpawnLookingNorth);

				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
				do {
					streamer.update();
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				} while (!streamer.isIdle() && std::chrono::steady_clock::now() < deadline);
				REQUIRE(streamer.isIdle());
			}

			THEN("the same chunks arrive, with the same amount of block data") {
				const auto summarise = [](const test::ChunkEventRecorder& source) {
					std::vector<std::tuple<int, int, int, size_t>> summary;
					summary.reserve(source.loaded.size());
					for (const auto& [position, chunk]: source.loaded)
						summary.emplace_back(position.x, position.y, position.z, chunk.dataBytes());
					std::ranges::sort(summary);
					return summary;
				};

				REQUIRE(summarise(recorder) == summarise(reference.recorder));
				REQUIRE(stats.chunkDataBytes == reference.stats.chunkDataBytes);
			}
		}
	}
}

SCENARIO("The chunks around the player form a sphere, not a box", "[chunk-streamer]") {
	GIVEN("a streamer with the same render distance on every axis") {
		constexpr RenderDistance sphere      = {8, 8, 8};
		constexpr size_t         boxSide     = 9;
		constexpr size_t         chunksInBox = boxSide * boxSide * boxSide;
		StreamerEnv              env(sphere);

		WHEN("the first update runs") {
			env.streamer.update();
			const test::ChunkSet requested = test::toSet(env.recorder.requested);

			THEN("the chunks on the axes are requested") {
				REQUIRE(requested.contains({4, 0, 0}));
				REQUIRE(requested.contains({0, -4, 0}));
			}
			AND_THEN("the corners of the surrounding box are not") {
				REQUIRE_FALSE(requested.contains({3, 3, 3}));
				REQUIRE_FALSE(requested.contains({4, 4, 4}));
			}
			AND_THEN("fewer chunks than the box are requested") {
				REQUIRE(requested.size() == test::renderAreaSize(sphere));
				REQUIRE(requested.size() < chunksInBox);
			}
		}
	}

	GIVEN("a streamer whose render distance is shorter vertically than horizontally") {
		constexpr RenderDistance flattened = {8, 2, 8};
		StreamerEnv              env(flattened);

		WHEN("the first update runs") {
			env.streamer.update();
			const test::ChunkSet requested = test::toSet(env.recorder.requested);

			THEN("the area is wide and flat") {
				REQUIRE(requested == test::renderAreaAround({0, 0, 0}, flattened));
				REQUIRE(requested.contains({4, 0, 0}));
				REQUIRE_FALSE(requested.contains({0, 2, 0}));
			}
		}
	}
}
