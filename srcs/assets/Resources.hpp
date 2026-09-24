#pragma once

#include <vector>
#include <functional>

#include "../render/GpuTypes.hpp"

namespace assets {
	struct MeshData {
		std::vector<render::Vertex> vertices;
		std::vector<uint32_t>       indices;

		void scaleTextureCoordinates(float scale);
	};

	struct TextureData {
		uint32_t                   width{};
		uint32_t                   height{};
		uint32_t                   mipLevels{};
		void*                      pixels{};
		bool                       pixelPerfect = false;
		std::function<void(void*)> freePixels;
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
