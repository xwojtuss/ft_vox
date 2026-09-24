#pragma once

#include <string>
#include <tiny_obj_loader.h>

#include "IModelLoader.hpp"
#include "Resources.hpp"

namespace assets {
	class TinyObjLoader : public IModelLoader {
	public:
		[[nodiscard]] MeshData toMeshData(const char* path) override;
		[[nodiscard]] MeshData toMeshData(const std::string& path) override;
	};
}
