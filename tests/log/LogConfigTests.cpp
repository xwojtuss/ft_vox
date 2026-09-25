#include <catch2/catch_test_macros.hpp>

#include "error/Exception.hpp"
#include "log/LogConfig.hpp"
#include "support/TemporaryFile.hpp"

SCENARIO("Logging works out of the box without a configuration", "[log]") {
	GIVEN("a configuration file that does not exist") {
		const logging::LogConfig config = logging::loadLogConfig("config/does_not_exist.json");

		THEN("sensible defaults are used: everything from info up goes to logs/ft_vox.log") {
			REQUIRE(config.file == "logs/ft_vox.log");
			REQUIRE(config.level == "info");
			REQUIRE(config.flushLevel == "warn");
			REQUIRE(config.loggerLevels.empty());
		}
	}

	GIVEN("an empty configuration") {
		const logging::LogConfig config = logging::parseLogConfig("{}", "logging.json");

		THEN("the same defaults are used") {
			REQUIRE(config.file == logging::LogConfig{}.file);
			REQUIRE(config.level == logging::LogConfig{}.level);
		}
	}
}

SCENARIO("Every logging setting can be changed in the configuration file", "[log]") {
	GIVEN("a configuration that sets every option") {
		const test::TemporaryFile file("logging.json", R"({
			"file": "logs/client.log",
			"pattern": "%v",
			"level": "debug",
			"flushLevel": "error",
			"flushIntervalSeconds": 5,
			"maxFileSizeMegabytes": 10,
			"maxFiles": 7,
			"loggers": { "Render": "trace", "ECS": "warn" }
		})");

		WHEN("it is loaded") {
			const logging::LogConfig config = logging::loadLogConfig(file.path());

			THEN("every value is taken from the file") {
				REQUIRE(config.file == "logs/client.log");
				REQUIRE(config.pattern == "%v");
				REQUIRE(config.level == "debug");
				REQUIRE(config.flushLevel == "error");
				REQUIRE(config.flushIntervalSeconds == 5);
				REQUIRE(config.maxFileSizeMegabytes == 10);
				REQUIRE(config.maxFiles == 7);
			}
			AND_THEN("each part of the game can have its own level") {
				REQUIRE(config.loggerLevels.at("Render") == "trace");
				REQUIRE(config.loggerLevels.at("ECS") == "warn");
			}
		}
	}
}

SCENARIO("Mistakes in the configuration file are reported instead of ignored", "[log]") {
	THEN("a file that is not valid JSON is rejected") {
		REQUIRE_THROWS_AS(logging::parseLogConfig("{ \"level\": ", "logging.json"), error::FileError);
	}
	AND_THEN("a misspelled level is rejected") {
		REQUIRE_THROWS_AS(logging::parseLogConfig(R"({ "level": "inf" })", "logging.json"), error::FileError);
	}
	AND_THEN("a misspelled level for one part of the game is rejected") {
		REQUIRE_THROWS_AS(logging::parseLogConfig(R"({ "loggers": { "Render": "verbose" } })", "logging.json"),
						error::FileError);
	}
	AND_THEN("a setting with the wrong type is rejected") {
		REQUIRE_THROWS_AS(logging::parseLogConfig(R"({ "maxFiles": "three" })", "logging.json"), error::FileError);
	}
}

SCENARIO("The configuration tracked in the repository is valid", "[log]") {
	THEN("config/logging.json loads without errors") {
		REQUIRE_NOTHROW(logging::loadLogConfig("config/logging.json"));
	}
}
