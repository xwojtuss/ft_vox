#include <catch2/catch_test_macros.hpp>

#include "assets/Resources.hpp"

SCENARIO("Scaling texture coordinates repeats the texture more often", "[assets][resources]") {
	GIVEN("a mesh with texture coordinates (0.5, 1) and (1, 0.25)") {
		assets::MeshData mesh;
		mesh.vertices = {
			{{1, 2, 3}, {1, 1, 1}, {0.5f, 1.0f}},
			{{4, 5, 6}, {1, 1, 1}, {1.0f, 0.25f}},
		};

		WHEN("its texture coordinates are scaled by 4") {
			mesh.scaleTextureCoordinates(4.0f);

			THEN("they become (2, 4) and (4, 1)") {
				REQUIRE(mesh.vertices[0].texCoord == glm::vec2(2.0f, 4.0f));
				REQUIRE(mesh.vertices[1].texCoord == glm::vec2(4.0f, 1.0f));
			}
			AND_THEN("positions are not touched") {
				REQUIRE(mesh.vertices[0].pos == glm::vec3(1, 2, 3));
			}
		}
	}
}

SCENARIO("Every GPU resource handle gets its own id", "[assets][resources]") {
	THEN("two mesh handles never share an id") {
		const assets::MeshHandle first;
		const assets::MeshHandle second;
		REQUIRE(first.id != second.id);
	}
	AND_THEN("two texture handles never share an id") {
		const assets::TextureHandle first;
		const assets::TextureHandle second;
		REQUIRE(first.id != second.id);
	}
}
