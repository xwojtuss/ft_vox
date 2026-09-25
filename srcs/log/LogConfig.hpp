#pragma once

#include <cstddef>
#include <map>
#include <string>

namespace logging {
	struct LogConfig {
		std::string                        file                 = "logs/ft_vox.log";
		std::string                        pattern              = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [thread %t] %v";
		std::string                        level                = "info";
		std::string                        flushLevel           = "warn";
		int                                flushIntervalSeconds = 1;
		std::size_t                        maxFileSizeMegabytes = 5;
		std::size_t                        maxFiles             = 3;
		std::map<std::string, std::string> loggerLevels;
	};

	[[nodiscard]] LogConfig parseLogConfig(const std::string& json, const std::string& source);
	[[nodiscard]] LogConfig loadLogConfig(const std::string& path);
}
