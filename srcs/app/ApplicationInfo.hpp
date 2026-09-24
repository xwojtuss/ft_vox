#pragma once

namespace app {
	constexpr const char* appName = "scop";

	constexpr float simulationFrameRate = 1.0f / 60.0f;

	constexpr bool VSyncEnabled = true;

#ifndef PROJECT_ROOT_DIR
# error "PROJECT_ROOT_DIR must be defined by the build system"
#endif

	constexpr const char* projectRoot = PROJECT_ROOT_DIR;
}
