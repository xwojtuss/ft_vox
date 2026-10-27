#pragma once

#include <string_view>
#include <filesystem>

bool hasAllowedExtension(const std::filesystem::path& filename, std::initializer_list<std::string_view> allowedExt);
