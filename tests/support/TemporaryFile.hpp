#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace test {
	class TemporaryFile {
	private:
		std::filesystem::path m_path;

	public:
		TemporaryFile(const std::string& name, const std::string& content)
			: m_path(std::filesystem::temp_directory_path() / ("ft_vox_tests_" + std::to_string(getpid()) + "_" + name)) {
			std::ofstream(m_path, std::ios::binary) << content;
		}

		~TemporaryFile() {
			std::filesystem::remove(m_path);
		}

		TemporaryFile(const TemporaryFile&)            = delete;
		TemporaryFile& operator=(const TemporaryFile&) = delete;

		std::string path() const {
			return m_path.string();
		}
	};
}
