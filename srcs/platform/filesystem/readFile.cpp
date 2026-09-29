#include "platform/filesystem/readFile.hpp"

#include <cstring>
#include <fstream>

#include "platform/filesystem/resolvePath.hpp"
#include "error/Exception.hpp"

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

std::vector<uint32_t> readSpirv(const std::string& filename) {
	const std::vector<char> bytes = readFile(filename);

	if (bytes.size() % sizeof(uint32_t) != 0)
		throw error::FileError(filename, "is not SPIR-V: its size is not a whole number of 32-bit words");

	std::vector<uint32_t> words(bytes.size() / sizeof(uint32_t));
	std::memcpy(words.data(), bytes.data(), bytes.size());
	return words;
}
