#include <catch2/catch_test_macros.hpp>

#include "ecs/system/types/ChunkSystem.hpp"
#include "support/Blocks.hpp"
#include "support/FakeRenderer.hpp"

using game::planet::chunkXSize;

namespace {
	struct ChunkSystemPlanet {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ecs::Registry           registry{blockDatas};
		test::FakeRenderer      renderer;
		profiling::ServerStats  stats;

		ChunkSystemPlanet() {
			registry.createSystem<ecs::ChunkSystem>(registry, renderer, stats, glm::vec<3, unsigned short>{2, 4, 2});
		}

		[[nodiscard]] size_t meshCount() const {
			return renderer.createdMeshes.size();
		}

		void movePlayer(glm::vec3 from, glm::vec3 to) {
			registry.getSystemManager().getDispatcher().emit(ecs::PlayerMoveEvent(from, to));
		}
	};

	ChunkSystemPlanet& sharedChunkSystemPlanet() {
		static ChunkSystemPlanet instance;
		return instance;
	}
}

// TODO: make pass
SCENARIO("Chunks load around the player as they move", "[ecs][chunk-system][!mayfail]") {
	GIVEN("a planet whose chunk system has loaded the area around spawn") {
		ChunkSystemPlanet& env          = sharedChunkSystemPlanet();
		const size_t       meshesBefore = env.meshCount();

		REQUIRE(meshesBefore > 0);

		WHEN("the player moves 100 chunks away") {
			env.movePlayer({0.0f, 40.0f, 0.0f}, {100.0f * chunkXSize, 40.0f, 100.0f * chunkXSize});

			THEN("the chunks around the new position are loaded") {
				REQUIRE(env.meshCount() > meshesBefore);
			}
		}
	}
}
