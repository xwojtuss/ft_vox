#pragma once

#include "Logger.hpp"
#include "LogConfig.hpp"
#include "../error/Exception.hpp"

namespace logging {
	inline constexpr const char* appLoggerName = "App";

	void init(const LogConfig& config);
	void flush();
	void shutdown();

	[[nodiscard]] Logger get(error::Domain domain);
	[[nodiscard]] Logger app();

	void logException(const error::Exception& exception);

	class ShutdownGuard {
	public:
		ShutdownGuard() = default;
		~ShutdownGuard();

		ShutdownGuard(const ShutdownGuard&)            = delete;
		ShutdownGuard& operator=(const ShutdownGuard&) = delete;
	};
}
