#pragma once

#include <string>
#include <stdexcept>
#include <tiny_obj_loader.h>

#include "IModelLoader.hpp"
#include "Resources.hpp"
#include "../render/GpuTypes.hpp"
#include "../platform/filesystem/resolvePath.hpp"

namespace assets {
class TinyObjLoader : public IModelLoader {
public:
	TinyObjLoader() = default;
	virtual ~TinyObjLoader();

	MeshData	toMeshData(const char* path) override;
	MeshData	toMeshData(std::string path) override;
};
}
