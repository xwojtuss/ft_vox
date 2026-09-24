#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace render {
	// One per frame
	struct alignas(16) FrameUBO {
		glm::mat4 view;
		glm::mat4 proj;
	};

	/** One per draw/object
	* For now a push constant
	* using 64 bytes of the 128 available
	*/
	struct alignas(16) ObjectUBO {
		glm::mat4 model;
	};

	struct alignas(16) TextUBO {
		glm::vec2 position;
		glm::vec2 size;
		glm::vec4 color;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color && texCoord == other.texCoord;
		}
	};

	struct Vertex2D {
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		bool operator==(const Vertex2D& other) const {
			return pos == other.pos && color == other.color && texCoord == other.texCoord;
		}
	};

	struct InstanceData {
		glm::vec2 texCoord;
		int       charIndex{};
	};
}

template<>
struct std::hash<render::Vertex> {
	size_t operator()(const render::Vertex& vertex) const noexcept {
		return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
