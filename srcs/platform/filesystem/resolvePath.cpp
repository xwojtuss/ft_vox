#include "resolvePath.hpp"

#include <filesystem>

#include "../../app/ApplicationInfo.hpp"

std::string resolvePath(const std::string& path) {
	const std::filesystem::path p(path);

	if (p.is_absolute())
		return path;
	return (std::filesystem::path(app::projectRoot) / p).lexically_normal().string();
}
