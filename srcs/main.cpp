#include <cstdlib>
#include <exception>
#include <iostream>

#include "app/Application.hpp"
#include "error/Exception.hpp"

int main() {
	try {
		app::Application app;

		app.run();
	} catch (const error::Exception& e) {
		std::cerr << '[' << e.domainName() << "] " << e.what() << '\n';
		return EXIT_FAILURE;
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
