#include <catch2/catch_test_macros.hpp>

#include "error/Exception.hpp"

SCENARIO("Errors say which part of the game they come from", "[error]") {
	THEN("each domain has a readable name for the logs") {
		REQUIRE(error::domainName(error::Domain::Filesystem) == "Filesystem");
		REQUIRE(error::domainName(error::Domain::Asset) == "Asset");
		REQUIRE(error::domainName(error::Domain::Ecs) == "ECS");
		REQUIRE(error::domainName(error::Domain::Input) == "Input");
		REQUIRE(error::domainName(error::Domain::Window) == "Window");
		REQUIRE(error::domainName(error::Domain::Render) == "Render");
	}

	GIVEN("an error from the ECS") {
		const error::EcsError failure("entity 7 has no component of this type");

		THEN("it knows its domain and keeps its message") {
			REQUIRE(failure.domain() == error::Domain::Ecs);
			REQUIRE(failure.domainName() == "ECS");
			REQUIRE(std::string(failure.what()) == "entity 7 has no component of this type");
		}
		AND_THEN("it can still be caught as a standard error") {
			REQUIRE_THROWS_AS(throw failure, std::runtime_error);
		}
	}

	GIVEN("an input error") {
		const error::InputError failure("bindings are missing");

		THEN("it belongs to the input domain") {
			REQUIRE(failure.domain() == error::Domain::Input);
		}
	}
}

SCENARIO("File and asset errors remember which file failed", "[error]") {
	GIVEN("a file that could not be opened") {
		const error::FileError failure("shaders/shader.vert.spv", "could not be opened");

		THEN("the path is available on its own and in the message") {
			REQUIRE(failure.path() == "shaders/shader.vert.spv");
			REQUIRE(failure.domain() == error::Domain::Filesystem);
			REQUIRE(std::string(failure.what()) == "shaders/shader.vert.spv: could not be opened");
		}
	}

	GIVEN("an asset that could not be loaded") {
		const error::AssetError failure("models/cube.obj", "could not be loaded");

		THEN("the path is available on its own and in the message") {
			REQUIRE(failure.path() == "models/cube.obj");
			REQUIRE(failure.domain() == error::Domain::Asset);
			REQUIRE(std::string(failure.what()) == "models/cube.obj: could not be loaded");
		}
	}
}
