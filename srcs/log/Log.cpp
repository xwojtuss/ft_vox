#include "Log.hpp"

#include <array>
#include <chrono>
#include <string>
#include <vector>
#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "../platform/filesystem/resolvePath.hpp"

namespace logging {
	namespace {
		constexpr std::size_t queueSize         = 8192;
		constexpr std::size_t backgroundThreads = 1;
		constexpr std::size_t bytesPerMegabyte  = 1024 * 1024;

		constexpr std::array domains = {
			error::Domain::Filesystem, error::Domain::Asset, error::Domain::Ecs,
			error::Domain::Input, error::Domain::Window, error::Domain::Render
		};

		std::shared_ptr<spdlog::logger> loggerNamed(const std::string& name) {
			if (auto logger = spdlog::get(name))
				return logger;

			auto discarding = std::make_shared<spdlog::logger>(name, std::make_shared<spdlog::sinks::null_sink_mt>());
			spdlog::register_logger(discarding);
			return discarding;
		}
	}

	void init(const LogConfig& config) {
		spdlog::shutdown();
		spdlog::init_thread_pool(queueSize, backgroundThreads);

		const auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
			resolvePath(config.file), config.maxFileSizeMegabytes * bytesPerMegabyte, config.maxFiles);
		file->set_pattern(config.pattern);

		std::vector<std::string> names = {appLoggerName};
		for (const error::Domain domain: domains)
			names.emplace_back(error::domainName(domain));

		for (const std::string& name: names) {
			auto logger = std::make_shared<spdlog::async_logger>(name, file, spdlog::thread_pool(),
																spdlog::async_overflow_policy::block);
			const auto level = config.loggerLevels.find(name);
			logger->set_level(
				spdlog::level::from_str(level != config.loggerLevels.end() ? level->second : config.level));
			logger->flush_on(spdlog::level::from_str(config.flushLevel));
			spdlog::register_logger(logger);
		}
		spdlog::flush_every(std::chrono::seconds(config.flushIntervalSeconds));
	}

	void flush() {
		spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) { logger->flush(); });
	}

	void shutdown() {
		spdlog::shutdown();
	}

	Logger get(const error::Domain domain) {
		return Logger(loggerNamed(std::string(error::domainName(domain))));
	}

	Logger app() {
		return Logger(loggerNamed(appLoggerName));
	}

	void logException(const error::Exception& exception) {
		get(exception.domain()).error("{}", exception.what());
	}

	ShutdownGuard::~ShutdownGuard() {
		shutdown();
	}
}
