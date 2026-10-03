#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <limits>
#include <utility>

#include "ecs/component/Components.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "render/chunks/ChunkRenderer.hpp"
#include "support/Blocks.hpp"
#include "support/FakeRenderer.hpp"
#include "support/ManualThreadPool.hpp"

using game::Block;
using game::planet::Chunk;
using game::planet::ChunkChangedEvent;
using game::planet::ChunkLoadedEvent;
using game::planet::ChunkRequestedEvent;
using game::planet::ChunkUnloadedEvent;
using game::planet::chunkXSize;
using game::planet::chunkYSize;
using game::planet::chunkZSize;
using render::chunks::ChunkRenderer;
using render::chunks::ChunkRenderSettings;

namespace {
	constexpr size_t trianglesOnTop = 2uz * chunkXSize * chunkZSize;

	Chunk solidChunk() {
		Chunk chunk;
		for (int x = 0; x < chunkXSize; ++x)
			for (int y = 0; y < chunkYSize; ++y)
				for (int z = 0; z < chunkZSize; ++z)
					chunk.setBlock(x, y, z, Block(test::dirt));
		return chunk;
	}

	struct RendererEnv {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ecs::Registry           registry{blockDatas};
		test::FakeRenderer      renderer;
		test::ManualThreadPool  pool;
		profiling::ClientStats  stats;
		ChunkRenderer           chunkRenderer;

		explicit RendererEnv(const size_t uploadsPerFrame = std::numeric_limits<size_t>::max()) :
			chunkRenderer(blockDatas, registry, renderer, pool, stats, ChunkRenderSettings{uploadsPerFrame}) {
		}

		void load(const glm::ivec3 position, const Chunk& chunk) {
			chunkRenderer.onChunkLoaded(ChunkLoadedEvent(position, chunk));
		}

		void unload(const glm::ivec3 position) {
			chunkRenderer.onChunkUnloaded(ChunkUnloadedEvent(position));
		}

		void settle() {
			constexpr int maxRounds = 100;
			for (int round = 0; round < maxRounds; ++round) {
				pool.runAll();
				chunkRenderer.update();
				if (chunkRenderer.isIdle() && pool.pending() == 0)
					return;
			}
		}

		[[nodiscard]] size_t drawnChunkCount() {
			size_t count = 0;
			for (auto&& drawn: registry.query<const ecs::component::Mesh>()) {
				static_cast<void>(drawn);
				++count;
			}
			return count;
		}

		[[nodiscard]] size_t meshCount() const {
			return renderer.createdMeshes.size();
		}
	};
}

SCENARIO("A chunk without neighbours shows only its top", "[chunk-renderer]") {
	GIVEN("a solid chunk that was delivered") {
		RendererEnv env;
		env.load({0, 0, 0}, solidChunk());
		env.settle();

		THEN("one mesh is drawn, made of the top of the chunk only") {
			REQUIRE(env.drawnChunkCount() == 1);
			REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop);
		}
		AND_THEN("its entity is a textured chunk mesh at the chunk's place in the planet") {
			const auto entities = env.registry.query<const ecs::component::Mesh, const ecs::component::Transform>();
			for (auto&& [entity, mesh, transform]: entities) {
				REQUIRE(mesh.pipelineType == assets::PipelineType::Chunk);
				REQUIRE(transform.position == glm::vec3(0.0f));
				REQUIRE(env.registry.getEntity(entity).has<ecs::component::Texture>());
			}
		}
	}

	GIVEN("a chunk far from the origin") {
		RendererEnv env;
		env.load({40, -1, -40}, solidChunk());
		env.settle();

		THEN("its entity sits at the chunk's position on the 16-block grid") {
			for (auto&& [entity, transform]: env.registry.query<const ecs::component::Transform>())
				REQUIRE(transform.position == glm::vec3(40 * chunkXSize, -1 * chunkYSize, -40 * chunkZSize));
		}
	}

	GIVEN("a chunk that holds only air") {
		RendererEnv env;
		env.load({0, 8, 0}, Chunk());
		env.settle();

		THEN("nothing is meshed or drawn for it") {
			REQUIRE(env.meshCount() == 0);
			REQUIRE(env.drawnChunkCount() == 0);
		}
	}
}

SCENARIO("A chunk waits for the chunks around it that are still on their way", "[chunk-renderer]") {
	GIVEN("a delivered chunk whose neighbour was announced but has not arrived") {
		RendererEnv env;
		env.chunkRenderer.onChunkRequested(ChunkRequestedEvent({1, 0, 0}));
		env.load({0, 0, 0}, solidChunk());
		env.chunkRenderer.update();

		THEN("it is not meshed yet, and the statistics say one chunk is waiting") {
			REQUIRE(env.pool.pending() == 0);
			REQUIRE(env.stats.pendingMeshing == 1);
			REQUIRE_FALSE(env.chunkRenderer.isIdle());
		}

		WHEN("the neighbour arrives") {
			env.load({1, 0, 0}, solidChunk());
			env.settle();

			THEN("both are meshed, each without the side that faces the other") {
				REQUIRE(env.drawnChunkCount() == 2);
				REQUIRE(env.renderer.createdMeshTriangleCounts == std::vector{trianglesOnTop, trianglesOnTop});
			}
		}

		WHEN("the neighbour is unloaded before it arrives") {
			env.unload({1, 0, 0});
			env.settle();

			THEN("the chunk is meshed without waiting any longer") {
				REQUIRE(env.drawnChunkCount() == 1);
			}
		}
	}

	GIVEN("a delivered chunk that is requested again") {
		RendererEnv env;
		env.load({0, 0, 0}, solidChunk());
		env.settle();
		const size_t meshesBefore = env.meshCount();

		WHEN("it is announced again and has not arrived") {
			env.load({1, 0, 0}, solidChunk());
			env.chunkRenderer.onChunkRequested(ChunkRequestedEvent({0, 0, 0}));
			env.chunkRenderer.update();
			env.pool.runAll();
			env.chunkRenderer.update();

			THEN("neither it nor its neighbour is meshed again until the new data arrives") {
				REQUIRE(env.meshCount() == meshesBefore);
			}
		}
	}
}

SCENARIO("A chunk covered by its neighbours loses the covered sides", "[chunk-renderer]") {
	constexpr glm::ivec3 lower(300, -3, 300);
	constexpr glm::ivec3 upper = lower + glm::ivec3(0, 1, 0);

	GIVEN("a solid chunk without neighbours") {
		RendererEnv env;
		env.load(lower, solidChunk());
		env.settle();

		WHEN("a solid chunk is delivered on top of it") {
			env.load(upper, solidChunk());
			env.settle();

			THEN("the lower chunk is meshed again with nothing left to draw, the upper one draws its top") {
				REQUIRE(env.drawnChunkCount() == 1);
				REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop);
			}

			AND_WHEN("the upper chunk is unloaded again") {
				env.unload(upper);
				env.settle();

				THEN("the lower chunk shows its top again") {
					REQUIRE(env.drawnChunkCount() == 1);
					REQUIRE(env.renderer.createdMeshTriangleCounts.back() == trianglesOnTop);
				}
			}
		}

		WHEN("a solid chunk is delivered next to it") {
			env.load(lower + glm::ivec3(1, 0, 0), solidChunk());
			env.settle();

			THEN("each draws only its top: the sides facing each other are hidden, the others have no neighbour") {
				REQUIRE(env.drawnChunkCount() == 2);
				const auto& counts = env.renderer.createdMeshTriangleCounts;
				REQUIRE(counts[counts.size() - 1] == trianglesOnTop);
				REQUIRE(counts[counts.size() - 2] == trianglesOnTop);
			}
		}
	}
}

SCENARIO("A chunk buried inside solid terrain is not drawn", "[chunk-renderer]") {
	constexpr glm::ivec3 center(100, -2, 100);

	GIVEN("a 3 x 3 x 3 block of solid chunks delivered together") {
		RendererEnv env;
		for (int x = -1; x <= 1; ++x)
			for (int y = -1; y <= 1; ++y)
				for (int z = -1; z <= 1; ++z)
					env.chunkRenderer.onChunkRequested(ChunkRequestedEvent(center + glm::ivec3(x, y, z)));
		for (int x = -1; x <= 1; ++x)
			for (int y = -1; y <= 1; ++y)
				for (int z = -1; z <= 1; ++z)
					env.load(center + glm::ivec3(x, y, z), solidChunk());
		env.settle();

		THEN("only the nine chunks on top are drawn, with the tops that have no chunk above them") {
			REQUIRE(env.drawnChunkCount() == 9);
		}

		WHEN("the chunk above the middle one is unloaded") {
			env.unload(center + glm::ivec3(0, 1, 0));
			env.settle();

			THEN("the middle chunk is drawn, because its top is exposed") {
				REQUIRE(env.drawnChunkCount() == 8 + 1);
			}
		}
	}
}

SCENARIO("A chunk is meshed again when a block next to its border changes", "[chunk-renderer]") {
	constexpr glm::ivec3 left(310, -1, 310);
	constexpr glm::ivec3 right = left + glm::ivec3(1, 0, 0);

	GIVEN("two solid chunks side by side") {
		RendererEnv env;
		env.load(left, solidChunk());
		env.load(right, solidChunk());
		env.settle();
		const size_t meshesBefore = env.meshCount();

		WHEN("a block on the border of the right chunk is removed") {
			Chunk changed = solidChunk();
			changed.removeBlock(0, 5, 5);
			env.chunkRenderer.onChunkChanged(ChunkChangedEvent(right, {0, 5, 5}, changed));
			env.settle();

			THEN("both chunks are meshed again") {
				REQUIRE(env.meshCount() == meshesBefore + 2);
			}
			AND_THEN("the left chunk now shows the one face that is no longer covered, next to its top") {
				const auto& counts = env.renderer.createdMeshTriangleCounts;
				REQUIRE(std::ranges::count(counts.end() - 2, counts.end(), trianglesOnTop + 2) == 1);
			}
		}

		WHEN("a block in the middle of the right chunk is removed") {
			Chunk changed = solidChunk();
			changed.removeBlock(8, 8, 8);
			env.chunkRenderer.onChunkChanged(ChunkChangedEvent(right, {8, 8, 8}, changed));
			env.settle();

			THEN("only the right chunk is meshed again") {
				REQUIRE(env.meshCount() == meshesBefore + 1);
			}
		}
	}
}

SCENARIO("Work for a chunk that was unloaded in the meantime is dropped", "[chunk-renderer]") {
	GIVEN("a delivered chunk whose mesh job is queued") {
		RendererEnv env;
		env.load({0, 0, 0}, solidChunk());
		env.chunkRenderer.update();
		REQUIRE(env.pool.pending() == 1);

		WHEN("the chunk is unloaded before the mesh is built") {
			env.unload({0, 0, 0});
			env.settle();

			THEN("it is never drawn and nothing is left waiting") {
				REQUIRE(env.drawnChunkCount() == 0);
				REQUIRE(env.meshCount() == 0);
				REQUIRE(env.chunkRenderer.isIdle());
			}
		}
	}

	GIVEN("a delivered chunk whose mesh is built but not uploaded yet") {
		RendererEnv env(0);
		env.load({0, 0, 0}, solidChunk());
		env.chunkRenderer.update();
		env.pool.runAll();
		env.chunkRenderer.update();
		REQUIRE(env.meshCount() == 0);

		WHEN("the chunk is unloaded and uploads are allowed again") {
			env.unload({0, 0, 0});
			env.settle();

			THEN("the finished mesh is thrown away") {
				REQUIRE(env.meshCount() == 0);
				REQUIRE(env.drawnChunkCount() == 0);
			}
		}
	}
}

SCENARIO("An unloaded chunk disappears and gives its mesh back", "[chunk-renderer]") {
	GIVEN("a drawn chunk") {
		RendererEnv env;
		env.load({0, 0, 0}, solidChunk());
		env.settle();
		REQUIRE(env.drawnChunkCount() == 1);

		WHEN("it is unloaded") {
			env.unload({0, 0, 0});

			THEN("it is no longer drawn") {
				REQUIRE(env.drawnChunkCount() == 0);
			}
			AND_THEN("its mesh is handed back to the renderer so the GPU memory can be reused") {
				REQUIRE(env.renderer.destroyedMeshes.size() == 1);
				REQUIRE(env.renderer.destroyedMeshes.back().id == env.renderer.createdMeshes.front().id);
			}
		}
	}

	GIVEN("a chunk that was never delivered") {
		RendererEnv env;

		THEN("unloading it does nothing") {
			REQUIRE_NOTHROW(env.unload({999, 999, 999}));
			REQUIRE(env.meshCount() == 0);
		}
	}
}

SCENARIO("Only a limited number of meshes reach the GPU per frame", "[chunk-renderer]") {
	GIVEN("a renderer limited to 2 mesh uploads per update, and nine solid chunks far apart") {
		RendererEnv env(2);
		for (int i = 0; i < 9; ++i)
			env.load({i * 10, 0, 0}, solidChunk());
		env.chunkRenderer.update();
		env.pool.runAll();

		WHEN("the world is updated once") {
			env.chunkRenderer.update();

			THEN("only two meshes were created") {
				REQUIRE(env.meshCount() == 2);
				REQUIRE_FALSE(env.chunkRenderer.isIdle());
			}
		}

		WHEN("the world is updated until it is idle") {
			int updates = 0;
			while (!env.chunkRenderer.isIdle() && updates < 20) {
				env.chunkRenderer.update();
				++updates;
			}

			THEN("all nine chunks are drawn, two per update") {
				REQUIRE(env.meshCount() == 9);
				REQUIRE(updates == 5);
			}
		}
	}
}
