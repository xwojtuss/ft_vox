#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>

#include "error/Exception.hpp"
#include "app/ApplicationInfo.hpp"
#include "platform/filesystem/readFile.hpp"
#include "platform/filesystem/resolvePath.hpp"
#include "support/TemporaryFile.hpp"

namespace fs = std::filesystem;

namespace {
	class WorkingDirectory {
	private:
		fs::path m_previous;

	public:
		explicit WorkingDirectory(const fs::path& directory) : m_previous(fs::current_path()) {
			fs::current_path(directory);
		}

		~WorkingDirectory() {
			fs::current_path(m_previous);
		}

		WorkingDirectory(const WorkingDirectory&)            = delete;
		WorkingDirectory& operator=(const WorkingDirectory&) = delete;
		WorkingDirectory(WorkingDirectory&&)                 = delete;
		WorkingDirectory& operator=(WorkingDirectory&&)      = delete;
	};
}

SCENARIO("Relative paths point into the project, absolute paths are kept", "[filesystem]") {
	THEN("an absolute path is returned unchanged") {
		REQUIRE(resolvePath("/tmp/some/file.txt") == "/tmp/some/file.txt");
	}
	AND_THEN("a relative path is placed under the project root") {
		REQUIRE(resolvePath("shaders/shader.vert.spv") ==
				(fs::path(app::projectRoot) / "shaders/shader.vert.spv").string());
	}
	AND_THEN("a relative path is cleaned up") {
		REQUIRE(resolvePath("shaders/../textures/./default.png") ==
				(fs::path(app::projectRoot) / "textures/default.png").string());
	}
	AND_THEN("the result does not depend on the directory the game was started from") {
		const std::string      fromProject = resolvePath("textures/default.png");
		const WorkingDirectory startedElsewhere(fs::temp_directory_path());
		REQUIRE(resolvePath("textures/default.png") == fromProject);
	}
}

SCENARIO("Reading a file returns its exact bytes", "[filesystem]") {
	GIVEN("a file with text, a zero byte and a newline") {
		const std::string         content("ab\0c\n", 5);
		const test::TemporaryFile file("bytes.bin", content);

		THEN("reading it by absolute path returns exactly those bytes") {
			REQUIRE(readFile(file.path()) == std::vector<char>(content.begin(), content.end()));
		}
	}

	GIVEN("an empty file") {
		const test::TemporaryFile file("empty.bin", "");

		THEN("reading it returns nothing") {
			REQUIRE(readFile(file.path()).empty());
		}
	}

	GIVEN("a project file read by a relative path, from another working directory") {
		const WorkingDirectory startedElsewhere(fs::temp_directory_path());

		THEN("the file is still found") {
			REQUIRE(readFile("tests/fixtures/2x2.png").size() ==
					fs::file_size(fs::path(app::projectRoot) / "tests/fixtures/2x2.png"));
		}
	}

	THEN("a file that does not exist is rejected") {
		REQUIRE_THROWS_AS(readFile("does/not/exist.bin"), error::FileError);
	}
}

SCENARIO("A SPIR-V file is read as 32-bit words", "[filesystem]") {
	GIVEN("a file of two 32-bit words") {
		const std::array<uint32_t, 2> words = {0x07230203U, 0x00010000U};
		std::string                   content(sizeof(words), '\0');
		std::memcpy(content.data(), words.data(), sizeof(words));
		const test::TemporaryFile file("shader.spv", content);

		THEN("it is read back as those two words") {
			REQUIRE(readSpirv(file.path()) == std::vector<uint32_t>(words.begin(), words.end()));
		}
	}

	GIVEN("a file whose size is not a whole number of words") {
		const test::TemporaryFile file("broken.spv", "abcde");

		THEN("it is rejected") {
			REQUIRE_THROWS_AS(readSpirv(file.path()), error::FileError);
		}
	}
}
