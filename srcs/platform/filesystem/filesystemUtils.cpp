#include "filesystemUtils.hpp"
#include "error/Exception.hpp"

#include <algorithm>

bool hasAllowedExtension(const std::filesystem::path&                  filename,
						 const std::initializer_list<std::string_view> allowedExt) {
	const auto ext = filename.extension().string();

	return std::ranges::any_of(allowedExt, [&ext](const auto& allowed) { return ext == allowed; });
}
