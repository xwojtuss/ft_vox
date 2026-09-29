#pragma once

#include <cstdint>
#include <string>
#include <vector>

[[nodiscard]] std::vector<char>     readFile(const std::string& filename);
[[nodiscard]] std::vector<uint32_t> readSpirv(const std::string& filename);
