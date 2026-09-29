#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "app/Application.hpp"
#include "app/ApplicationInfo.hpp"
#include "error/Assert.hpp"
#include "error/Exception.hpp"
#include "log/Log.hpp"

namespace {
	void reportFatalError(const error::Exception& exception) noexcept {
		try {
			logging::logException(exception);
			std::cerr << '[' << exception.domainName() << "] " << exception.what() << '\n';
		} catch (...) {
			std::fputs("a fatal error happened and could not be reported\n", stderr);
		}
	}

	void reportFatalError(const char* message) noexcept {
		try {
			logging::app().critical("{}", message);
			std::cerr << message << '\n';
		} catch (...) {
			std::fputs("a fatal error happened and could not be reported\n", stderr);
		}
	}
}

int main() {
	error::installAssertionHandler();
	const logging::ShutdownGuard loggingShutdown;

	try {
		logging::init(logging::loadLogConfig("config/logging.json"));
		logging::app().info("{} is starting", app::appName);

		app::Application app;

		app.run();
		logging::app().info("{} closed normally", app::appName);
		return EXIT_SUCCESS;
	} catch (const error::Exception& e) {
		reportFatalError(e);
	} catch (const std::exception& e) {
		reportFatalError(e.what());
	} catch (...) {
		reportFatalError("an unknown error stopped the game");
	}
	return EXIT_FAILURE;
}
