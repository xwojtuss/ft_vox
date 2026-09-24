#pragma once

#include <string>

#include "Resources.hpp"

namespace assets {
	class IModelLoader {
	public:
		virtual ~IModelLoader() = default;

		[[nodiscard]] virtual MeshData toMeshData(const char* path) = 0;
		[[nodiscard]] virtual MeshData toMeshData(const std::string& path) = 0;
	};
}
