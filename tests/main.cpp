#include <catch2/catch_session.hpp>

#include "support/Assertions.hpp"

int main(const int argc, char* argv[]) {
	test::reportAssertionsAsExceptions();
	return Catch::Session().run(argc, argv);
}
