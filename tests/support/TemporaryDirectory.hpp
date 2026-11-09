#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace test {
	class TemporaryDirectory {
	private:
		std::filesystem::path m_path;

	public:
		explicit TemporaryDirectory(const std::string& name) {
			static std::atomic<unsigned int> counter = 0;
			m_path                                   = std::filesystem::temp_directory_path() /
					 ("ft_vox_tests_" + std::to_string(getpid()) + "_" + name + "_" + std::to_string(counter++));
			std::filesystem::create_directories(m_path);
		}

		~TemporaryDirectory() {
			std::filesystem::remove_all(m_path);
		}

		TemporaryDirectory(const TemporaryDirectory&)            = delete;
		TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
		TemporaryDirectory(TemporaryDirectory&&)                 = delete;
		TemporaryDirectory& operator=(TemporaryDirectory&&)      = delete;

		void write(const std::filesystem::path& relativePath, const std::string& content) const {
			const std::filesystem::path file = m_path / relativePath;
			std::filesystem::create_directories(file.parent_path());
			std::ofstream(file) << content;
		}

		[[nodiscard]] const std::filesystem::path& path() const {
			return m_path;
		}
	};
}
