#include <cstdlib>
#include <exception>
#include <iostream>

#include "app/Application.hpp"
#include "app/ApplicationInfo.hpp"
#include "error/Exception.hpp"
#include "log/Log.hpp"

int main() {
	const logging::ShutdownGuard loggingShutdown;

	try {
		logging::init(logging::loadLogConfig("config/logging.json"));
		logging::app().info("{} is starting", app::appName);

		app::Application app;

		app.run();
	} catch (const error::Exception& e) {
		logging::logException(e);
		std::cerr << '[' << e.domainName() << "] " << e.what() << '\n';
		return EXIT_FAILURE;
	} catch (const std::exception& e) {
		logging::app().critical("{}", e.what());
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
	logging::app().info("{} closed normally", app::appName);
	return EXIT_SUCCESS;
}
