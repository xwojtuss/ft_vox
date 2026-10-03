#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <thread>
#include <tuple>
#include <vector>

#include "concurrency/IThreadPool.hpp"
#include "game/planet/ChunkStreamer.hpp"
#include "scene/PlanetInfo.hpp"
#include "support/ChunkEventRecorder.hpp"
#include "support/ManualThreadPool.hpp"
#include "support/TestSeed.hpp"

using game::planet::ChunkStreamer;
using game::planet::ChunkStreamSettings;
namespace planetinfo = scene::planetinfo;

namespace {
	constexpr glm::vec<3, unsigned short> spawnRenderDistance = {2, planetinfo::maxVerticalRenderDistance, 2};

	constexpr int chunksAroundSpawn = (spawnRenderDistance.x + 1) * (spawnRenderDistance.z + 1) * spawnRenderDistance.y;

	struct StreamerEnv {
		ecs::Dispatcher          dispatcher;
		test::ChunkEventRecorder recorder;
		test::ManualThreadPool   pool;
		profiling::ServerStats   stats;
		ChunkStreamer            streamer;

		StreamerEnv() : streamer(dispatcher, pool, stats, test::seed, ChunkStreamSettings{spawnRenderDistance}) {
			recorder.listenTo(dispatcher);
		}

		void settle() {
			constexpr int maxRounds = 100;
			for (int round = 0; round < maxRounds; ++round) {
				if (streamer.isIdle() && pool.pending() == 0)
					return;
				pool.runAll();
				streamer.update();
			}
		}
	};
}

SCENARIO("Requested chunks are announced at once and delivered later", "[chunk-streamer]") {
	GIVEN("a streamer that was asked for the area around spawn") {
		StreamerEnv env;
		env.streamer.requestSpawnArea();

		THEN("every chunk of the area was announced, and none has been delivered") {
			REQUIRE(env.recorder.requested.size() == static_cast<size_t>(chunksAroundSpawn));
			REQUIRE(env.recorder.loaded.empty());
		}
		AND_THEN("one generation job is queued per chunk, and the statistics say so") {
			env.streamer.update();
			REQUIRE(env.pool.pending() == static_cast<size_t>(chunksAroundSpawn));
			REQUIRE(env.stats.pendingGeneration == static_cast<size_t>(chunksAroundSpawn));
			REQUIRE(env.stats.loadedChunks == 0);
		}

		WHEN("the pool finishes one job") {
			env.pool.runNext();
			env.streamer.update();

			THEN("exactly one chunk is delivered, the one at the centre") {
				REQUIRE(env.recorder.loaded.size() == 1);
				REQUIRE(env.recorder.loaded.front().first == glm::ivec3(0, 0, 0));
				REQUIRE(env.stats.loadedChunks == 1);
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
			AND_THEN("chunks that hold only air take no memory") {
				constexpr size_t bytesPerChunk = game::planet::chunkVolume * sizeof(game::Block);

				REQUIRE(env.stats.chunkDataBytes > 0);
				REQUIRE(env.stats.chunkDataBytes % bytesPerChunk == 0);
				REQUIRE(env.stats.chunkDataBytes < env.stats.loadedChunks * bytesPerChunk);
			}
		}
	}
}

SCENARIO("Chunks closer to the centre are generated first", "[chunk-streamer]") {
	GIVEN("a streamer whose generation jobs are queued") {
		StreamerEnv env;
		env.streamer.requestSpawnArea();

		WHEN("the pool runs the first 9 jobs") {
			for (int i = 0; i < 9; ++i)
				env.pool.runNext();
			env.streamer.update();

			THEN("the centre chunk is among them and none of them is more than one chunk away from it") {
				REQUIRE(env.recorder.loaded.size() == 9);
				REQUIRE(env.streamer.isLoaded({0, 0, 0}));
				for (const auto& [position, chunk]: env.recorder.loaded)
					REQUIRE(std::max({std::abs(position.x), std::abs(position.y), std::abs(position.z)}) <= 1);
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

	GIVEN("a chunk that was requested but not generated yet") {
		StreamerEnv env;
		env.streamer.requestChunk(chunk);

		WHEN("it is unloaded before the pool gets to it") {
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
	GIVEN("the spawn area generated with jobs run one by one") {
		StreamerEnv reference;
		reference.streamer.requestSpawnArea();
		reference.settle();

		WHEN("the same area is generated by a pool of four worker threads") {
			ecs::Dispatcher          dispatcher;
			test::ChunkEventRecorder recorder;
			recorder.listenTo(dispatcher);
			profiling::ServerStats stats;
			const auto             pool = concurrency::createThreadPool(4);
			{
				ChunkStreamer streamer(dispatcher, *pool, stats, test::seed, ChunkStreamSettings{spawnRenderDistance});
				streamer.requestSpawnArea();

				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
				while (!streamer.isIdle() && std::chrono::steady_clock::now() < deadline) {
					streamer.update();
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
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
