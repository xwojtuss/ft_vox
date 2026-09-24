#include <catch2/catch_test_macros.hpp>

#include "ecs/system/types/ChunkSystem.hpp"
#include "support/Blocks.hpp"
#include "support/FakeRenderer.hpp"

using game::world::chunkXSize;

namespace {
	struct ChunkSystemWorld {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ecs::World              world{blockDatas};
		test::FakeRenderer      renderer;

		ChunkSystemWorld() {
			world.createSystem<ecs::ChunkSystem>(world, renderer);
		}

		size_t meshCount() const {
			return renderer.createdMeshes.size();
		}

		void movePlayer(glm::vec3 from, glm::vec3 to) {
			world.getSystemManager().getDispatcher().emit(ecs::PlayerMoveEvent(from, to));
		}
	};

	ChunkSystemWorld& sharedChunkSystemWorld() {
		static ChunkSystemWorld instance;
		return instance;
	}
}

// TODO: make pass
SCENARIO("Chunks load around the player as they move", "[ecs][chunk-system]") {
	GIVEN("a world whose chunk system has loaded the area around spawn") {
		ChunkSystemWorld& env          = sharedChunkSystemWorld();
		const size_t      meshesBefore = env.meshCount();

		REQUIRE(meshesBefore > 0);

		WHEN("the player moves 100 chunks away") {
			env.movePlayer({0.0f, 40.0f, 0.0f}, {100.0f * chunkXSize, 40.0f, 100.0f * chunkXSize});

			THEN("the chunks around the new position are loaded") {
				REQUIRE(env.meshCount() > meshesBefore);
			}
		}
	}
}
