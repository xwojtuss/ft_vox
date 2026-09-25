#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <unistd.h>

#include "error/Exception.hpp"
#include "log/Log.hpp"

using Catch::Matchers::ContainsSubstring;

namespace {
	struct LogFile {
		std::filesystem::path path = std::filesystem::temp_directory_path() /
									("ft_vox_tests_" + std::to_string(getpid()) + "_log.txt");

		LogFile() {
			std::filesystem::remove(path);
		}

		~LogFile() {
			logging::shutdown();
			std::filesystem::remove(path);
		}

		LogFile(const LogFile&)            = delete;
		LogFile& operator=(const LogFile&) = delete;

		[[nodiscard]] logging::LogConfig config() const {
			logging::LogConfig config;
			config.file    = path.string();
			config.pattern = "[%n] [%l] %v";
			return config;
		}

		[[nodiscard]] std::string contents() const {
			logging::shutdown();
			std::ifstream     file(path);
			std::stringstream text;
			text << file.rdbuf();
			return text.str();
		}
	};
}

SCENARIO("Messages are written to the log file, labelled with the part of the game they come from", "[log]") {
	GIVEN("logging set up to write to a file") {
		const LogFile logFile;
		logging::init(logFile.config());

		WHEN("the renderer and the app each log a message") {
			logging::get(error::Domain::Render).info("Using GPU {}", "Test GPU");
			logging::app().warn("low memory");

			THEN("both messages are in the file with their origin and level") {
				const std::string contents = logFile.contents();
				REQUIRE_THAT(contents, ContainsSubstring("[Render] [info] Using GPU Test GPU"));
				REQUIRE_THAT(contents, ContainsSubstring("[App] [warning] low memory"));
			}
		}
	}
}

SCENARIO("Every log level can be written", "[log]") {
	GIVEN("logging that lets every level through") {
		const LogFile      logFile;
		logging::LogConfig config = logFile.config();
		config.level              = "trace";
		logging::init(config);

		WHEN("one message is logged at each level") {
			const logging::Logger logger = logging::app();
			logger.trace("a {} message", "trace");
			logger.debug("a {} message", "debug");
			logger.info("a {} message", "info");
			logger.warn("a {} message", "warn");
			logger.error("a {} message", "error");
			logger.critical("a {} message", "critical");

			THEN("each one appears with its level") {
				const std::string contents = logFile.contents();
				REQUIRE_THAT(contents, ContainsSubstring("[App] [trace] a trace message"));
				REQUIRE_THAT(contents, ContainsSubstring("[App] [debug] a debug message"));
				REQUIRE_THAT(contents, ContainsSubstring("[App] [info] a info message"));
				REQUIRE_THAT(contents, ContainsSubstring("[App] [warning] a warn message"));
				REQUIRE_THAT(contents, ContainsSubstring("[App] [error] a error message"));
				REQUIRE_THAT(contents, ContainsSubstring("[App] [critical] a critical message"));
			}
		}
	}
}

SCENARIO("Each part of the game logs at its own level", "[log]") {
	GIVEN("logging at info, but with the renderer set to errors only") {
		const LogFile      logFile;
		logging::LogConfig config         = logFile.config();
		config.loggerLevels["Render"] = "error";
		logging::init(config);

		WHEN("the renderer and the ECS log an info message") {
			logging::get(error::Domain::Render).info("renderer detail");
			logging::get(error::Domain::Ecs).info("ecs detail");

			THEN("only the ECS message is written") {
				const std::string contents = logFile.contents();
				REQUIRE_THAT(contents, !ContainsSubstring("renderer detail"));
				REQUIRE_THAT(contents, ContainsSubstring("[ECS] [info] ecs detail"));
			}
		}
	}
}

SCENARIO("Errors are logged under the part of the game they come from", "[log]") {
	GIVEN("logging set up to write to a file") {
		const LogFile logFile;
		logging::init(logFile.config());

		WHEN("an asset error is logged") {
			logging::logException(error::AssetError("models/cube.obj", "could not be loaded"));

			THEN("it is written as an error of the asset loader") {
				REQUIRE_THAT(logFile.contents(), ContainsSubstring("[Asset] [error] models/cube.obj: could not be loaded"));
			}
		}
	}
}

SCENARIO("Logging before it is set up does nothing instead of crashing", "[log]") {
	GIVEN("logging that was never initialised") {
		logging::shutdown();

		THEN("every part of the game can still log") {
			REQUIRE_NOTHROW(logging::get(error::Domain::Render).warn("nobody is listening"));
			REQUIRE_NOTHROW(logging::app().error("nobody is listening"));
		}
	}
}

SCENARIO("Logging can be used from many threads at once", "[log]") {
	GIVEN("logging set up to write to a file") {
		const LogFile logFile;
		logging::init(logFile.config());

		WHEN("four threads each log 100 messages") {
			std::vector<std::thread> threads;
			for (int thread = 0; thread < 4; ++thread) {
				threads.emplace_back([thread] {
					for (int message = 0; message < 100; ++message)
						logging::get(error::Domain::Ecs).info("thread {} message {}", thread, message);
				});
			}
			for (std::thread& thread: threads)
				thread.join();

			THEN("all 400 messages end up in the file, none lost or mixed together") {
				std::istringstream lines(logFile.contents());
				std::string        line;
				int                count = 0;
				while (std::getline(lines, line)) {
					REQUIRE_THAT(line, ContainsSubstring("[ECS] [info] thread "));
					++count;
				}
				REQUIRE(count == 400);
			}
		}
	}
}
