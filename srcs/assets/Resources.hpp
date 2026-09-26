#pragma once

#include <cstdint> // for std::uint32_t
#include <vector>

#include "../render/GpuTypes.hpp"

namespace assets {
	struct MeshData {
		std::vector<render::Vertex> vertices;
		std::vector<std::uint32_t>  indices;

		void scaleTextureCoordinates(float scale);
	};

	struct TextureData {
		uint32_t                   width{};
		uint32_t                   height{};
		uint32_t                   mipLevels{};
		std::vector<unsigned char> pixels;
		bool                       pixelPerfect = false;
	};

	enum class PipelineType {
		Textured,
		VertexColor,
		Text
	};

	struct MeshHandle {
		static uint64_t nextId;
		uint64_t        id{nextId++};

		MeshHandle() = default;
	};

	struct TextureHandle {
		static uint64_t nextId;
		uint64_t        id{nextId++};

		TextureHandle() = default;
	};
}
