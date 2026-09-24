#include "readFile.hpp"

#include <fstream>

#include "resolvePath.hpp"
#include "../../error/Exception.hpp"

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(resolvePath(filename), std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw error::FileError(filename, "could not be opened");
	}
	const auto        fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
	file.close();

	return buffer;
}
