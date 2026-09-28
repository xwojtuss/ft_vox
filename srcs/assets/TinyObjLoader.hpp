#pragma once

#include <string>
#include "assets/third_party/tiny_obj_loader.h"

#include "assets/IModelLoader.hpp"
#include "assets/Resources.hpp"

namespace assets {
	class TinyObjLoader : public IModelLoader {
	public:
		[[nodiscard]] MeshData toMeshData(const char* path) override;
		[[nodiscard]] MeshData toMeshData(const std::string& path) override;
	};
}
