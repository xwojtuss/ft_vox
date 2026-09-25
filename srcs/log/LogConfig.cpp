#include "LogConfig.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <nlohmann/json.hpp>

#include "../error/Exception.hpp"
#include "../platform/filesystem/resolvePath.hpp"

namespace logging {
	namespace {
		constexpr std::array<std::string_view, 8> knownLevels = {
			"trace", "debug", "info", "warn", "warning", "error", "critical", "off"
		};

		std::string readLevel(const nlohmann::json& value, const std::string& setting, const std::string& source) {
			const auto level = value.get<std::string>();
			if (std::find(knownLevels.begin(), knownLevels.end(), level) == knownLevels.end())
				throw error::FileError(source, "unknown log level \"" + level + "\" for " + setting);
			return level;
		}

		template<typename T>
		void readIfPresent(const nlohmann::json& json, const char* key, T& target) {
			if (json.contains(key))
				target = json.at(key).get<T>();
		}
	}

	LogConfig parseLogConfig(const std::string& json, const std::string& source) {
		LogConfig config;

		try {
			const nlohmann::json root = nlohmann::json::parse(json);

			readIfPresent(root, "file", config.file);
			readIfPresent(root, "pattern", config.pattern);
			readIfPresent(root, "flushIntervalSeconds", config.flushIntervalSeconds);
			readIfPresent(root, "maxFileSizeMegabytes", config.maxFileSizeMegabytes);
			readIfPresent(root, "maxFiles", config.maxFiles);
			if (root.contains("level"))
				config.level = readLevel(root.at("level"), "level", source);
			if (root.contains("flushLevel"))
				config.flushLevel = readLevel(root.at("flushLevel"), "flushLevel", source);
			if (root.contains("loggers")) {
				for (const auto& [logger, level]: root.at("loggers").items())
					config.loggerLevels[logger] = readLevel(level, "logger " + logger, source);
			}
		} catch (const nlohmann::json::exception& exception) {
			throw error::FileError(source, std::string("is not valid logging configuration: ") + exception.what());
		}
		return config;
	}

	LogConfig loadLogConfig(const std::string& path) {
		const std::string resolved = resolvePath(path);
		if (!std::filesystem::exists(resolved))
			return {};

		std::ifstream file(resolved);
		if (!file.is_open())
			throw error::FileError(path, "could not be opened");

		std::stringstream contents;
		contents << file.rdbuf();
		return parseLogConfig(contents.str(), path);
	}
}
