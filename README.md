# ft_vox
A simple voxel engine built with modern C++ and Vulkan. The project is designed to be a learning experience for graphics programming and engine development, with a focus on clean code and modular design.

**WORK IN PROGRESS**

![WIP image of ft_vox](https://github.com/xwojtuss/scop/blob/6d7157b6b6f3806dffff252d198427a8c6543e07/ft_vox/WIP.png)

## Prerequisites

- C++26 compiler with `std::format` support
- CMake 3.25 or later
- Vulkan SDK
- GLFW (auto-fetched)
- GLM (auto-fetched)
- ImGui (auto-fetched)
- stb_image (single header, included)
- tinyobjloader (single header, included)
- EnTT (auto-fetched)
- spdlog (auto-fetched)
- nlohmann/json (auto-fetched)
- volk (auto-fetched)
- libassert (auto-fetched)
- Catch2 v3 (auto-fetched, tests only)
- gcovr (coverage only): `python3 -m pip install --user gcovr`
- pre-commit (formats commits): `pipx install pre-commit`
- clang-tidy 21 (static analysis, optional locally): `pipx install clang-tidy==21.1.6`

## Getting Started
After cloning the repository you can build the project using cmake:

```bash
cmake --preset dev-clang
cmake --build --preset dev-clang
```

Then run the executable:
```bash
./build/dev-clang/ft_vox
```

## Tests

Tests use [Catch2 v3](https://github.com/catchorg/Catch2) BDD-style and live in `tests/`, mirroring `srcs/`. Any `*.cpp` added there is picked up automatically.

Build and run all tests:
```bash
cmake --workflow --preset test
```

Run only some scenarios by tag, e.g. `[chunk]`:
```bash
./build/dev-clang/tests/ft_vox_tests "[chunk]"
```

Run the tests with a coverage report (fails if line coverage is below 80%):
```bash
cmake --workflow --preset coverage
```

The HTML report is written to `build/coverage/coverage/index.html`. What counts towards coverage (and the 80% threshold) is configured in `gcovr.cfg`. On 42 workstations use the `test42` and `coverage42` presets instead.

## Code style

Formatting is defined in `.clang-format`. The formatter version is pinned in `.pre-commit-config.yaml`; [pre-commit](https://pre-commit.com) downloads it by itself, so you don't install clang-format manually. Configuring the project with CMake installs the git hook when `pre-commit` is found. On commit, the hook formats your staged C++ files; if it changed anything, the commit stops so you can review and stage the result.

Static analysis rules are in `.clang-tidy`. To check the whole project the same way CI does (any finding fails the build):
```bash
cmake --workflow --preset lint
```

CI runs both checks and fails on formatting differences or any clang-tidy finding.

## Documentation

To generate documentation (might take a while):
```bash
cmake --build --preset dev-clang --target generate_docs
```

## Author
* **Wojtek Kornatowski** - [xwojtuss](https://github.com/xwojtuss)
