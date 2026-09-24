#include <catch2/catch_test_macros.hpp>

#include "error/Exception.hpp"
#include "assets/TinyObjLoader.hpp"
#include "support/TemporaryFile.hpp"

namespace {
	const std::string texturedSquare =
			"v 0 0 0\n"
			"v 1 0 0\n"
			"v 1 1 0\n"
			"v 0 1 0\n"
			"vt 0 0\n"
			"vt 1 0\n"
			"vt 1 1\n"
			"vt 0 1\n"
			"f 1/1 2/2 3/3\n"
			"f 1/1 3/3 4/4\n";

	assets::MeshData loadObj(const std::string& content) {
		const test::TemporaryFile file("model.obj", content);
		assets::TinyObjLoader     loader;
		return loader.toMeshData(std::string(file.path()));
	}
}

SCENARIO("A model is loaded as it is written, with shared corners stored once", "[assets][tinyobj]") {
	GIVEN("a textured square made of two triangles that share two corners") {
		WHEN("it is loaded") {
			const assets::MeshData mesh = loadObj(texturedSquare);

			THEN("each distinct corner is stored once and the triangles point at them") {
				REQUIRE(mesh.vertices.size() == 4);
				REQUIRE(mesh.indices == std::vector<uint32_t>{0, 1, 2, 0, 2, 3});
			}
			AND_THEN("positions are kept exactly as in the file") {
				REQUIRE(mesh.vertices[0].pos == glm::vec3(0, 0, 0));
				REQUIRE(mesh.vertices[2].pos == glm::vec3(1, 1, 0));
			}
			AND_THEN("texture coordinates are flipped vertically, since images start at the top") {
				REQUIRE(mesh.vertices[0].texCoord == glm::vec2(0, 1));
				REQUIRE(mesh.vertices[2].texCoord == glm::vec2(1, 0));
			}
			AND_THEN("every vertex is white, so the texture shows its own colors") {
				for (const render::Vertex& vertex: mesh.vertices)
					REQUIRE(vertex.color == glm::vec3(1.0f));
			}
		}
	}
}

SCENARIO("A model file that does not exist is rejected", "[assets][tinyobj]") {
	THEN("loading it fails") {
		assets::TinyObjLoader loader;
		REQUIRE_THROWS_AS(loader.toMeshData("models/does_not_exist.obj"), error::AssetError);
	}
}

SCENARIO("A model without texture coordinates can be loaded", "[assets][tinyobj]") {
	GIVEN("a triangle without texture coordinates") {
		const std::string content = "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";

		THEN("it loads as one triangle") {
			REQUIRE(loadObj(content).indices.size() == 3);
		}
	}
}
